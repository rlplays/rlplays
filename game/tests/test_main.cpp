#include <gtest/gtest.h>
#include <raylib_utils.h>

// To filter tests:
// Add cli arg:
// e.g. --gtest_filter=RLPlaysTest.MultiThreaded_RunEnvs


class TEnvironment : public ::testing::Environment
{
public:
  void SetUp() override
  {
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_WARNING], 0);
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_ERROR], 0);
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_FATAL], 0);
  }

  void TearDown() override
  {
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_WARNING], 0);
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_ERROR], 0);
    EXPECT_EQ(TEST_TRACE_LOG_COUNTS_[LOG_FATAL], 0);
  }
};

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  RLPlays::SetupGlobal();
  // Environment is auto-deleted by testing framework.
  testing::AddGlobalTestEnvironment(new TEnvironment());
  auto ret = RUN_ALL_TESTS();
#ifdef RLPLAYS_WAIT_AFTER_RUN
  getchar();
#endif
  return ret;
}

