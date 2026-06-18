#include <game.h>
#include <iostream>

#include <memory>
#include <raylib.h>
#include <utility>

using namespace RLPlays;


namespace RLPlays
{
TGameInfo LoadGame(const TWorldFiles& worldFileset, TGameLoadInfo& gameLoadInfo)
{
  const auto game = std::make_shared<TGame>(worldFileset.FPS);
  gameLoadInfo.WorldFile = worldFileset.GetWorldFileRef(gameLoadInfo.Filename);
  gameLoadInfo.RLTrain = worldFileset.RLTrain;
  game->LoadFile(gameLoadInfo);
  return TGameInfo{.Game = game, .WorldFilename = gameLoadInfo.Filename, .RLTrain = worldFileset.RLTrain};
}

void ReloadGame(TGameInfo& gameInfo, int fps, const std::shared_ptr<TGameActions>& actions)
{
  if (gameInfo.WorldFilename.empty()) { TLOG(TERROR, "No world filename set. Cannot reload game."); }

  int round = 1;
  if (gameInfo.Game != nullptr) { round = gameInfo.Game->GetRound(); }
  gameInfo.Game = std::make_shared<TGame>(fps);
  TGameLoadInfo loadInfo = {
      .Filename = gameInfo.WorldFilename,
      .Round = round,
      .ReplayActions = actions,
      .RLTrain = gameInfo.RLTrain,

  };
#if DEBUG
  loadInfo.DebugInfo = gameInfo.DebugInfo;
#endif
  gameInfo.Game->LoadFile(loadInfo);
}

void LoadContent(const TGameInfo& gameInfo) { gameInfo.Game->Context->LoadContent(gameInfo.Game->Context); }


void SaveGame(const TGameInfo& gameInfo)
{
  gameInfo.Game->SaveFile(DefaultDir + gameInfo.Game->World->WorldInfo.Filename);
}


void UpdateFrame(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->Tick();
}

#ifdef RLPLAYS_EDITOR
void ResetGame(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->ResetGame(true);
}


#endif


// Set axis deadzones
constexpr float leftStickDeadzoneX = 0.2f;
constexpr float leftStickDeadzoneY = 0.1f;
constexpr float rightStickDeadzoneX = 0.1f;
constexpr float rightStickDeadzoneY = 0.1f;
constexpr float leftTriggerDeadzone = -0.9f;
constexpr float rightTriggerDeadzone = -0.9f;

// From config.h:
#define MAX_GAMEPADS 4
int GetGamepadId()
{
  for (int i = 0; i < MAX_GAMEPADS; ++i)
  {
    if (IsGamepadAvailable(i)) { return i; }
  }
  return -1;
}


TPlayerInputs MapCurrentActions()
{
  bool isReset = false;
  TPlayerAction actions = TPlayerAction::None;
  // Keyboard events
  if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) { actions = AddEnumValue(actions, TPlayerAction::Jump); }
  if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { actions = AddEnumValue(actions, TPlayerAction::WalkLeft); }
  if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { actions = AddEnumValue(actions, TPlayerAction::WalkRight); }
  if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) { actions = AddEnumValue(actions, TPlayerAction::Crouch); }
  if (IsKeyReleased(KEY_E)) { actions = AddEnumValue(actions, TPlayerAction::Activate); }
  if (IsKeyReleased(KEY_SPACE)) { actions = AddEnumValue(actions, TPlayerAction::Use); }
  if (IsKeyReleased(KEY_ENTER)) { actions = AddEnumValue(actions, TPlayerAction::GameStart); }
  if (IsKeyReleased(KEY_TAB) || IsKeyReleased(KEY_C)) { actions = AddEnumValue(actions, TPlayerAction::GameMenu); }


  auto gamepad = GetGamepadId();
  // Gamepad events.
  if (gamepad >= 0 && IsGamepadAvailable(gamepad))
  {
    // DrawText(TextFormat("GP%d: %s", gamepad, GetGamepadName(gamepad)), 10, 10, 10, BLACK);

    // Get axis values
    float leftStickX = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
    float leftStickY = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
    float rightStickX = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X);
    float rightStickY = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y);
    float leftTrigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);
    float rightTrigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER);

    // Calculate deadzones
    if (leftStickX > -leftStickDeadzoneX && leftStickX < leftStickDeadzoneX) leftStickX = 0.0f;
    if (leftStickY > -leftStickDeadzoneY && leftStickY < leftStickDeadzoneY) leftStickY = 0.0f;
    if (rightStickX > -rightStickDeadzoneX && rightStickX < rightStickDeadzoneX) rightStickX = 0.0f;
    if (rightStickY > -rightStickDeadzoneY && rightStickY < rightStickDeadzoneY) rightStickY = 0.0f;
    if (leftTrigger < leftTriggerDeadzone) leftTrigger = -1.0f;
    if (rightTrigger < rightTriggerDeadzone) rightTrigger = -1.0f;

    if (leftStickX < 0 || rightStickX < 0) { actions = AddEnumValue(actions, TPlayerAction::WalkLeft); }
    if (leftStickX > 0 || rightStickX > 0) { actions = AddEnumValue(actions, TPlayerAction::WalkRight); }
    if (leftStickY > 0 || rightStickY > 0) { actions = AddEnumValue(actions, TPlayerAction::Crouch); }
    if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))
    {
      actions = AddEnumValue(actions, TPlayerAction::Activate);
    }
    if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
    {
      actions = AddEnumValue(actions, TPlayerAction::Jump);
      actions = AddEnumValue(actions, TPlayerAction::GameStart);
    }
    if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
    {
      actions = AddEnumValue(actions, TPlayerAction::Use);
    }
    if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP))
    {
      actions = AddEnumValue(actions, TPlayerAction::Activate);
    }

    if (IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
        IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
    {
      actions = AddEnumValue(actions, TPlayerAction::GameMenu);
    }
  }

  return {actions, isReset};
}


std::shared_ptr<TGameActions> GetPlayerActions(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return nullptr; }
  return gameInfo.Game->Context->GetActionsReplay(gameInfo.Game->Context);
}


void HandleInput(const TGameInfo& gameInfo, const TPlayerAction actions) { gameInfo.Game->HandleActions(actions); }

void AddFrameSkip(const TGameInfo& gameInfo, const int numFrames)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->NumFrameSkips += numFrames;
}

void TogglePause(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->TogglePause();
}

#if DEBUG
void SetDebug(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->Context->SetDebug(gameInfo.DebugInfo);
}
#endif

void DrawFrame(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) return;
  gameInfo.Game->Draw();
}

std::string GetGameDesc(const TGameInfo& gameInfo) { return gameInfo.Game->World->WorldInfo.Desc; }

void UnloadGame(TGameInfo& gameInfo)
{
  if (gameInfo.Game == nullptr) return;
  if (!gameInfo.Game->IsValid()) return;
  gameInfo.Game->Unload();
  gameInfo.Game = nullptr;
}

//! @brief Root game/ dir (if it exists) e.g. /blah/rlplays/game without the trailing forward slash.
std::string ROOT_GAME_DIR_;

//! @brief Caches the data/ dir.
std::string DATA_DIR_;
//! @brief Caches the data/game dir.
std::string DATA_GAME_DIR_;
//! @brief Caches the data/game/worlds dir.
std::string DATA_WORLDS_DIR_;

//! @brief Protects access to the above data dir strings.
// Using a recursive_mutex as its reentrant in the same thread: I guess the mutex, by default,
// favors performance, but it's a footgun (similar to in Go, but dissimilar to Java/C#).
std::recursive_mutex DATA_DIR_MUTEX_;
//! @brief The root game/ directory (if it exists) e.g. /blah/rlplays/game without the trailing forward slash.
//!        On Web, returns "".
const std::string& GetRootGameDir()
{
  std::lock_guard lock(DATA_DIR_MUTEX_);
  if (ROOT_GAME_DIR_.empty())
  {
    std::string dir = GetWorkingDirectory();

    // Search for both slash variants
    std::string gameDirLinux = "/game";
    std::string gameDirWin = "\\game";
    size_t pos1 = dir.find(gameDirLinux);
    size_t pos2 = dir.find(gameDirWin);

    size_t pos = (pos1 != std::string::npos) ? pos1 : ((pos2 != std::string::npos) ? pos2 : std::string::npos);

    if (pos != std::string::npos)
    {
      dir = dir.substr(0, pos + gameDirLinux.size());
      return (ROOT_GAME_DIR_ = dir);
    }
  }
  return ROOT_GAME_DIR_;
}

//! @brief The data (rlplays/game/data) directory path for our game (works on *Nix/Windows) with the trailing forward
//! slash.
const std::string& GetDataDir()
{
  std::lock_guard lock(DATA_DIR_MUTEX_);
  if (DATA_DIR_.empty())
  {
    std::string dir = GetRootGameDir();
    if (dir.empty())
    {
      // Typically used on Web builds, as we don't have a root game dir.
      dir = "resources/";
    }
    else
    {
      // Works on both Win/*Nix (i.e. c:\foo\game\data/blah.json is a valid path on Windows).
#if RLPLAYS_EDITOR || RLPLAYS_CONVERTER || RLPLAYS_TRAIN || RLPLAYS_TEST
      // Use all private content/data for our local editor/converter builds.
      // Although all the data is CC0, to prevent abuse/re-hosting, we keep the data private to ourselves.
      dir += "/editor/alldata/";
#else
      // Used by public/web builds with minimal data/content.
      dir += "/data/";
#endif
    }
    TLOG(TINFO, "Using data dir: %s (pwd: %s)", dir.c_str(), ::GetWorkingDirectory());
    DATA_DIR_ = dir;
  }
  return DATA_DIR_;
}


//! @brief The game directory path for our game (works on *Nix/Windows).
const std::string& GetGameLevelsDir()
{
  std::lock_guard lock(DATA_DIR_MUTEX_);
  if (DATA_GAME_DIR_.empty())
  {
    DATA_GAME_DIR_ = GetDataDir() + GAME_LEVELS_SUBDIR;
    TLOG(TINFO, "Using data dir: %s", DATA_GAME_DIR_.c_str());
  }
  return DATA_GAME_DIR_;
}

const std::string& GetWorldsDir()
{
  std::lock_guard lock(DATA_DIR_MUTEX_);
  if (DATA_WORLDS_DIR_.empty())
  {
    DATA_WORLDS_DIR_ = GetDataDir() + "worlds/";
    TLOG(TINFO, "Using worlds dir: %s", DATA_WORLDS_DIR_.c_str());
  }
  return DATA_WORLDS_DIR_;
}

void SetupRL(const TGameInfo& gameInfo)
{
  if (!gameInfo.Game->IsValid()) { return; }
  gameInfo.Game->Context->SetupRL(gameInfo.Game->Context, gameInfo.RLTrain);
}
} // namespace RLPlays
