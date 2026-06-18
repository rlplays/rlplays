#include <context.h>

#include <game.h>
#include <world.h>

#include "log.h"
#include <base_types.h>


namespace RLPlays
{
inline std::string GetFontKey(const std::string& fontfile, int fontSize, int numCodepoints)
{
  return fontfile + "_" + std::to_string(fontSize);
}

//! @brief Loads the font with the specified font file/size (once per font) and unloads when the
//         context object is reset.
Font TContext::LoadFont(std::string fontFile, int fontSize, int numCodepoints)
{
  auto key = GetFontKey(fontFile, fontSize, -1);

  auto it = fonts_.find(key);
  if (it != fonts_.end()) { return it->second; }
  fontFile = GetDataDir() + fontFile;
  auto font = RLPlays_LoadFontEx(fontFile.c_str(), fontSize, nullptr, -1);
  return (fonts_[key] = font);
}

void TContext::DrawText(const std::string& fontFile, const Font& font, const std::string str, Vector2 pos, int fontSize,
  float spacing, Color color)
{
  auto key = GetFontKey(fontFile, fontSize, -1);
  if (font.glyphs == nullptr || fonts_.find(key) == fonts_.end()) { return; }
  DrawTextEx(font, str.c_str(), pos, fontSize, spacing, color);
}

//! @brief Obtains the world camera (stored camera, not current) - does not change after the player starts.
TCamera& TContext::GetCamera() const { return World()->WorldInfo.Camera; }

//! @brief Loads/caches the texture from the filename.
Texture2D TContext::LoadTexture(const std::string& filename)
{
  auto it = textures_.find(filename);
  if (it != textures_.end()) { return it->second; }
  // Sometimes, we start with an empty filename and then add a filename later - here don't spam the logs.
  if (filename.empty()) { return {}; }
  auto fullFileName = GetDataDir() + filename;
  const auto texture = RLPlays_LoadTexture(fullFileName.c_str());
  if (texture.id == 0)
  {
    TLOG(TERROR, "Failed to load texture %s", fullFileName.c_str());
    return {};
  }

  return (textures_[filename] = texture);
}

//! @brief Loads/caches a sprite sheet from a file - each instance though controls its own frames/indices.
TSpriteSheet& TContext::LoadSpriteSheet(TSpriteSheet& spriteSheet)
{
  if (spriteSheet.Texture.id != 0 && !IsZeroVec(spriteSheet.SpriteSize)) { return spriteSheet; }
  spriteSheet.Texture = this->LoadTexture(spriteSheet.Filename);
  if (spriteSheet.Texture.id == 0 || IsZeroVec(spriteSheet.SpriteSize))
  {
    if (!spriteSheet.Filename.empty())
    {
      TLOG(TERROR, "Failed to load sprite sheet %s @ (%d, %d)", spriteSheet.Filename.c_str(), spriteSheet.SpriteSize.x,
        spriteSheet.SpriteSize.y);
    }
    return spriteSheet;
  }
  spriteSheet.NumFrames = (spriteSheet.Texture.width / spriteSheet.SpriteSize.x);
  spriteSheet.SpriteIndex = 0;                               // TODO(perumaal): Allow other non-zero indices to be set?
  spriteSheet.currentSpriteIndex_ = spriteSheet.SpriteIndex; // Prevent clobbering the editor/serialized state.
  spriteSheet.RunningTimeNs = 0;
  spriteSheet.FrameTimeNs = spriteSheet.SpriteFPS > 0 ? DurationFromFPS(spriteSheet.SpriteFPS) : FrameDurationNanos;
  return spriteSheet;
}

//! @brief Obtains the cached texture if one exists.
Texture2D TContext::GetTexture(const std::string& string) const
{
  auto it = textures_.find(string);
  if (it != textures_.end()) { return it->second; }
  return {0};
}

void TContext::DrawAnimSprite(TSpriteSheet& sheet, const Rectangle dest, const Color color,
  const int stopAtIndex, const float angle, Vector2 origin) const
{
  if (sheet.Texture.id == 0) { return; }
  sheet.RunningTimeNs += FrameDurationNanos;
  if (sheet.RunningTimeNs >= sheet.FrameTimeNs)
  {
    // A simple way to gracefully stop at the provided index (or keep running normally otherwise).
    if (sheet.currentSpriteIndex_ != stopAtIndex) { sheet.currentSpriteIndex_++; }
    sheet.RunningTimeNs = 0;
  }
  if (sheet.currentSpriteIndex_ >= sheet.NumFrames) { sheet.currentSpriteIndex_ = 0; }
  const int sheetX = sheet.SpriteSize.x * (sheet.currentSpriteIndex_);
  origin.x *= (dest.width);
  origin.y *= (dest.height);
  DrawTexturePro(sheet.Texture, {TO_FLT(sheetX), 0, TO_FLT(sheet.SpriteSize.x), TO_FLT(sheet.SpriteSize.y)}, dest,
    origin, angle, color);
}

void TContext::HandleInactivePlayerActions_(TContextPtr context)
{
  const auto gameType = context->GetGameProgress()->GameType;
  if (!rlPlayer_.IsRLPlayer() && replayActions_ != nullptr)
  {
    TPlayerActions actions = {};
    if (replayActions_->GetAction(frame_, &actions))
    {
      HandleActions(context, actions, true);
    }
  }
  else if (rlPlayer_.IsRLPlayer())
  {
    rlPlayer_.HandleRLPlayerActions(context);
  }
}

void TContext::HandlePostInactivePlayerActions_(TContextPtr context)
{
  const auto gameType = context->GetGameProgress()->GameType;
  if (!rlPlayer_.IsRLPlayer()) { return; }
  rlPlayer_.HandlePostUpdateActions(context);
}


void TContext::DrawTexture(const TTexture& texture, const Rectangle dest, const Color color) const
{
  auto& tex2d = texture.Texture;
  if (tex2d.id == 0) { return; }
  if (texture.TileSize.Empty())
  {
    ::DrawTexturePro(tex2d, {0, 0, TO_FLT(tex2d.width), TO_FLT(tex2d.height)}, dest, {0, 0}, 0, color);
  }
  else
  {
    // Width may be negative to indicate a flipped texture.
    auto width = std::abs(dest.width);
    Vector2 repeat = {width / TO_FLT(texture.TileSize.x), dest.height / TO_FLT(texture.TileSize.y)};
    Rectangle dest2;
    dest2.width = TO_FLT(texture.TileSize.x) * FloatSign(dest.width);
    dest2.height = TO_FLT(texture.TileSize.y);
    for (float y = 0.0f; y < repeat.y; y++)
    {
      dest2.y = dest.y + y * TO_FLT(texture.TileSize.y);
      for (float x = 0.0f; x < repeat.x; x++)
      {
        dest2.x = dest.x + x * TO_FLT(texture.TileSize.x);
        ::DrawTexturePro(tex2d, {0, 0, TO_FLT(tex2d.width), TO_FLT(tex2d.height)}, dest2, {0, 0}, 0, color);
      }
    }
  }
}


bool TContext::HandleActions(TContextPtr context, TPlayerActions& actions, const bool replay)
{
  TBlockId targetBlockId;
  if (replay) { targetBlockId = replayActionsHandlerId_; }
  else { targetBlockId = actionsHandlerId_; }
  if (targetBlockId <= InvalidBlockId) { return false; }
  const auto block = GetBlock(targetBlockId);
  if (block == nullptr) { return false; }
  block->HandleActions(context, actions);

  actions.FrameIndex = frame_;
  // Only track the 'active' realtime actions not the replay actions. Which would result in a dumb algorithm.
  if (!replay) { AddActionsForFrame_(actions); }
  return true;
}


//! @brief Initializes the given block if needed (based on previously set traits) and updates the traits to include Inited.
void TContext::InitBlockIfNeeded(TContextPtr context, const std::shared_ptr<ABlock>& block)
{
  if (block->IsInitialized()) { return; }
  block->Init(context);
  block->SetInitialized();
}

std::shared_ptr<TBlock> TContext::SpawnBlock(TContextPtr context, std::shared_ptr<TBlock> blueprint, const Vector2& pos)
{
  auto block = std::make_shared<TBlock>(CopyBlock(*blueprint));
  SetRectPos(block->Block->Box, pos);
  AddBlock(*block);
  InitBlockIfNeeded(context, block->Block);
  return block;
}

void TContext::RemoveSpawnedBlock(ABlock& block)
{
  if (block.BlockId == InvalidBlockId) return;
  block.RunState = AddEnumValue(block.RunState, TRunState::Removed);
  toRemove_.push_back(block.BlockId);
}

void TContext::AddActionsForFrame_(const TPlayerActions& actions)
{
  if (TPlayerFrameActions::IsEmptyAction(actions.Action)) { return; }
  const auto& worldInfo = world_->WorldInfo;
  // While replaying, we check the "next" frame not the current frame due to how Update/Input/Replay
  // are sequenced.
  actions_->Score = static_cast<float>(worldInfo.GameProgress.GetActivePlayerProgress().NumRewards);
  actions_->AddAction(actions.FrameIndex, actions.Action);
}

void TContext::AddActivePlayerPosition_(TContextPtr context)
{
  if (actions_ == nullptr) return;
  auto activePlayer = context->GetActionsHandlerBlock();
  if (activePlayer != nullptr)
  {
    actions_->AddPositionToLastFrame(RectTopLeft(activePlayer->Box));
  }
}

void TContext::RemoveMarkedSubBlocks_(TContextPtr context)
{
  // We should be a bit smart here as we should not be removing in the main thread this often.
  // Perhaps stack things up and remove in one go?
  for (const auto& blockId : toRemove_)
  {
    auto& block = blocks_[blockId];
    grid_->RemoveBlock(*block);
    blocks_.erase(blockId);
  }
  toRemove_.clear();
}

void TContext::DrawBlocks_(std::shared_ptr<TContext> context, TLayerDepth layerDepth) const
{
  // TODO(perumaal): check if we have blocks with the layer-depth ahead of time and cull this call completely.
  // Or better yet, have a list of lists based on layer depth->blocks.
  for (const auto& [_, block] : blocks_)
  {
    if (!HasOneOfEnumValue(block->GetRunState(), AddEnumValue(TRunState::Invisible, TRunState::Removed)))
    {
      if (block->Depth == layerDepth)
      {
        // TODO(perumaal): Move this into a separate LoadContent that the caller must call at an opportune time.
        // For now, we have a simple level system, so we can afford to load at the time of drawing.
        if (!HasEnumValue(block->GetRunState(), TRunState::ContentLoaded))
        {
          block->LoadContent(context);
          block->RequestRunState(TRunState::ContentLoaded);
        }
        block->Draw(context);
      }
    }
  }
}

void TContext::LoadContent(TContextPtr context)
{
  THeadless::ClearContent();
  for (const auto& [_, block] : blocks_) { block->LoadContent(context); }
}

std::string TContext::GetWorldFilename() const { return World()->WorldInfo.Filename; }

void TContext::DrawFrame(TContextPtr context)
{
  ::BeginMode2D(world_->Get2DCamera());
  screenRect_ = world_->WorldInfo.Camera.Viewport;

  // Now draw the blocks in the world in layers.
  {
    ::ClearBackground(world_->WorldInfo.Camera.BgColor);
    // ::DrawRectangleRec(world_->WorldInfo.Camera.Viewport, CORN_FLOWER_BLUE);

    world_->CheckForResize_();
    DrawBlocks_(context, TLayerDepth::Background);
    DrawBlocks_(context, TLayerDepth::Foreground);
    DrawBlocks_(context, TLayerDepth::Overlay);

#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
    if (debugInfo_.ShowDebugView)
    {
      for (const auto& [_, block] : blocks_) { block->DrawDebug(context); }
      grid_->DebugRender(context);
    }
    if (debugInfo_.ShowRLViz)
    {
      rlPlayer_.DebugRender(context);
    }
#endif
  }
  ::EndMode2D();
}

void TContext::DrawEditor(const std::shared_ptr<TEditorData>& editorData, const std::shared_ptr<TContext>& context)
{
#if RLPLAYS_EDITOR
  ::BeginMode2D(world_->Get2DCamera());
  {
    for (const auto& [_, block] : blocks_)
    {
      block->EditorDraw(editorData, context);
    }
  }

  if (World()->WorldInfo.GameProgress.HasMirrorMode)
  {
    const auto& viewport = World()->WorldInfo.Camera.Viewport;
    DrawLine(viewport.width / 2.0f, 0, viewport.width / 2.0f, viewport.height, {255, 96, 96, 255});
  }

  ::EndMode2D();
#endif
}

void TContext::MoveBlockAbs(ABlock* block, const Vector2& pos)
{
  if (AreVectorsSame(RectTopLeft(block->Box), pos)) { return; }
  block->RequestRunState(TRunState::RequirePostUpdate);
  const Rectangle oldRect = block->Box;
  SetRectPos(block->Box, pos);
  grid_->MoveBlock(block, oldRect);
}

void TContext::MoveBlockBy(ABlock* block, const Vector2& vector2)
{
  if (IsZeroVec(vector2)) { return; }
  MoveBlockAbs(block, Vector2Add(RectTopLeft(block->Box), vector2));
}

//! @brief Returns a read-only progress (for most part, we don't want to allow modifications).
const TGameProgress* TContext::GetGameProgress() const { return &(world_->WorldInfo.GameProgress); }

//! @brief Returns an updateable game progress.
TGameProgress* TContext::UpdateGameProgress() { return &(world_->WorldInfo.GameProgress); }

void TContext::InitDebug_(TContextPtr context)
{
#if DEBUG
  if (!debugInit_)
  {
    LoadFont(DebugSmallFont);
    LoadFont(DebugLargeFont);
    debugInit_ = true;
  }
#endif
}

#if DEBUG
void TContext::ResetDebug_()
{
  debugInit_ = false;
  debugInfo_ = {};
}
#endif

void TContext::UpdateFrame(TContextPtr context)
{
  HandleInactivePlayerActions_(context);
  InitDebug_(context);
  grid_->PrepareFrame(debugInfo_);
  screenRect_ = world_->WorldInfo.Camera.Viewport;

  TGameProgress* progress = &World()->WorldInfo.GameProgress;
  progress->TrackTimeLeft(context);

  Tick();

  auto& grid = context->Grid();
  for (const auto& [_, block] : blocks_)
  {
    if (HasEnumValue(block->RunState, TRunState::Removed) || HasEnumValue(block->Traits, TBlockTraits::Cosmetic))
      continue;
    Rectangle rect = block->Box;
    block->RunState = RemoveEnumValue(block->RunState, TRunState::RequirePostUpdate);
    block->PrevPos = RectTopLeft(block->Box);
    block->Update(context);
    grid->MoveBlock(block.get(), rect);
  }

  for (const auto& [_, block] : blocks_)
  {
    if (HasEnumValue(block->RunState, TRunState::Removed) || HasEnumValue(block->Traits, TBlockTraits::Cosmetic))
      continue;
    if (!HasEnumValue(block->RunState, TRunState::RequirePostUpdate)) { continue; }
    Rectangle rect = block->Box;

    //EnsureSnapToInt(block->Box);
    block->PostUpdate(context);
    grid->MoveBlock(block.get(), rect);
    block->RunState = RemoveEnumValue(block->RunState, TRunState::RequirePostUpdate);
  }
  RemoveMarkedSubBlocks_(context);
  AddActivePlayerPosition_(context);
  HandlePostInactivePlayerActions_(context);
}


void TContext::AddSubBlock(TContextPtr context, const ABlock& parentBlock, const TBlock& subBlock,
  const std::string& subBlockName, const TEditorUpdateFn& editorUpdateBlockFn)
{
  subBlock.Block->BlockId = -1;
  AddBlock(subBlock);
  // We have to be careful here as we may have a recursive call right here.
  InitBlockIfNeeded(context, subBlock.Block);


  ConnectChildBlockId_(parentBlock.GetBlockId(), subBlock.Block->GetBlockId(), subBlockName, editorUpdateBlockFn);
}

void TContext::AddBlock(const TBlock& block)
{
  block.Block->BlockType = block.BlockType;
  if (block.Block->BlockId < 0)
  {
    block.Block->BlockId = ++maxBlockId_; // Starting with -1 initially.
  }
  else
  {
    maxBlockId_ = block.Block->BlockId;
  }
  blocks_[block.Block->BlockId] = block.Block;
  if (!HasEnumValue(block.Block->RunState, TRunState::Removed) &&
    !HasEnumValue(block.Block->Traits, TBlockTraits::Cosmetic))
  {
    grid_->AddBlock(block.Block);
  }
}

bool TContext::ConnectChildBlockId_(const TBlockId parentBlockId, const TBlockId childBlockId,
  const std::string& subBlockName, const TEditorUpdateFn& editorUpdateBlockFn)
{
  if (childToParentIds_.find(childBlockId) == childToParentIds_.end())
  {
#if RLPLAYS_EDITOR
    childToParentIds_[childBlockId] = {subBlockName, editorUpdateBlockFn, parentBlockId};
#else
    childToParentIds_[childBlockId] = {parentBlockId};
#endif
    return true;
  }
  return false;
}

std::shared_ptr<TGameActions> TContext::GetActionsReplay(TContextPtr context, int nextRound) const
{
  if ((actions_ == nullptr && replayActions_ == nullptr) || context->GetGameProgress() == nullptr) { return nullptr; }
  const auto* progress = GetGameProgress();
  const auto round = progress->CurrentRound;
  if (nextRound < 0) { nextRound = round + 1; }
  const auto filename = World()->WorldInfo.Filename;
  //if (progress->GameType == TGameType::SinglePlayer)
  //{
  //  if (actions_ == nullptr)
  //  {
  //    TLOG(LOG_FATAL, "GetActionsReplay: SinglePlayer but no actions_ (may be round is incorrect?)");
  //  }
  //  actions_->PlayerId = PlayerId1;
  //  return TGameActions::Copy(actions_, round, filename);
  //}
  if (actions_ != nullptr && actions_->CurrentRound != nextRound)
  {
    return TGameActions::Copy(actions_, actions_->CurrentRound, filename);
  }
  if (replayActions_ != nullptr && replayActions_->CurrentRound != nextRound)
  {
    return TGameActions::Copy(replayActions_, replayActions_->CurrentRound, filename);
  }

  // If we are here, we have probably entered The Matrix by mistake. Best to let it happen.
  return nullptr;
}

void TContext::SetupInitialGameplay_(const std::shared_ptr<TGameActions>& actions)
{
  if (World() == nullptr) { return; }
  replayActions_ = actions;
  // World()->WorldInfo.GameProgress.GameType = gameType;
  TLOG(LOG_TRACE, "Game Type set to %s, round = %d",
    TGameProgress::GetGameTypeStr(World()->WorldInfo.GameProgress.GameType),
    World()->WorldInfo.GameProgress.CurrentRound);
}

void TContext::SetupActionsHandlers_()
{
  // TLOG(LOG_TRACE, "Setting up actions for %s", World()->GetGameTypeStr());
  actionsHandlerId_ = world_->GetActionHandlerBlockId();
  replayActionsHandlerId_ = world_->GetReplayActionHandlerBlockId();
  if (actions_ == nullptr)
  {
    const auto& worldInfo = world_->WorldInfo;
    actions_ = std::make_shared<TGameActions>();
    // If we are replaying actions for Player1, then we need to set the player id to PlayerId2.
    if (replayActions_ != nullptr && replayActions_->PlayerId == PlayerId1) { actions_->PlayerId = PlayerId2; }
    else { actions_->PlayerId = PlayerId1; }
    actions_->CurrentRound = worldInfo.GameProgress.CurrentRound;
    actions_->Filename = worldInfo.Filename;
    actions_->Version = worldInfo.Version;
    const auto activePlayer = GetActionsHandlerBlock();
    if ( IsInvalidVector(actions_->StartPosition)) { UpdateActivePlayerPos(RectTopLeft(activePlayer->Box)); }
  }
  const auto gameType = GetGameProgress()->GameType;
  const auto inactivePlayer = GetReplayHandlerBlock();
  if ((inactivePlayer != nullptr) && (gameType == TGameType::PlayerVsPrior || gameType == TGameType::PriorVsPlayer) &&
    (replayActions_ != nullptr) && (replayActionsHandlerId_ != InvalidBlockId) &&
    !IsInvalidVector(replayActions_->StartPosition))
  {
    // When replaying a previous game with stored actions from a prior player, and we are not using RL trained agent for this round,
    // and if we have a stored start position (due to randomization), use that instead.
    MoveBlockAbs(inactivePlayer.get(), replayActions_->StartPosition);
  }
}

void TContext::UpdateActivePlayerPos(const Vector2& startPos)
{
    const auto activePlayer = GetActionsHandlerBlock();
    if (activePlayer != nullptr)
    {
      MoveBlockAbs(activePlayer.get(), startPos);
      actions_->StartPosition = startPos;
    }
}

std::set<TBlockId> TContext::GetSelectedObjects(const Vector2 screenPos, bool onlySelectOne) const
{
  std::vector<std::shared_ptr<ABlock>> selection;
  const auto worldPos = GetScreenToWorld2D(screenPos, world_->Get2DCamera());
  for (auto& [_, ablock] : blocks_)
  {
    if (CheckCollisionPointRec(worldPos, ablock->Box))
    {
      selection.push_back(ablock);
    }
  }

  // Sort selection by layer depth (descending)
  std::sort(selection.begin(), selection.end(),
    [](const std::shared_ptr<ABlock>& a, const std::shared_ptr<ABlock>& b)
    {
      return static_cast<int>(a->Depth) > static_cast<int>(b->Depth);
    });

  std::set<TBlockId> ret;
  if (!selection.empty())
  {
    if (onlySelectOne)
    {
      // Only add the topmost block (highest layer depth)
      ret.insert(selection[0]->BlockId);
    }
    else
    {
      // Add all blocks
      for (const auto& block : selection) { ret.insert(block->BlockId); }
    }
  }

  return ret;
}

Vector2 TContext::GetWorldPosFromScreenPos(const Vector2 screenPos) const
{
  return GetScreenToWorld2D(screenPos, world_->Get2DCamera());
}


void TContext::InitGrid_()
{
  const auto cellSize = world_->WorldInfo.Camera.CellSize;
  const Vector2 worldSize = RectSize(world_->WorldInfo.Camera.Viewport);
  // TODO(perumaal): Grid maps 1:1 with cell size, this is not ideal. However, a lot of rendering code uses
  //                 the grid cell size. Instead, we should separate out the grid internal cell size and the
  //                 external rendering one.  
  grid_ = std::make_shared<TGrid>(FromVector(worldSize), Vec2i{int(cellSize.x * 1), int(cellSize.y * 1)});
}

std::shared_ptr<TGameActions> TGameActions::Copy(const std::shared_ptr<TGameActions>& actions, const int round,
  const std::string& filename)
{
  auto copy = std::make_shared<TGameActions>();
  if (actions == nullptr) { return copy; }
  copy->FrameActions.reserve(actions->FrameActions.size());
  for (const auto& action : actions->FrameActions) { copy->FrameActions.push_back(action); }
  copy->PlayerId = actions->PlayerId;
  copy->CurrentRound = round;
  copy->Filename = filename;
  copy->Version = TVersion::BlockVersion;
  copy->StartPosition = actions->StartPosition;
  return copy;
}


void TContext::DrawText(TFont& font, const Vector2& pos, const std::string& str, const Color color)
{
  DrawText(font.FontFile, font.font_, str, pos, font.FontSize, font.FontSpacing, color);
}

Rectangle TContext::DrawCenteredText(TFont& font, const Rectangle& rect, const std::string& text, Color color)
{
  if (text.empty() || font.font_.baseSize == 0) return {};

  // Measure the text dimensions
  Vector2 textSize = MeasureTextEx(font.font_, text.c_str(), font.FontSize, font.FontSpacing);

  // Calculate the centered position
  Vector2 position = {
    std::max(0.0f, rect.x + (rect.width - textSize.x) * 0.5f),
    std::max(0.0f, rect.y + (rect.height - textSize.y) * 0.5f)
  };


  DrawText(font.FontFile, font.font_, text, position, font.FontSize, font.FontSpacing, color);
  return {position.x, position.y, textSize.x, textSize.y};
}


void TContext::SetWorld(const std::shared_ptr<TWorld>& world, TContextPtr context,
  const std::shared_ptr<TGameActions>& replayActions)
{
  world_ = world;
  blocks_ = std::map<TBlockId, std::shared_ptr<ABlock>>();
  screenRect_ = world_->WorldInfo.Camera.Viewport;

  InitGrid_();
  SetupInitialGameplay_(replayActions);
  for (const auto& block : world->Blocks)
  {
    auto& ablock = block.Block;
    ablock->AddBlock(context, block);
    // This will ask the world to add the block(s) to the context.
    // There may not be a 1:1 mapping between the saved blocks data
    // and what the block thinks it wants us to do.
  }
  // Now we have added the blocks to blocks_, lets init them. Again, blocks may choose to add more sub-blocks.
  // Hence, we don't use an iterator here (that will get invalidated).
  for (int i = 0; i < (int)blocks_.size(); ++i)
  {
    const auto& block = blocks_[i];
    InitBlockIfNeeded(context, block);
    // Note blocks_ might change - it may expand (but never contract) - so we keep looping through with the new size.
  }
  SetupActionsHandlers_();
}

void TContext::UpdateRLPlayerWeights(std::shared_ptr<RLWeights> weights)
{
  rlPlayer_.UpdateSelfPlayWeights(weights);
}

void TContext::SetupRL(TContextPtr context, std::shared_ptr<TRLTrain> rlTrain, std::shared_ptr<RLWeights> shared_weights)
{
  auto progress = GetGameProgress();
  bool isRLPlayer = progress->GameType == TGameType::PlayerVsAI || progress->GameType == TGameType::AIVsPlayer;
  bool activePlayer = false;
#if DEBUG
  if (GetDebugInfo().RLControlMainPlayer && (progress->GameType == TGameType::SinglePlayer || progress->GameType == TGameType::PlayerVsPrior || progress->GameType == TGameType::PriorVsPlayer))
  {
    activePlayer = true;
    isRLPlayer = true;
  }
#endif
  rlPlayer_.SetupRLEnv(context, rlTrain, isRLPlayer, activePlayer, shared_weights);
}

//! @brief Clones using an inefficient but extremely versatile jsoncpp way.
TBlock CopyBlock(const TBlock& block)
{
  const json data(block);
  return data.template get<TBlock>();
}

std::shared_ptr<ABlock> GetABlock(const std::shared_ptr<TBlock>& block) { return block->Block; }
std::shared_ptr<ABlock> GetABlock(const TBlock& block) { return block.Block; }

#if !RLPLAYS_EDITOR
void TContext::MirrorBlockViaEditor(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
  const TBlock& originalBlock, ABlock* mirroredBlock)
{
  // Defined in base_editor.cpp;
}
#endif
} // namespace RLPlays
