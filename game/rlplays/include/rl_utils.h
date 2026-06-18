#pragma once
#include <serialize.h>

#include <context.h>
#include <timer.h>
#include <game_types.h>
#include <game.h>
#include <rl_env.h>
#include <base_block.h>
#include "config.h"
#include <actions.h>

namespace RLPlays
{
// Used by the converter/test - but not directly by the RL code itself.
inline bool IsValidRLPlays(const std::shared_ptr<TGame>& game)
{
  //if (game->World->WorldInfo.GameProgress.MaxNumRewards == 0) return false;
  // if (game->World->WorldInfo.GameProgress.TimeLimit.TimeSet <= 0) return false;

  //bool rewardFound = false;
  bool goalFound = false;
  int numPlayers = 0;
  for (const auto& tblock : game->World->Blocks)
  {
    auto block = tblock.Block;
    // if (block->BlockType == TBlockType::Reward) { rewardFound = true; }
    if (block->BlockType == TBlockType::Goal) { goalFound = true; }
    if (block->BlockType == TBlockType::Player) { ++numPlayers; }
  }
  return (goalFound && numPlayers >= 1);
}

//! @brief Contains the initial configuration to train RLPlays.
inline std::string ConfigToTrainFilepath() { return GetRootGameDir() + "/rlplays/rlplays.ini"; }

inline void FillRLStuff(const std::shared_ptr<TGame>& game, const std::shared_ptr<TRLTrain>& rlTrain,
  TWorldFile* rlFile)
{
  const auto world = game->World;
  const auto numCells = world->GetBlocksTotalCells(game->Context);
  if (rlFile->SupportsRLTraining)
  {
    // Add some slop so we can allow for dynamic cells (spawnable items like weapons/bullets/items etc).
    rlTrain->MaxNumCells = std::max(rlTrain->MaxNumCells, int(((float)numCells) * 1.5f));
  }
  const auto& worldInfo = game->World->WorldInfo;
  const auto& context = game->Context;
  if (rlFile->SupportsRLTraining && worldInfo.GameProgress.HasMirrorMode &&
    (context->GetActionsHandlerBlock() != nullptr) &&
    (context->GetReplayHandlerBlock() != nullptr))
  {
    rlFile->NumSelfPlayTraining = std::max(rlFile->NumSelfPlayTraining, 0);
    rlFile->SupportsSelfPlay = true;
  }
  else
  {
    rlFile->NumSelfPlayTraining = 0;
    rlFile->SupportsSelfPlay = false;
  }

  rlTrain->MaxNumCells = std::min(rlTrain->MaxNumCells, rlTrain->CapMaxCells);

  rlFile->Filename = world->WorldInfo.Filename;

  rlFile->NumCells = numCells;
  rlFile->NumCellTypes = world->GetBlocksTotalCellTypes(game->Context);

  auto config = TConfig(ConfigToTrainFilepath());
  config
      .SetInt("env", "num_obs", GetNumObs(rlTrain))
      .SetInt("env", "width", world->WorldInfo.Camera.Viewport.width)
      .SetInt("env", "height", world->WorldInfo.Camera.Viewport.height)
      .SetInt("env", "num_actions", MAX_NUM_ACTIONS)
      .SaveFile();
}
}
