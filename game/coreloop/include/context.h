#pragma once
#include <memory>

#include <grid.h>
#include <map>
#include <string>

#include <base_types.h>
#include <grid.h>
#include <serialize.h>
#include <set>
#include "log.h"
#include "main_game.h"
#include "game_progress.h"
#include "serialize_macros.h"
#include <cassert>

#include <raylib_utils.h>
#include "game_types.h"

#include "game_actions.h"
#include "rl_player.h"
#include "game_info.h"

namespace RLPlays
{
enum class TGameState : uint8_t;

struct RLWeights;

//! @brief A SpriteSheet uses a single texture indexed via (x, y) to draw an animation.
struct TSpriteSheet
{
  std::string Filename;
  int SpriteFPS = -1;
  Vec2i SpriteSize = {0, 0};
  Serializer(TSpriteSheet, Filename, SpriteIndex, SpriteFPS, SpriteSize)
  [[nodiscard]] bool Empty() const { return Filename.size() == 0 || SpriteSize.Empty(); }

  explicit TSpriteSheet(std::string filename, const int fps, const Vec2i& spriteSize) :
    Filename(std::move(filename)), SpriteFPS(fps), SpriteSize(spriteSize) {}

  TSpriteSheet() = default;

private:
  friend struct TContext;
  int SpriteIndex = 0;
  int NumFrames = 0;
  Texture2D Texture = {0};
  TimeNanos RunningTimeNs = 0;
  TimeNanos FrameTimeNs = 0;
  int currentSpriteIndex_ = 0;
};

typedef std::function<void(std::shared_ptr<TBlock>)> TEditorUpdateFn;

//
// Some block information for the editor (to create sub-blocks, to go through templatized blocks etc).
//
//! @brief Block with info to update the sub-block in the editor.
//! At runtime (non-editor), we use the block-id to track the parents alone - we don't need anything else.
struct TBlockInfo
{
#ifdef RLPLAYS_EDITOR
  std::string Name;
  TEditorUpdateFn UpdateBlockFn;
#endif
  TBlockId BlockId;
};

#if defined(RLPLAYS_EDITOR) || defined(RLPLAYS_TEST)
//! @brief Template block with a name to show in the editor.
struct TTemplateBlock
{
  std::string Name;
  std::shared_ptr<TBlock> Block;
  bool IsFromFile = false;
};
#else
//! @brief Basic test to ensure we haven't mistakenly polluted the sub-block info with editor data.
static_assert(sizeof(TBlockInfo) == sizeof(TBlockId), "TBlockInfo must only hold the block id in non-editor builds.");
#endif

struct TFont
{
  std::string FontFile = "fonts/pixantiqua.ttf";
  int FontSize = 32;
  Color FontColor = RED;
  float FontSpacing = 2;
  Serializer(TFont, FontFile, FontSize, FontColor, FontSpacing)
  TFont() = default;

  TFont(std::string fontFile, const int fontSize, const Color fontColor)
    : FontFile(std::move(fontFile)),
      FontSize(fontSize),
      FontColor(fontColor) {}

private:
  friend struct TContext;
  Font font_ = {};
};

//! @brief Stores a texture and its filename. Optionally allows tiling of the texture.
struct TTexture
{
  std::string Filename;

  Vec2i TileSize = {0, 0};
  Serializer(TTexture, Filename, TileSize)
  TTexture() = default;

  TTexture(std::string filename, const Vec2i& tileSize = {0, 0}) :
    Filename(std::move(filename)), TileSize(tileSize) {}

private:
  friend struct TContext;
  Texture2D Texture = {0};
};

//! @brief Data used by the editor to highlight certain stuff in editor mode.
struct TEditorData
{
  std::set<int> selectedBlockIds;
};


struct TContext
{
  const int TargetFPS;
  const TimeNanos FrameDurationNanos;
  int X = 0;

  explicit TContext(const int fps) : TargetFPS(fps), FrameDurationNanos(DurationFromFPS(fps)),
                                     rlPlayer_({}) {}

  void Tick()
  {
    frame_++;

    // Frame duration is fixed: We can run the core update (and draw)
    // loop as fast as we want. Deterministically producing frames
    // helps us record input events tagged to a specific frame.
    // Replaying the frames is then easy as we will always play them back
    // at the same 'rate' regardless of the vsync rate.
    // The core game logic also ensures that the system is idempotent:
    // Same inputs give the same outputs.
    // (a) Randomness/PRNG is seeded with specific values
    // (b) Most operations are over ints (not floats) - although the final
    //     rendering / per-frame values may still be in floats (so we 'snap'
    //     to determinstic ints within a frame). (Note: Floats are
    //     platform dependent especially on ARM/Android/Apple Silicon)
    //
    // These properties help us replay the game state deterministically and
    // also allow us to do cool things like train RL algorithms.
    frameTimeNs_ += FrameDurationNanos;
    frameTimeMs_ = MillisFromNanos(frameTimeNs_);
  }

  void SetupActionsHandlers_();
  //! @brief When randomizing the active player pos, use this to set up the initial position.
  void UpdateActivePlayerPos(const Vector2& startPos);
  //! @brief Sets a previously saved world for the coreloop to use.
  //! World objects are 'readonly' after this point. The grid/context
  //! will be updated to reflect the player actions/drawing/update cycles.
  void SetWorld(const std::shared_ptr<TWorld>& world, TContextPtr context,
    const std::shared_ptr<TGameActions>& replayActions);
  void SetupRL(TContextPtr context, std::shared_ptr<TRLTrain> rlTrain, std::shared_ptr<RLWeights> shared_weights = nullptr);
  void UpdateRLPlayerWeights(std::shared_ptr<RLWeights> weights);

  //! @brief Returns the stored world (readonly).
  std::shared_ptr<TWorld> World() const { return world_; }

  //! @brief Returns the current frame index.
  int Frame() const { return frame_; }

  //! @brief Returns frame time (ns) since the start of the update cycle.
  TimeNanos FrameTimeNs() const { return frameTimeNs_; }

  //! @brief Returns frame time (ms) since the start of the update cycle.
  TimeMillis FrameTimeMs() const { return frameTimeMs_; }

  //! \brief Loads a texture from a file (returns the cached version if it exists).
  Texture2D LoadTexture(const std::string& filename);

  //! \brief Loads a texture from the given texture and returns the raylib texture.
  Texture2D LoadTexture(TTexture& texture)
  {
    if (texture.Texture.id != 0) { return texture.Texture; }
    return (texture.Texture = LoadTexture(texture.Filename));
  }

  //! \brief Loads a sprite sheet from a file - uses the cached texture (if one exists)
  //! and fills in the sprite information.
  TSpriteSheet& LoadSpriteSheet(TSpriteSheet& spriteSheet);

  //! \brief Loads a texture from the cache.
  Texture2D GetTexture(const std::string& string) const;

  //! @brief Direct font accessor (legacy).
  Font LoadFont(std::string fontFile, int fontSize, int numCodepoints);

  //! @brief Loads and stores the font info in the given structur.
  Font LoadFont(TFont& font)
  {
    return (font.font_ = LoadFont(font.FontFile, font.FontSize, -1));
  }

  void DrawText(TFont& font, const Vector2& pos, const std::string& text,
    Color color);

  Rectangle DrawCenteredText(TFont& font, const Rectangle& rect, const std::string& text, Color color);


  //! @brief Draws animated sprite with the given sprite sheet and destination rectangle (and moves frames forward).
  //! Stops the animation if the stopAtIndex matches the current index (helps stop naturally instead of suddenly).
  // origin is in normalized [0, 1] range, where 0.5f is the center of the sprite.
  void DrawAnimSprite(TSpriteSheet& sheet, Rectangle dest, Color color, int stopAtIndex = -1,
    const float angle = 0.0f, Vector2 origin = {0.0f, 0.0f}) const;

  void AddActivePlayerPosition_(TContextPtr context);
  void InitDebug_(TContextPtr context);
  //! \brief Runs one tick (one frame) by moving time forward by frameTimeNs_.
  //! Update and Draw are independent - either or both can be called in the main loop.
  //! In generaly, if UpdateFrame is being called, but not DrawFrame, then the game
  //! is being run in a headless mode (i.e. no rendering).
  //! If DrawFrame is being called but not UpdateFrame, then the game is waiting for input.
  //! If many UpdateFrames are being issued followed by a single DrawFrame, it means the
  //! game will run at a higher speed (i.e. skipping frames).
  //! On the contrary, if many DrawFrames are issued following a single UpdateFrame, then
  //! the game runs at a lower speed (i.e. slo-mo).
  //! Pausing the game typically involves not calling UpdateFrame.
  void UpdateFrame(TContextPtr context);

  void DrawBlocks_(std::shared_ptr<TContext> context, TLayerDepth layerDepth) const;
  void DrawFrame(TContextPtr context);
  void DrawTexture(const TTexture& texture, Rectangle box, Color color) const;

  bool HandleActions(TContextPtr context, TPlayerActions& actions, bool replay);
  void InitBlockIfNeeded(TContextPtr context, const std::shared_ptr<ABlock>& block);

  std::shared_ptr<TBlock> SpawnBlock(TContextPtr context, std::shared_ptr<TBlock> blueprint, const Vector2& pos);
  void RemoveSpawnedBlock(ABlock& block);

  //! @brief Obtains the grid for the current game.
  const std::shared_ptr<TGrid>& Grid() const { return grid_; }


  //! @brief Returns the actions replay for the active player. 
  std::shared_ptr<TGameActions> GetActionsReplay(TContextPtr context, int nextRound = -1) const;

  //! @brief Returns the block with the given blockId.
  std::shared_ptr<ABlock> GetBlock(const TBlockId blockId) const
  {
    if (blockId < 0) { return nullptr; }
    const auto& it = blocks_.find(blockId);
    if (it != blocks_.end()) { return it->second; }
    else { return nullptr; }
  }

  //! @brief Gets the active block that consumes player actions (not replay actions).
  std::shared_ptr<ABlock> GetActionsHandlerBlock() const { return GetBlock(actionsHandlerId_); }

  //! @brief Gets the non-active/replay handler block that consumes replay actions (not active player actions).
  std::shared_ptr<ABlock> GetReplayHandlerBlock() const { return GetBlock(replayActionsHandlerId_); }

  //! @brief Returns the list of blocks that are selected by the screen position.
  std::set<TBlockId> GetSelectedObjects(Vector2 screenPos, bool onlySelectOne) const;

  //! @brief Converts screen pos to world pos based on the current camera.
  Vector2 GetWorldPosFromScreenPos(const Vector2 screenPos) const;

  void Reset(TContextPtr context)
  {
    rlPlayer_.UnloadRLEnv(context);
    frame_ = 0;
    frameTimeMs_ = 0;
    frameTimeNs_ = 0;
    world_ = nullptr;
    blocks_.clear();
    maxBlockId_ = InvalidBlockId;
    grid_ = nullptr;
    for (auto& [_, texture] : textures_) { RLPlays_UnloadTexture(texture); }
    textures_.clear();
    actions_ = nullptr;
    replayActions_ = nullptr;
    actionsHandlerId_ = InvalidBlockId;
    replayActionsHandlerId_ = InvalidBlockId;
    for (auto& [key, font] : fonts_) { RLPlays_UnloadFont(font); }
    fonts_.clear();
    childToParentIds_.clear();
    toRemove_.clear();
    screenRect_ = {};
#if DEBUG
    ResetDebug_();
#endif
    THeadless::ClearContent();
  }

  inline bool IsActionReplaying() const { return replayActions_ != nullptr; }
  void DrawEditor(const std::shared_ptr<TEditorData>& editorData, const std::shared_ptr<TContext>& context);
  void MoveBlockAbs(ABlock* block, const Vector2& vector2);
  void MoveBlockBy(ABlock* block, const Vector2& vector2);
  const TGameProgress* GetGameProgress() const;
  TGameProgress* UpdateGameProgress();
  void DrawText(const std::string& fontFile, const Font& font, std::string str, Vector2 pos, int fontSize,
    float spacing, Color color);
  TCamera& GetCamera() const;
  const Vector2& GetCellSize() const { return GetCamera().CellSize; }
  const TDebugGameInfo& GetDebugInfo() const { return debugInfo_; }
#if DEBUG
  void ResetDebug_();
  void SetDebug(const TDebugGameInfo& debugInfo) { debugInfo_ = debugInfo; }
#endif
  //! @brief Adds a sub-block for the given parent block and connects the dots within the context
  //! for further lookups.
  void AddSubBlock(TContextPtr context, const ABlock& parentBlock, const TBlock& subBlock,
    const std::string& subBlockName, const TEditorUpdateFn& editorUpdateBlockFn);


  //! @brief Returns the parent block id for the given child block id (if one exsts;
  //! otherwise, returns InvalidBlockId).
  [[nodiscard]] TBlockId GetParentBlockId(const TBlockId childBlockId) const
  {
    const auto it = childToParentIds_.find(childBlockId);
    if (it != childToParentIds_.end()) { return it->second.BlockId; }
    return InvalidBlockId;
  }

  //! @brief Returns the parent block info for the given child block id (if one exsts;
  //! otherwise, returns nullptr).
  [[nodiscard]] const TBlockInfo* GetParentBlockInfo(const TBlockId childBlockId) const
  {
    const auto it = childToParentIds_.find(childBlockId);
    if (it != childToParentIds_.end()) { return &it->second; }
    return nullptr;
  }

  [[nodiscard]] std::vector<TBlockId> GetChildrenBlockIds(const TBlockId parentBlockId) const
  {
    std::vector<TBlockId> children;
    for (const auto& [child, parent] : childToParentIds_)
    {
      if (parent.BlockId == parentBlockId) { children.push_back(child); }
    }
    return children;
  }

  void MirrorBlockViaEditor(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& originalBlock, ABlock* mirroredBlock);
  const Rectangle& ScreenRect() const { return screenRect_; }
  const std::map<TBlockId, std::shared_ptr<ABlock>>& GetBlocks() const { return blocks_; }

  void LoadContent(TContextPtr context);
  std::string GetWorldFilename() const;

#if DEBUG
  TFont DebugSmallFont = {"fonts/pixantiqua.ttf", 32, RED};
  TFont DebugLargeFont = {"fonts/pixantiqua.ttf", 48, RED};
#endif

private:
  //! @brief Sets the replay actions buffer for the next run (initial that can be overriden by the user
  //!        via the menu if scene transitions are enabled).
  //! @note While replaying the actions, the context will still record the actions for the active player
  //!       that is not receiving the recorded replay actions.
  void SetupInitialGameplay_(const std::shared_ptr<TGameActions>& actions);

  //! @brief Adds a new block to the grid with the given block box.
  void AddBlock(const TBlock& block);

  bool ConnectChildBlockId_(TBlockId parentBlockId, TBlockId childBlockId, const std::string& subBlockName,
    const TEditorUpdateFn& editorUpdateBlockFn);

  int frame_ = 0;
  TimeNanos frameTimeNs_ = 0;
  TimeMillis frameTimeMs_ = 0;

  std::shared_ptr<TWorld> world_;

  // Maintains the blocks based on unique block ids that can refer to other blocks.
  // This is especially useful for maintaining robustness when we are adding sub-blocks
  // and removing blocks at runtime to ensure we don't mistakenly leave dangling block/sub-block
  // links.
  std::map<TBlockId, std::shared_ptr<ABlock>> blocks_;
  std::vector<TBlockId> toRemove_;

  TBlockId maxBlockId_ = InvalidBlockId;

  Rectangle screenRect_ = {};
  std::shared_ptr<TGrid> grid_;
  std::map<std::string, Texture2D> textures_;
  std::map<std::string, Font> fonts_;
  std::shared_ptr<TGameActions> actions_;
  std::shared_ptr<TGameActions> replayActions_;
  std::map<TBlockId, TBlockInfo> childToParentIds_;

  TBlockId actionsHandlerId_ = InvalidBlockId;
  TBlockId replayActionsHandlerId_ = InvalidBlockId;

  void InitGrid_();
  void AddActionsForFrame_(const TPlayerActions& actions);
  void RemoveMarkedSubBlocks_(TContextPtr context);

  // This is where the magic happens: Replay the actions for the non-active player or run the RL env.
  void HandleInactivePlayerActions_(TContextPtr context);
  // Split into two methods so that we can get the actions first, update frame, then do post-update obs.
  void HandlePostInactivePlayerActions_(TContextPtr context);

  bool debugInit_ = false;
  TDebugGameInfo debugInfo_;
  TRLPlayer rlPlayer_;
  // Allow access to AddBlock from ABlock alone.
  friend struct ABlock;
};

//! @brief Simple accessor to prevent cyclic dependency of header files (blocks->tblock->ablock<->other blocks)
[[nodiscard]] std::shared_ptr<ABlock> GetABlock(const std::shared_ptr<TBlock>& block);
[[nodiscard]] std::shared_ptr<ABlock> GetABlock(const TBlock& block);

template <class T_Block>
[[nodiscard]] std::shared_ptr<T_Block> GetABlockFor(const TBlock& block)
{
  return std::static_pointer_cast<T_Block>(GetABlock(block));
}

//! @brief Clones the block and returns a copy of it.
TBlock CopyBlock(const TBlock& block);
} // namespace RLPlays
