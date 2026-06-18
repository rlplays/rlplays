#include <gtest/gtest.h>
#include <grid.h>
#include <grid_colliders.h>
#include <base_block.h>
#include <iostream>

#include "world_fileset.h"
#include "game.h"
#include <world_editor.h>
#include <test_utils.h>

using namespace RLPlays;
constexpr Vector2 CellSize = {20, 20};
constexpr Vec2i CellSizei = {20, 20};

TEST(GridTest, TestCells)
{
  auto fileset = TWorldFiles::Load(DefaultWorldFilesetName);
  TGameLoadInfo loadInfo = {.Filename = fileset->SelectedFile.Filename};

  auto gameInfo = LoadGame(*fileset, loadInfo);
  auto ctx = gameInfo.Game->Context;
  auto numCells = gameInfo.Game->World->GetBlocksTotalCells(ctx);
  EXPECT_GT(numCells, 1);

  auto playerBlock = TPlayerBlock();
  playerBlock.Box = {0, 0, 96, 96};
  EXPECT_EQ(playerBlock.GetTotalCells(ctx), 1);
  playerBlock.Box = {0, 0, 96, 192};
  EXPECT_EQ(playerBlock.GetTotalCells(ctx), 2);
  playerBlock.Box = {0, 0, 192, 192};
  EXPECT_EQ(playerBlock.GetTotalCells(ctx), 4);
  // Also, it's rounded down.
  playerBlock.Box = {0, 0, 196, 196};
  EXPECT_EQ(playerBlock.GetTotalCells(ctx), 4);
}

TEST(GridTest, TGridCells_Basics)
{
  {
    TGridCells cells;

    EXPECT_EQ(cells.size(), 0);
    auto block1 = TTemplateBlocks::MakeBrickBlock({0, 0}, {96, 196}, 1);
    auto block2 = TTemplateBlocks::MakeBrickBlock({50, 0}, {96, 196}, 2);
    auto block3 = TTemplateBlocks::MakeBrickBlock({100, 0}, {96, 196}, 3);
    auto block4 = TTemplateBlocks::MakeBrickBlock({0, 100}, {96, 196}, 4);
    auto block5 = TTemplateBlocks::MakeBrickBlock({100, 100}, {96, 196}, 5);
    auto block6 = TTemplateBlocks::MakeBrickBlock({200, 100}, {96, 196}, 6);
    auto block7 = TTemplateBlocks::MakeBrickBlock({300, 100}, {96, 196}, 7);
    cells.push_back(block1);
    EXPECT_EQ(cells.size(), 1);
    cells.push_back(block2);
    EXPECT_EQ(cells.size(), 2);

    cells.push_back(block3);
    EXPECT_EQ(cells.size(), 3);

    cells.push_back(block4);
    EXPECT_EQ(cells.size(), 4);

    cells.push_back(block5);
    EXPECT_EQ(cells.size(), 5);

    cells.push_back(block6);
    EXPECT_EQ(cells.size(), 6);

    cells.push_back(block7);
    EXPECT_EQ(cells.size(), 7);

    for (int i = 0; i < 6; ++i)
    {
      EXPECT_EQ(cells[i]->GetBlockId(), i + 1);
      EXPECT_EQ(cells[i]->Box.width, 96);
      EXPECT_EQ(cells[i]->Box.height, 196);
      EXPECT_EQ(cells[i]->Box.y, i < 3 ? 0 : 100);
    }
    EXPECT_EQ(cells[0]->Box.x, 0);
    EXPECT_EQ(cells[1]->Box.x, 50);
    EXPECT_EQ(cells[2]->Box.x, 100);
    EXPECT_EQ(cells[3]->Box.x, 0);

    cells.remove_at(1);

    for (int i = 0; i < 6; ++i)
    {
      EXPECT_EQ(cells[i]->GetBlockId(), i == 0 ? i + 1 : i + 2);
      EXPECT_EQ(cells[i]->Box.width, 96);
      EXPECT_EQ(cells[i]->Box.height, 196);
      EXPECT_EQ(cells[i]->Box.y, i < 2 ? 0 : 100);
    }

    EXPECT_EQ(cells.size(), 6);
    EXPECT_EQ(cells[0]->Box.x, 0);
    // EXPECT_EQ(cells[1]->Box.x, 50);
    EXPECT_EQ(cells[1]->Box.x, 100);
    EXPECT_EQ(cells[2]->Box.x, 0);
    EXPECT_EQ(cells[3]->Box.x, 100);
    EXPECT_EQ(cells[4]->Box.x, 200);
    EXPECT_EQ(cells[5]->Box.x, 300);

    for (int i = 0; i < 6; ++i)
    {
      cells.remove_at(0);
      EXPECT_EQ(5 - i, cells.size());
    }
    // Ensure cells frees up correctly here.
  }
  {
    // Check empty cells logic.
    TGridCells cells;
    EXPECT_EQ(cells.size(), 0);

    std::vector<std::shared_ptr<ABlock>> blocks;
    for (int run = 0; run < 10; ++run)
    {
      printf("Run %d / cells size %d\n", run, cells.size());
      constexpr auto COUNT = 100;
      // Add an arbitrary number of cells.
      for (int i = 0; i < COUNT; ++i)
      {
        auto block = TTemplateBlocks::MakeBrickBlock({float(i * 100), 0}, {96, 196}, i + 1);
        blocks.push_back(block); // Cells only maintain a raw pointer, so we have to hold on to the shared pointer.
        EXPECT_EQ(cells.push_back(block), i);
        EXPECT_EQ(cells.size(), i + 1);
        EXPECT_EQ(cells[i]->GetBlockId(), i + 1);
        EXPECT_EQ(cells[i]->Box.x, i * 100);
        for (int j = 0; j < i; ++j)
        {
          EXPECT_EQ(cells[j]->GetBlockId(), j + 1);
          EXPECT_EQ(cells[j]->Box.x, j * 100);
        }
      }
      for (int i = 0; i < COUNT; ++i)
      {
        EXPECT_TRUE(cells.remove_at(0));
        EXPECT_EQ(cells.size(), COUNT - i - 1);
        auto next = i + 2;
        for (int j = 0; j < COUNT - i - 1; ++j)
        {
          EXPECT_EQ(cells[j]->GetBlockId(), next + j);
          EXPECT_EQ(cells[j]->Box.x, (next + j - 1) * 100);
        }
      }

      EXPECT_EQ(cells.size(), 0);
      blocks.clear();
      for (int i = 0; i < COUNT; ++i)
      {
        auto block = TTemplateBlocks::MakeBrickBlock({float(i * 100), 0}, {96, 196}, i + 1);
        blocks.push_back(block); // Cells only maintain a raw pointer, so we have to hold on to the shared pointer.
        EXPECT_EQ(cells.push_back(block), i);

        EXPECT_EQ(cells.size(), i + 1);
        EXPECT_EQ(cells[i]->GetBlockId(), i + 1);
        EXPECT_EQ(cells[i]->Box.x, i * 100);
        for (int j = 0; j < i; ++j)
        {
          EXPECT_EQ(cells[j]->GetBlockId(), j + 1);
          EXPECT_EQ(cells[j]->Box.x, j * 100);
        }
      }
      for (int i = 0; i < COUNT; ++i) { EXPECT_TRUE(cells.RemoveBlock(TBlockId(i + 1))); }
      EXPECT_EQ(cells.size(), 0);
      for (int i = 0; i < COUNT; ++i) { EXPECT_FALSE(cells.RemoveBlock(TBlockId(i + 1))); }
      blocks.clear();
    }
  }
}


TEST(GridTest, TGrid_Basics)
{
  // 6x5 grids, 30 cells total.
  TGrid grid({120, 100}, CellSizei);

  EXPECT_EQ(grid.IndexOfPixel(0, 0), 0);
  EXPECT_EQ(grid.IndexOfPixel(60, 50), 15); // half-way through; rounding down 30/2.
  EXPECT_EQ(grid.IndexOfPixel(100, 80), 29);
  EXPECT_EQ(grid.IndexOfPixel(-100, -50), 30 + 0);
  EXPECT_EQ(grid.IndexOfPixel(50, -50), 30 + 1);
  EXPECT_EQ(grid.IndexOfPixel(500, -200), 30 + 2);
  EXPECT_EQ(grid.IndexOfPixel(500, 50), 30 + 3);
  EXPECT_EQ(grid.IndexOfPixel(700, 300), 30 + 4);
  EXPECT_EQ(grid.IndexOfPixel(30, 100), 30 + 5);
  EXPECT_EQ(grid.IndexOfPixel(-1, 101), 30 + 6);
  EXPECT_EQ(grid.IndexOfPixel(-1, 0), 30 + 7);

  int verifyCount = 0;
  int totalCount = 0;
  for (int ix = 0; ix < grid.Size.x; ++ix)
  {
    for (int iy = 0; iy < grid.Size.y; ++iy)
    {
      ++totalCount;
      grid.FindGridCellsFn({float(ix), float(iy), 1, 1},
        [&](int index, const TGridCells& cells) -> bool
        {
          EXPECT_EQ(cells.size(), 0);
          ++verifyCount;
          return true;
        });
    }
  }
  EXPECT_EQ(verifyCount, totalCount);
}

std::shared_ptr<ABlock> AddBrickBlock(std::vector<std::shared_ptr<ABlock>>& blocks, const Vector2 pos = {},
  const Vector2 size = {96, 96}, TBlockId blockId = 0)
{
  const auto ret = TTemplateBlocks::MakeBrickBlock(pos, size, blockId);
  blocks.push_back(ret);
  return ret;
}

TEST(GridTest, TGrid_CellTest)
{
  TGrid grid({120, 100}, CellSizei);

  const auto cx = CellSizei.x;
  const auto cy = CellSizei.y;

  // Add, Remove, Update blocks as usual.
  // Find grid cells that overlap with a given rectangle.
  // Check for collisions.
  std::vector<std::shared_ptr<ABlock>> blocks;
  for (int run = 0; run < 10; ++run)
  {
    TBlockId id = 0;
    // Looks like this:
    // ***
    //  * (2 blocks per cell here)
    //  **
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {0, 0}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx, 0}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx * 2, 0}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx, cx}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx, cx}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx, cx * 2}, CellSize, ++id)));
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {cx * 2, cx * 2}, CellSize, ++id)));

    int verifyCount = 0;
    grid.FindGridCellsFn({float(cx), float(cx), CellSize.x, CellSize.y},
      [&](int index, const TGridCells& cells) -> bool
      {
        EXPECT_EQ(cells.size(), 2);
        ++verifyCount;
        return true;
      });
    EXPECT_EQ(verifyCount, 1);
    int numBlocks = 0;
    for (int i = 0; i < grid.MaxCellIncludingBorderIndex; ++i)
    {
      numBlocks += grid.GetCellAt(i).size();
    }

    EXPECT_EQ(numBlocks, id);

    for (int i = 0; i < blocks.size(); ++i)
    {
      grid.RemoveBlock(*blocks[i]);
    }

    int count = 0;
    for (int j = 0; j < grid.MaxCellIncludingBorderIndex; ++j)
    {
      count += grid.GetCellAt(j).size();
    }
    EXPECT_EQ(count, 0);
    blocks.clear();
  }
}

TEST(GridTest, TGrid_MoveBlock)
{
  TGrid grid({120, 100}, CellSizei);
  // Add, Remove, Update blocks as usual.
  // Find grid cells that overlap with a given rectangle.
  // Check for collisions.
  std::vector<std::shared_ptr<ABlock>> blocks;
  for (int run = 0; run < 10; ++run)
  {
    TBlockId id = 0;
    // Looks like this:
    // ***
    //  * (2 blocks per cell here)
    //  **
    EXPECT_TRUE(grid.AddBlock(AddBrickBlock(blocks, {0, 0}, CellSize, ++id)));
    auto block = blocks[0].get();
    float dcxy = 7;
    for (float cxy = 0; cxy < 100; cxy += dcxy)
    {
      auto oldRect = block->Box;
      block->Box.x += (cxy);
      block->Box.y += (cxy) / 2.0f;
      grid.MoveBlock(block, oldRect);
      std::vector<int> containedCellIndices;
      grid.FindGridCellsFn(block->Box, [&](int cellIndex, TGridCells& cells) -> bool
      {
        EXPECT_TRUE(cells.IndexOfBlock(block->GetBlockId()) >= 0) << " Run " << run << " ; cxy " << cxy <<
            "; cellIndex " << cellIndex << "; \n";;
        containedCellIndices.push_back(cellIndex);
        return true;
      });
      EXPECT_FALSE(containedCellIndices.empty());
      for (int cx = 0; cx < grid.Size.x; ++cx)
      {
        for (int cy = 0; cy < grid.Size.y; ++cy)
        {
          auto cellIndex = grid.IndexOfCell(cx, cy);
          auto& cells = grid.GetCellAt(cellIndex);
          if (std::find(containedCellIndices.begin(), containedCellIndices.end(), cellIndex) != containedCellIndices.
            end())
          {
            EXPECT_TRUE(cells.IndexOfBlock(block->GetBlockId()) >= 0) << " Run " << run << " ; cx " << cx << "; cy " <<
                cy << " ; cxy " << cxy << "; \n";
          }
          else
          {
            EXPECT_TRUE(cells.IndexOfBlock(block->GetBlockId()) < 0) << " Run " << run << " ; cx " << cx << "; cy " <<
                cy << " ; cxy " << cxy << "; \n";
          }
        }
      }
      dcxy += 0.1f;
    }
    grid.RemoveBlock(*block);
    blocks.clear();
    for (int i = 0; i < grid.MaxCellIncludingBorderIndex; ++i)
    {
      EXPECT_EQ(grid.GetCellAt(i).size(), 0) << " Found stale cell @" << i;
    }
  }
}

TEST(GridTest, TGrid_FindBlocksIn)
{
  TGrid grid({1000, 1000}, CellSizei);

  std::vector<std::shared_ptr<ABlock>> blocks;
  std::vector<TBlockId> blockIds;

  TBlockId id = 0;
  for (int y = 0; y < grid.Size.y; ++y)
  {
    for (int x = 0; x < grid.Size.x; ++x)
    {
      Vector2 pos = {static_cast<float>(x * CellSizei.x), static_cast<float>(y * CellSizei.y)};
      auto block = AddBrickBlock(blocks, pos, {CellSizei.x, CellSizei.y}, ++id);
      EXPECT_TRUE(grid.AddBlock(block));
      blockIds.push_back(id);
    }
  }


  // Helper to count blocks in a region
  for (int i = 0; i < (IsReleaseMode() ? 100 : 10); ++i)
  {
    int count = 0;
    auto cellIndex = (i == 0) ? 0 : rand() % grid.NumCells;
    std::vector<TBlockId> copyBlockIds = blockIds;
    grid.FindNeighborsWithinDistance(grid.GetCellAt(cellIndex)[0],
      [&](TGridBlockInfo& info) -> bool
      {
        if (info.Neighbor != nullptr)
        {
          count++;
          copyBlockIds.erase(std::find(copyBlockIds.begin(), copyBlockIds.end(), info.Neighbor->GetBlockId()));
        }
        else
        {
          TLOG(LOG_ERROR, "Found null block in FindBlocksAround");
        }
        return true;
      });
    // Ignore the block that we searched around.
    EXPECT_EQ(copyBlockIds.size(), 1) << "Run# " << i << "; " << cellIndex;
    for (auto& blockId : copyBlockIds)
    {
      if (blockId == grid.GetCellAt(cellIndex)[0]->GetBlockId()) { continue; }
      TLOG(LOG_ERROR, "Missing blockId %d", blockId);
    }
    EXPECT_EQ(grid.NumCells-1, count) << "Run# " << i << "; " << cellIndex;
  }

  auto testFn = [&](int index, int searchRadius) -> int
  {
    int count = 0;
    grid.FindNeighborsWithinDistance(grid.GetCellAt(index)[0], [&](TGridBlockInfo& info) -> bool
    {
      if (info.Neighbor != nullptr) { count++; }
      return true;
    }, searchRadius);
    return count;
  };
  EXPECT_EQ(3, testFn(0, 1)) << "First cell top-left has exactly 3 neighbors.";
  EXPECT_EQ(3, testFn(grid.IndexOfCell(grid.Size.x-1,0), 1)) << "Top right cell has exactly 3 neighbors too.";
  EXPECT_EQ(3, testFn(grid.NumCells-1, 1)) << "Bottom right cell has exactly 3 neighbors too.";
  EXPECT_EQ(3, testFn(grid.IndexOfCell(0, grid.Size.y-1), 1)) << "Bottom left cell has exactly 3 neighbors too.";
  for (int i = 0; i < 6; ++i)
  {
    auto len = ((i + 1) * 2) + 1;
    EXPECT_EQ((len * len) - 1, testFn(grid.IndexOfCell(15, 15), i+1))
        << "Cell @ 15, 15 with search radius " << (i + 1) <<
           " must have sqr(" << len << ")-1 neighbors.";
  }
}
