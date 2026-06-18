#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
#include <world_fileset.h>
#include <rl_utils.h>
#include <rl_env.h>

#include "tblock.h"
using ::testing::FloatLE;
using ::testing::DoubleLE;
using namespace RLPlays;

TEST(ActionsTest, ActionsTest)
{
  for (int j = 0; j < 100; ++j)
  {
    TGameActions actions;
    std::vector<TPlayerAction> verifyList;
    for (int i = 0; i < 100; ++i)
    {
      if (i >= 20 && i <= 40)
      {
        verifyList.push_back(TPlayerAction::None);
        continue;
      }
      const TPlayerAction a = static_cast<TPlayerAction>(i % 5 == 2 ? 1 : (j % 7 == 1 ? 2 : 3));
      actions.AddAction(i, a);
      actions.AddPositionToLastFrame({float(i), float(j)});
      verifyList.push_back(a);
    }

    const auto serialized = actions.GetSerialized();
    // std::cout << "Serialized: # " << j << "; " << serialized << "\n";
    const auto deserialized = TGameActions::FromSerialized(serialized);
    EXPECT_TRUE(deserialized != nullptr);
    EXPECT_EQ(deserialized->FrameActions.size(), actions.FrameActions.size());
    for (int i = 0; i < 100; ++i)
    {
      if (i < deserialized->FrameActions.size())
      {
        EXPECT_EQ(deserialized->FrameActions[i].Action, (actions.FrameActions[i].Action));
        EXPECT_EQ(deserialized->FrameActions[i].FrameIndex, actions.FrameActions[i].FrameIndex);
      }
      TPlayerActions out = {TPlayerAction::Crouch, 200};
      if (actions.GetAction(i, &out))
      {
        EXPECT_EQ(out.Action, verifyList[i]) << " @ " << i;
        EXPECT_EQ(out.FrameIndex, i) << " @ " << i;
        EXPECT_GE(out.Position.x, float(i)) << " @ " << i;
        EXPECT_GE(out.Position.y, float(j)) << " @ " << i;
      }
      else
      {
        EXPECT_TRUE(i >= 20 && i <= 40) << " @ " << i;
      }
    }
  }
}

TEST(ActionsTest, BasicConverstionTest)
{
  TPlayerAction playerActions[] = {
    AddEnumValue(TPlayerAction::WalkLeft, TPlayerAction::WalkRight),
    TPlayerAction::WalkRight,
    TPlayerAction::Jump,
    AddEnumValue(TPlayerAction::WalkLeft, TPlayerAction::Jump),
    TPlayerAction::None
  };
  for (int i = 0; i < sizeof(playerActions) / sizeof(TPlayerAction); ++i)
  {
    int actions[MAX_NUM_ACTIONS] = {-1};
    auto act = playerActions[i];
    ConvertFromPlayerAction(act, actions, MAX_NUM_ACTIONS);
    EXPECT_EQ(act, ConvertToPlayerAction(actions, MAX_NUM_ACTIONS));
  }
}

TEST(ActionsTest, ActionsRLETest)
{
  TGameActions actions;
  std::vector<TPlayerAction> verifyList;
  for (int i = 0; i < 100; ++i)
  {
    if (i >= 20 && i <= 40)
    {
      verifyList.push_back(TPlayerAction::None);
      continue;
    }
    const TPlayerAction a = static_cast<TPlayerAction>(i % 20 == 0 ? 1 : 0);
    actions.AddAction(i, a);
    verifyList.push_back(a);
  }
  for (int i = 0; i < actions.FrameActions.size(); ++i)
  {
    std::cout << "# " << i << ": "
        << (uint32_t)actions.FrameActions[i].Action << " @ "
        << (uint32_t)actions.FrameActions[i].FrameIndex << " / "
        << (uint32_t)actions.FrameActions[i].RunLength << "\n";
  }
  EXPECT_LE(actions.FrameActions.size(), 7);

  for (int i = 0; i < 100; ++i)
  {
    TPlayerActions out = {TPlayerAction::Crouch, 200};
    if (actions.GetAction(i, &out))
    {
      EXPECT_EQ(out.Action, verifyList[i]) << " @ " << i;
      EXPECT_EQ(out.FrameIndex, i) << " @ " << i;
    }
    else
    {
      EXPECT_TRUE(i >= 20 && i <= 40) << " @ " << i;
    }
  }
}

TEST(ActionsTest, CheckRLActions)
{
  const std::vector<std::string> actions = {
    //"00002,1,-1.000000,1,18,31101,904051,FF10F1,FF10F11,FF10F12,FF10F13,D210F14,9040D44,FF10754,FF10755,FF10756,FF10757,FF10758,FF10759,FF1075A,FF1075B,FF1075C,8B1075D",
    //"00002,1,-1.000000,1,15,FF101,FF10101,FF10102,FF10103,FF10104,FF10105,FF10106,FF10107,FF10108,FF10109,FF1010A,FF1010B,FF1010C,FF1010D,E01010E"
  };

  for (const auto& actionStr : actions)
  {
    const auto deserialized = TGameActions::FromSerialized(actionStr);
    ASSERT_TRUE(deserialized != nullptr);
    for (int i = 0; i < deserialized->FrameActions.size(); ++i)
    {
      std::cout << "# " << i << ": "
          << uint32_t(deserialized->FrameActions[i].Action) << " @ "
          << uint32_t(deserialized->FrameActions[i].FrameIndex) << " / "
          << uint32_t(deserialized->FrameActions[i].RunLength) << "\n";
    }
  }
}


TEST(ActionsTest, TestGameWithActions)
{
  const std::vector<std::string> actions = {
  };

  int maxCount = actions.size();
#if DEBUG
  maxCount = 5;
#endif


  for (const auto& actionStr : actions)
  {
    RLPlaysEnv env = {};
    env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    env.num_frame_skips = 1;
    allocate(&env, 0, 1, "test_level1.json");
    bool ended = false;
    const auto deserialized = TGameActions::FromSerialized(actionStr);
    for (int i = 0; i < deserialized->GetNumFrames(); ++i)
    {
      TPlayerActions actions;
      deserialized->GetAction(i, &actions);
      ConvertFromPlayerAction(actions.Action, env.actions, MAX_NUM_ACTIONS);
      c_step(&env);
      if (env.log.n > 0)
      {
        ended = true;
        // EXPECT_LT(env.log.episode_return, -0.01);

        EXPECT_EQ(env.terminals[0], 1);
        break;
      }
      EXPECT_EQ(env.step_count, i + 1);
      EXPECT_EQ(env.terminals[0], 0);
    }
    EXPECT_TRUE(ended);
    free_allocated(&env);
    if (--maxCount <= 0) { break; }
  }
  //EXPECT_TRUE(gotRewards);
}
