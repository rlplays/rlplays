#include <context.h>
#include <game.h>
#include <log.h>
#include <math.h>
#include <rl_env.h>
#include <rl_player.h>
#include <rl_utils.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "main_game.h"
#include "raylib.h"
#include "rl_utils.h"

namespace RLPlays
{
#include "puffernet.h"


struct TRLState
{
  TRLState(TContextPtr context, std::shared_ptr<TRLTrain> rlTrain, bool activePlayer,
      std::shared_ptr<RLWeights> shared_weights)
  {
    this->activePlayer_ = activePlayer;
    auto configFilePath = TrainedConfigFilepath();
    TLOG(LOG_INFO, "Starting RL Player: Loading config from %s ", configFilePath.c_str());
    auto modelConfig = TConfig(configFilePath);
    env_ = RLPlaysEnv{
        .num_obs = modelConfig.GetInt("", "env.num_obs", 0),
        .num_actions = modelConfig.GetInt("", "env.num_actions", 0),
        .max_steps = 1000000, // TODO: Must use the same logic as during training.
        .num_frame_skips = modelConfig.GetInt("", "env.num_frame_skips", 1),
        .render_supported = 0,
    };
    int hidden_size = modelConfig.GetInt("", "rnn.hidden_size", 0);
    int input_size = modelConfig.GetInt("", "rnn.input_size", 0);
    if (env_.num_obs != GetNumObs(rlTrain)) 
    {
      TLOG(LOG_INFO, "(RL Disabled). RL obs size changed: env_.num_obs %d != num_obs %d", env_.num_obs, GetNumObs(rlTrain));
      return;
    }
    if (input_size != hidden_size || input_size == 0)
    {
      TLOG(LOG_ERROR, "Invalid RL environment config: input_size %d != num_obs %d, num_actions %d", input_size,
          env_.num_obs, env_.num_actions);
      throw std::runtime_error("Invalid RL environment config: input_size != num_obs");
    }

    logitSizes_ = new int[MAX_NUM_ACTIONS];
    for (int i = 0; i < MAX_NUM_ACTIONS; ++i)
    {
      logitSizes_[i] = 2;
    }

    auto path = TrainedModelPath();
#if (RLPLAYS_EDITOR)
    path = TrainedModelPathWith(TRLTrain::WEIGHT_FILE_INDEX, rlTrain);
    if (!FileExists(path.c_str()))
    {
      TLOG(LOG_ERROR,
          "Trained model file not found at %s. Make sure to run the converter to generate the trained model file from "
          "the training weights.",
          path.c_str());
      throw std::runtime_error("Trained model file not found");
    }
#endif
    int num_weights = modelConfig.GetInt("", "num_weights", 0);
    if (shared_weights != nullptr)
    {
      TLOG(LOG_INFO, "Using shared weights for self-play: num_weights %d", shared_weights->size);
      shared_weights_ = shared_weights;
      weights_ = new Weights{.data = shared_weights->weights, .size = shared_weights->size, .idx = 0};
    }
    else
    {
      TLOG(LOG_INFO, "Model path %s (%d), # weights %d / num frame skips %d", path.c_str(), TRLTrain::WEIGHT_FILE_INDEX,
          num_weights, env_.num_frame_skips);
      weights_ = load_weights(path.c_str(), num_weights);
    }
    net_ = make_linearlstm(weights_, 1, env_.num_obs, logitSizes_, MAX_NUM_ACTIONS, hidden_size);
    AllocEnv_(context);
    enabled_ = true;
  }

  // This must be called BEFORE any update has happened as the RL logic is:
  // Obs/Rewards/Done (previous frame) -> GetActions -> ApplyActions -> UpdateFrame -> Store FillObs/Rewards/Done (for
  // the next frame)
  void HandleRLPlayerActions(TContextPtr context)
  {
    if (!enabled_) { return; }
    auto frame = context->Frame();
    // This is called before the frame has 'settled' so the frame has not yet been incremented.
    if (frame % env_.num_frame_skips == 0)
    {
      env_.rewards[0] = 0;
      env_.terminals[0] = 0;

      ::FillEnv(&env_, context, activePlayer_);
      const auto done = ::FillRewards(&env_, context, activePlayer_);
      ::forward_linearlstm(net_, env_.observations, env_.actions);
    }
    StepEnv_(context);
  }

  void FillEnvObs(TContextPtr context)
  {
    if (!enabled_) { return; }
  }

  // We assume that the weights pointer simply changes outside of this code, we keep the same reference.
  void UpdateWeights(std::shared_ptr<RLWeights> w)
  {
    if (!enabled_ || weights_ == nullptr || net_ == nullptr) return;
    if (w->size != weights_->size || w->weights != weights_->data)
    {
      TLOG(LOG_FATAL, "Size mismatch when updating weights (or incorrect pointer): new size %d, expected size %d", w->size, weights_->size);
      return;
    }
    // TODO: Reset when updating at the start of a horizon? (In the middle of things?).
    // reset_linearlstm(net_);
  }

  ~TRLState()
  {
    if (!enabled_) { return; }
    FreeEnv_();
    delete[] logitSizes_;
    free_linearlstm(net_);
    if (shared_weights_ == nullptr) { free(weights_); }
    else
    {
      shared_weights_ = nullptr;
      weights_ = nullptr;
    }
  }

private:
  // Minimal version of allocate in rl_env.h
  void AllocEnv_(TContextPtr context)
  {
    // TASSERT(env_.num_obs == GetNumObs(context->));
    TASSERT(env_.num_actions == MAX_NUM_ACTIONS);
    srand((unsigned int)clock()); // TODO: Must get the seed from the context for deterministic playback.
    env_.live_data = std::make_shared<TLiveData>();
    env_.log = {0};
    env_.observations = (float*)calloc(env_.num_obs, sizeof(float));
    env_.actions = (int*)calloc(MAX_NUM_ACTIONS, sizeof(int));
    env_.rewards = (float*)calloc(1, sizeof(float));
    env_.terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
    // Fill the environment ego-centric from the 'other player' (inactive player) perspective.
    // The human player becomes the 'enemy' block to mimic what the RL agent observed during training.
    ResetEnv_(context);
    ::FillEnv(&env_, context, /* useActivePlayer */ activePlayer_);
    env_.last_pos = env_.current_agent_pos;
  }


  // Minimal versions of c_step to be used during RLPlays gameplay split into two parts:
  // 1. GetActions for the current frame.
  //  --> The game play occurs here outside of this code.
  // 2. Fill the obs for the next frame.
  void StepEnv_(TContextPtr context)
  {
    // The agent being trained gets the playing agent's rewards.
    env_.player_action = ConvertToPlayerAction(env_.actions, MAX_NUM_ACTIONS);
    TPlayerActions actions = {env_.player_action, context->Frame(), {}};
    // If activePlayer_ = false: Apply the actions to the OTHER (inactive) player. (Let the human control the active
    // player.)
    context->HandleActions(context, actions, !activePlayer_);
  }

  // Minimal form of c_reset
  void ResetEnv_(TContextPtr context)
  {
    if (!enabled_) { return; }
    c_minreset(&env_, context, SecondsFromNanos(context->World()->WorldInfo.GameProgress.TimeLimit.TimeSet));
    env_.num_idle_timesteps = 1000000; // Effectively disable idle reset.

    FillEnvObs(context);
    env_.last_pos = env_.current_agent_pos;
    enabled_ = true;
  }

  void FreeEnv_()
  {
    env_.live_data = nullptr;
    free(env_.observations);
    free(env_.actions);
    free(env_.rewards);
    free(env_.terminals);
  }

  RLPlaysEnv env_;
  int* logitSizes_ = nullptr;
  Weights* weights_ = nullptr;
  // If shared_weights is true, the weights are owned by the caller (for self-play) and should not be freed by this
  // class.
  std::shared_ptr<RLWeights> shared_weights_;
  LinearLSTM* net_ = nullptr;
  bool enabled_ = false;
  // int prevActions_[MAX_NUM_ACTIONS] = {0};
  //  Whether the RL Player controls the human player (left) or the RL Player controls (right).
  bool activePlayer_ = false;
  friend struct TRLPlayer;
};

void TRLPlayer::SetupRLEnv(TContextPtr context, std::shared_ptr<TRLTrain> rlTrain, bool isRLPlayer, bool activePlayer,
    std::shared_ptr<RLWeights> shared_weights)
{
  isRLPlayer_ = isRLPlayer;
  if (!isRLPlayer_) { return; }
  if (state_ == nullptr)
  {
    UnloadRLEnv(context);
    const auto gameType = context->GetGameProgress()->GameType;
    state_ = new TRLState(context, rlTrain, activePlayer, shared_weights);
  }
  else
  {
    state_->ResetEnv_(context);
  }
}

void TRLPlayer::HandleRLPlayerActions(TContextPtr context)
{
  if (!isRLPlayer_) { return; }
  state_->HandleRLPlayerActions(context);
}

void TRLPlayer::UpdateSelfPlayWeights(std::shared_ptr<RLWeights> new_weights)
{
  if (!isRLPlayer_ || state_ == nullptr) return;
  state_->UpdateWeights(new_weights);
}

void TRLPlayer::HandlePostUpdateActions(TContextPtr context)
{
  if (!isRLPlayer_) { return; }
  // TODO: This and the HandleRLPlayerActions can be called from a bg thread.
  // The context / grid is isolated enough to be thread-safe and while we swap backbuffer/frontbuffer
  // we can do this expensive operation.
  // So far this does not have any impact as we can do close to 20K+ FPS on a 13th Gen Intel(R) Core(TM) i9-13900HK
  state_->FillEnvObs(context);
}

inline void DrawRect_(TContextPtr context, const Vector2& offset, const Vector2& pos, const Vector2& size, float fade,
    const float* traitsOneHot)
{
  float c[4] = {0};
  for (int i = 0; i < TBlockTraitsCount; ++i)
  {
    c[i % 4] += (traitsOneHot[i] ? 32.0f : 0);
  }
  DrawRectangleRec({offset.x + pos.x, offset.y + pos.y, size.x, size.y},
      Fade({static_cast<unsigned char>(c[0]), (unsigned char)c[1], (unsigned char)c[2], (unsigned char)c[3]}, fade));
}

bool AllMinusOne_(const Obs& obs)
{
  return obs.Pos.x <= -1 && obs.Pos.y <= -1 && obs.PrevPos.x <= -1 && obs.PrevPos.y <= -1 &&
      std::all_of(std::begin(obs.TraitsOneHot), std::end(obs.TraitsOneHot), [](float v) { return v == -1.0f; });
}

// Decodes the obs from rl_env.h FillObs.
void TRLPlayer::DebugRender(TContextPtr context)
{
  if (!isRLPlayer_) { return; }
  const auto& env = state_->env_;
  const float* obs = env.observations;
  const auto actionsBlock =
      state_->activePlayer_ ? context->GetActionsHandlerBlock() : context->GetReplayHandlerBlock();

  const auto cellSize = context->GetCamera().CellSize;
  const Vector2 size = {context->ScreenRect().width, context->ScreenRect().height};
  const auto actionsBlockPos = Vector2Multiply(env.current_agent_pos, {-1, -1});
  // Track the distance from the center outwards to fade out blocks further away.
  float centerOut = 0.0f;
  const float maxCount = float(env.current_obs_count - NUM_CONST_OBS) / float(GetNumObsPerBlock());
  for (int i = NUM_CONST_OBS; i < env.current_obs_count; i += GetNumObsPerBlock())
  {
    const Obs& block = *(reinterpret_cast<Obs*>(const_cast<float*>(&obs[i])));
    auto pos = Vector2Multiply(block.Pos, size);
    float centerFade = (1.0f - (centerOut / maxCount));
    DrawRect_(context, actionsBlockPos, pos, cellSize, 0.6f * centerFade, block.TraitsOneHot);
    DrawRect_(context, actionsBlockPos, Vector2Multiply(block.PrevPos, size), cellSize, 0.25f * centerFade,
        block.TraitsOneHot);
    ++centerOut;
  }
}

void TRLPlayer::UnloadRLEnv(TContextPtr context)
{
  if (state_ != nullptr) { state_->ResetEnv_(context); }
}
} // namespace RLPlays
