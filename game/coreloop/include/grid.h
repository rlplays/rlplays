#pragma once

#include <base_block.h>
#include <base_types.h>
#include <functional>
#include <memory>
#include <vector>
#include <grid_colliders.h>

#include "game_actions.h"

// TODO(perumaal): This is a very initial implementation of a grid-based collision detection. It's not optimized
//                 and may need refactoring. It works really well though for all the use-cases we care about
//                 and an LLM can generate new blocks/characters using this header.

namespace RLPlays
{
struct ABlock;
struct TGrid;

struct TGridStats
{
  int NumCellsFnCalls = 0;
  int NumBlocksFnCalls = 0;
  int TotalCellsFnCalls = 0;
  int TotalBlocksFnCalls = 0;
  int NumFrames = 0;
  float AvgBlocksFnCalls;
  float AvgCellsFnCalls;
};


//! @brief Organizes blocks in cells (each of size gridCellSize) with O(1) lookup/add/remove/updates.
//! Only blocks that require UpdateFrame (i.e. non-cosmetic blocks) are added to the grid.
//! Currently, we have a 'screenful' of blocks but this should work well for any large screen sizes.
//! For infinite spanning canvases perhaps a different LOD-based approach might work.
struct TGrid
{
  //! @brief Total size of the grid in world pixels.
  const Vec2i TotalSizeInPixels;
  //! @brief Size of each cell in world pixels.
  const Vec2i CellSize;
  //! @brief Size in number of cells.
  const Vec2i Size;
  //! @brief Total number of cells in this grid {@code Size.x*Size.y}.
  const int NumCells;
  // const std::vector<std::vector<std::shared_ptr<ABlock>>> Blocks;

  TGrid(const Vec2i size, const Vec2i gridCellSize) :
    TotalSizeInPixels(size), CellSize(gridCellSize), Size{size.x / gridCellSize.x, size.y / gridCellSize.y},
    MaxCellInsideIndex(int(Size.x) * int(Size.y)), MaxCellIncludingBorderIndex(MaxCellInsideIndex + 8),
    expandRect_({TO_FLT(CellSize.x), TO_FLT(CellSize.y), TO_FLT(CellSize.x), TO_FLT(CellSize.y)}),
    NumCells(Size.x * Size.y)
  {
    // 8 cells to cover the outside area TL; T; TR; R; BR; B; BL; L
    cells_ = new TGridCells[MaxCellIncludingBorderIndex];
#if RLPLAYS_TEST && DEBUG
    isDebugMode_ = true;
#endif
  }

  //! @brief Finds all blocks that overlap with the given block (optionally extended by extendOffset).
  void FindNeighbors(const ABlock* block, const std::function<bool(TGridBlockInfo&)>& fnHandleBlock,
    const TFindBlocksInOptions& options = {});

  void EnsureInView_(int& x, int& y) const
  {
    if (x < 0) { x = 0; }
    else if (x >= Size.x) { x = Size.x - 1; }
    if (y < 0) { y = 0; }
    else if (y >= Size.y) { y = Size.y - 1; }
  }

  //! @brief Finds blocks around the provided block expanding outward in a spiral. (i.e. ensuring
  //! nearest blocks are found first and then the farther ones).
  //! NOTE: No de-dupe-ing; blocks may be replicated in the callbacks.
  void FindNeighborsWithinDistance(const ABlock* block, const std::function<bool(TGridBlockInfo&)>& fnHandleBlock,
    int maxCellDistance = -1);
  ~TGrid() { delete[] cells_; }

#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  static void DrawDebugCollisions();
  void DebugVerifyBlock(const ABlock& block, const Rectangle& rect, bool present) const;
  static void AddDebugMoveVec(const Rectangle& box, const Vector2& moveVec);
#endif
PUBLIC_TEST:
  void PrepareFrame(const TDebugGameInfo& debug);
  inline TGridStats GetStats() const { return stats_; }

  // The Add/Remove/Move blocks etc are not public APIs to prevent inadvertent use outside; but we do need to test them as an API.
  bool AddBlock(const std::shared_ptr<ABlock>& block);
  void RemoveBlock(const ABlock& block);
  bool MoveBlock(ABlock* block, const Rectangle& oldRect);

  // Stack-based temporary function to find grid cells that overlap with the given rectangle.
  // It's super cheap compared to an iterator or other mechanisms.
  void FindGridCellsFn(const Rectangle& rect, const std::function<bool(int cellIndex, TGridCells&)>& fn,
    bool includeEmptyCells = true) const;

  inline int CellXFromPixel(const float px) const { return px < 0 ? px : int(px / CellSize.x); }
  inline int CellYFromPixel(const float py) const { return py < 0 ? py : int(py / CellSize.y); }

  inline const TGridCells& GetCellAt(int index) const { return cells_[index]; }

  inline bool OutOfView_(int x, int y) const { return x < 0 || y < 0 || x >= Size.x || y >= Size.y; }


  inline int IndexOfPixel(const float px, const float py) const
  {
    return IndexOfCell(CellXFromPixel(px), CellYFromPixel(py));
  }

  //! @brief Get the cell index for a particular cell location - has some special sauce to
  //! handle the out-of-bounds cases too.
  inline int IndexOfCell(const int x, const int y) const
  {
    if (x >= 0 && x < Size.x && y >= 0 && y < Size.y)
    {
      return y * Size.x + x;
    }
    // TL; T; TR; R; BR; B; BL; L
    if (x < 0 && y < 0) { return MaxCellInsideIndex; }                          // TL
    if (x >= 0 && y < 0 && x < Size.x) { return MaxCellInsideIndex + 1; }       // Top
    if (x >= Size.x && y < 0) { return MaxCellInsideIndex + 2; }                // TR
    if (x >= Size.x && y >= 0 && y < Size.y) { return MaxCellInsideIndex + 3; } // Right
    if (x >= Size.x && y >= Size.y) { return MaxCellInsideIndex + 4; }          // BR
    if (x >= 0 && x < Size.x && y >= Size.y) { return MaxCellInsideIndex + 5; } // Bottom
    if (x < 0 && y >= Size.y) { return MaxCellInsideIndex + 6; }                // BL
    if (x < 0) { return MaxCellInsideIndex + 7; }                               // Left

    // To please the compiler; we will never reach here.
    DBG_ASSERT(false);
    return -1;
  }

#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  void DebugRender(TContextPtr context);
#endif
  const int MaxCellInsideIndex;
  const int MaxCellIncludingBorderIndex;

private:
  const Rectangle expandRect_;
  TGridCells* cells_ = nullptr;
  TGridBlockInfo blockInfo_ = {};
  bool isDebugMode_ = false;
  mutable TGridStats stats_;
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  // This is called from the render thread, so it's thread-safe (and debug-only).
  static bool DebugTrackCollisions;
  static std::vector<TGridCollision> DebugCollisions;
  friend struct TGridBlockInfo;
#endif
  
  friend struct TContext;
};

inline bool IsRectOnPlatform(const Rectangle& rect, const Rectangle& platform)
{
  if (IsEmptyRect(rect) || IsEmptyRect(platform))
  {
    return false;
  }
  if (DoesRectContainX(platform, rect.x) || DoesRectContainX(rect, platform.x))
  {
    return (Bottom(rect) >= platform.y && Bottom(rect) <= Bottom(platform));
  }
  return false;
}
} // namespace RLPlays
