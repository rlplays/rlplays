#pragma once
#if !(defined(RLPLAYS_EDITOR) || defined(RLPLAYS_TEST))
static_assert(false, "block_templates.h MUST ONLY BE USED IN THE EDITOR/TEST BUILD.");
// Avoid having a giant #if/#endif spanning the entire file, as VS/VSCode will dim the display down.
#endif

#include <context.h>
#include <nlohmann/json.hpp>
#include <vector>

#include <tblock.h>

using json = nlohmann::json;

namespace RLPlays
{
//! @brief Holds the templatized blocks/names etc and helpers to create blocks with type info easily
//! for the editor primarily.
struct TTemplateBlocks
{
  std::vector<TTemplateBlock> Blocks;
  // Bunch of helpers to ease creating and manipulating blocks with types. These are a bit verbose but by
  // keeping them in one place, the rest of the code is clutter-free / duplication-free.

  //! @brief This variant helps create a new block of type T_Block and assigns it to the given TBlock shared_ptr
  //! reference.
  template <class T_Block>
  [[nodiscard]] static std::shared_ptr<T_Block> AssignNewBlockTo(const TBlockType& blockType,
    std::shared_ptr<TBlock>& blockPtrRef)
  {
    blockPtrRef = std::make_shared<TBlock>();
    blockPtrRef->BlockType = blockType;
    auto tBlock = std::make_shared<T_Block>();
    blockPtrRef->Block = std::static_pointer_cast<ABlock>(tBlock);
    blockPtrRef->Block->BlockType = blockType;
    blockPtrRef->Block->BlockId = 0;
    return tBlock;
  }

#if RLPLAYS_TEST
// Need access to block id for testing.
  static std::shared_ptr<ABlock> MakeBrickBlock(const Vector2 pos = {}, const Vector2 size = {96, 96},
    TBlockId blockId = 0)
  {
    std::shared_ptr<TBlock> tblock;
    auto block = TTemplateBlocks::AssignNewBlockTo<TBrickBlock>(TBlockType::Brick, tblock);
    block->Box = {pos.x, pos.y, size.x, size.y};
    block->Tex = TTexture("sprites/tile_0047.png", {(int)size.x, (int)size.y});
    block->BlockId = blockId;
    return block;
  }
#endif


  template <class T_Block>
  static std::shared_ptr<TBlock> CreateBlock(std::shared_ptr<ABlock> ablock, const TBlockType& blockType)
  {
    TBlock block;
    block.BlockType = blockType;
    auto tBlock = std::make_shared<T_Block>();
    block.Block = ablock;
    block.Block->BlockType = blockType;
    return std::make_shared<TBlock>(block);
  }

  template <class T_Block>
  std::shared_ptr<TBlock> AddBlock(const std::string& name, std::shared_ptr<ABlock> ablock, const TBlockType& blockType)
  {
    auto block = CreateBlock<T_Block>(ablock, blockType);
    block->Block->BlockId = 0;
    Blocks.push_back({name, block, false});
    return block;
  }

  void AddBlockFromTemplateFile(const std::string& name, TBlock& block, const TBlockType& blockType)
  {
    auto tblock = std::make_shared<TBlock>(blockType, block.Block);
    Blocks.push_back({name, tblock, true});
  }

  template <class T_Block>
  std::shared_ptr<T_Block> AddBlock(const std::string& name, const TBlockType& blockType)
  {
    auto block = std::make_shared<TBlock>();
    block->BlockType = blockType;
    auto tBlock = std::make_shared<T_Block>();
    block->Block = std::static_pointer_cast<ABlock>(tBlock);
    block->Block->BlockType = blockType;
    block->Block->BlockId = 0;
    Blocks.push_back({name, block, false});
    return tBlock;
  }
};
}
