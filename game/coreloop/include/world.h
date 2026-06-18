#pragma once
#include <context.h>
#include <game_blocks.h>
#include <game_types.h>
#include <nlohmann/json.hpp>
#include <serialize.h>
#include <vector>

#include <tblock.h>
#include "game.h"
#include "game_progress.h"

using json = nlohmann::json;

namespace RLPlays
{
//! @brief Common info such as camera, world state such as file info, progress, levels, version etc.
struct TWorldInfo
{
  TCamera Camera = {};
  Vector2 WorldSize = {};
  std::string Filename = DefaultWorldFilename;
  //! @brief Brief description (for templates).
  std::string Desc = "";
  //! @brief Main block id (for templates).
  TBlockId MainBlockId = InvalidBlockId;
  //! @brief Brief description for the secondary block (for templates).
  std::string Desc2 = "";
  //! @brief Secondary block id (for templates).
  TBlockId MainBlockId2 = InvalidBlockId;
  //! @brief Brief description for the tertiary block (for templates).
  std::string Desc3 = "";
  //! @brief Tertiary block id (for templates).
  TBlockId MainBlockId3 = InvalidBlockId;
  int MaxRounds = 1;
  uint8_t MajorVersion = TVersion::MajorVersion;
  uint8_t MinorVersion = TVersion::MinorVersion;
  uint16_t PatchVersion = TVersion::PatchVersion;
  uint32_t Version = TVersion::BlockVersion;

  //! @brief Live play state for this current loaded world (only certain things need to be serialized).
  TGameProgress GameProgress;

  Serializer(TWorldInfo, Camera, WorldSize, Filename, GameProgress, Version, MajorVersion, MinorVersion, PatchVersion,
      MaxRounds, Desc, MainBlockId, Desc2, MainBlockId2, Desc3,
      MainBlockId3) void Convert(std::shared_ptr<TContext> context);
};

//! @brief Root node for all blocks across various rounds. Each round within a level in the game
//! is a variant of how the player plays solo/against AI/other players/timed replay and must
//! be instantiated afresh with a new world state / progress from the save 'map'/game.
struct TWorld
{
  TWorldInfo WorldInfo;

  //! @brief Original (stored) blocks. DO NOT USE THIS DIRECTLY. Use Context->GetBlocks() during runtime.
  std::vector<TBlock> Blocks;

  Serializer(TWorld, WorldInfo, Blocks, ActionHandlerBlockId, ReplayActionHandlerBlockId)

      void SetupCamera_();
  void CheckForResize_();
  void Load(const TGameLoadInfo* loadInfo, TContextPtr context);

  /**
   * @brief Get the cell rectangle with the specified coordinates.
   * By default, returns a 1x1 cell if either wc or hc is not provided.
   */
  Rectangle CellAt(int xc, int yc, int wc = 1, int hc = 1) const;


  const Camera2D& Get2DCamera() const { return camera_; }
  void Reload();
  void Convert(TContextPtr context);


#ifdef RLPLAYS_EDITOR
  std::shared_ptr<TBlock> AddBlock(const TBlock& block)
  {
    auto tBlock = std::make_shared<TBlock>();
    *tBlock = block;
    tBlock->Block->BlockId = Blocks.size();
    Blocks.push_back(*tBlock);
    return tBlock;
  }


  void UpdateBlock(const TBlock& block, int blockId, TBlockType blockType);
  void DeleteBlock(int blockId);

  //! @brief Returns true if the block is on the 'left' side of the screen/viewport.
  bool ShouldMirrorBlock(const TBlock& block) const;
  TBlock* GetBlockWithIdForEditor(TBlockId blockId);
  void ResetBlocks();
#endif

  void PrepareForSave(const std::shared_ptr<TContext>& shared);
  int GetBlocksTotalCells(TContextPtr context) const;
  int GetBlocksTotalCellTypes(TContextPtr context) const;

  // Useful for something like the editor to ensure we always start fresh
  // when loading a new world. (State changes will not affect the world blocks)
  static bool ShouldCopyAddBlocks;

  //! @brief Returns true if the player is on the 'left' side (primary action side).
  [[nodiscard]] bool IsPlayerPrimaryAction() const
  {
    switch (WorldInfo.GameProgress.GameType)
    {
    case TGameType::SinglePlayer:
    case TGameType::PlayerVsPrior:
    case TGameType::PlayerVsAI:
    case TGameType::PlayerVsPlayer: return true;
    case TGameType::AIVsPlayer:
    case TGameType::PriorVsPlayer: return false;
    default: return false;
    }
  }

  inline TBlockId GetActionHandlerBlockId() const
  { return IsPlayerPrimaryAction() ? ActionHandlerBlockId : ReplayActionHandlerBlockId; }

  inline TBlockId GetReplayActionHandlerBlockId() const
  { return IsPlayerPrimaryAction() ? ReplayActionHandlerBlockId : ActionHandlerBlockId; }

private:
  Camera2D camera_;
  Vector2 screenSize_;

  // These are serialized with the world blocks. Not accessible as we may switch based on the game type.
  TBlockId ActionHandlerBlockId;
  TBlockId ReplayActionHandlerBlockId;
};
} // namespace RLPlays
