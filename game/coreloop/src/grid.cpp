#include <base_block.h>
#include <grid.h>

#include "raymath.h"
using namespace RLPlays;

using enum RLPlays::TBlockTraits;

void TGrid::PrepareFrame(const TDebugGameInfo& debug)
{
  stats_.TotalBlocksFnCalls += stats_.NumBlocksFnCalls;
  stats_.TotalCellsFnCalls += stats_.NumCellsFnCalls;
  stats_.NumCellsFnCalls = 0;
  stats_.NumBlocksFnCalls = 0;
  stats_.NumFrames++;
  stats_.AvgBlocksFnCalls = float(stats_.TotalBlocksFnCalls) / float(stats_.NumFrames);
  stats_.AvgCellsFnCalls = float(stats_.TotalCellsFnCalls) / float(stats_.NumFrames);
  isDebugMode_ = debug.ShowDebugView;
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  TGrid::DebugTrackCollisions = debug.ShowDebugView;
  TGrid::DebugCollisions.reserve(10);
  TGrid::DebugCollisions.clear();
#endif
}


bool TGrid::AddBlock(const std::shared_ptr<ABlock>& block)
{
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  DebugVerifyBlock(*block, block->Box, false);
#endif
  FindGridCellsFn(block->Box, [&](int _, auto& cells)
  {
    if (cells.IndexOfBlock(block->BlockId) == InvalidBlockId)
    {
      cells.push_back(block);
    }
    return true;
  });
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  DebugVerifyBlock(*block, block->Box, true);
#endif
  return true;
}

void TGrid::RemoveBlock(const ABlock& block)
{
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  DebugVerifyBlock(block, block.Box, true);
#endif
  FindGridCellsFn(block.Box, [&](int _, auto& cells)
  {
    cells.RemoveBlock(block.BlockId);
    return true;
  }, false);
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  DebugVerifyBlock(block, block.Box, false);
#endif
}

bool TGrid::MoveBlock(ABlock* block, const Rectangle& oldRect)
{
  // Most blocks don't move, so early-out.
  if (AreRectsSame(oldRect, block->Box)) { return false; }

  const auto blockId = block->BlockId;
  const auto& toRemoveRect = oldRect;
  // We should do a diff and only selectively remove/add e.g. use a variation of DiffRect(oldRect, newRect);
  // This is complicated and requires handling different cases for which the tradeoffs are not worth it:
  // 1. If the oldRect and newRect are in the same cell, we may end up removing / adding the same cell nevertheless, so moot point.
  // 2. Diff'ing two rects may be a region comprising at most 3 rects, which is expensive.
  // Computationally, it is not clear if it's worth the tradeoff. We also optimize the grid cell to not use fragmented heap
  // (the first few elements are directly indexed into the 2D grid, so it's cache friendly and non-fragmented).
  // So for now, we do the simple thing and perhaps not the '100% efficient' thing.
  //    //auto toRemoveRect = DiffRect(oldRect, newRect);
  //    //auto toAddRect = DiffRect(newRect, oldRect);

  FindGridCellsFn(toRemoveRect, [&](int _, auto& cells)
  {
    cells.RemoveBlock(blockId);
    return true;
  }, false);
  const auto& toAddRect = block->Box;
  FindGridCellsFn(toAddRect, [&](int _, auto& cells)
  {
    if (cells.IndexOfBlock(blockId) == InvalidBlockId)
    {
      cells.push_back(block);
    }
    return true;
  });
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  DebugVerifyBlock(*block, block->Box, true);
#endif

  return true;
}

void TGrid::FindNeighbors(const ABlock* const block, const std::function<bool(TGridBlockInfo&)>& fnHandleBlock,
  const TFindBlocksInOptions& options)
{
  // Make sure to include neighbors.
  blockInfo_.Reset(block, options, expandRect_);
  auto& blocks = blockInfo_.foundBlocks_;
  const auto blockId = block->BlockId;
  FindGridCellsFn(blockInfo_.searchRect_, [&](int _, const TGridCells& cells)
  {
    for (int i = 0; i < cells.size(); ++i)
    {
      auto* other = cells[i];
      auto otherBlockId = other->BlockId;
      // O(n^2) but n is small (usually < 3), so okay for now and is unfragmented (inside the 2D array directly instead of a separate
      // array outside the grid) so it's cache friendly too.
      if (otherBlockId == blockId ||
        std::find(blocks.begin(), blocks.end(), otherBlockId) != blocks.end()) { continue; }
      // This won't allocate unless we have a lot of blocks in the same cell (which is rare, and if so,
      // it will retain that capacity).
      blocks.push_back(otherBlockId);
      // Do as much work here before calling the lambda to avoid doing unnecessary work.
      // Also, helps keep the caller simple and focus on logic rather than worrying about these knitty gritty details.
      if (options.filterOnlyInterestingCandidates && !blockInfo_.IsCandidate(other)) { continue; }
      if (options.allowedTraits != TBlockTraits::None && !other->IsOneOf(options.allowedTraits)) { continue; }
      if (options.checkCollision)
      {
#if DEBUG
        constexpr bool isDebug = true;
#else
        constexpr bool isDebug = false;
#endif
        blockInfo_.CollisionBackoff = blockInfo_.CheckCollision(blockInfo_.ourRect_, options.moveDelta, other);
        if (IsEmptyVec(blockInfo_.CollisionBackoff)) { continue; }
      }
      blockInfo_.Neighbor = other;
      ++stats_.NumBlocksFnCalls;
      if (!fnHandleBlock(blockInfo_)) return false;
      // TODO(perumaal): check for invalidation if the lambda added/removed/moved a block in our cell, rerun the loop if needed.
      // If it added, no problem, size will change, we will naturally visit the new block.
      // If a block was removed from the middle, we might revisit a block but that's okay, we have checks to prevent it.
      // So for now, don't do anything special to handle invalidation.     
    }
    return true;
  }, /*includeEmptyCells*/false);
}

void TGrid::FindNeighborsWithinDistance(const ABlock* block, const std::function<bool(TGridBlockInfo&)>& fnHandleBlock,
  int maxCellDistance)
{
  blockInfo_.Reset(block, {}, expandRect_);
  const int blockId = block->BlockId;
  int dx = 0, dy = 0, tx = 0, ty = 0, x = 0, y = 0;
  int dir = 0;
  const int cellX = CellXFromPixel(block->Box.x), cellY = CellYFromPixel(block->Box.y);
  blockInfo_.CellBox.width = CellSize.x;
  blockInfo_.CellBox.height = CellSize.y;
  if (maxCellDistance < 0)
  {
    maxCellDistance = std::max(
      std::max(std::abs(Size.x - cellX), std::abs(Size.y - cellY)),
      std::max(std::abs(cellX), std::abs(cellY)));
  }

  // Spiral outwards in a square pattern.
  // Pos tracks top-left corner of the square.
  for (int pos = 1; pos <= maxCellDistance; pos++)
  {
    const int to = (pos * 2);
    for (dir = 0; dir <= 3; ++dir)
    {
      dx = dy = 0;
      x = y = (-pos); // Start from top-left.
      tx = x;
      ty = y;
      if (dir == 0)
      {
        dx = 1;
        tx += to;
      } // right
      else if (dir == 1)
      {
        dy = 1;
        x += to;
        tx += to;
        ty += to;
      } // down
      else if (dir == 2)
      {
        dx = -1;
        x += to;
        y += to;
        ty += to;
      } // left
      else if (dir == 3)
      {
        dy = -1;
        y += to;
      } // up back to where we started
      // If the expanded region is completely out of view, skip.
      x += cellX;
      y += cellY;
      tx += cellX;
      ty += cellY;
      // TODO(perumaal): Need to optimize this a bit...
      //if (OutOfView_(x, y) && OutOfView_(tx, ty)) { continue; }
      //EnsureInView_(x, y);
      //EnsureInView_(tx, ty);
      for (; x != tx || y != ty; x += dx, y += dy)
      {
        if (OutOfView_(x, y)) { continue; }
        //TLOG(LOG_INFO, "Spiral: dir=%d, pos=%d, len=%d, x=%d, y=%d", dir, pos, to, x, y);
        const auto cellIndex = IndexOfCell(x, y);
        auto& cell = cells_[cellIndex];
        const auto cellSize = cell.size();
        if (cellSize == 0) { continue; }
        blockInfo_.CellBox.x = float(x * CellSize.x);
        blockInfo_.CellBox.y = float(y * CellSize.y);
        for (int i = 0; i < cellSize; ++i)
        {
          auto* other = cell[i];
          if (other->BlockId == blockId) { continue; }
          blockInfo_.Neighbor = other;
          ++stats_.NumBlocksFnCalls;
          if (!fnHandleBlock(blockInfo_)) return;
        }
      }
    }
  }
}

#define CALL_FN(index)                          \
  auto& cell = cells_[index];                   \
  if (includeEmptyCells || cell.size() > 0)     \
  {                                             \
    ++stats_.NumCellsFnCalls;                   \
    if (!fn(index, cell)) { return; }           \
  }                                             \
  void(0)

void TGrid::FindGridCellsFn(const Rectangle& rect, const std::function<bool(int cellIndex, TGridCells&)>& fn,
  const bool includeEmptyCells) const
{
  // If we had something akin to C#'s yield with simple stack-based iterator tracking, this wouldn't require an ugly
  // lambda being passed in. But it is what it is; at least avoids an expensive heap alloc, and the optimizer is pretty
  // good in inlining as needed.
  const auto tl = IndexOfPixel(rect.x, rect.y);
  const int R = int(rect.x + (rect.width + 0.5f) - 1.0f);
  const int B = int(rect.y + (rect.height + 0.5f) - 1.0f);
  CALL_FN(tl);
  // 80% case - just a rect covering a single cell or at most 4 cells.
  auto br = IndexOfPixel(R, B);
  if (tl == br) return;

  const auto tr = IndexOfPixel(R, rect.y);
  const auto bl = IndexOfPixel(rect.x, B);
  if (rect.width <= CellSize.x && rect.height <= CellSize.y)
  {
    CALL_FN(br);
    if (br != tr)
    {
      CALL_FN(tr);
    }
    if (bl != tl)
    {
      CALL_FN(bl);
    }
    return;
  }

  for (int ix = rect.x; ix < R; ix += CellSize.x)
  {
    for (int iy = rect.y; iy < B; iy += CellSize.y)
    {
      const auto idx = IndexOfPixel(ix, iy);
      if (idx != tl)
      {
        CALL_FN(idx);
      }
    }
  }
}

#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
//! @brief Renders the grid cells and highlights cells that have blocks in them.
//! Written with copilot/claude assist.
void TGrid::DebugRender(TContextPtr context)
{
  TGrid::DrawDebugCollisions();
  const auto pos = GetMousePosition();
  const auto worldPos = context->GetWorldPosFromScreenPos(pos);

  for (int ix = 0; ix < Size.x; ix++)
  {
    for (int iy = 0; iy < Size.y; iy++)
    {
      const auto cellIndex = IndexOfCell(ix, iy);
      const auto& cell = cells_[cellIndex];
      if (cell.size() > 0)
      {
        auto cellRect = Rectangle{
          TO_FLT(ix*CellSize.x), TO_FLT(iy*CellSize.y), TO_FLT(CellSize.x), TO_FLT(CellSize.y)
        };
        DrawRectangleLinesEx(cellRect, cell.size() * 3, LIGHTGRAY);
        for (int i = 0; i < cell.size(); ++i)
        {
          // Check if cell has blocks in order.
          const auto* block = cell[i];
          if (!CheckCollisionRecs(block->Box, cellRect))
          {
            DrawRectangleLinesEx(block->Box, 5, DARKGRAY);
            context->DrawText(context->DebugSmallFont, RectTopLeft(cellRect), "INVALID", RED);
          }

          if (DoesRectContainPos(cellRect, worldPos))
          {
            const float dx = float(context->DebugSmallFont.FontSize) * 1.5;
            Vector2 cellPos = {cellRect.x + 2, cellRect.y + 6};
            DrawRectangleLinesEx(block->Box, 5, BLUE);
            context->DrawText(context->DebugSmallFont, cellPos, std::to_string(block->BlockId), RED);
            cellPos.x += dx;
          }
        }
      }
    }
  }
}

void TGrid::DrawDebugCollisions()
{
  for (const auto& r : TGrid::DebugCollisions)
  {
    DrawRectangleRec(r.targetRect, YELLOW);
    DrawRectangleRec(r.collisionRect, GREEN);
    if (!IsZeroVec(r.start) && !IsZeroVec(r.end))
    {
      DrawLineEx(r.start, r.end, 6, RED);
      
      DrawLineEx(Vector2Lerp(r.start, r.end, 0.9), r.end, 8, BLUE);
    }
  }
}


//! @brief Verifies that the given block is either present/absent in the grid.
//!        Also verifies that no other cell contains the given block. 
void TGrid::DebugVerifyBlock(const ABlock& block, const Rectangle& rect, const bool present) const
{
  if (!isDebugMode_) { return; }
  int verifiedCount = 0;
  FindGridCellsFn(rect, [&](const int cellIndex, const TGridCells& cells)
  {
    for (int i = 0; i < cells.size(); ++i)
    {
      const auto isBlockPresent = cells.IndexOfBlock(block.GetBlockId()) >= 0;
      if (isBlockPresent != present)
      {
        TLOG(LOG_ERROR, "Invalid block - %s present in cells - id# %d @ cell index %d {%d, %d} x {%d, %d}",
          (present ? "is not" : "should not be"),
          block.BlockId, cellIndex, int(block.Box.x), int(block.Box.y),
          int(block.Box.width), int(block.Box.height));
        verifiedCount = -1;
      }
      else
      {
        if (verifiedCount >= 0) { ++verifiedCount; }
      }
    }
    if (cells.size() == 0 && !present && verifiedCount >= 0) { ++verifiedCount; }
    return true;
  }, true);
  if (verifiedCount <= 0)
  {
    TLOG(LOG_ERROR, "Invalid block - not present in grid id# %d @ {%d, %d} x {%d, %d}", block.BlockId, int(block.Box.x),
      int(block.Box.y), int(block.Box.width), int(block.Box.height));
  }

  for (int ix = 0; ix < Size.x; ix++)
  {
    for (int iy = 0; iy < Size.y; iy++)
    {
      const auto cellIndex = IndexOfCell(ix, iy);
      const auto& cell = cells_[cellIndex];
      if (cell.size() > 0)
      {
        const auto cellRect = Rectangle{
          TO_FLT(ix*CellSize.x), TO_FLT(iy*CellSize.y), TO_FLT(CellSize.x), TO_FLT(CellSize.y)
        };
        for (int i = 0; i < cell.size(); ++i)
        {
          // Check if cell has blocks in order.
          const auto* cellBlock = cell[i];
          if (!CheckCollisionRecs(cellBlock->Box, cellRect))
          {
            TLOG(LOG_ERROR, "Invalid block - not present in grid id# %d @ {%d, %d} x {%d, %d}", block.BlockId,
              int(cellBlock->Box.x),
              int(cellBlock->Box.y), int(cellBlock->Box.width), int(cellBlock->Box.height));
          }
        }
      }
    }
  }
}

void TGrid::AddDebugMoveVec(const Rectangle& box, const Vector2& moveVec)
{
  if (!TGrid::DebugTrackCollisions || IsZeroVec(moveVec)) { return; }
  const auto center = RectCenter(box);
  TGrid::DebugCollisions.push_back({.start = center, .end = Vector2Add(center, Vector2Scale(moveVec, 5))});
}


bool TGrid::DebugTrackCollisions = false;
std::vector<TGridCollision> TGrid::DebugCollisions = {};

#endif
