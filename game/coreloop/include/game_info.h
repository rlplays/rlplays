#pragma once
#include "base_types.h"
#include "game_actions.h"

namespace RLPlays
{

// TODO(perumaal): I am not happy with this level of indirection, but it is what it is.
//                 This mainly helps me avoid the cyclical deps as well as having to pollute the `TGame*` classes
//                 and importantly help the RL training code carry the levels across curriculum ladder and during 
//                 self-play.

//! @brief Tracks progress across rounds as the game proceeds (and instances of {@class TGame} change).
struct TGameLoadInfo
{
  std::string Filename;
  int Round = 1;
  std::shared_ptr<TGameActions> ReplayActions = nullptr;
  std::string NextProgressText1;
  std::string NextProgressText2;
  std::string NextProgressText3;
  std::string NextProgressText4;
  TPlayerState LastPlayerState;
  bool FinishedAllRounds = false;
  TGameType GameType = TGameType::SinglePlayer;
  int PreviousRound = 1;
  bool ShowMenuAnim = true;
  std::shared_ptr<TRLTrain> RLTrain = nullptr;

  //! @brief If true, then stores cached data in the returned game info for future use.
  bool UseCachedData = false;
  json Data;
  TWorldFile* WorldFile = nullptr;
#if DEBUG
  TDebugGameInfo DebugInfo;
#endif
};
}
