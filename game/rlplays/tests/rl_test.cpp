// Because of silliness in how the binding.c is included, we have to do this gymnastics.


TEST(RLPlaysTest, TestFilePackages)
{
  auto rlset = TWorldFiles::Load();
  EXPECT_FALSE(rlset->Files.empty());

  // Get obs/reward/terminal state from each file.
  bool foundSelectedFile = false;
  for (const auto& file : rlset->Files)
  {
    TGameLoadInfo loadInfo = {.Filename = file.Filename};
    auto gameInfo = LoadGame(*rlset, loadInfo);
    EXPECT_GT(gameInfo.Game->Context->World()->Blocks.size(), 1);
    UpdateFrame(gameInfo);
    foundSelectedFile |= (file.Filename == rlset->SelectedFile.Filename);
    if (file.SupportsRLTraining)
    {
      EXPECT_TRUE(IsValidRLPlays(gameInfo.Game)) << "Error in file: " << file.Filename;
    }

    // Env = new.
    // FillEnv(env);
  }
  EXPECT_GT(rlset->RLTrain->MaxNumCells, 0);

  EXPECT_TRUE(foundSelectedFile) << "Missing selected file  " << rlset->SelectedFile.Filename;
}


void ExpectTraits(const Obs& obs, int pos1, int pos2 = -1, int pos3 = -1, int pos4 = -1)
{
  for (int i = 0; i < TBlockTraitsCount; ++i)
  {
    if (i == pos1 || i == pos2 || i == pos3 || i == pos4)
    {
      EXPECT_EQ(obs.TraitsOneHot[i], 1.0f) << "  @ " << i << " :" << pos2 << "," << pos2 << "," << pos3 << "," << pos4;
    }
    else
    {
      EXPECT_EQ(obs.TraitsOneHot[i], 0.0f) << "  @ " << i << " :" << pos2 << "," << pos2 << "," << pos3 << "," << pos4;;
    }
  }
}

void ExpectBlockType(const Obs& obs, int pos1)
{
  for (int i = 0; i < TBlockTypeCount; ++i)
  {
    if (i == pos1)
    {
      EXPECT_EQ(obs.BlockType[i], 1.0f) << "  @ " << i;
    }
    else
    {
      EXPECT_EQ(obs.BlockType[i], 0.0f) << "  @ " << i;
    }
  }
}


int NumOfBits(uint64_t n)
{
  int count = 0;
  while (n)
  {
    n >>= 1;
    count++;
  }
  return count;
}

TEST(RLPlaysTest, TestEnv)
{
  ClearGlobalState();

  // Verify num of observations so we can encode the float array confidently (and also allow changes easily in only two places).
  EXPECT_EQ(GetNumObsPerBlock() * sizeof(float), sizeof(Obs));
  EXPECT_GE(TBlockTraitsCount, NumOfBits(static_cast<uint64_t>(TBlockTraits::LastUseableBlockTrait)));
  auto modelConfig = TConfig(TrainedConfigFilepath());
  int num_weights = modelConfig.GetInt("", "num_weights", 0);
  EXPECT_GT(num_weights, 10);
  { // Check traits encoding.
    Obs obs;
    EncodeTraits(TBlockTraits::Solid, &obs, true);
    ExpectTraits(obs, 0);
    EncodeTraits(TBlockTraits::CauseDamage, &obs, true);
    ExpectTraits(obs, 10);
    EncodeTraits(TBlockTraits::Cosmetic, &obs, true);
    ExpectTraits(obs, 13);
    EncodeTraits(TBlockTraits::MainPlayerBlock, &obs, true);
    ExpectTraits(obs, 7, 1, 2, 11);
    EncodeTraits(TBlockTraits::OtherPlayerBlock, &obs, true);
    ExpectTraits(obs, 1, 2, 11, 12);

    // Change the order here, as the RL agent can take the role of the active player or the inactive player.
    EncodeTraits(TBlockTraits::MainPlayerBlock, &obs, false);
    ExpectTraits(obs, 1, 2, 11, 12);
    EncodeTraits(TBlockTraits::OtherPlayerBlock, &obs, false);
    ExpectTraits(obs, 7, 1, 2, 11);
  }

  // Encode block type
  {
    Obs obs;
    EncodeBlockType(TBlockType::Player, &obs, true);
    ExpectBlockType(obs, static_cast<int>(TBlockType::Player) - 1);
  }

  // Encode env+obs
  {
    RLPlaysEnv env = {};
    env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    env.num_frame_skips = 1;
    TLOG(LOG_INFO, "Num obs: %d, num actions: %d / num obs per block", env.num_obs, env.num_actions,
      GetNumObsPerBlock());
    if (!CheckRLEnv(&env)) { return; }

    allocate(&env, 0);
    EXPECT_EQ(env.step_count, 0);
    c_reset(&env);
    EXPECT_EQ(env.step_count, 0);
    c_step(&env);
    EXPECT_EQ(env.step_count, 1);
    free_allocated(&env);
  }
}


TEST(RLPlaysTest, PlaygroundTestEnv_GoLeft_FallOff_Cliff)
{
  ClearGlobalState();

  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  env.num_frame_skips = 1;
  if (!CheckRLEnv(&env)) { return; }

  allocate(&env, 0, 1, "test/pg_goal_reward.json");
  EXPECT_EQ(env.step_count, 0);
  EXPECT_DOUBLE_EQ(env.rewards[0], 0);
  EXPECT_DOUBLE_EQ(env.terminals[0], 0);
  bool ended = false;
  bool gotRewards = false;
  for (int i = 0; i < 100; ++i)
  {
    ConvertFromPlayerAction(TPlayerAction::WalkLeft, env.actions, MAX_NUM_ACTIONS); // Walk left and fall off the cliff.
    c_step(&env);
    if (env.log.n > 0)
    {
      ended = true;
      //EXPECT_LT(env.log.episode_return, -0.01);
      EXPECT_FALSE(env.proceed_to_next_round_for_test);
      EXPECT_EQ(env.terminals[0], 1);

      break;
    }
    if (env.rewards[0] > 0.001) { gotRewards = true; }
    EXPECT_EQ(env.step_count, i + 1);
    EXPECT_FALSE(env.proceed_to_next_round_for_test);
    EXPECT_EQ(env.terminals[0], 0);
  }
  EXPECT_TRUE(ended);
  EXPECT_FALSE(env.proceed_to_next_round_for_test);
  //EXPECT_TRUE(gotRewards);
  free_allocated(&env);
}


TEST(RLPlaysTest, PlaygroundTestEnv_StayIdle)
{
  ClearGlobalState();

  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  env.num_frame_skips = 1;
  if (!CheckRLEnv(&env)) { return; }

  allocate(&env, 0, 1, "test/pg_goal_reward.json");
  EXPECT_EQ(env.step_count, 0);
  EXPECT_DOUBLE_EQ(env.rewards[0], 0);
  EXPECT_DOUBLE_EQ(env.terminals[0], 0);
  bool ended = false;
  bool gotRewards = false;
  auto prevMaxSteps = env.max_steps;
  for (int i = 0; i < prevMaxSteps; ++i)
  {
    ConvertFromPlayerAction(TPlayerAction::None, env.actions, MAX_NUM_ACTIONS); // Walk left and fall off the cliff.
    c_step(&env);
    if (env.log.n > 0)
    {
      ended = true;
      EXPECT_LT(i, prevMaxSteps) << "Must have incurred idle penalty " << i;
      EXPECT_LT(env.log.episode_return, -0.01);
      EXPECT_FALSE(env.proceed_to_next_round_for_test);
      EXPECT_EQ(env.terminals[0], 1);

      break;
    }
    if (env.rewards[0] > 0.001) { gotRewards = true; }
    EXPECT_EQ(env.step_count, i + 1);
    EXPECT_FALSE(env.proceed_to_next_round_for_test);
    EXPECT_EQ(env.terminals[0], 0);
  }
  EXPECT_TRUE(ended);
  EXPECT_FALSE(env.proceed_to_next_round_for_test);
  //EXPECT_TRUE(gotRewards);
  free_allocated(&env);
}

void PrintObs(const std::string& msg, const RLPlaysEnv& env)
{
  std::cout << "BEGIN : " << msg << " ************** \n";
  for (int obsIndex = 0; obsIndex < 4 * GetNumObsPerBlock(); ++obsIndex)
  {
    std::cout << "[" << obsIndex << "]: " << env.observations[obsIndex] << "\n";
  }
  std::cout << "END : " << msg << " ************** \n";
}

TEST(RLPlaysTest, PlaygroundTestEnv_GoRight_ReachGoal)
{
  ClearGlobalState();

  for (int count = 0; count < 10; ++count)
  {
    RLPlaysEnv env = {};
    env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    if (!CheckRLEnv(&env)) { return; }

    allocate(&env, 0, 1, "test/pg_goal_reward.json");

    EXPECT_DOUBLE_EQ(env.rewards[0], 0) << " #" << count;
    EXPECT_DOUBLE_EQ(env.terminals[0], 0) << " #" << count;

    //constexpr auto expectedGoalReward = 0.0;
    // Test both reset and step ensuring we reach the final goal.
    for (int run = 0; run < 10; ++run)
    {
      EXPECT_EQ(env.step_count, 0) << " Run: " << run;
      bool ended = false;
      bool gotRewards = false;
      for (int i = 0; i < 1000; ++i)
      {
        // First walk left until you get the reward, then walk right and get the reward.
        // Otherwise, walk right and go to goal.
        const auto action = gotRewards ? TPlayerAction::WalkRight : TPlayerAction::WalkLeft;
        ConvertFromPlayerAction(action, env.actions, MAX_NUM_ACTIONS);

        c_step(&env);
        if (count == 0 && run == 0 && (i == 0 || i == 10)) { PrintObs("Env", env); }
        if (env.log.n > run)
        {
          if (count == 0 && run == 0) { PrintObs("EpisodeDone", env); }
          ended = true;
          EXPECT_EQ(env.terminals[0], 1);

          //EXPECT_TRUE(env.proceedToNextRoundForTest);
          // EXPECT_GE(env.log.episode_return, expectedGoalReward) << " Run: " << run << " / step " << i;
          break;
        }
        if (env.rewards[0] > 0.001) { gotRewards = true; }
        EXPECT_EQ(env.step_count, i + 1) << " Run: " << run << " / step " << i;
        EXPECT_EQ(env.terminals[0], 0);
        EXPECT_FALSE(env.proceed_to_next_round_for_test);
      }
      EXPECT_TRUE(ended);
      //EXPECT_TRUE(env.proceedToNextRoundForTest);
      //EXPECT_TRUE(gotRewards);
    }

    free_allocated(&env);
    // Try closing the env one more time...
    c_close(&env);
  }
}


TEST(RLPlaysTest, PerfTest_RunEnvs)
{
  ClearGlobalState();

  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  if (!CheckRLEnv(&env)) { return; }

  allocate(&env, 0, 1, "test/pg_goal_reward.json");
  const double startTime = clock();
  double endTime = clock();
  double timeDiffSec = (endTime - startTime) / CLOCKS_PER_SEC;

  float steps = 0;
  for (int run = 0; run < 1000; ++run)
  {
    for (int i = 0; i < 1000; ++i)
    {
      ConvertFromPlayerAction(TPlayerAction::WalkRight, env.actions, MAX_NUM_ACTIONS);

      c_step(&env);
      ++steps;
    }
    endTime = clock();
    timeDiffSec = (endTime - startTime) / CLOCKS_PER_SEC;
    if (timeDiffSec >= 1.0) break;
    //EXPECT_TRUE(gotRewards);
  }

  auto debug = "";
#if DEBUG
  debug = "(!!DEBUG MODE!!) ";
#endif
  std::cout << debug << "Time taken for " << steps << " steps: " << (timeDiffSec) << " seconds\n";
  std::cout << debug << "Num of steps per second (single core):  " << (steps / timeDiffSec) << "\n";
  std::cout << debug << "Num of steps per second for 8 cores :  " << long((steps / timeDiffSec) * 8) << "\n";

  free_allocated(&env);
  // Try closing the env one more time...
  c_close(&env);
}


TEST(RLPlaysTest, Raw_RunEnvs)
{
  ClearGlobalState();

  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  if (!CheckRLEnv(&env)) { return; }

  allocate(&env, 0, 1, "");

  for (int run = 0; run < 10; ++run)
  {
    for (int i = 0; i < 1000; ++i)
    {
      ConvertFromPlayerAction(TPlayerAction::WalkRight, env.actions, MAX_NUM_ACTIONS);
      c_step(&env);
      if (env.log.n > run)
      {
        break;
      }
    }
  }


  free_allocated(&env);
  // Try closing the env one more time...
  c_close(&env);
}

// Written with assistance from copilot/claude 3.7
TEST(RLPlaysTest, PlaygroundTestEnv_Jump_GoRight)
{
  ClearGlobalState();

  for (int count = 0; count < 10; ++count)
  {
    RLPlaysEnv env = {};
    env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    if (!CheckRLEnv(&env)) { return; }

    allocate(&env, 0, 1, "test/pg_goal_reward.json");

    EXPECT_DOUBLE_EQ(env.rewards[0], 0) << " #" << count;
    EXPECT_DOUBLE_EQ(env.terminals[0], 0) << " #" << count;

    // Test both reset and step ensuring we reach the final goal.
    for (int run = 0; run < 10; ++run)
    {
      EXPECT_EQ(env.step_count, 0) << " Run: " << run;
      bool ended = false;
      bool gotRewards = false;
      int jumpCounter = 0;

      for (int i = 0; i < 1000; ++i)
      {
        // Alternate between jumping and walking right to test more complex movement
        TPlayerAction action;
        if (jumpCounter == 0)
        {
          action = TPlayerAction::Jump;
          jumpCounter = 5; // Jump, then move right for 5 steps
        }
        else
        {
          action = TPlayerAction::WalkRight;
          jumpCounter--;
        }

        ConvertFromPlayerAction(action, env.actions, MAX_NUM_ACTIONS);

        c_step(&env);
        if (count == 0 && run == 0 && (i == 0 || i == 10)) { PrintObs("Env", env); }
        if (env.log.n > run)
        {
          if (count == 0 && run == 0) { PrintObs("EpisodeDone", env); }
          ended = true;
          EXPECT_EQ(env.terminals[0], 1);
          break;
        }

        if (env.rewards[0] > 0.001) { gotRewards = true; }
        EXPECT_EQ(env.step_count, i + 1) << " Run: " << run << " / step " << i;
        EXPECT_EQ(env.terminals[0], 0);
        EXPECT_FALSE(env.proceed_to_next_round_for_test);
      }

      EXPECT_TRUE(ended);
    }

    free_allocated(&env);
    c_close(&env);
  }
}

TEST(RLPlaysTest, PerfTest_MultiThreaded_RunEnvs)
{
  ClearGlobalState();

  VecEnv vecEnv = {.num_envs = 100};
  vecEnv.envs = new RLPlaysEnv*[vecEnv.num_envs];


  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv*& env = vecEnv.envs[i];
    env = static_cast<RLPlaysEnv*>(calloc(1, sizeof(RLPlaysEnv)));
    env->num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env->num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    if (!CheckRLEnv(env)) { return; }

    allocate(env, 0, 1, "");
  }
  vecEnv.opts.num_threads_env = 4;
  vecEnv.opts.num_threads_batch = 4;
  EXPECT_EQ(0, c_vecinit(&vecEnv));
  const double startTime = clock();
  double endTime = clock();
  double timeDiffSec = 0;
  double timeSec = 1.0;
  int steps = 0;
  for (int run = 0; ; ++run)
  {
    for (int step = 0; step < 10000; ++step)
    {
      // Random actions for all envs.
      for (int i = 0; i < vecEnv.num_envs; ++i)
      {
        RLPlaysEnv& env = *vecEnv.envs[i];
        ConvertFromPlayerAction(TPlayerAction::WalkRight, env.actions, MAX_NUM_ACTIONS);
        ++steps; // Simulate each step here as c_vecstep would have.
      }
      EXPECT_EQ(0, c_vecstep(&vecEnv));
      endTime = clock();
      timeDiffSec = (endTime - startTime) / CLOCKS_PER_SEC;
      if (timeDiffSec >= timeSec) break;
    }
    if (timeDiffSec >= timeSec) break;
  }
  auto debug = "";
#if DEBUG
  debug = "(!!DEBUG MODE!!) ";
#endif
  std::cout << debug << "Time taken for " << steps << " steps: " << (timeDiffSec) << " seconds\n";
  std::cout << debug << "Num of steps per second (across all threads):  " << (steps / timeDiffSec) << "\n";

  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv* env = vecEnv.envs[i];
    EXPECT_NE(env, nullptr);
    EXPECT_NE(env->observations, nullptr);
    EXPECT_NE(env->actions, nullptr);
    EXPECT_NE(env->rewards, nullptr);
    EXPECT_NE(env->terminals, nullptr);
    free_allocated(env);
    c_close(env);
    free(env);
  }
  c_vecclose(&vecEnv);
  delete [] vecEnv.envs;
}

TEST(RLPlaysTest, MultiThreaded_GoRightAll)
{
  ClearGlobalState();

  VecEnv vecEnv = {.num_envs = 100};
  vecEnv.envs = new RLPlaysEnv*[vecEnv.num_envs];


  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv* env = (vecEnv.envs[i] = static_cast<RLPlaysEnv*>(calloc(1, sizeof(RLPlaysEnv))));
    env->num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
    env->num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
    if (!CheckRLEnv(env)) { return; }

    allocate(env, 0, 1, "test/pg_goal_reward.json");

    EXPECT_DOUBLE_EQ(env->rewards[0], 0) << " #" << i;
    EXPECT_DOUBLE_EQ(env->terminals[0], 0) << " #" << i;
  }
  vecEnv.opts.num_threads_env = 4;
  vecEnv.opts.num_threads_batch = 4;

  EXPECT_EQ(0, c_vecinit(&vecEnv));
  const double startTime = clock();
  double endTime = clock();
  double timeDiffSec = 0;
  double timeSec = 1.0;
  int steps = 0;
  bool reachedGoal = false;
  for (int run = 0; ; ++run)
  {
    for (int step = 0; step < 10000; ++step)
    {
      // Random actions for all envs.
      for (int i = 0; i < vecEnv.num_envs; ++i)
      {
        RLPlaysEnv* env = vecEnv.envs[i];
        const auto action = TPlayerAction::WalkRight;
        ConvertFromPlayerAction(action, env->actions, MAX_NUM_ACTIONS);
        ++steps; // Simulate each step here as c_vecstep would have.
      }
      EXPECT_EQ(0, c_vecstep(&vecEnv));
      for (int i = 0; i < vecEnv.num_envs; ++i)
      {
        RLPlaysEnv& env = *vecEnv.envs[i];
        // If an env reaches the goal first, then all other envs must have reached (simultaneously).
        if (reachedGoal) { EXPECT_DOUBLE_EQ(env.terminals[0], 1); }
        if (env.terminals[0] >= 1) { reachedGoal = true; }
      }
      endTime = clock();
      timeDiffSec = (endTime - startTime) / CLOCKS_PER_SEC;
      if (reachedGoal || timeDiffSec >= timeSec) break;
    }
    if (reachedGoal || timeDiffSec >= timeSec) break;
  }
  EXPECT_TRUE(reachedGoal);
  auto debug = "";
#if DEBUG
  debug = "(!!DEBUG MODE!!) ";
#endif
  std::cout << debug << "Time taken for " << steps << " steps: " << (timeDiffSec) << " seconds\n";
  std::cout << debug << "Num of steps per second (across all threads):  " << (steps / timeDiffSec) << "\n";

  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv* env = vecEnv.envs[i];
    EXPECT_NE(env, nullptr);
    EXPECT_NE(env->observations, nullptr);
    EXPECT_NE(env->actions, nullptr);
    EXPECT_NE(env->rewards, nullptr);
    EXPECT_NE(env->terminals, nullptr);
    free_allocated(env);
    c_close(env);
    free(env);
  }
  c_vecclose(&vecEnv);
  delete [] vecEnv.envs;
}


TEST(RLPlaysTest, PlaygroundTestEnv_MirrorTest)
{
  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  if (!CheckRLEnv(&env)) { return; }
  auto rlTrain = std::make_shared<TRLTrain>();
  rlTrain->CurriculumList = {"test/mirror.json", "test/mirror2.json"};

  allocate(&env, 0, 1, "", false, rlTrain);

  EXPECT_DOUBLE_EQ(env.rewards[0], 0);
  EXPECT_DOUBLE_EQ(env.terminals[0], 0);

  //constexpr auto expectedGoalReward = 0.0;
  // Test both reset and step ensuring we reach the final goal.
  int numSkips = 9;
  int run = 0;
  while (numSkips > 1)
  {
    EXPECT_EQ(env.step_count, 0) << " Run: " << run;
    bool ended = false;
    bool gotRewards = false;
    for (int i = 0; i < 1000; ++i)
    {
      // Run 0: Active player on the left side. Walk right/pause a bit to reward.
      // Run 1: Active player on the right side (Replay guy on the left), Walk left/pause to reward.
      // etc... each run, we pause a bit less often so the current side always wins.
      const auto action = (run % 2 == 0)
                            ? (i < numSkips ? TPlayerAction::None : TPlayerAction::WalkRight)
                            : (i < numSkips ? TPlayerAction::None : TPlayerAction::WalkLeft);
      ConvertFromPlayerAction(action, env.actions, MAX_NUM_ACTIONS);

      c_step(&env);
      if (run == 0 && (i == 0 || i == 10)) { PrintObs("Env", env); }
      if (env.log.n > run)
      {
        if (run == 0) { PrintObs("EpisodeDone", env); }
        ended = true;
        EXPECT_EQ(env.terminals[0], 1);
        if (!env.proceed_to_next_round_for_test)
        {
          EXPECT_GT(run, 2) << " Must proceed to next round for test after initial runs ";
        }
        else
        {
          EXPECT_GT(env.rewards[0], 0.001f);
        }

        // EXPECT_GE(env.log.episode_return, expectedGoalReward) << " Run: " << run << " / step " << i;
        break;
      }
      if (env.rewards[0] > 0.001) { gotRewards = true; }
      EXPECT_EQ(env.step_count, i + 1) << " Run: " << run << " / step " << i;
      EXPECT_EQ(env.terminals[0], 0);
      EXPECT_FALSE(env.proceed_to_next_round_for_test);
    }
    EXPECT_TRUE(ended);
    if (env.proceed_to_next_round_for_test) { EXPECT_TRUE(gotRewards); }
    numSkips -= 1;
    ++run;
  }

  free_allocated(&env);
  // Try closing the env one more time...
  c_close(&env);
}

// To filter any test here --gtest_filter=RLPlaysTest.MultiThreaded_RunEnvs (replace with the test method/name).
