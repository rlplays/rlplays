#pragma once
#include <atomic>
#include <iostream>
#include <math.h>
#include <string.h>
#include <time.h>
#include "base_types.h"
#include "main_game.h"
#include "raylib.h"
#include "raymath.h"
#include "rl_types.h"
#include "world.h"
#include "world_fileset.h"
using namespace RLPlays;
using namespace std;
// Some of the code here is copied from PufferLib.
// MIT License in rlplays/game/thirdparty/PufferLib/LICENSE


constexpr float FRUIT_REWARD = 0.5f; // 0.1f;
constexpr float ENEMY_REWARD = 0.8f; // 0.1f;
constexpr float GOAL_REWARD = 1.0f;
constexpr float DEAD_REWARD = -1.0f;
constexpr float TIME_REWARD = -0.001f;         // Negative reward to encourage faster completion
constexpr float IDLE_REWARD = -0.01f;          // Negative reward if the agent is stuck in a place.
constexpr int NUM_TIMESTEPS_TIME_REWARD = 180; // every 3 seconds.
// Const Obs:
// 0: current rewards/maxRewards ratio
// 1: current time/max time
constexpr int NUM_CONST_OBS = 2;


// LibTorch throws exceptions on errors, log them correctly in debug mode only.
#if DEBUG
#define BEGIN_CATCH try
#else
#define BEGIN_CATCH
#endif

#if DEBUG
#define END_CATCH                                                                                                      \
  catch (const std::exception& e)                                                                                      \
  {                                                                                                                    \
    std::cerr << "Error: " << e.what() << std::endl;                                                                   \
    throw;                                                                                                             \
  }

#else
#define END_CATCH
#endif


// Track resets ACROSS all envs. Mainly to write to a file (at most once per time across all envs).
static atomic_int reset_count = {0};
static atomic_int agent_id = {0};
static atomic_int global_syllabus_index = {0};
static atomic_int last_selfplay_syllabus_index = {-1};

static std::shared_ptr<RLWeights> global_selfplay_weights = nullptr;
static int num_weights_changed = 0;

template <class T>
inline T clip(const T val, const T min, const T max)
{
  return val < min ? min : (val > max ? max : val);
}

inline float ZeroToOne(const float val) { return val > 0 ? float(val) : 1.0f; }


// # Traits (interactions.h) | # BlockTypes (tblock.h) | CurrentPos (norm) | PrevPos (norm)
inline int GetNumObsPerBlock() { return (TBlockTraitsCount + TBlockTypeCount + 2 + 2); }

// Number of floats to allocate.
inline int GetNumObs(const std::shared_ptr<TRLTrain>& rlTrain)
{
  return NUM_CONST_OBS + (rlTrain->MaxNumCells * GetNumObsPerBlock());
}

inline void EncodeTraits(TBlockTraits traits, Obs* obs, const bool useActivePlayer)
{
  // If activePlayer = true, then this is an RL training time agent, which controls the active player.
  // Otherwise, the RL agent is controlled during play-time inference and controls the inactive player.
  // TODO: Must we do this check for EVERY block? Is there a short-circuit we can do in the outer loop?
  if (!useActivePlayer)
  {
    // The trained agent is expecting the specific traits to identify itself vs the other player.
    // We have to perform this switcheroo as the human player has taken control of the active/main player
    // block (which was the RL agent's block during training).
    if (HasEnumValue(traits, TBlockTraits::MainPlayerBlock))
    {
      traits = AddEnumValue(RemoveEnumValue(traits, TBlockTraits::MainPlayerBlock), TBlockTraits::OtherPlayerBlock);
    }
    else if (HasEnumValue(traits, TBlockTraits::OtherPlayerBlock))
    {
      traits = AddEnumValue(RemoveEnumValue(traits, TBlockTraits::OtherPlayerBlock), TBlockTraits::MainPlayerBlock);
    }
  }
  for (unsigned long mask = 0; mask < TBlockTraitsCount; ++mask)
  {
    obs->TraitsOneHot[mask] = (static_cast<unsigned long>(traits) & (1U << mask)) ? 1.0f : 0.0f;
  }
}

inline void EncodeBlockType(TBlockType blockType, Obs* obs, const bool useActivePlayer)
{
  for (unsigned long mask = 0; mask < TBlockTypeCount; ++mask)
  {
    // Ignore TBlockType::None (0).
    obs->BlockType[mask] = (static_cast<unsigned long>(blockType) == (mask + 1)) ? 1.0f : 0.0f;
  }
}

inline void EncodeBlock(ABlock* block, int& index, const RLPlaysEnv* env, const bool useActivePlayer, Vector2& prevPos,
    Vector2& boxPos, const Rectangle& box, const Vector2& actionsBlockPos, const Vector2& sizeInvert)
{
  if (HasOneOfEnumValue(block->GetRunState(), AddEnumValue(TRunState::Invisible, TRunState::Removed)) ||
      block->IsA(TBlockTraits::Cosmetic))
  {
    return;
  }

  // Get the relevant box that is within the cell.
  if (box.width <= 0 || box.height <= 0) { return; }
  boxPos = RectTopLeft(box);
  prevPos = block->PrevPos;
  if (IsInvalidVector(prevPos)) { prevPos = boxPos; }
  prevPos = block->PrevPos; // Vector2Subtract(block->PrevPos, boxPos);
  // block->PrevPos = boxPos;
  auto* currentObs = reinterpret_cast<Obs*>(&env->observations[index]);
  currentObs->Pos = Vector2Multiply(Vector2Add(actionsBlockPos, boxPos), sizeInvert);
  currentObs->PrevPos = Vector2Multiply(Vector2Add(actionsBlockPos, prevPos), sizeInvert);
  // The RL agent has to observe the environment as a series of boxes around it with some traits per box.
  // It has to understand how each block works by observing it in a real environment with no special affinity
  // towards how a specific block might move or hit or jump. It has to learn everything tabula rasa.
  // It also means that if there is a new block type, we have to 'teach' the agent anew. It's fine - similar to how
  // a human might have never seen a snake or a dog before, but would have to learn how they behave by observing
  // them in the real world (in addition to whatever evolution happened to encode in human DNA to trigger
  // flight-or-fright response, seratonin/dopamine etc).

  // Also encodes 'other player' vs current 'main player' correctly which works in 1p or 2p mode automatically
  // without any changes to the code.
  EncodeTraits(block->Traits, currentObs, useActivePlayer);
  EncodeBlockType(block->BlockType, currentObs, useActivePlayer);
  index += GetNumObsPerBlock();
}

// Fills the environment for RL training or "play-time inference":
// - During RL training, the RL agent controls the active player actions.
// - During RL playtime inference, the RL agent controls the inactive player while the human controls
//   the active player actions.
inline void FillEnv(RLPlaysEnv* env, TContextPtr context, const bool useActivePlayer)
{
  int index = 0;
  const auto actionsBlock = useActivePlayer ? context->GetActionsHandlerBlock() : context->GetReplayHandlerBlock();
  const auto cellSize = context->GetCamera().CellSize;

  Vector2 actionsBlockPos = RectTopLeft(actionsBlock->Box);
  // Invert the block pos so all other blocks are relative to the actions block.
  actionsBlockPos = Vector2Multiply(actionsBlockPos, {-1, -1});
  env->current_agent_pos = actionsBlockPos;
  auto liveData = env->live_data;
  Vector2 prevPos = {0, 0};
  Vector2 boxPos = {0, 0};
  const Vector2 sizeInvert = {1.0f / context->ScreenRect().width, 1.0f / context->ScreenRect().height};
  env->observations[index++] = env->rewards[0] / ZeroToOne(context->World()->WorldInfo.GameProgress.MaxNumRewards);
  env->observations[index++] = float(env->step_count) / (float)env->max_steps;
  const auto& grid = context->Grid();
  // This results in a good focused spiral outward search around the actions block.
  // However, it's a bit expensive:
  //  22522.1 SPS (spiral out) vs 37634 SPS (raw block access) per-core Ubuntu 24.04 on a 13th Gen Intel(R) Core(TM)
  //  i9-13900HK.
  // I think we can use this with smaller search area so the RL agent doesn't have to process too many blocks especially
  // those out of view.
  EncodeBlock(
      actionsBlock.get(), index, env, useActivePlayer, prevPos, boxPos, actionsBlock->Box, actionsBlockPos, sizeInvert);
  grid->FindNeighborsWithinDistance(actionsBlock.get(),
      [&](const TGridBlockInfo& blockInfo)
      {
        auto block = blockInfo.Neighbor;
        Rectangle box = block->Box; // Copy temporarily.
        box = GetCollisionRec(box, blockInfo.CellBox);

        EncodeBlock(block, index, env, useActivePlayer, prevPos, boxPos, box, actionsBlockPos, sizeInvert);
        // Check if we ran out of space, if so, stop the callbacks.
        return (index < env->num_obs);
      });
  env->current_obs_count = index;

  for (; index < env->num_obs; ++index)
  {
    env->observations[index] = -1.0f;
  }
}


inline bool FillRewards(RLPlaysEnv* env, TContextPtr context, bool useActivePlayer)
{
  const auto& playerProgress = useActivePlayer ? context->GetGameProgress()->GetActivePlayerProgress()
                                               : context->GetGameProgress()->GetInactivePlayerProgress();

  auto done = RLPlays::IsDone(playerProgress.PlayerState);
  if (!done && (env->step_count >= env->max_steps)) { done = true; }
  env->proceed_to_next_round = false;
  env->current_round = context->GetGameProgress()->CurrentRound;
  if (done)
  {
    env->live_data->Recorded = context->GetActionsReplay(context);

    if (RLPlays::IsWinning(playerProgress.PlayerState))
    {
      if (context->World()->WorldInfo.GameProgress.MaxNumRewards <= 0 ||
          playerProgress.NumRewards >= context->World()->WorldInfo.GameProgress.MaxNumRewards ||
          (context->World()->WorldInfo.GameProgress.MaxNumRewards >= 4 && playerProgress.NumRewards >= 2))
      {
        env->proceed_to_next_round = true;
        // Penalize based on time taken...
        const auto timeMultiplier = clip((1 - (float(env->step_count) / ZeroToOne(env->max_steps))), 0.6f, 1.0f);
        env->rewards[0] = env->max_rewards * timeMultiplier;
      }
      else
      {
        const auto rewards = env->max_rewards *
            ((ZeroToOne(context->World()->WorldInfo.GameProgress.MaxNumEnemies)) +
                (float(playerProgress.NumRewards) / ZeroToOne(context->World()->WorldInfo.GameProgress.MaxNumRewards)));

        env->rewards[0] = clip(rewards, 0.0f, float(env->max_rewards));
        // (int(playerProgress.NumRewards) * FRUIT_REWARD); // Only collect part of the goal reward
      }
      // Favor episodic rewards that involve more self-play than single-player.
      // env->rewards[0] *= clip(float(env->current_self_play_count + 1) / (ZeroToOne(env->max_rounds) / 5.0f),
      // 0.1f, 2.0f);
      env->rewards[0] *= ZeroToOne((float)env->current_round);
    }
    else
    {
      env->rewards[0] = -(env->max_rewards);
    }
  }
  else
  {
    const auto enemyCountFrame = playerProgress.NumEnemies - env->prev_enemies;
    env->prev_enemies = playerProgress.NumEnemies;
    if (enemyCountFrame > 0)
    {
      // Delayed gratification
      // env->rewards[0] = enemyCountFrame * ENEMY_REWARD;
    }
    const auto rewardCountFrame = playerProgress.NumRewards - env->prev_rewards;
    env->prev_rewards = playerProgress.NumRewards;
    if (rewardCountFrame > 0)
    {
      // Delayed gratification
      // env->rewards[0] = FRUIT_REWARD * rewardCountFrame;
      env->total_reward_count += rewardCountFrame;
      // float(playerProgress.NumRewards * FRUIT_REWARD) /
      // float(context->World()->WorldInfo.GameProgress.MaxNumRewards);
    }

    if (env->rewards[0] <= 0)
    {
      if ((context->Frame() % NUM_TIMESTEPS_TIME_REWARD == 0)) { env->rewards[0] += TIME_REWARD; }

      if ((env->step_count - env->last_pos_timestep > env->num_idle_timesteps))
      {
        const auto cellSize = context->GetCamera().CellSize;
        if (AreVectorsSame(env->last_pos, env->current_agent_pos, cellSize.x * 1))
        {
          env->max_steps -= env->num_idle_timesteps;
          env->max_steps = std::max(env->step_count, env->max_steps);
        }
        env->rewards[0] += IDLE_REWARD;
        env->last_pos_timestep = env->step_count;
        env->last_pos = env->current_agent_pos;
      }
    }
    else
    {
      env->last_pos_timestep = env->step_count;
      env->last_pos = env->current_agent_pos;
    }
  }
  env->rewards[0] /= static_cast<float>(env->max_rewards);
  env->rewards[0] = clip(env->rewards[0], -1.0f, 1.0f);
  env->total_rewards += env->rewards[0];
  return done;
}


inline void ApplyActions(RLPlaysEnv* env, const int* actionVals, const int numActions)
{
  env->player_action = ConvertToPlayerAction(actionVals, numActions);
  HandleInput(*env->game_info, env->player_action);
}

inline void WriteLiveData(RLPlaysEnv* env)
{
  if (env->live_data != nullptr)
  {
    // std::string filename = GetDataDir() + "temp/file_counts_" + std::to_string(currTime) + "_" + std::to_string(1000
    // + (rand() % 9000)) + ".txt"; if (!env->LiveData->FileCounts.empty())
    auto str = env->live_data->LogStream.str();
    env->live_data->LogStream.str("");
#if RLPLAYS_TRAIN
    // Only log to the source-control tracked dir during training...
    auto LOG_DIR = "/private/logs/";
#else
    // For all other runs (editor/test etc) use a temp dir that doesn't pollute our source control.
    auto LOG_DIR = "/private/temp/";
#endif
    if (!env->render_supported && !str.empty())
    {
      auto currTime = std::time(nullptr);
      std::string filename = GetRootGameDir() + LOG_DIR + "rl_" + env->live_data->LogFilePrefix + ".json.txt";
      std::ofstream outFile(filename, std::ios::app);
      if (outFile.is_open())
      {
        if (atomic_load(&reset_count) < 10)
        {
          outFile << ctime(&currTime) << str << "\n--------------------------------------\n";
        }
        else
        {
          outFile << str;
        }
        outFile.close();
        // Only clear if we successfully wrote to file.
        env->live_data->LogStream.str("");
      }
    }
  }
}

/*
 * Init/alloc the environment once. c_reset is called multiple times during training which
 * should ideally not re-alloc stuff.
 * MUST BE CALLED FROM A SINGLE-THREADED CONTEXT! We initialize globals across envs here.
 */
inline void init(RLPlaysEnv* env)
{
  env->agent_id = atomic_fetch_add(&agent_id, 1);
  env->rl_fileset = TWorldFiles::Load();
  TLOG(LOG_TRACE, "Loading RLPlays fileset: %s, num obs: %d ", env->rl_fileset->WorldFileSetName.c_str(),
      GetNumObs(env->rl_fileset->RLTrain));
  TLOG(LOG_TRACE, "Env num obs: %d, num actions %d ", env->num_obs, env->num_actions);
  TASSERT(env->num_obs == GetNumObs(env->rl_fileset->RLTrain));
  TASSERT(env->num_actions == MAX_NUM_ACTIONS);
  srand((unsigned int)clock());
  env->log = {0};
  env->start_time_ms = CurrentTimeMs();
  env->end_time_ms = CurrentTimeMs();
  env->proceed_to_next_round = false;
  env->proceed_to_next_round_for_test = false;
  // resetCount = 0;
  env->live_data = std::make_shared<TLiveData>();
  env->live_data->LogFilePrefix = GetTimeStr("%Y_%m_%d_%H_%M");
  env->live_data->LogResets = 0;

  // env->randomize_player_pos = true;
  env->syllabus_index = 0;
  env->max_success_per_file_training_count = env->rl_fileset->RLTrain->MaxNumSuccessfulTrainingPerFile;
  env->max_failure_per_file_training_count = env->rl_fileset->RLTrain->MaxNumFailureTrainingPerFile;
  env->current_self_play_count = 0;
  env->total_self_play_count = 0;
  env->total_self_play_success_count = 0;
  env->max_rounds = 0;
  env->num_trains_for_file = 0;
  env->current_successful_training_count = env->current_failure_training_count = 0;
  env->sentinel_first = 42 + 1;
  env->sentinel_last = 42 + 2;
  env->last_player_box = INVALID_RECT;
  env->total_step_count = new atomic_int(0);
}

inline void c_close(RLPlaysEnv* env)
{
  if (env->game_info != nullptr)
  {
    UnloadGame(*env->game_info);
    env->game_info = nullptr;
  }
  env->rl_fileset = nullptr;
  if (env->client != nullptr)
  {
    const Client* client = env->client;
    CloseWindow();
    delete client;
  }
  WriteLiveData(env);
  env->live_data = nullptr;
  if (env->total_step_count != nullptr)
  {
    delete env->total_step_count;
    env->total_step_count = nullptr;
  }
}


inline void WriteLogsIfNeeded(RLPlaysEnv* env)
{
  if (env->render_supported || env->live_data == nullptr) { return; }
  std::string filename = env->live_data->CurrentTrainingFilename;
  if (filename.empty()) { return; }
  int resetCount = env->live_data->NumResets - env->live_data->LogResets;
  // int resetCount = atomic_load(&reset_count);
  if (env->live_data->LogResets == 0)
  {
    env->live_data->LogStream << " * Starting " << filename << "\n";
    WriteLiveData(env);
    env->live_data->LogResets = env->live_data->NumResets;
  }
  if (resetCount >= 2000 || (resetCount > 1000 && env->rewards[0] > 0.2f))
  {
    env->live_data->LogStream << "\n***** ID: " << env->agent_id << " @ " << GetTimeStr("%H_%M_%S") << " "
                              << " / " << filename << " (GoToNextRound? " << (env->proceed_to_next_round ? "yes" : "no")
                              << ") "
                              << " Current round: " << env->current_round << " SelfPlay? "
                              << (env->live_data->SelfPlayTraining ? "yes" : "no") << " / "
                              << " previous rewards " << env->rewards[0] << " ep return " << env->log.episode_return
                              << " ep length : " << env->log.episode_length << " /\n******* Num resets: " << resetCount
                              << " / [[ " << env->current_failure_training_count << " fails / "
                              << env->current_successful_training_count << " successes / "
                              << env->current_self_play_count << " self plays / " << env->max_rounds
                              << " self plays (max) rounds / " << (env->num_trains_for_file - 1)
                              << " total for this file ]] "
                              << " [[ Totals: " << env->total_self_play_count << " total self plays / "
                              << env->total_self_play_success_count << " total (successful) self plays / "
                              << "]]"
                              << " | GameType: " << int(env->last_game_type) 
                              << " | Weight Updates: " << num_weights_changed
                              << "\n";
    if (env->rewards[0] > 0.2) // || RESET_COUNT_GLOBAL % 10000 == 0) // uncomment this to log non-rewarded episodes too
    {
      if (env->live_data->Recorded != nullptr)
      {
        env->live_data->LogStream << "\"" << env->live_data->Recorded->GetSerialized() << "\",\n";
      }
    }
    WriteLiveData(env);
    env->live_data->LogResets = env->live_data->NumResets;
  }
  // if (resetCount >= 10000) { WriteLiveData(env); }
}

inline TGameLoadInfo* GetGameLoadInfo(RLPlaysEnv* env, const std::string filename)
{
  for (auto& cached : env->live_data->LoadedGames)
  {
    if (cached.Filename == filename) { return &cached; }
  }
  return nullptr;
}


inline std::string GetFileForTraining(RLPlaysEnv* env)
{
  const auto& curriculum = env->rl_fileset->RLTrain->CurriculumList;
  const int filesCount = int(curriculum.size());
  int index = atomic_load(&global_syllabus_index);
  index = clip(index, 0, filesCount - 1);
  auto trainingFile = curriculum[index];
  const auto* worldFile = env->rl_fileset->GetWorldFileRef(trainingFile);
  bool trainSelfPlay = false;
  const auto liveData = env->live_data;
  if (env->render_supported)
  {
    env->syllabus_index = index;
    global_syllabus_index = rand() % filesCount;
    return trainingFile;
  }
  if (env->proceed_to_next_round && env->rewards[0] >= 0.2f)
  {
    env->current_successful_training_count++;
    env->last_player_box = INVALID_RECT;
    if (env->current_successful_training_count >=
        (env->max_success_per_file_training_count * worldFile->TrainingMultiplier))
    {
      if (index == env->syllabus_index) { atomic_fetch_add(&global_syllabus_index, 1); }
      if (liveData->SelfPlayTraining)
      {
        ++env->total_self_play_success_count;
        env->total_self_play_prev_rewards = env->log.episode_return;
        env->total_self_play_prev_time = env->log.episode_length;
      }
    }
    trainSelfPlay = true;
  }
  else if (env->num_trains_for_file > 0)
  {
    // num_trains_for_file > 0 means we have already trained on this file at least once (success/failure).
    env->current_failure_training_count++;
    // Don't randomize player box if there is a failure: The agent failed to learn this level/file well enough.
    if (env->current_failure_training_count >= env->max_failure_per_file_training_count)
    {
      if (index == env->syllabus_index) { atomic_fetch_add(&global_syllabus_index, 1); }
      // env->syllabus_index = 0;
      // Now time to randomize as we failed too many times. Maybe the starting position is too hard?
      env->last_player_box = INVALID_RECT;
    }
    else
    {
      trainSelfPlay = true;
    }
    // If we do spend too much trying to train from one place, let's try some other pos...
    if (env->current_failure_training_count % 100 == 0) { env->last_player_box = INVALID_RECT; }
    else
    {
      trainSelfPlay = false;
    }
    // If we fail to train, well good luck, we will keep training (forever?!) until we get a lot of 'success' runs for
    // this file. There is no use climbing the curriculum ladder unless we are solid in our foundation. But if we keep
    // getting failured and never progress, well, we will just reset back to the start (i.e. first file!). This means
    // the agent is not learning / progressing very well or getting stuck in a level.
  }
  // Note the vectorized environments all c_step/c_reset together, so the goal is to fly with the flock - i.e. use the
  // same syllabus index. The only 'global' var is the global_syllabus_index, which is a hint for all the envs to follow
  // along. This is lock-free and may have some trouble where one agent increases, the other one decreases, but it's
  // fine.... as long as we proceed as a cohort.
  env->syllabus_index = atomic_load(&global_syllabus_index);
  env->syllabus_index = clip(env->syllabus_index, 0, filesCount - 1);
  atomic_store(&global_syllabus_index, env->syllabus_index);
  trainingFile = curriculum[env->syllabus_index];
  env->live_data->SelfPlayTraining = false;
  if (env->num_trains_for_file == 0 || trainingFile != env->live_data->CurrentTrainingFilename)
  {
    env->last_player_box = INVALID_RECT;
    env->num_trains_for_file = env->current_successful_training_count = env->current_failure_training_count = 0;
    env->current_self_play_count = 0;
    env->max_rounds = 0;
  }
  else if (trainSelfPlay)
  {
    const auto* loadInfo = GetGameLoadInfo(env, trainingFile);
    if (loadInfo != nullptr && liveData->Recorded != nullptr && liveData->Recorded->Filename == trainingFile)
    {
      if (loadInfo->WorldFile != nullptr && liveData->Recorded != nullptr && env->game_info != nullptr &&
          env->game_info->Game != nullptr)
      {
        if (env->current_self_play_count < loadInfo->WorldFile->NumSelfPlayTraining)
        {
          liveData->SelfPlayTraining = true;
          ++env->current_self_play_count;
          ++env->total_self_play_count;
        }
      }
    }
  }
  env->num_trains_for_file++;
  return trainingFile;
}

inline std::string PrepareNextRun(RLPlaysEnv* env)
{
  std::string filename = env->live_data->TrainingForcedFilename;
  auto liveData = env->live_data;
  if (filename.empty()) { filename = GetFileForTraining(env); }
  const auto* worldFile = env->rl_fileset->GetWorldFileRef(filename);

  TGameLoadInfo* loadInfo = GetGameLoadInfo(env, filename);
  std::shared_ptr<TGameActions> replayActions = nullptr;
  int nextRound = 1;
  auto gameType = TGameType::SinglePlayer;
#if PUFFERLIB_SELFPLAY
  if (worldFile != nullptr && worldFile->SupportsSelfPlay) { gameType = TGameType::PlayerVsAI; }
#endif

  if (loadInfo == nullptr)
  {
    loadInfo = (&liveData->LoadedGames.emplace_back());
    loadInfo->UseCachedData = true;
    loadInfo->Filename = filename;
  }
  else if (liveData->SelfPlayTraining && liveData->Recorded != nullptr && liveData->Recorded->Filename == filename)
  {
    auto progress = env->game_info->Game->Context->GetGameProgress();
    replayActions = liveData->Recorded;
    nextRound = progress->CurrentRound + 1;
    env->max_rounds = (nextRound > env->max_rounds) ? nextRound : env->max_rounds;
    // If the current game had a single-player mode, then we can use self-play to continue training.
    if ((progress->CurrentRound % 2 == 1))
    {
      // We have proper recorded actions for this file already and the training algo requests continuing to the next
      // level. For the next run: The replay actions are for player 1 (left side / prior); the active (RL Training)
      // player is player 2 (right side / current).
#if PUFFERLIB_SELFPLAY
      gameType = TGameType::AIVsPlayer;
#else
      gameType = TGameType::PriorVsPlayer;
#endif
    }
    else if ((progress->CurrentRound % 2 == 0))
    {
      // For the next run: The replay actions are for player 2 (right side / prior);
      // the active (RL Training) player is player 1 (left side / current).
#if PUFFERLIB_SELFPLAY
      gameType = TGameType::PlayerVsAI;
#else
      gameType = TGameType::PlayerVsPrior;
#endif
    }
  }
  loadInfo->ReplayActions = replayActions;
  loadInfo->Round = nextRound;
  loadInfo->GameType = gameType;
  env->last_game_type = gameType;
  // Check for round info.
  // Use replay actions if filename didn't change.
  if (env->game_info != nullptr)
  {
    UnloadGame(*env->game_info);
    env->game_info = nullptr;
  }
  env->game_info = std::make_shared<TGameInfo>(LoadGame(*env->rl_fileset, *loadInfo));
  return filename;
}

// Used by both train/eval and inference steps.
inline void c_minreset(RLPlaysEnv* env, TContextPtr context, const int maxTimeSeconds)
{
  env->step_count = 0;
  env->prev_rewards = 0;
  env->prev_enemies = 0;
  env->last_pos = {0, 0};
  env->last_pos_timestep = 0;
  env->current_agent_pos = {0, 0};
  env->total_rewards = 0;
  env->max_steps = context->TargetFPS * (int)maxTimeSeconds;
  env->num_idle_timesteps = (context->TargetFPS * 5); // Don't sit idle beyond a certain time limit.
  env->max_rewards = ((context->World()->WorldInfo.GameProgress.MaxNumRewards * FRUIT_REWARD) +
      (context->World()->WorldInfo.GameProgress.MaxNumEnemies * ENEMY_REWARD) + GOAL_REWARD);
  env->total_reward_count = 0;
  env->total_self_play_prev_rewards = 0;
  env->total_self_play_prev_time = 0;
  memset(env->observations, 0, env->num_obs * sizeof(float));
}

// Called during training/eval (for "inference", see rl_player.cpp, uses c_minreset above).
inline void c_reset(RLPlaysEnv* env)
{
  BEGIN_CATCH
  {
    int last_reset_count = atomic_fetch_add(&reset_count, 1);
#if RLPLAYS_TRAIN
    if (last_reset_count == 0)
    {
      RLPlays::SetupGlobal();
      if (THeadless::IsHeadless) { SetTraceLogLevel(LOG_WARNING); }
    }
#endif
    // srand((unsigned int)clock());

    const float maxTimeSeconds = env->rl_fileset->RLTrain->MaxNumSecondsToTrain;

    WriteLogsIfNeeded(env);
    const auto filename = PrepareNextRun(env);
    env->live_data->CurrentTrainingFilename = filename;
    env->live_data->Recorded = nullptr;
    env->live_data->PrevPositions.clear();
    env->proceed_to_next_round_for_test = env->proceed_to_next_round;
    env->proceed_to_next_round = false;
    // std::cout << "*** Loading file " << filename <<  "\n";
    env->live_data->NumResets++;
    TASSERT(!filename.empty(), "Expected proper filename");
    TASSERT(env->sentinel_first == 43, "Expected sentinel (first) to remain constant");
    TASSERT(env->sentinel_last == 44, "Expected sentinel (first) to remain constant");
    if (env->randomize_player_pos) { env->randomize_player_pos_next = true; }
    auto game = env->game_info->Game;

    c_minreset(env, game->Context, (int)maxTimeSeconds);
    // env->max_steps = game->Context->TargetFPS * env->rlFileset->RLTrain->MaxNumSecondsToTrain;
    // If you make any changes to this (training) c_step that are needed in the inference c_step, add to c_minreset()
    // instead. During training, we always use the active player to train. During inference, we use the inactive player.
    FillEnv(env, env->game_info->Game->Context, true);

    env->last_pos = env->current_agent_pos;
  }
  END_CATCH
}

inline void add_log(RLPlaysEnv* env)
{
  env->log.perf += env->rewards[0] / env->max_rewards;
  env->log.score += env->rewards[0];
  env->log.episode_return += env->total_rewards;
  env->log.episode_length += env->step_count;
  env->log.syllabus_index = env->syllabus_index;
  env->log.current_self_play_count = env->current_self_play_count;
  env->log.num_trains_for_file = env->num_trains_for_file;
  env->log.total_self_play_prev_rewards = env->total_self_play_prev_rewards;
  env->log.total_self_play_count = env->total_self_play_count;
  env->log.total_reward_count = env->total_reward_count;
  env->log.current_obs_count = env->current_obs_count;
  env->log.total_self_play_prev_time = env->total_self_play_prev_time;
  env->log.last_game_type = (float)env->last_game_type;
  env->log.total_self_play_success_count = env->total_self_play_success_count;

  env->log.n++;

  env->end_time_ms = CurrentTimeMs();
  // double diffMs = env->endTimeMs - env->startTimeMs;
  // if (diffMs <= 0.000001) { diffMs = 0.000001; }
  env->live_data->TotalNumSteps += env->log.episode_length;
  env->log.n++;
}

// Called during training/eval (for "inference", see rl_player.cpp).
inline void c_step(RLPlaysEnv* env)
{
  BEGIN_CATCH
  {
    // Update the player/agent pos.
    env->step_count += 1;
    atomic_fetch_add(env->total_step_count, 1);
    env->proceed_to_next_round_for_test = false;

    if (env->randomize_player_pos_next)
    {
      if (AreRectsSame(env->last_player_box, INVALID_RECT))
      {
        env->last_player_box = TBlockUtils::RandomizePlayerStartPos(env->game_info->Game->Context);
      }
      TBlockUtils::SetPlayerStartPos(env->game_info->Game->Context, env->last_player_box);
      env->randomize_player_pos_next = false;
    }

    // The agent being trained gets the playing agent's rewards.
    env->rewards[0] = 0;
    env->terminals[0] = 0;

    for (int i = 0; i < env->num_frame_skips; ++i)
    {
      ApplyActions(env, env->actions, MAX_NUM_ACTIONS);
      UpdateFrame(*env->game_info);
    }

    // During training, we always use the active player to train. During inference/eval, we use the inactive player.
    FillEnv(env, env->game_info->Game->Context, true);
    const auto done = FillRewards(env, env->game_info->Game->Context, /* trainingTime = ActivePlayer */ true);

    // Done == terminal or truncated.
    if (done)
    {
      env->terminals[0] = 1.0f;
      add_log(env);
      c_reset(env);
    }
  }
  END_CATCH
}

inline void ResetForRender(RLPlaysEnv* env) { env->render_supported = true; }

inline void allocate(RLPlaysEnv* env, const int allowRender, const int num_frame_skips = 1,
    const std::string& filename = "", const bool randomizePlayerPos = false,
    const std::shared_ptr<TRLTrain>& rlTrain = nullptr, bool alloc_obs = true)
{
  env->render_supported = allowRender;
  env->num_frame_skips = num_frame_skips;
  init(env);
  if (rlTrain != nullptr) { env->rl_fileset->RLTrain = rlTrain; }

  env->randomize_player_pos = randomizePlayerPos;
  env->live_data->TrainingForcedFilename = filename;

  if (alloc_obs) { env->observations = (float*)calloc(env->num_obs, sizeof(float)); }
  env->actions = (int*)calloc(MAX_NUM_ACTIONS, sizeof(int));
  env->rewards = (float*)calloc(1, sizeof(float));
  env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));

  c_reset(env);
}


inline void free_allocated(RLPlaysEnv* env)
{
  free(env->observations);
  free(env->actions);
  free(env->rewards);
  free(env->terminals);
  c_close(env);
}

inline bool c_should_transfer_selfplay_weights()
{
#ifndef PUFFERLIB_SELFPLAY
  // c_trainsfer_selfplay_weights won't be called if selfplay is not enabled.
  return false;
#endif
  int current = atomic_load(&global_syllabus_index);
  int last = atomic_exchange(&last_selfplay_syllabus_index, current);
  return current != last;
}

inline void c_transfer_selfplay_weights(RLPlaysEnv* env, int env_index, float* encoder_w, int encoder_w_size,
    float* encoder_b, int encoder_b_size, float* decoder_w, int decoder_w_size, float* decoder_b, int decoder_b_size,
    float* value_w, int value_w_size, float* value_b, int value_b_size, float* weight_ih, int weight_ih_size,
    float* weight_hh, int weight_hh_size, float* bias_ih, int bias_ih_size, float* bias_hh, int bias_hh_size)
{
  if (env->game_info == nullptr || env->game_info->Game == nullptr || env->rl_fileset->RLTrain == nullptr) return;
  auto context = env->game_info->Game->Context;
  if (context == nullptr) return;

  if (env_index == 0)
  {
    // Pack weights into contiguous buffer in puffernet make_linearlstm order:
    // encoder_w, encoder_b, decoder_w, decoder_b, value_w, value_b, lstm weights
    int total = encoder_w_size + encoder_b_size + decoder_w_size + decoder_b_size + value_w_size + value_b_size +
        weight_ih_size + weight_hh_size + bias_ih_size + bias_hh_size;

    if (global_selfplay_weights == nullptr)
    {
      global_selfplay_weights = std::make_shared<RLWeights>(new float[total], total);
      std::cout << "--- Allocated global self-play weights of size: " << total << "\n";
    }
    else
    {
      if (global_selfplay_weights->weights == nullptr || global_selfplay_weights->size != total)
      {
        throw std::runtime_error("Size mismatch for self-play weights.");
      }
    }
    context->SetupRL(context, env->rl_fileset->RLTrain, global_selfplay_weights);

    float* dst = global_selfplay_weights->weights;
    auto copyw = [&dst](float* src, int sz)
    {
      memcpy(dst, src, sz * sizeof(float));
      dst += sz;
    };
    copyw(encoder_w, encoder_w_size);
    copyw(encoder_b, encoder_b_size);
    copyw(decoder_w, decoder_w_size);
    copyw(decoder_b, decoder_b_size);
    copyw(value_w, value_w_size);
    copyw(value_b, value_b_size);
    copyw(weight_ih, weight_ih_size);
    copyw(weight_hh, weight_hh_size);
    copyw(bias_ih, bias_ih_size);
    copyw(bias_hh, bias_hh_size);
    ++num_weights_changed;
  }
  // No need to copy over the same thing - we share the weights across envs.
  context->UpdateRLPlayerWeights(global_selfplay_weights);
}
