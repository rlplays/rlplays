#pragma once

#include <base_block.h>
#include <base_types.h>
#include <functional>
#include <memory>
#include <vector>

#include "base_block.h"
#include <interactions.h>

// TODO(perumaal): This is a very initial implementation of a grid-based collision detection. It's not optimized
//                 and may need refactoring. It works really well though for all the use-cases we care about
//                 and an LLM can generate new blocks/characters using this header.

namespace RLPlays
{
struct ABlock;
struct TGrid;

//! @brief Options for finding blocks in the grid.
struct TFindBlocksInOptions
{
  //! @brief Expands the block's rectangle by this offset in all directions (if negative, contracts).
  //!        Works with {@related overrideRect} too if provided.
  Vector2 expandRect = {0, 0};

  //! @brief If passed in, then uses this rectangle instead of the block's rectangle.
  const Rectangle* overrideRect = nullptr;

  // TODO(perumaal): Sort this / compact so the struct is much smaller. For now, it's optimized to be a stack
  //                 var by the compiler as a const& so not a big deal.

  //! @brief If true, filter out trivial candidates that do not pass IsCandidate().
  bool filterOnlyInterestingCandidates = true;

  //! @brief If specified, only candidates that match these allowed traits are returned.
  //! If multiple traits are specified, matches one or more specified.
  TBlockTraits allowedTraits = TBlockTraits::None;

  //! @brief If true, then checks for collision and output the collision rectangle to {@see collisionMove.
  bool checkCollision = false;

  //! @brief Used by collision detection to calculate the back off vector to prefer a specific backward direction.
  Vector2 moveDelta = {};
};


//! @brief For the most common usecase, we have fewer than a handful of blocks per grid cell.
//!        So we optimize for that case by having 5 manual pointers and then an optional array.
//!        This ensures a 2D array with optional fragmented memory for the few rare cases where
//!        there are a bunch of moving blocks in the same cell.
struct TGridCells
{
  static constexpr auto ManualCellsCount = 5;
  ABlock *Cell1 = nullptr, *Cell2 = nullptr, *Cell3 = nullptr, *Cell4 = nullptr, *Cell5 = nullptr;
  ABlock** Cells = nullptr;

private:
  uint8_t Size = 0;
  uint8_t Capacity = ManualCellsCount;

public:
  TGridCells(const uint8_t capacity = ManualCellsCount) : Capacity(capacity)
  {
    if (Capacity > ManualCellsCount)
    {
      Cells = static_cast<ABlock**>(malloc((Capacity - ManualCellsCount) * sizeof(ABlock*)));
    }
  }

  uint8_t push_back(std::shared_ptr<ABlock> cell) { return push_back(cell.get()); }

  uint8_t push_back(ABlock* cell)
  {
    // Just to make sure we are not about to overflow our rather small grid cells' limits.
    DBG_ASSERT(Size < 250 && Capacity < 250);
    if (Size < ManualCellsCount)
    {
      if (Size == 0) { Cell1 = cell; }
      else if (Size == 1) { Cell2 = cell; }
      else if (Size == 2) { Cell3 = cell; }
      else if (Size == 3) { Cell4 = cell; }
      else if (Size == 4) { Cell5 = cell; }
      Size++;
      return (Size - 1);
      static_assert(ManualCellsCount == 5, "Push-back code only for 5 manual cells.");
    }
    const auto arraySize = Size - ManualCellsCount;
    auto arrayCapacity = Capacity - ManualCellsCount;
    if (arraySize >= arrayCapacity)
    {
      arrayCapacity = arraySize + ManualCellsCount;
      Capacity = uint8_t(arrayCapacity) + ManualCellsCount;
      const auto cells = static_cast<void*>(Cells);
      // No need to initialize all existing cells, just need a few more at the end.
      Cells = static_cast<ABlock**>(malloc(arrayCapacity * sizeof(ABlock*)));
      if (cells != nullptr)
      {
        memcpy(static_cast<void*>(Cells), cells, arraySize * sizeof(ABlock*));
        free(cells);
      }
    }
    const auto retIndex = Size++;
    Cells[arraySize] = cell;
    return retIndex;
  }

  int IndexOfBlock(TBlockId blockId) const;
  bool RemoveBlock(TBlockId blockId);

  bool remove_at(int index)
  {
    if (index < 0 || index >= Size) { return false; }
    if (index < ManualCellsCount)
    {
      if (index == 0) { Cell1 = Cell2; }
      if (index <= 1) { Cell2 = Cell3; }
      if (index <= 2) { Cell3 = Cell4; }
      if (index <= 3) { Cell4 = Cell5; }
      if (index <= 4)
      {
        if (Size > ManualCellsCount)
        {
          Cell5 = Cells[0];
          index = ManualCellsCount;
        }
        else { Cell5 = nullptr; }
      }
      static_assert(ManualCellsCount == 5, "Removal code only for 5 manual cells.");
    }

    if (index >= ManualCellsCount)
    {
      // e.g. index = 1; Size = 4;
      // Cells[2] through Cells[3] (2 cells = Size-index-1)
      // copied over to Cells[1]

      const auto arrayIndex = index - ManualCellsCount;
      memmove(static_cast<void*>(Cells + arrayIndex), static_cast<void*>(Cells + arrayIndex + 1),
        (Size - index - 1) * sizeof(ABlock*));
    }
    Size--;
    return true;
  }

  inline ABlock* operator[](const int index) const
  {
    DBG_ASSERT(index >= 0 && index < Size);
    if (index < ManualCellsCount)
    {
      if (index == 0) { return Cell1; }
      if (index == 1) { return Cell2; }
      if (index == 2) { return Cell3; }
      if (index == 3) { return Cell4; }
      if (index == 4) { return Cell5; }
      static_assert(ManualCellsCount == 5, "Indexing code only for 5 manual cells.");
    }
    return Cells[index - ManualCellsCount];
  }

  [[nodiscard]] int size() const { return Size; }

  ~TGridCells()
  {
    // No need to invoke the array destructor, as it's a simple flat PODS. In general, not recommended, but okay for our specific case.
    if (Cells != nullptr) { free(static_cast<void*>(Cells)); }
    Size = Capacity = 0;
  }
};

#if DEBUG
struct TGridCollision
{
  Rectangle collisionRect;
  Rectangle targetRect;
  Vector2 start, end;
};
#endif

struct TGridBlockInfo
{
  ABlock* Neighbor = nullptr;
  Vector2 CollisionBackoff = {0, 0};
  [[nodiscard]] bool IsCandidate(const ABlock* collider) const;
  //! @brief Checks if ourBox collides with thatBlock and if so, uses the moveBy hint to move back to avoid collision.
  //! i.e. moveBy is a hint that indicates how the block has moved in this frame to reach the current ourBox position.
  //! Might end up not using the moveBy at all if it's not possible to resolve the collision without doing some
  //! drastic moves.
  Vector2 CheckCollision(Rectangle ourBox, Vector2 movedBy, const ABlock* thatBlock) const;

  //! @brief Applies collision backoff to {@param t}'s {@code Box} rectangle and {@related ourRect_} 
  //! uniformly so collision detection continues correctly.
  void ApplyCollisionBackoff(ABlock* t);

  // Box for the grid cell that this block is part of.
  Rectangle CellBox;

  //! @brief Returns the current search rect for cells being searched.
  [[nodiscard]] const Rectangle& GetSearchRect() const { return searchRect_; }

  //! @brief Returns the current source rect for checking collisions if requested in the options.
  [[nodiscard]] const Rectangle& GetCollisionSourceRect() const { return ourRect_; }

private:
  void Reset(const ABlock* currentBlock, const TFindBlocksInOptions& options, Rectangle expandRect);

  Rectangle searchRect_;
  TBlockId ourBlockId_ = InvalidBlockId;
  Rectangle ourRect_ = {};
  std::vector<TBlockId> foundBlocks_;
  friend struct TGrid;
};
}
