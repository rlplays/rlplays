#pragma once
#include <memory>
#include <string>
#include <mutex>
#include "actions.h"
#include "log.h"
#include "world_fileset.h"

#include "game_actions.h"
#include "game_info.h"
namespace RLPlays
{
struct ABlock;
struct TGame;
struct TGameActions;

// NOTE: This file is C-like API to ensure all C++ code/object lifecycle is hidden from the highest
//       level of the game code/logic.

//! @brief Struct holding onto a game instance that might change as we progress through rounds or update
//! or create maps through the editor.
struct TGameInfo
{
  // Caller can hold on to the game info, while the editor/gameplay code can update the game/world
  // especially as we progress through rounds.
  std::shared_ptr<TGame> Game;
  std::string WorldFilename;
  std::shared_ptr<TRLTrain> RLTrain;
#if DEBUG
  TDebugGameInfo DebugInfo;
#endif
};



//! @brief Default FPS ensures we can track frames recording/replay in a stable manner.
//! Also allows the game loop to run (without rendering anything) as fast as possible while
//! still respecting the FPS.
constexpr int DEFAULT_FPS = 60;

//! @brief Loads the game with the world specified by the filename in the default dir.
//! \param fps The frames per second to set for the game.
[[nodiscard]] TGameInfo LoadGame(const TWorldFiles& worldFileset, TGameLoadInfo& gameLoadInfo);

//! @brief Loads the same game as before but with replay actions. This is where the fun begins.
void ReloadGame(TGameInfo& gameInfo, int fps, const std::shared_ptr<TGameActions>& actions = nullptr);

//! @brief Saves the current game world to the filename specified by the world.
void SaveGame(const TGameInfo& gameInfo);

//! @brief Unloads the game world and the resources, stops drawing/updating here on.
void UnloadGame(TGameInfo& gameInfo);

//! @brief Draws the game world for the current timestep.
void DrawFrame(const TGameInfo& gameInfo);

//! @brief Loads the content (textures, sounds etc) for the game world.
//! For headless mode, may jot down the files instead of loading them.
void LoadContent(const TGameInfo& gameInfo);

//! @brief Updates the game world and forwards the time by 1 frame (or many if AddFrameSkip is called).
void UpdateFrame(const TGameInfo& gameInfo);

//! @brief Handles input actions (enum mask) for the active player.
void HandleInput(const TGameInfo& gameInfo, TPlayerAction actions);

//! @brief Skips the {@param numFrames} frames each UpdateFrame call.
//! Can be zero (no update called), negative (slow-mo) or positive (fast-forward).
void AddFrameSkip(const TGameInfo& gameInfo, int numFrames);

//! @brief Setup RL based on game type and if needed, loads the weights based on the provided training config in `gameInfo.RLTrain`.
void SetupRL(const TGameInfo& gameInfo);

#ifdef RLPLAYS_EDITOR
//! @brief Reloads the game world.
void ResetGame(const TGameInfo& gameInfo);
#endif

struct TPlayerInputs
{
  TPlayerAction Action = TPlayerAction::None;
  bool IsReset = false;
};

//! @brief Gets the current gamepad id (or -1 if unavailable).
int GetGamepadId();

//! @brief Maps the actions for the current frame to TPlayerAction enum.
TPlayerInputs MapCurrentActions();

std::shared_ptr<TGameActions> GetPlayerActions(const TGameInfo& gameInfo);

void TogglePause(const TGameInfo& gameInfo);
void SetDebug(const TGameInfo& gameInfo);

std::string GetGameDesc(const TGameInfo& gameInfo);

} // namespace RLPlays


