#pragma once
#include <actions.h>
#include <base_types.h>
#include <context.h>
#include <cstdint>
#include <memory>
#include <interactions.h>

namespace RLPlays
{
struct TEditorData;
struct TBlock;

enum class TBlockType : int8_t;

// Runtime flags that are not serialized.
enum class TRunState : uint8_t
{
  None              = 0x00,
  RequirePostUpdate = 0x01,
  Inited            = 0x02,
  Removed           = 0x04,
  Invisible         = 0x08,
  ContentLoaded     = 0x10,
};


/**
 * @brief Base-class for all blocks. Derived classes must be of the form TXyzBlock where
 * Xyz is an enum value defined in TBlockType.
 * Block classes have the latitude to define their own Update/Draw cycles.

This special class has some glue in the serializer to help all derived blocks easily serialize their blocks
and add 'their' block types to the global TBlockType enum.

This is the only place where 'inheritance' makes a little bit of sense (and it's a hack).
And purely as an interface (although, in C++ terms, it's really an abstract base class not an interface with
pure virtual member functions).

Don't do anything complicated in the base 'class'/interface.

Interfaces/inheritance/etc are strictly prohibited elsewhere in this codebase.

In fact, use simple structs, minimal abstractions when possible.
 */
struct ABlock
{
  Rectangle Box = {0, 0, 0, 0};

  //! TODO Packing these 8-bit/16-bit enums leaves some space out... hmm.
  TLayerDepth Depth = TLayerDepth::Foreground;

  //! @brief The round this block is available in (and hence, for all subsequent rounds in this level).
  //!  This is specific to this game. Indicates which blocks show up in which rounds (for this current game/level).
  uint8_t Round = 1;


  // The following are not serialized but instead initialized by each block impl/instance as needed.
  TBlockTraits Traits = TBlockTraits::None;
  Serializer(ABlock, Box, Depth, Round, Traits)

  TBlockType BlockType = static_cast<TBlockType>(0);
  virtual ~ABlock() = default;

  //! @brief Called once per block before the first Update call.
  //! Flags are checked/set to TRunState::Inited.
  //! Must not initialize any rendering state. That's done in LoadContent.
  virtual void Init(TContextPtr context) {}


  //! @brief Called once per block before the first Draw call.
  //! Initialize textures, sounds, sprite sheets used for rendering/sounds.
  virtual void LoadContent(TContextPtr context) {}

  /**
   * @brief Update block for the current frame.
   *        NOTE: SHOULD NOT INVOKE ANY DRAWING LOGIC.
   *        Blocks may be updated in any order, so no assumptions should be made about
   *        the order in which neighboring blocks are updated.
   * @param context provides timing info, utilities and looking up the environment.
   */
  virtual void Update(TContextPtr context) {}

  /**
   * @brief Update after update block is called in case there are any changes
   *        from other blocks that affect this block.
   *        NOTE: SHOULD NOT INVOKE ANY DRAWING LOGIC.
   *        Blocks may be updated in any order, so no assumptions should be made about
   *        the order in which neighboring blocks are updated.
   * @param context provides timing info, utilities and looking up the environment.
   */
  virtual void PostUpdate(TContextPtr context) {}

  /**
   * @brief Draws the block for the current frame.
   *        MUST BE IDEMPOTENT AND ONLY DEPENDENT ON BLOCK/CONTEXT
   *        (PRNG MUST BE USED FROM THE CONTEXT ONLY).
   *        MUST NOT CHANGE ANY SERIALIZABLE STATE.
   * @param context provides timing info, utilities etc.
   */
  virtual void Draw(TContextPtr context) {}

  virtual void AddBlock(TContextPtr context, const TBlock& block);

  virtual void HandleActions(TContextPtr context, const TPlayerActions& actions) {}

  TBlockId GetBlockId() const { return BlockId; }

#if RLPLAYS_EDITOR
  // Editor stuff goes here.
  //! @brief Draws the block in editor mode. Override and draw box/hit-box etc as needed.
  virtual void EditorDraw(const std::shared_ptr<TEditorData>& editorData, TContextPtr context);

  virtual void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& originalBlock) {}
#endif
  virtual bool IsCandidateForMirroring(TContextPtr context) const { return true; }
  virtual bool DoesHandleActions(bool isReplay) const { return false; }

  virtual int GetCollectibleReward() { return 0; }

  //! @brief Convert from an old version of the block to the current version when we update/upgrade the block code.
  virtual void Convert(std::shared_ptr<TContext> context) {}

  //! @brief Gets the cells per block (use context's cell size to measure # of cells based on box size).
  int GetTotalCells(TContextPtr context);

  //! @brief Interact with another block. This serves as an API that avoid knowing about the specific type of blocks.
  //! Elevates all blocks to the same level (including human players, AI bots, dumb enemies and rocks).
  //! Fill in an (optional) interaction result if one was passed in (for example, how much hit was taken or if the bullet fired).
  virtual void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) {}
#if DEBUG
  //! @brief Called after all the blocks have been drawn (only in debug mode).
  virtual void DrawDebug(TContextPtr context) {}
#endif


  // Traits-related methods go here.
  [[nodiscard]] bool IsInitialized() const { return HasEnumValue(RunState, TRunState::Inited); }
  void SetInitialized() { RunState = AddEnumValue(RunState, TRunState::Inited); }
  const TRunState& GetRunState() const { return RunState; }

  void SetVisible(const bool visible)
  {
    if (visible) { RunState = RemoveEnumValue(RunState, TRunState::Invisible); }
    else { RunState = AddEnumValue(RunState, TRunState::Invisible); }
  }

#if RLPLAYS_TEST
  int GetBlockId() { return BlockId; }
#endif
  Vector2 PrevPos = INVALID_POS;

  // Just a bunch of shortcut code to avoid verbose, confusing stuff.
  // Exact match (all bits match).
  //! @brief Returns true if any of the {@param traits} match.
  [[nodiscard]] bool IsA(const TBlockTraits& traits) const { return HasEnumValue(Traits, traits); }

  //! @brief Returns the mask of the current traits with the provided block traits.
  [[nodiscard]] TBlockTraits MaskTraits(const TBlockTraits& blockTraits) const
  {
    return static_cast<TBlockTraits>(static_cast<uint32_t>(Traits) & static_cast<uint32_t>(blockTraits));
  }


  // At least one bit matches.
  [[nodiscard]] bool IsOneOf(const TBlockTraits& traits) const { return HasOneOfEnumValue(Traits, traits); }

  [[nodiscard]] bool IsOneOf(const TBlockTraits& trait1, const TBlockTraits& trait2) const
  {
    return HasOneOfEnumValue(Traits, AddEnumValue(trait1, trait2));
  }

  [[nodiscard]] bool IsOneOf(const TBlockTraits& trait1, const TBlockTraits& trait2,
    const TBlockTraits& trait3) const
  {
    return HasOneOfEnumValue(Traits, AddEnumValue(trait1, trait2, trait3));
  }

  [[nodiscard]] bool HasRunState(const TRunState& r1) const { return HasEnumValue(RunState, r1); }

  [[nodiscard]] bool HasRunState(const TRunState& r1, const TRunState& r2) const
  {
    return HasEnumValue(RunState, AddEnumValue(r1, r2));
  }

  // TODO(perumaal): The enum values are not aligned well - if there was an option to let the compiler
  //                 do the auto padding/alignment for us that will be great, but ah well. Especially
  //                 as we have private (8-bits), public (16/8-bits) interleaved :(.
  TRunState RunState = TRunState::None;

protected:
  ABlock()
  {
#if DEBUG
    debugContextBlockId_ = ++ABlock::DEBUG_BLOCK_ID;
#endif
  }
#if DEBUG
  TBlockId debugContextBlockId_ = 0;
  static TBlockId DEBUG_BLOCK_ID;
#endif

  TBlockId BlockId = -1;

  inline void RequestRunState(const TRunState&& runState) { RunState = AddEnumValue(RunState, runState); }

private:
  friend struct TWorld;
  friend struct TContext;
  friend struct TGrid;
  friend struct TGridColliders;
  friend struct TGridBlockInfo;
  friend struct TGridCells;
  friend struct TTemplateBlocks;
  friend struct TTemplateBlock;
};
} // namespace RLPlays
