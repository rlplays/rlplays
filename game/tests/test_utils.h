#pragma once
#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
using ::testing::FloatLE;
using ::testing::DoubleLE;
using namespace RLPlays;


namespace RLPlays
{
inline void EXPECT_RECT_EQ(const Rectangle& expected, const Rectangle& actual)
{
  EXPECT_FLOAT_EQ(expected.x, actual.x);
  EXPECT_FLOAT_EQ(expected.y, actual.y);
  EXPECT_FLOAT_EQ(expected.width, actual.width);
  EXPECT_FLOAT_EQ(expected.height, actual.height);
}

inline bool IsReleaseMode()
{
#if DEBUG
  return false;
#else
  return true;
#endif
}
}
