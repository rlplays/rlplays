#include "game.h"
#include <fstream>
#include <iostream>
#include <log.h>

namespace RLPlays
{
TGame::TGame(int fps) : Context(std::make_shared<TContext>(fps)), NumFrameSkips(1) {}

static bool IsDead(const TPlayerState playerState)
{
  return playerState == TPlayerState::Dead || playerState == TPlayerState::TimeOut;
}


void TGame::Tick()
{
  TGameProgress* progress = &World->WorldInfo.GameProgress;

  auto gameState = progress->UpdateNextState();
  if (gameState == TGameState::StopGame) { return; }

  if (gameState == TGameState::RunningGame)
  {
    if (NumFrameSkips >= 0)
    {
      for (int i = 0; i < NumFrameSkips; ++i)
      {
        Context->UpdateFrame(Context);
      }
    }
    else
    {
      // Assume negative frame skips means 'slow-mo' i.e. hold the current frame for -NumFrameSkips frames..
      if (++frameSkips_ >= -NumFrameSkips)
      {
        Context->UpdateFrame(Context);
        frameSkips_ = 0;
      }
    }
  }
  gameState = progress->UpdateNextState();
  if (progress->ShouldReloadGame)
  {
    AutoContinue_(false);
  }
}

void TGame::AutoContinue_(const bool goToNextRound)
{
  { // Unload the current world and load the next one.
    auto& progress = World->WorldInfo.GameProgress;
    const auto prevRound = progress.CurrentRound;
    const auto winningPlayerId = progress.GetWinningPlayerId();
    const auto lastFilename = World->WorldInfo.Filename;
    const auto nextRound = goToNextRound ? progress.UpdateNextRound() : progress.CurrentRound;
    std::shared_ptr<TGameActions> actions = Context->GetActionsReplay(Context, nextRound);
    // if (!goToNextRound && progress.GameType == TGameType::SinglePlayer) { nextRound = 1; }
    // Even if we finished the current round, check if the player won and get the right round/game type.
    const auto nextGameType = goToNextRound ? progress.GetNextGameType() : progress.GameType;
    TLOG(LOG_TRACE, "******Player%d done, next up %s / level: %d / next game type: %s***", winningPlayerId,
      (progress.IsPlayer1TheActivePlayer() ? "Player1" : "Player2"),
      nextRound, TGameProgress::GetGameTypeStr(nextGameType));
    TGameLoadInfo loadInfo = {
      lastFilename, nextRound, actions, progress.NextProgressText1,
      progress.NextProgressText2, progress.NextProgressText3, progress.NextProgressText4, progress.LastPlayerState,
      progress.FinishedAllRounds,
      nextGameType,
      prevRound,
      /*ShowMenuAnim*/ goToNextRound,
      rlTrain_
      // TODO: Use cached load info here! We might already have the data.
    };
#if DEBUG
    loadInfo.DebugInfo = Context->GetDebugInfo();
#endif
    LoadFile(loadInfo);
  }
  { // Use the newly loaded World now as the previous World instance is destroyed.
    auto& progress = World->WorldInfo.GameProgress;
    progress.SetGameState(TGameState::StartGame);
  }
}

void TGame::Draw() const { Context->DrawFrame(Context); }

void TGame::Unload()
{
  frameSkips_ = 0;
  World = nullptr;
  Context->Reset(Context);
}

std::shared_ptr<TWorld> TGame::LoadFile(TGameLoadInfo& loadInfo)
{
  if (World != nullptr) { Unload(); }

  const auto hasCachedData = (loadInfo.UseCachedData && !loadInfo.Data.empty());
  if (!hasCachedData)
  {
    auto fullFilename = GetGameLevelsDir() + loadInfo.Filename;
    if (!FileExists(fullFilename.c_str()))
    {
      TLOG(TERROR, "Failed to load %s (Does not exist)", fullFilename.c_str());
      return nullptr;
    }
    std::ifstream is(fullFilename);
    if (fullFilename.find(".json") != std::string::npos)
    {
      // Read stream and close json/stream before we load the game.
      loadInfo.Data = json::parse(is);
      is.close();
    }
  }
  else
  {
    TLOG(LOG_TRACE, "**** Using cached data for %s", loadInfo.Filename.c_str());
  }

  if (!loadInfo.Data.empty())
  {
    World = std::make_shared<TWorld>(loadInfo.Data.template get<TWorld>());
  }
  if (World != nullptr)
  {
    TLOG(LOG_TRACE, "**** Loaded game %s", loadInfo.Filename.c_str());
    rlTrain_ = loadInfo.RLTrain;
    LoadGame_(loadInfo);
    return World;
  }

  TLOG(TERROR, "Failed to load %s", loadInfo.Filename.c_str());
  return nullptr;
}

//! @brief Initializes the world with game progress initial info.
void TGame::LoadGame_(TGameLoadInfo& loadInfo)
{
  menuDebounceTimer_.Start();
  auto actions = loadInfo.ReplayActions;

#if DEBUG
#if RLPLAYS_EDITOR
  // For the first ever launch, choose the RL game type if applicable (for local debug builds?).
  if (loadInfo.ShowMenuAnim && !THeadless::IsSkipInputMode())
  {
    if (loadInfo.GameType == TGameType::SinglePlayer)
    {
      //loadInfo.GameType = TGameType::PlayerVsAI;
    }
  }
#endif
  Context->SetDebug(loadInfo.DebugInfo);
#endif
  if (loadInfo.GameType == TGameType::SinglePlayer && loadInfo.ReplayActions != nullptr)
  {
    actions = nullptr;
  }
  World->Load(&loadInfo, Context);
  Context->SetWorld(World, Context, actions);
}

bool TGame::SaveFile(std::string filename) const
{
  if (World == nullptr) { return false; }
  filename = GetDataDir() + filename;
  std::ofstream os(filename);
  if (filename.find(".json") != std::string::npos)
  {
    World->PrepareForSave(Context);
    const json data(*World);
    os << data.dump(2);
    os.close();
    TLOG(LOG_INFO, "Saved to %s", filename.c_str());
    return true;
  }
  return false;
}

bool TGame::IsValid() const { return World != nullptr; }

void TGame::HandleActions(const TPlayerAction& action)
{
  if (Context == nullptr || Context->World() == nullptr) { return; }

  auto& progress = World->WorldInfo.GameProgress;
  if (progress.GetGameState() == TGameState::StopGame)
  {
    // Show scene transitions and continue to next stage.
    if (!THeadless::IsSkipInputMode()) { AutoContinue_(true); }
    return;
  }

  bool hasMenuActions = progress.CheckGameMenuAction(action);
  //! @brief Whether we handle game actions or wait for menu/transitions.
  bool handleGameActions = false;
  if (progress.GetGameState() == TGameState::AboutToRun ||
    progress.GetGameState() == TGameState::MenuDismissing ||
    progress.GetGameState() == TGameState::MenuDismissedBeforeRunning)
  {
    // In headless mode (or if scene transitions are disabled) skip going through the progress screen.
    if (THeadless::IsSkipInputMode()) { handleGameActions = true; }
    else
    {
      menuDebounceTimer_.TickTimerPerFrame(Context);
      if (!menuDebounceTimer_.IsRunning())
      {
        handleGameActions = hasMenuActions;
      }
    }
  }
  else if (progress.GetGameState() == TGameState::RunningGame)
  {
    handleGameActions = !TPlayerFrameActions::IsEmptyAction(action);
  }

  if (handleGameActions)
  {
    // Check if we are starting the game for the first time; if so, ignore that action (if we are in non-headless mode).
    if (progress.SetActionsStarted(true))
    {
      if (!THeadless::IsSkipInputMode()) { return; }
    }
    TPlayerActions actions = {action};
    Context->HandleActions(Context, actions, false);
  }
}

#ifdef RLPLAYS_EDITOR

void TGame::ResetGame(bool resetBlocks)
{
  if (World == nullptr) { return; }
  if (resetBlocks)
  {
    World->ResetBlocks();
  }
  World->Reload();
  Context->Reset(Context);
  World->Load(nullptr, Context);
  Context->SetWorld(World, Context, nullptr);
  frameSkips_ = 0;
  menuDebounceTimer_.Stop();
}

#endif
void TGame::TogglePause()
{
  const auto gameState = World->WorldInfo.GameProgress.GetGameState();
  if (gameState == TGameState::RunningGame) { World->WorldInfo.GameProgress.RequestGameState(TGameState::PauseGame); }
  else if (gameState == TGameState::PauseGame)
  {
    World->WorldInfo.GameProgress.RequestGameState(TGameState::RunningGame);
  }
}

int TGame::GetRound() const
{
  return World ? World->WorldInfo.GameProgress.CurrentRound : 1;
}

bool THeadless::TrackContentFiles = false;
} // namespace RLPlays
