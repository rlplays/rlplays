#include <base_types.h>
#include <world.h>
#include <game_progress.h>
#include <unordered_set>
#include <atomic>
#include <mutex>

namespace RLPlays
{
void TWorld::CheckForResize_()
{
  if (IsWindowResized())
  {
    SetupCamera_();
  }
}

bool TGameProgress::IsDone()
{
  if (!TimeLimit.HasStarted()) { return false; }
  if (TimeLimit.IsValid() && !TimeLimit.IsRunning()) { return true; }
  // auto PlayerState1 = PlayerProgress[0].PlayerState;
  // auto PlayerState2 = PlayerProgress[1].PlayerState;


  // The goal is for the active player to collect at least as many fruit and reach the goal before the other player does.
  // If the active player reached the goal before the other player, check if they have more fruit.
  // If the inactive player reached the goal, they won regardless of how many fruit they collected.
  auto& activePlayer = PlayerProgress[IsActivePlayer(PlayerId1) ? 0 : 1];
  auto& inactivePlayer = PlayerProgress[IsActivePlayer(PlayerId1) ? 1 : 0];
  if (inactivePlayer.HasReachedGoal)
  {
    // Is it possible BOTH players reached the goal at the exact same frame?
    // If so, tie-break in advantage to the previous player (i.e. you have to do better!)
    inactivePlayer.PlayerState = TPlayerState::Won;
    // Inactive player has reached the goal - do they at least have as many rewards as the other player?
    if ((inactivePlayer.NumRewards >= activePlayer.NumRewards))
    {
      inactivePlayer.PlayerState = TPlayerState::Won;
      if (activePlayer.PlayerState == TPlayerState::Alive) { activePlayer.PlayerState = TPlayerState::TimeOut; }
    }
    else
    {
      inactivePlayer.PlayerState = TPlayerState::Dead;
    }
  }
  
  if (inactivePlayer.PlayerState != TPlayerState::Won && activePlayer.HasReachedGoal)
  {
    // Active player has reached the goal - do they at least have as many rewards as the other player?
    if ((inactivePlayer.NumRewards <= activePlayer.NumRewards))
    {
      activePlayer.PlayerState = TPlayerState::Won;
      if (inactivePlayer.PlayerState == TPlayerState::Alive) { inactivePlayer.PlayerState = TPlayerState::TimeOut; }
    }
    else
    {
      activePlayer.PlayerState = TPlayerState::Dead;
    }
  }
  // Human player can continue playing if they collect more rewards and haven't reached the goal yet compared
  // with the inactive player. (i.e. inactive player reached the goal, and hence has Died).

  // If Inactive player dies first, the game continues.
  if (activePlayer.PlayerState != TPlayerState::Alive) { return true; }
  if (inactivePlayer.PlayerState == TPlayerState::Won) { return true; }
  return false;
}


void TGameProgress::TrackTimeLeft(TContextPtr context)
{
  if (!TimeLimit.HasStarted()) { TimeLimit.Start(); }
  TimeLimit.TickTimerPerFrame(context);
}

void TWorldInfo::Convert(std::shared_ptr<TContext> context)
{
  // Add stuff here, convert, then remove it.
}

void TWorld::SetupCamera_()
{
  screenSize_ = {TO_FLT(GetScreenWidth()), TO_FLT(GetScreenHeight())};
  const float zoomW = screenSize_.x / WorldInfo.Camera.Viewport.width;
  const float zoomH = screenSize_.y / WorldInfo.Camera.Viewport.height;
  float zoom;
  Vector2 offset = {0, 0};
  if (zoomW < zoomH)
  {
    zoom = zoomW;
    offset.y = (screenSize_.y - WorldInfo.Camera.Viewport.height * zoom) / 2;
  }
  else
  {
    zoom = zoomH;
    offset.x = (screenSize_.x - WorldInfo.Camera.Viewport.width * zoom) / 2;
  }
  camera_ = {offset, {0, 0}, 0, zoom};
}

void TWorld::Load(const TGameLoadInfo* loadInfo, TContextPtr context)
{
  SetupCamera_();

  auto& progress = WorldInfo.GameProgress;
  progress.GameType = progress.GetInitialGameType();

  if (loadInfo != nullptr)
  {
    //if (loadInfo->ReplayActions != nullptr)
    //{
    //  gameType = (loadInfo->ReplayActions->PlayerId == PlayerId1 ? TGameType::PriorVsPlayer : TGameType::PlayerVsPrior);
    //}
    WorldInfo.Filename = loadInfo->Filename;
    progress.CurrentRound = loadInfo->Round;
    progress.MaxRounds = WorldInfo.MaxRounds;
    progress.NextProgressText1 = loadInfo->NextProgressText1;
    progress.NextProgressText2 = loadInfo->NextProgressText2;
    progress.NextProgressText3 = loadInfo->NextProgressText3;
    progress.NextProgressText4 = loadInfo->NextProgressText4;
    progress.LastPlayerState = loadInfo->LastPlayerState;
    progress.FinishedAllRounds = loadInfo->FinishedAllRounds;
    progress.GameType = loadInfo->GameType;
    progress.PreviousRound = loadInfo->PreviousRound;
    progress.ShowMenuAnim = loadInfo->ShowMenuAnim;
  }
  progress.Setup(context);
  TBlockId blockId = 0;
  for (auto& b : Blocks)
  {
    b.Block->BlockType = b.BlockType;
    b.Block->BlockId = blockId++;
    if (b.Block->DoesHandleActions(false))
    {
      ActionHandlerBlockId = b.Block->BlockId;
    }
    else if (b.Block->DoesHandleActions(true))
    {
      ReplayActionHandlerBlockId = b.Block->BlockId;
    }
  }
}


Rectangle TWorld::CellAt(const int xc, const int yc, const int wc, const int hc) const
{
  return {
    TO_FLT(xc) * WorldInfo.Camera.CellSize.x, TO_FLT(yc) * WorldInfo.Camera.CellSize.y,
    WorldInfo.Camera.CellSize.x * TO_FLT(wc), WorldInfo.Camera.CellSize.y * TO_FLT(hc)
  };
}

void TWorld::Reload() { TLOG(TINFO, "Reloading world"); }

void TWorld::Convert(TContextPtr context)
{
  WorldInfo.Convert(context);
  for (auto it = Blocks.begin(); it != Blocks.end(); ++it)
  {
    const auto& ablock = it->Block;
    ablock->Convert(context);
  }
}

void TWorld::PrepareForSave(const std::shared_ptr<TContext>& context)
{
  Convert(context);
}

int TWorld::GetBlocksTotalCells(TContextPtr context) const
{
  int totalCells = 0;
  for (const auto& block : Blocks)
  {
    if (HasEnumValue(block.Block->Traits, TBlockTraits::Cosmetic)) continue;
    totalCells += block.Block->GetTotalCells(context);
  }
  return totalCells;
}

int TWorld::GetBlocksTotalCellTypes(TContextPtr context) const
{
  std::unordered_set<TBlockType> blockTypes;
  for (const auto& block : Blocks)
  {
    if (HasEnumValue(block.Block->Traits, TBlockTraits::Cosmetic)) continue;
    blockTypes.insert(block.Block->BlockType);
  }
  return blockTypes.size();
}

std::shared_ptr<TWorldFiles> TWorldFiles::CachedDefault = nullptr;
std::mutex TWorldFiles::CachedDefaultMutex;

#if RLPLAYS_EDITOR

void TWorld::UpdateBlock(const TBlock& block, const int blockId, const TBlockType blockType)
{
  for (auto& b : Blocks)
  {
    if (b.Block->BlockId == blockId)
    {
      b = block;
      b.BlockType = blockType;
      b.Block->BlockType = blockType;
      b.Block->BlockId = blockId;
      return;
    }
  }
}

void TWorld::DeleteBlock(const int blockId)
{
  // TODO(perumaal): Should be a simple map here...
  for (auto it = Blocks.begin(); it != Blocks.end(); ++it)
  {
    if (it->Block->BlockId == blockId)
    {
      Blocks.erase(it);
      return;
    }
  }
}

bool TWorld::ShouldMirrorBlock(const TBlock& block) const
{
  const auto halfWidth = WorldInfo.Camera.Viewport.width / 2.0f;
  const auto& box = block.Block->Box;
  if (box.height > 0 && (box.x < halfWidth && Right(box) - 1 < halfWidth))
  {
    return true;
  }
  return false;
}

TBlock* TWorld::GetBlockWithIdForEditor(TBlockId blockId)
{
  for (auto it = Blocks.begin(); it != Blocks.end(); ++it)
  {
    if (it->Block->BlockId == blockId)
    {
      return &(*it);
    }
  }

  return nullptr;
}


void TWorld::ResetBlocks()
{
  auto blocksCopy = std::vector<TBlock>();
  blocksCopy.reserve(Blocks.size());
  for (auto& old : Blocks) { blocksCopy.push_back(CopyBlock(old)); }
  Blocks.clear();
  for (auto& block : blocksCopy)
  {
    block.Block->BlockId = Blocks.size();
    Blocks.push_back(block);
  }
}

// TODO: Remove this Editor-specific stuff from here... we are already copying stuff over.
bool TWorld::ShouldCopyAddBlocks = false;
#endif
} // namespace RLPlays
