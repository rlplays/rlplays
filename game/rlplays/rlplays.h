#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "main_game.h"
#include "rl_env.h"
#include <raylib_utils.h>
#include <world_fileset.h>

using namespace RLPlays;

// Required function. Should handle creating the client on first call
void c_render(RLPlaysEnv* env)
{
  if (env->client == NULL)
  {
    env->render_supported = 1;
    THeadless::IsHeadless = false;
    Vector2 monitorSize;
    Vector2 windowSize;
    GetWindowSize(monitorSize, windowSize);
    InitWindow(windowSize.x, windowSize.y, "RLPlays");

    env->client = new Client();
    env->rl_fileset = TWorldFiles::Load();
    ResetForRender(env);
    SetTargetFPS(env->rl_fileset->FPS);
    c_reset(env);
  }

  // Standard across our envs so exiting is always the same
  if (IsKeyDown(KEY_ESCAPE)) { exit(0); }
  if (IsKeyReleased(KEY_R)) { c_reset(env); }

  auto gameInfo = env->game_info;
  if (gameInfo != nullptr && gameInfo->Game != nullptr)
  {
    BeginDrawing();
    {
      ClearBackground(BLACK);
      DrawFrame(*gameInfo);
    }

    EndDrawing();
  }
}
