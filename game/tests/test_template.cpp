#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
#include <raylib_utils.h>
using ::testing::FloatLE;
using ::testing::DoubleLE;
using namespace RLPlays;

TEST(RLPlaysTest, DummyTest) 
{
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "hell2o");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);

  EXPECT_PRED_FORMAT2(DoubleLE, 3.0, 3);
  DisplayProgramInfo();
}
