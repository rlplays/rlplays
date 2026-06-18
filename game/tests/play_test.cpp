#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
#include <iostream>

#include "world_fileset.h"
#include "game.h"

using namespace RLPlays;

inline TPlayerAction GetPlayerAction(const int actionVal)
{
  // Avoid none for now.
  switch (actionVal + 1)
  {
  case 0: return TPlayerAction::None;
  case 1: return TPlayerAction::WalkLeft;
  case 2: return TPlayerAction::WalkRight;
  case 3: return TPlayerAction::Jump;
  case 4: return TPlayerAction::Use;
  case 5: return TPlayerAction::Activate;
  case 6: return TPlayerAction::Crouch;
  default: return TPlayerAction::None;
  }
}


TEST(PlayTest, GameTest)
{
  auto fileset = TWorldFiles::Load(DefaultWorldFilesetName);
  int maxFileIndex = fileset->Files.size();
  int maxTestActions = 100;
#if DEBUG
  // Just test 2 files if we are debugging the tests - otherwise, it takes a lot of time.
  maxFileIndex = 1;
  maxTestActions = 0;
#endif
  for (int file = -1; file < maxFileIndex; ++file)
  {
    auto filename = fileset->GetFile(file);
    if (filename.find("jump_enemy") != std::string::npos)
    {
      std::cout << "Skipping test for: " << filename << " as switching player sides changes replay state!\n";
      continue;
    }
    std::cout << "Testing: " << filename << "\n";
    for (int a = 0; a <= MAX_NUM_ACTIONS + maxTestActions; ++a)
    {
      TGameLoadInfo loadInfo = {.Filename = filename};
      auto gameInfo = LoadGame(*fileset, loadInfo);
      auto ctx = gameInfo.Game->Context;
      std::vector<Rectangle> box1s = {};
      std::vector<TPlayerAction> actionsList = {};
      for (int i = 0; i < 100; ++i)
      {
        TPlayerAction action = GetPlayerAction((a >= MAX_NUM_ACTIONS) ? (rand() % MAX_NUM_ACTIONS) : (a));
        HandleInput(gameInfo, action);
        UpdateFrame(gameInfo);
        box1s.push_back(ctx->GetActionsHandlerBlock()->Box);
        actionsList.push_back(action);
      }

      auto replayActions = GetPlayerActions(gameInfo);
      //EXPECT_EQ((a) == (int)TPlayerAction::None, (replayActions == nullptr))
      //    << actionsList.size() << " / "
      //    << (replayActions == nullptr ? "no-replay-actions" : replayActions->Filename)
      //    << "\n";
      if (replayActions == nullptr) continue;
      loadInfo = {
        .Filename = filename, .Round = 2, .ReplayActions = replayActions,
        .NextProgressText1 = "", .NextProgressText2 = "", .NextProgressText3 = "", .NextProgressText4 = "",
        .LastPlayerState = TPlayerState::Won, .FinishedAllRounds = false, .GameType = TGameType::PriorVsPlayer,
        .PreviousRound = 1, .ShowMenuAnim = true, .RLTrain = nullptr, .UseCachedData = false
      };
      gameInfo.Game->LoadFile(loadInfo);
      ctx = gameInfo.Game->Context;
      Rectangle box2 = {};
      for (int i = 0; i < 100; ++i)
      {
        UpdateFrame(gameInfo);
        box2 = ctx->GetReplayHandlerBlock()->Box;
        EXPECT_TRUE(AreRectsSame(box1s[i], box2)) << RectStr(box1s[i]) << " vs " << RectStr(box2) << "\n";
      }
      //std::cout << "Actions count " << replayActions->FrameActions.size() << " final box " << RectStr(box2) << "\n";
    }
  }
}

TEST(PlayTest, CacheTest)
{
  auto fileset = TWorldFiles::Load(DefaultWorldFilesetName);
  TGameLoadInfo loadInfo = {.Filename = fileset->SelectedFile.Filename, .UseCachedData = true};

  for (int i = 0; i < 100; i++)
  {
    auto gameInfo = LoadGame(*fileset, loadInfo);
    auto ctx = gameInfo.Game->Context;
    auto numCells = gameInfo.Game->World->GetBlocksTotalCells(ctx);
    UnloadGame(gameInfo);
  }
}
