#pragma once
#include <sstream>
#include <nlohmann/json.hpp>
#include "base_types.h"
#include "game_actions.h"
#include "raylib_utils.h"
#include "base_types.h"
#include "timer.h"

using json = nlohmann::json;

namespace RLPlays
{
#if defined (_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4505)  // MSVC's equivalent to -Wunused-function
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static bool IsWinning(const TPlayerState playerState) { return playerState == TPlayerState::Won; }
static bool IsDone(const TPlayerState playerState) { return playerState != TPlayerState::Alive; }

struct TPlayerProgress
{
  int NumRewards = 0;
  TPlayerState PlayerState = TPlayerState::Alive;
  bool HasReachedGoal = false;
  int NumEnemies = 0;
};

//! @brief Contains ongoing game progress for the current game. May include duplicate (copied over
//! info from TWorld to prevent allowing TWorld to be exposed to downstream classes).
//! This is a bit complicated unfortunately (but makes for interesting gameplay and is constrained to a single place).
//! This controls how the game progresses through rounds, who is the active/non-active player, when to show transitions,
//! player state, rewards etc.
//! Terminalogy: "Active" Player is the "human" player that controls via the keyboard/controller.
//! "Non-active" Player is the other player (AI or replay) that is not controlled by the human.
//! Player1 is always the left side player, Player2 is always the right side player.
//! In SinglePlayer mode, Player1 is the active player, Player2 is dormant
//! In other modes, the Player2 or Player1 (non-active player) may be AI or replay actions.
//! This also lets us train the AI with itself and also against a prior recorded version of itself.
struct TGameProgress
{
  //! @brief Tracks active player progress and the other non-active player proress as well.
  //! Could be expanded beyond 2 players later on.
  std::vector<TPlayerProgress> PlayerProgress = {};
  bool HasMirrorMode = false;
  // Round is used to indicate 'switch the left/right for the active player'
  // with replay actions on the other side (except round 1 which always begins on the left
  // side).
  int CurrentRound = 1;

  // For convenience, we also store the max rounds from TWorldInfo here (as TWorldInfo is pristine data).

  //! @brief Max number of rounds for this game. We could rollover past this limit and allow the game to progress forever btw.
  int MaxRounds = 1;

  //! @brief If {@related MaxRounds} is reached, do we end the game or continue forever?
  bool InfiniteRounds = true;

  TGameType GameType = TGameType::SinglePlayer;


  //! @brief In display-based runs, show slow (human-centric) transitions.
  //! NOTE: Use AllowTransitions() instead which tracks both custom scene transitions AND headless mode.
  bool ShowSceneTransitions = true;
  TCountdownTimer TimeLimit = {NanosFromSeconds(60)};
  bool HasActions;

  // Shortcut for menu input action to simplify the menu screen (otherwise we would need to special-case the
  // input handling).
  TPlayerAction CurrentMenuInput = TPlayerAction::None;
  bool FinishedAllRounds = false;

  //! @brief Per-round information.
  std::string NextProgressText1 = {};
  std::string NextProgressText2 = {};
  std::string NextProgressText3 = {};
  std::string NextProgressText4 = {};
  TPlayerState LastPlayerState = TPlayerState::Alive;

  // Rewards are spread throughout the map. Once all players get all rewards (does not matter
  // who gets what), the level is considered complete.
  int MaxNumRewards = 0;

  // Number of enemies killed by the active player.
  int MaxNumEnemies = 0;
  //! @brief Set to true by progress block if game type is changed or other progress info changes requiring a full game reload.
  bool ShouldReloadGame = false;
  int PreviousRound = 0;
  bool ShowMenuAnim = true;

  // Only serialize the minimal stuff we need - everything else is for tracking progress including
  // duplicate stuff purely to ease access (without needing TWorld stuff).
  Serializer(TGameProgress, TimeLimit, HasMirrorMode, ShowSceneTransitions, MaxRounds, InfiniteRounds)

  //! @brief Is the game done because the player won/died or did the time ran out?
  [[nodiscard]] bool IsDone();

  //! @brief Gets the player state for the given player id.
  TPlayerState GetPlayerState(const int playerId) const
  {
    if (playerId == PlayerId1) { return GetPlayerProgress(playerId).PlayerState; }
    if (playerId == PlayerId2) { return GetPlayerProgress(playerId).PlayerState; }
    return TPlayerState::Dead;
  }


  //! @brief Returns the player progress for the given player id (1-indexed).
  const TPlayerProgress& GetPlayerProgress(const int playerId) const
  {
    return playerId == PlayerId1 ? PlayerProgress[0] : PlayerProgress[1];
  }

  //! @brief In specific cases, we may want to obtain an update-able progress for the player with id (1-indexed).
  TPlayerProgress* UpdatePlayerProgress(const int playerId)
  {
    return playerId == PlayerId1 ? &PlayerProgress[0] : &PlayerProgress[1];
  }

  //! @brief Tracks the current time once the game starts.
  void TrackTimeLeft(TContextPtr context);

  [[nodiscard]] bool IsPlayer1TheActivePlayer() const
  {
    // Excludes AIVsPlayer and PriorVsPlayer (as the active human player is Player2).
    return GameType == TGameType::SinglePlayer || GameType == TGameType::PlayerVsAI ||
        GameType == TGameType::PlayerVsPrior || GameType == TGameType::PlayerVsPlayer;
  }

  int GetWinningPlayerId() const
  {
    switch (GameType)
    {
    case TGameType::SinglePlayer:
    // We return Player2 (if active player1 lost) simply to avoid an 'invalid' player id confusion.
    case TGameType::PlayerVsPrior:
    case TGameType::PlayerVsAI:
    case TGameType::PlayerVsPlayer: return IsWinning(GetPlayerState(PlayerId1)) ? PlayerId1 : PlayerId2;
    // For the cases where the active (human or RL training) player is on the right side.
    // NOTE: During RL Training, the active player is receiving the inputs just like a normal human player would.
    // During RL "inference" (i.e. playing the game), the active player is still the human player.  
    case TGameType::AIVsPlayer:
    case TGameType::PriorVsPlayer: return IsWinning(GetPlayerState(PlayerId2)) ? PlayerId2 : PlayerId1;
    default: return -1;
    }
  }


  [[nodiscard]] int GetActivePlayerId() const
  {
    if (IsPlayer1TheActivePlayer()) { return PlayerId1; }
    else { return PlayerId2; }
  }

  [[nodiscard]] int GetInactivePlayerId() const
  {
    if (IsPlayer1TheActivePlayer()) { return PlayerId2; }
    else { return PlayerId1; }
  }

  [[nodiscard]] bool ShouldShowPlayer(const int playerId) const
  {
    switch (GameType)
    {
    case TGameType::SinglePlayer: { return (playerId == PlayerId1); }
    case TGameType::PlayerVsAI:
    case TGameType::AIVsPlayer:
    case TGameType::PlayerVsPrior:
    case TGameType::PriorVsPlayer:
    case TGameType::PlayerVsPlayer: { return (playerId == PlayerId2 || playerId == PlayerId1); }
    default: return false;
    }
  }

  [[nodiscard]] int GetLeftSidePlayer() const { return PlayerId1; }

  [[nodiscard]] int GetRightSidePlayer() const { return PlayerId2; }


  [[nodiscard]] bool IsActivePlayer(const int playerId) const
  {
    switch (GameType)
    {
    case TGameType::SinglePlayer:
    case TGameType::PlayerVsAI:
    case TGameType::PlayerVsPrior:
    case TGameType::PlayerVsPlayer: { return (playerId == PlayerId1); }
    case TGameType::AIVsPlayer:
    case TGameType::PriorVsPlayer: { return (playerId == PlayerId2); }
    default: return false;
    }
  }

  [[nodiscard]] TGameState GetGameState() const { return gameState_; }

  //! @brief Used by scene transitions to control progression.
  //! In headless mode, scene transitions are disabled, so no outside game transitions are allowed.
  void RequestGameState(const TGameState gameState) const
  {
#ifdef RLPLAYS_EDITOR
    if (gameState == TGameState::EditorMode) { gameState_ = gameState; }
#endif
    gameState_ = gameState;
  }

  void Setup(std::shared_ptr<TContext> context)
  {
    PlayerProgress.resize(2, {});
    gameState_ = TGameState::StartGame;
    CurrentMenuInput = TPlayerAction::None;
  }

  std::string GetRoundName(int round) const
  {
    return "Round " + std::to_string(round);
  }

  std::string GetGameType(int round) const
  {
    std::string name;
    if (round == 1) { name = "You - Single Player"; }
    else { name = "You vs Your Time Ghost"; }
    return name;
  }


  [[nodiscard]] static const char* GetGameTypeStr(const TGameType gameType)
  {
    switch (gameType)
    {
    case TGameType::SinglePlayer: return "SinglePlayer";
    case TGameType::PlayerVsPrior: return "PlayerVsPrior";
    case TGameType::PlayerVsAI: return "PlayerVsAI";
    case TGameType::PlayerVsPlayer: return "PlayerVsPlayer";
    case TGameType::PriorVsPlayer: return "PriorVsPlayer";
    case TGameType::AIVsPlayer: return "AIVsPlayer";
    default: return "Unknown";
    }
  }


  static bool IsAIAgent() { return true; }
  
  //gameType == TGameType::PlayerVsAI || gameType == TGameType::AIVsPlayer; }
  [[nodiscard]] static TGameType GetInitialGameType()
  {
    if (IsAIAgent()) { return TGameType::PlayerVsAI; }
    else { return TGameType::SinglePlayer; }
  }


  [[nodiscard]] TGameType GetNextGameType() const
  {
    if (GetActivePlayerId() == GetWinningPlayerId())
    {
      // We won, so proceed to the next level.
      if (CurrentRound % 2 == 0) return IsAIAgent() ? TGameType::PlayerVsAI : TGameType::PlayerVsPrior;
      return IsAIAgent() ? TGameType::AIVsPlayer : TGameType::PriorVsPlayer;
    }
    return GameType;
  }

  [[nodiscard]] int UpdateNextRound()
  {
    int nextRound = CurrentRound;
    if (GetActivePlayerId() == GetWinningPlayerId())
    {
      ++nextRound;
      if (InfiniteRounds == false && nextRound > MaxRounds) { FinishedAllRounds = true; }
    }
    if (!InfiniteRounds) { nextRound = std::min(MaxRounds, nextRound); }
    return nextRound;
  }

  //! @brief Returns the player state.
  TPlayerState GetActivePlayerState() const { return GetPlayerState(GetActivePlayerId()); }

  //! @brief Returns a readonly reference to the active player's progress.
  const TPlayerProgress& GetActivePlayerProgress() const { return GetPlayerProgress(GetActivePlayerId()); }
  const TPlayerProgress& GetInactivePlayerProgress() const { return GetPlayerProgress(GetInactivePlayerId()); }

  bool CheckFinishedAllRounds() const { return CurrentRound >= MaxRounds && FinishedAllRounds && !InfiniteRounds; }

  //! @brief Checks if the given action and for the current state if we should proceed to gameplay or track the menu input action.
  [[nodiscard]] bool CheckGameMenuAction(const TPlayerAction action)
  {
    CurrentMenuInput = action;
    if (gameState_ == TGameState::AboutToRun || gameState_ == TGameState::MenuDismissing) { return false; }
    if (gameState_ == TGameState::MenuDismissedBeforeRunning) { return HasAnyEnumValue(action); }
    return true;
  }

private:
  //! @brief Sets whether the game has started receiving actions (to transition from AboutToRun to RunningGame).
  //! {@returns true} if there is a change in action started.
  [[nodiscard]] bool SetActionsStarted(bool hasActions)
  {
    if (hasActions != HasActions)
    {
      HasActions = hasActions;
      return true;
    }
    return false;
  }

  void SetGameState(TGameState gameState) { gameState_ = gameState; }

  // Allow this be the only one changeable once constructed.
  mutable TGameState gameState_ = TGameState::StartGame;

  //! @brief Returns the next state based on the provided previous state.
  //! If running in 'headless' or 'speed-run' modes (training, conversion, testing ,etc),
  //! This will simply and immediately transition to the next available state.
  //! State transitions/animations are updated by TProgressBlock if ShowSceneTransitions is true.
  [[nodiscard]] TGameState UpdateNextState()
  {
    if (gameState_ == TGameState::MenuDismissedBeforeRunning && (HasActions || !AllowTransitions()))
    {
      gameState_ = TGameState::RunningGame;
    }

    auto timeRemaining = TimeLimit.TimeRemaining();

    // Check timeout.
    if (TimeLimit.IsValid())
    {
      if (timeRemaining <= 0) { SetPlayerState(GetActivePlayerId(), TPlayerState::TimeOut); }
    }
    // Update active player state if they are still alive.
    if (GetPlayerState(GetActivePlayerId()) == TPlayerState::Alive)
    {
      if (GetPlayerState(GetInactivePlayerId()) == TPlayerState::Won)
      {
        // Inactive Player won, so Active Player is now timed out.
        SetPlayerState(GetActivePlayerId(), TPlayerState::TimeOut);
      }
    }

    if (!AllowTransitions() && gameState_ == TGameState::StartGame) { gameState_ = TGameState::RunningGame; }

    if (IsDone() && gameState_ == TGameState::RunningGame) { gameState_ = TGameState::StoppingGame; }

    if (!AllowTransitions() && gameState_ == TGameState::StoppingGame) { gameState_ = TGameState::StopGame; }

    if (AllowTransitions()) { return gameState_; }
    switch (gameState_)
    {
    case TGameState::StartGame: return TGameState::RunningGame;
    case TGameState::StoppingGame: return TGameState::StopGame;

    case TGameState::PauseGame:
    case TGameState::AboutToRun:
    case TGameState::MenuDismissedBeforeRunning: { return (gameState_); }

    case TGameState::StopGame:
    case TGameState::RunningGame:
    case TGameState::EditorMode:
    case TGameState::MenuDismissing:
    case TGameState::StoppingSequence: { break; }
    }
    return gameState_;
  }

  //! @brief The player block can set its own state.
  void SetPlayerState(const int playerId, const TPlayerState state)
  {
    if (playerId == PlayerId1) { PlayerProgress[0].PlayerState = state; }
    else if (playerId == PlayerId2) { PlayerProgress[1].PlayerState = state; }
  }

  //! @brief Whether there is a progress block with scene transitions (AND we are not in headless mode).
  inline bool AllowTransitions() const { return (!THeadless::IsSkipInputMode()) && this->ShowSceneTransitions; }

  // Things could go wrong if we expose this struct everywhere, so make the state changes
  // 'private' to a few special structs. It's a hacky thing but helps us be sane.
  friend struct TGame;
  friend struct TContext;
  friend struct TPlayerBlock;
  friend struct TEditor;
};
}
