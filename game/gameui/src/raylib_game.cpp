/*******************************************************************************************
 *
 *   raylib game template
 *
 *   RLPlays Game
 *   A playable 2D pixel platformer game with an editor.
 *   Mainly to explore and exploit RL.
 *
 *   This game has been created using raylib (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Copyright (c) 2021 Ramon Santamaria (@raysan5)
 *   Copyright (c) 2025 Perumaal Shanmugam (@perumaal_s)
 *
 ********************************************************************************************/


#include "main_game.h"
#include "raylib.h"
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif
#if RLPLAYS_EDITOR
#include <base_editor.h>
#endif
#include <world_fileset.h>
#include <raylib_utils.h>
#include <game.h>
using namespace RLPlays;

static void UpdateDrawFrame(); // Update and draw one frame
static bool shouldFinish = false;
static bool shouldRandomize = false;

static TGameInfo GAME_INFO;

int main(int argc, char** argv)
{
  srand(clock());
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  Vector2 monitorSize;
  Vector2 windowSize;
  GetWindowSize(monitorSize, windowSize);

  const auto worldFiles = TWorldFiles::Load();
  const auto selectedFilename = worldFiles->SelectedFile.Filename;

  DisplayProgramInfo();
  InitWindow(windowSize.x, windowSize.y, "RLPlays Game");

  InitAudioDevice();
  SetTargetFPS(worldFiles->FPS);

  RLPlays::SetupGlobal();
  TLOG(LOG_INFO, "Found %d world files, loading %s @ %d FPS", worldFiles->Files.size(),
    selectedFilename.c_str(), worldFiles->FPS);
  TGameLoadInfo loadInfo = {
    .Filename = worldFiles->SelectedFile.Filename, 
    .GameType = TGameProgress::GetInitialGameType(),
    .UseCachedData = true,
  };
  GAME_INFO = LoadGame(*worldFiles, loadInfo);
  SetupRL(GAME_INFO);

  const std::string windowName = "RLPlays Game: " + GetGameDesc(GAME_INFO);
  // TODO(perumaal): Set icon
  SetWindowTitle(windowName.c_str());

#if defined(PLATFORM_WEB)
  // TODO(perumaal): Must handle FPS inside the game loop and skip frames if FPS is too high (but we won't handle
  //       cases <60fps).
  // For now, set FPS = 0 to use requestAnimationFrame/setTimeout with the browser's refresh rate.
  emscripten_set_main_loop(UpdateDrawFrame, /*fps*/0, /*simulate_infinite_loop*/ true);
#else

  while (!WindowShouldClose() && !shouldFinish)
  {
    UpdateDrawFrame();
  }
#endif


  UnloadGame(GAME_INFO);
  CloseAudioDevice();
  CloseWindow();

  // TWorldFiles::Load()->Save();
  return 0;
}


// Update and draw game frame
static void UpdateDrawFrame()
{
  if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q))
  {
    shouldFinish = true;
    return;
  }

  bool isEditor = false;
  const auto [actions, IsReset] = MapCurrentActions();
#if RLPLAYS_EDITOR
  isEditor = RLPlays::IsEditorOpen();
#endif
  if (!isEditor)
  {
    HandleInput(GAME_INFO, actions);
#if RLPLAYS_EDITOR || DEBUG
    if (IsKeyReleased(KEY_R) || IsReset)
    {
      // Reset the game if R is pressed.
      UnloadGame(GAME_INFO);
      ReloadGame(GAME_INFO, DEFAULT_FPS);
      if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
      {
        shouldRandomize = true;
      }
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_K) || IsKeyReleased(KEY_L))
    {
      // Replay the actions.
      const auto data = GetPlayerActions(GAME_INFO);
      UnloadGame(GAME_INFO);

      ReloadGame(GAME_INFO, DEFAULT_FPS, data);
    }

    // Frame skip shortcuts
    if (IsControlKeyDown() && IsKeyReleased(KEY_F)) { AddFrameSkip(GAME_INFO, 1); }
    if (IsControlKeyDown() && IsKeyReleased(KEY_G)) { AddFrameSkip(GAME_INFO, -1); }
    if (IsControlKeyDown() && IsKeyReleased(KEY_H)) { AddFrameSkip(GAME_INFO, -1000000); }
    if (IsControlKeyDown() && IsKeyReleased(KEY_P)) { TogglePause(GAME_INFO); }
#if DEBUG
    if (IsControlKeyDown() && IsKeyReleased(KEY_ONE))
    {
      GAME_INFO.DebugInfo.ShowDebugView = !GAME_INFO.DebugInfo.ShowDebugView;
      SetDebug(GAME_INFO);
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_TWO))
    {
      GAME_INFO.DebugInfo.ShowGhostActions = !GAME_INFO.DebugInfo.ShowGhostActions;
      if (GAME_INFO.DebugInfo.ShowGhostActions) { GAME_INFO.DebugInfo.ShowDebugView = true; }
      ReloadGame(GAME_INFO, DEFAULT_FPS);
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_THREE))
    {
      GAME_INFO.DebugInfo.ShowRLViz = !GAME_INFO.DebugInfo.ShowRLViz;
      SetDebug(GAME_INFO);
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_FOUR))
    {
      GAME_INFO.DebugInfo.RLControlMainPlayer = !GAME_INFO.DebugInfo.RLControlMainPlayer;
      printf("Debug: RLControlMainPlayer = %d\n", GAME_INFO.DebugInfo.RLControlMainPlayer ? 1 : 0);
      ReloadGame(GAME_INFO, DEFAULT_FPS);
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_LEFT_BRACKET))
    {
      auto rlTrain = TWorldFiles::Load()->RLTrain;
      int maxIndex = (rlTrain && !rlTrain->Weights.empty()) ? static_cast<int>(rlTrain->Weights.size()) - 1 : 0;
      TRLTrain::WEIGHT_FILE_INDEX = (TRLTrain::WEIGHT_FILE_INDEX > 0) ? TRLTrain::WEIGHT_FILE_INDEX - 1 : maxIndex;
      printf("Debug: WEIGHT_FILE_INDEX = %d\n", TRLTrain::WEIGHT_FILE_INDEX);
      ReloadGame(GAME_INFO, DEFAULT_FPS);
    }
    if (IsControlKeyDown() && IsKeyReleased(KEY_RIGHT_BRACKET))
    {
      auto rlTrain = TWorldFiles::Load()->RLTrain;
      int maxIndex = (rlTrain && !rlTrain->Weights.empty()) ? static_cast<int>(rlTrain->Weights.size()) - 1 : 0;
      TRLTrain::WEIGHT_FILE_INDEX = (TRLTrain::WEIGHT_FILE_INDEX < maxIndex) ? TRLTrain::WEIGHT_FILE_INDEX + 1 : 0;
      printf("Debug: WEIGHT_FILE_INDEX = %d\n", TRLTrain::WEIGHT_FILE_INDEX);
      ReloadGame(GAME_INFO, DEFAULT_FPS);
    }
#endif // #if DEBUG
#endif // #if RLPLAYS_EDITOR || DEBUG

    if (shouldRandomize)
    {
      TBlockUtils::SetPlayerStartPos(GAME_INFO.Game->Context,
        TBlockUtils::RandomizePlayerStartPos(GAME_INFO.Game->Context));
      shouldRandomize = false;
    }

    UpdateFrame(GAME_INFO);
  }

  BeginDrawing();

  ClearBackground(BLACK);

  DrawFrame(GAME_INFO);

#if RLPLAYS_EDITOR
  GAME_INFO = RLPlays::HandleEditor(GAME_INFO);
#endif

  EndDrawing();
}
