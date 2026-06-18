void test_func(void* arg, int index);
void test_func_sleep(void* arg, int index);
void test_func_more_work(void* arg, int index);

struct TestWork
{
  VecEnv* vec_env;
  std::atomic_int i = {};
  std::atomic_int total = {};
  int extra_work = 100;
  atomic_int batch_completed_count = 0;
};

TEST(ThreadTest, BasicThreadTest)
{
  VecEnv vecEnv = {.num_envs = 8};
  SetupVecEnv(vecEnv);
  c_init_multithreading(&vecEnv);
  for (int j = 1; j <= 10; j++)
  {
    for (int k = 1000; k < 5000; k += 1000)
    {
    TestWork work = {};
      c_add_work_batched(&vecEnv, test_func, &work, 1, k);
      c_wait_all_done(&vecEnv);
      EXPECT_EQ(work.i.load(), k);
      EXPECT_EQ(work.total.load(), (k/2) * (k + 1)); // Gaussian sum 1..k
    }
  }
  c_shutdown_multithreading(&vecEnv);
  CleanupVecEnv(vecEnv, false);
}

TEST(ThreadTest, LotsOfWorkTest)
{
  VecEnv vecEnv = {.num_envs = 128};
  SetupVecEnv(vecEnv, 3);
  c_init_multithreading(&vecEnv);
  for (int j = 1; j <= 23; j++)
  {
    TestWork work = {};
    c_start_work(&vecEnv);
    const int N = j;
    atomic_int num_batch_calls = 0;
    add_work_batched(&vecEnv, test_func_sleep, &work, 1, N, [&](void*)
    {
      //cout << "Called batch1 callback\n";
      ++num_batch_calls;
    }, 1, PufferWorkType::BatchWork);
    add_work_batched(&vecEnv, test_func_sleep, &work, 0, N - 1, [&](void*)
    {
      //cout << "Called batch2 callback\n";
      ++num_batch_calls;
    }, 1, PufferWorkType::BatchWork);
    c_wait_all_done(&vecEnv);
    EXPECT_EQ(work.i.load(), N*2);
    EXPECT_EQ(num_batch_calls, 2);
  }
  c_shutdown_multithreading(&vecEnv);
  CleanupVecEnv(vecEnv, false);
}

TEST(ThreadTest, ForkWorkTest)
{
  VecEnv vecEnv = {.num_envs = 8};
  SetupVecEnv(vecEnv);
  c_init_multithreading(&vecEnv);

#if DEBUG
  constexpr int num_runs = 100000;
  constexpr int print_notif = 10000;
#else
  constexpr int num_runs = 5000000;
  constexpr int print_notif = 1000000;
#endif
  for (int j = 1; j <= num_runs; j++)
  {
    if (j % print_notif == 0)
    {
      std::cout << "ForkWorkTest run #" << j << "\n";
    }
    TestWork work = {.vec_env = &vecEnv};
    atomic_int done_tasks = 0;

    c_start_work(&vecEnv);
    add_work_batched(&vecEnv, test_func_more_work, &work, 0, 999, [&](void*)
    {
      work.batch_completed_count++;
      done_tasks++;
    }, 1, PufferWorkType::BatchWork);
    c_wait_all_done(&vecEnv);

    // The final work item adds two more just to ensure everything finishes on time.
    EXPECT_EQ(work.i.load(), (1000+work.extra_work)) << "j" << j;

    EXPECT_EQ(done_tasks.load(), 1);
    // One queued up inner batch plus one outer batch.
    EXPECT_EQ(work.batch_completed_count, 2);
  }
  c_shutdown_multithreading(&vecEnv);
  CleanupVecEnv(vecEnv, false);
}


void test_func(void* arg, int index)
{
  auto* work = static_cast<TestWork*>(arg);
  work->i.fetch_add(1);
  work->total.fetch_add(index);
}

void test_func_sleep(void* arg, int index)
{
  this_thread::sleep_for(chrono::microseconds(rand() % 1000));
  atomic_int& counter = static_cast<TestWork*>(arg)->i;
  counter.fetch_add(1);
}

// Fork more tasks and test the batching internally too.
void test_func_more_work(void* arg, int index)
{
  auto* work = static_cast<TestWork*>(arg);
  atomic_int& counter = work->i;
  if (index == 0)
  {
    // Add more work to test nested work addition.
    add_work_batched(work->vec_env, test_func_more_work, arg, 1, work->extra_work, [=](void*)
    {
      EXPECT_EQ(work, ((TestWork*)arg));
      // Must either be the first (inner) or second (outer) completion.
      EXPECT_LT(work->batch_completed_count, 2);
      work->batch_completed_count++;
    }, 1, PufferWorkType::BatchWork);
  }
  counter.fetch_add(1);
}
