#define PUFFERLIB_SELFPLAY 1
#include <gtest/gtest.h>
#include <grid.h>
#include <base_block.h>
#include <world_fileset.h>
#include <rl_utils.h>

#include <rl_env.h>
#include "puffernet.h"

#define Env RLPlaysEnv
#define EnvsThreadData VecThreadData
#define PUFFERLIB_MULTI_THREADED
#define PUFFERLIB_NUM_THREADS (8)
#include "rl_thread.h"
#if DEBUG
// Uncomment this to check CUDA fused kernels with their slower counterparts (evaluate both).
//#define PUFFER_DBG_CHECK_NETWORK_SLOW 1
//#define PUFFER_DBG_CHECK_COMPARE_BREAK 1
#endif

#include "puffer_native_eval.h"
#include "puffer_native_eval.cpp"

using ::testing::FloatLE;
using ::testing::DoubleLE;
using namespace RLPlays;

static bool headless = false;
static void RestoreGlobalState()
{ 
  THeadless::IsHeadless = headless;
}

static void ClearGlobalState()
{
  reset_count.store(0);
  agent_id.store(0);
  global_syllabus_index.store(0);
  TWorldFiles::ClearCache();
  headless = THeadless::IsHeadless;
  THeadless::IsHeadless = true;
}

static bool CheckRLEnv(RLPlaysEnv* env)
{
  auto rlTrain = TWorldFiles::Load()->RLTrain;
  if (env->num_obs != GetNumObs(rlTrain))
  {
    EXPECT_EQ(env->num_obs, GetNumObs(rlTrain)) << "Regenerate rl train config using the converter.";
    return false;
  }
  if (env->num_actions != MAX_NUM_ACTIONS)
  {
    EXPECT_EQ(env->num_actions, MAX_NUM_ACTIONS) << "Regenerate rl train config using the converter.";
    return false;
  }
  return true;
}

void SetupVecEnv(VecEnv& vecEnv, int numThreads, Tensor& obs_t, Tensor& rewards_t, Tensor& terminals_t)
{
  vecEnv.threading_env = nullptr;
  vecEnv.threading_batch = nullptr;
  vecEnv.envs = new RLPlaysEnv*[vecEnv.num_envs];
  auto config = TConfig(ConfigToTrainFilepath());
  ClearGlobalState();
  int num_obs = config.GetInt("env", "num_obs", 0);
  obs_t = torch::zeros({vecEnv.num_envs, num_obs}, torch::kFloat32);
  rewards_t = torch::zeros({vecEnv.num_envs, 1}, torch::kFloat32);
  terminals_t = torch::zeros({vecEnv.num_envs, 1}, torch::kFloat32);
  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv*& env = vecEnv.envs[i];
    env = static_cast<RLPlaysEnv*>(calloc(1, sizeof(RLPlaysEnv)));
    env->num_obs = num_obs;
    env->num_actions = config.GetInt("env", "num_actions", 0);
    EXPECT_TRUE(CheckRLEnv(env));

    env->observations = obs_t[i].data_ptr<float>();
    allocate(env, 0, 1, "", false, nullptr, false);
  }
  vecEnv.opts.obs_size = vecEnv.envs[0]->num_obs;
  vecEnv.opts.num_threads_env = numThreads;
  vecEnv.opts.num_threads_batch = numThreads;
}

void SetupVecEnv(VecEnv& vecEnv, int numThreads = 4)
{
  vecEnv.threading_env = nullptr;
  vecEnv.threading_batch = nullptr;

  vecEnv.envs = new RLPlaysEnv*[vecEnv.num_envs];
  auto config = TConfig(ConfigToTrainFilepath());
  ClearGlobalState();
  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv*& env = vecEnv.envs[i];
    env = static_cast<RLPlaysEnv*>(calloc(1, sizeof(RLPlaysEnv)));
    env->num_obs = config.GetInt("env", "num_obs", 0);
    env->num_actions = config.GetInt("env", "num_actions", 0);
    EXPECT_TRUE(CheckRLEnv(env));

    allocate(env, 0, 1, "");
  }
  vecEnv.opts.num_threads_env = numThreads;
  vecEnv.opts.num_threads_batch = numThreads;
}

void CleanupVecEnv(VecEnv& vecEnv, bool usesTensorObs)
{
  for (int i = 0; i < vecEnv.num_envs; ++i)
  {
    RLPlaysEnv* env = vecEnv.envs[i];
    // Don't free observations as we don't own them! It's from a tensor!
    if (usesTensorObs) { env->observations = nullptr; }
    free_allocated(env);
    c_close(env);
    free(env);
  }
  c_vecclose(&vecEnv);
  delete [] vecEnv.envs;
}


TEST(PufferLibTest, LSTM_CPU_Test)
{
  ClearGlobalState();

  auto configFilePath = TrainedConfigFilepath();
  TLOG(LOG_INFO, "CPU Test RL Player: Loading config from %s ", configFilePath.c_str());
  auto modelConfig = TConfig(configFilePath);
  RLPlaysEnv env = {};
  env.num_obs = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_obs", 0);
  env.num_actions = TConfig(ConfigToTrainFilepath()).GetInt("env", "num_actions", 0);
  auto rlTrain = std::make_shared<TRLTrain>();
  rlTrain->CurriculumList = {"test/mirror.json", "test/mirror2.json"};

  allocate(&env, 0, 1, "", false, rlTrain);
  TLOG(LOG_INFO, "CPU Test Player: Loading config from %s ", configFilePath.c_str());
  int* logitSizes = new int[MAX_NUM_ACTIONS];
  for (int i = 0; i < MAX_NUM_ACTIONS; ++i) { logitSizes[i] = 2; }
  int num_weights = modelConfig.GetInt("", "num_weights", 0);

  auto path = TrainedModelPath();
  TLOG(LOG_INFO, "Model path %s, # weights %d / num frame skips %d", path.c_str(), num_weights,
      env.num_frame_skips);
  auto weights = load_weights(path.c_str(), num_weights);
  auto net = make_linearlstm(weights, 1, env.num_obs, logitSizes, MAX_NUM_ACTIONS);
  ::forward_linearlstm(net, env.observations, env.actions);
  auto action = ConvertToPlayerAction(env.actions, env.num_actions);

  // TODO: Setup the Torch version and verify they are the same. LSTM nets will produce 
  // the same action for the same observation for a given timestep T (it is not idempotent). 
  // So check that the actions are the same for the first 100 steps.
  for (int i = 0; i < 100; ++i)
  {
    ::forward_linearlstm(net, env.observations, env.actions);
    auto newAction = ConvertToPlayerAction(env.actions, env.num_actions);
    //EXPECT_EQ(newAction, action);
  }
  delete[] logitSizes;
  free(weights);
  free_linearlstm(net);
  free_allocated(&env);
  RestoreGlobalState();
}

TEST(PufferLibTest, LibTorch_Simple)
{
  c_libtorch_info();
}

void weights_to_tensor(Weights* weights, const size_t num_weights, Tensor& tensor_to, int dim1 = -1, int dim2 = -1)
{
  auto* arr = get_weights(weights, num_weights);
  tensor_to = torch::from_blob(arr, {static_cast<int64_t>(num_weights)}, torch::kFloat32);
  if (dim1 > 0 && dim2 > 0)
  {
    tensor_to = tensor_to.view({dim1, dim2});
  }
#if PUFFER_CUDA
  tensor_to = tensor_to.to(torch::kCUDA);
#endif
}

void weights_to_linear(Weights* weights, const size_t input_dim, int output_dim, Tensor& w, Tensor& b)
{
  weights_to_tensor(weights, static_cast<uint64_t>(input_dim) * static_cast<uint64_t>(output_dim), w, output_dim,
      input_dim);
  weights_to_tensor(weights, static_cast<uint64_t>(output_dim), b);
}

#if PUFFER_CUDA
constexpr auto puff_device = torch::kCUDA;


#else
constexpr auto puff_device = torch::kCPU;
#endif


void torch_load_weights(PufferTorch* pt, Weights* weights) {}

static inline Tensor log_prob(Tensor logits, Tensor value)
{
  value = value.to(torch::kLong).unsqueeze(-1);
  auto res = torch::broadcast_tensors({value, logits});
  value = res[0];
  value = value.index({at::indexing::Ellipsis, at::indexing::Slice(0, 1)});
  auto log_pmf = res[1];
  log_pmf = log_pmf.gather(-1, value).squeeze(-1);
  res[0] = Tensor{};
  res[1] = Tensor{};
  return log_pmf;
}


static inline void sample_logits_old(Tensor logits, int num_actions, int64_t* logit_sizes,
    Tensor action_out, Tensor logprob_out)
{
  DBG_CHECK_LOGITS_INPUT(logits, num_actions, logit_sizes, action_out, logprob_out);
  if (num_actions == 1) { logits = logits.unsqueeze(0); }
  else
  {
    auto split_logits = logits.split(at::IntArrayRef(logit_sizes, num_actions), /*dim=*/1);
    logits = torch::stack(split_logits, /*dim=*/0);
  }
  print_tensor(logits, "logits", true);
  auto normalized_logits = logits - torch::logsumexp(logits, /*dim=*/-1, /*keepdim=*/true);
  print_tensor(normalized_logits, "normalized_logits");
  auto probs = torch::exp(torch::log_softmax(logits, -1));

  probs = torch::nan_to_num(probs, 1e-8, 1e-8, 1e-8);
  print_tensor(probs, "probs", true);

  auto action = torch::multinomial(probs.reshape({-1, probs.size(-1)}), 1, /*replacement=*/ true);
  print_tensor(action, "action", true);
  action = action.to(torch::kInt32);
  action = action.reshape(probs.sizes().slice(0, probs.dim() - 1));
  auto logprob = log_prob(normalized_logits, action);
  print_tensor(logprob, "final_logprob", true);
  if (num_actions == 1)
  {
    action = action.squeeze(0);
    logprob = logprob.squeeze(0);
  }
  else
  {
    logprob = logprob.sum(0);
    action = action.transpose(0, 1);
  }
  action_out.copy_(action, /* non_blocking */ false);
  logprob_out.copy_(logprob, /* non_blocking */ false);
  DBG_CHECK_LOGITS_OUTPUT(logits, num_actions, logit_sizes, action_out, logprob_out);
}


TEST(PufferLibTest, LibTorch_SampleLogits)
{
  ClearGlobalState();
  BEGIN_LIBTORCH_CATCH
  {
    // Discrete
    {
      int64_t atns[1] = {3};
      torch::manual_seed(42);
      torch::cuda::manual_seed(42);
      auto actions1 = torch::zeros({2}, torch::kLong);
      auto logprobs1 = torch::zeros({2}, torch::kFloat32);
      sample_logits_old(torch::tensor({{0.1f, 0.2f, 0.7f}, {2.0f, 0.5f, 0.5f}}).cpu(), 1, atns, actions1, logprobs1);
      torch::manual_seed(42);
      torch::cuda::manual_seed(42);
      auto actions2 = torch::zeros({2}, torch::kLong);
      auto logprobs2 = torch::zeros({2}, torch::kFloat32);
      sample_logits(torch::tensor({{0.1f, 0.2f, 0.7f}, {2.0f, 0.5f, 0.5f}}).cpu(), 1, atns, actions2, logprobs2);
      EXPECT_EQ(c_compare_tensorsi(actions2, "Actions new", actions1, "Actions old", true), true);
      EXPECT_EQ(c_compare_tensorsf(logprobs2, "Logprobs new", logprobs1, "Logprobs old", true), true);
    }
    // Multidiscrete
    {
      int64_t atns[3] = {2, 2, 2};
      auto t = torch::tensor({
          {0.0570, -0.1952, 0.2124, -0.0170, 0.0878, -0.1161},
          {0.0570, -0.1952, 0.2124, -0.0170, 0.0878, -0.1161},
          {0.0570, -0.1952, 0.2124, -0.0170, 0.0878, -0.1161},
          {0.0570, -0.1952, 0.2124, -0.0170, 0.0878, -0.1161},
          {0.0570, -0.1952, 0.2124, -0.0170, 0.0878, -0.1161},
      }).cpu();
      auto actions1 = torch::zeros({5, 3}, torch::kLong);
      auto logprobs1 = torch::zeros({5}, torch::kFloat32);
      torch::manual_seed(42);
      torch::cuda::manual_seed(42);
      sample_logits_old(t, 3, atns, actions1, logprobs1);
      auto actions2 = torch::zeros({5, 3}, torch::kLong);
      auto logprobs2 = torch::zeros({5}, torch::kFloat32);
      torch::manual_seed(42);
      torch::cuda::manual_seed(42);
      sample_logits(t, 3, atns, actions2, logprobs2);
      c_compare_tensorsi(actions2, "Actions new", actions1, "Actions old", true);
      c_compare_tensorsf(logprobs2, "Logprobs new", logprobs1, "Logprobs old", true);
    }
  }
  END_LIBTORCH_CATCH
}

TEST(PufferLibTest, LibTorch_LSTM_Eval)
{
  float num_total_horizon_steps = 0;
  int total_runs = 20;
  ClearGlobalState();
  BEGIN_LIBTORCH_CATCH
  {
    for (int run = 0; run < total_runs; run++)
    {
      auto configFilePath = TrainedConfigFilepath();
      TLOG(LOG_INFO, "Eval Test %d Player: Loading config from %s ", (run + 1), configFilePath.c_str());
      auto modelConfig = TConfig(configFilePath);
      Tensor full_obs_torch;
      Tensor full_rewards_torch;
      Tensor full_terminals_torch;
      VecEnv vec_env = {
          .num_envs = 32 * (run + 1), .opts = {
              .enable_native_libtorch = true, .is_continuous = false, .num_threads_env = 2 + run,
              .num_threads_batch = 2,
              .num_gpu_batches = 4,
              .bptt_horizon = 4 + run
          }
      };
      c_setup_pufferoptions(&vec_env, MAX_NUM_ACTIONS, /* num_logits_per_action*/ 2,
          DEFAULT_INPUT_SIZE, DEFAULT_HIDDEN_SIZE, false, vec_env.opts.num_gpu_batches);
      SetupVecEnv(vec_env, vec_env.opts.num_threads_env, full_obs_torch, full_rewards_torch, full_terminals_torch);
      TLOG(LOG_INFO, "Eval Test %d Player: Loading config from %s ", (run + 1), configFilePath.c_str());
      int num_weights = modelConfig.GetInt("", "num_weights", 0);
      auto path = TrainedModelPath();
      TLOG(LOG_INFO, "Model path %s, # weights %d / num frame skips %d", path.c_str(), num_weights);

      auto weights_puffertorch = load_weights(path.c_str(), num_weights);
      c_vecinit(&vec_env);
      auto torch = vec_env.puff_torch;
      torch_load_weights(torch, weights_puffertorch);
      Tensor encoder_linear_w;
      Tensor encoder_linear_b;
      Tensor decoder_linear_w;
      Tensor decoder_linear_b;
      Tensor value_w;
      Tensor value_b;
      Tensor weight_ih;
      Tensor weight_hh;
      Tensor bias_ih;
      Tensor bias_hh;
      const auto& opts = vec_env.opts;
      const int H = opts.bptt_horizon;
      const int N = vec_env.num_envs;
      Tensor obs_out = torch::zeros({N, H, opts.obs_size}, puff_device);
      Tensor actions_out = torch::zeros({N, H, opts.num_actions}, puff_device).to(torch::kInt32).contiguous();
      Tensor logprobs_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor rewards_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor terminals_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor values_out = torch::zeros({N, H}, puff_device).contiguous();

      torch::NoGradGuard no_grad;
      weights_to_linear(weights_puffertorch, opts.obs_size, opts.hidden_size, encoder_linear_w,
          encoder_linear_b);
      weights_to_linear(weights_puffertorch, opts.hidden_size, opts.num_atns,
          decoder_linear_w, decoder_linear_b);
      weights_to_linear(weights_puffertorch, opts.hidden_size, 1, value_w, value_b);
      weights_to_tensor(weights_puffertorch,
          static_cast<uint64_t>(opts.hidden_size) * static_cast<uint64_t>(opts.input_size) * 4,
          weight_ih, opts.input_size * 4, opts.hidden_size);
      weights_to_tensor(weights_puffertorch,
          static_cast<uint64_t>(opts.hidden_size) * static_cast<uint64_t>(opts.input_size) * 4,
          weight_hh, opts.input_size * 4, opts.hidden_size);
      weights_to_tensor(weights_puffertorch, static_cast<uint64_t>(opts.hidden_size) * 4, bias_ih);
      weights_to_tensor(weights_puffertorch, static_cast<uint64_t>(opts.hidden_size) * 4, bias_hh);
      PUFFER_ASSERT(weights_puffertorch->idx == weights_puffertorch->size, "Must have used all weights exactly.");
      for (int torch_run = 0; torch_run < 5; ++torch_run)
      {
        c_torch_start_eval_lstm(((uintptr_t)&vec_env), full_obs_torch.squeeze().pin_memory(),
            full_rewards_torch.squeeze().pin_memory(), full_terminals_torch.squeeze().pin_memory(),
            encoder_linear_w, encoder_linear_b, decoder_linear_w, decoder_linear_b, value_w,
            value_b, weight_ih, weight_hh, bias_ih, bias_hh,
            obs_out, actions_out, logprobs_out, rewards_out, terminals_out, values_out);
        c_torch_run_fulleval(uintptr_t(&vec_env));
        EXPECT_EQ(vec_env.threading_batch->work_batches.size(), 0) << "Not all batch tasks completed.";
        EXPECT_EQ(vec_env.threading_env->work_batches.size(), 0) << "Not all env tasks completed.";
        auto results = c_torch_finish_eval_lstm(uintptr_t(&vec_env));
        std::cout << "For libtorch run " << torch_run << ":\n";
        for (auto& perf : results.perf_stats)
        {
          std::cout << "LSTM eval segment perf (ms): "
              << perf.name << ": " << (perf.total_duration_ms / double(perf.num_batches)) << "ms\n";
        }
        num_total_horizon_steps++;
      }
      free(weights_puffertorch);
      CleanupVecEnv(vec_env, true);
      c_cleanup_pufferoptions(&vec_env);
    }
  }
  END_LIBTORCH_CATCH
  RestoreGlobalState();
}

TEST(PufferLibTest, LibTorch_LSTM_Train)
{
  float num_total_horizon_steps = 0;
  int total_runs = 20;
  ClearGlobalState();
  BEGIN_LIBTORCH_CATCH
  {
    for (int run = 0; run < total_runs; run++)
    {
      auto configFilePath = TrainedConfigFilepath();
      TLOG(LOG_INFO, "Train run # %d RL Player: Loading config from %s ", (run+1), configFilePath.c_str());
      auto modelConfig = TConfig(configFilePath);
      Tensor full_obs_torch;
      Tensor full_rewards_torch;
      Tensor full_terminals_torch;
      VecEnv vec_env = {
          .num_envs = 32 * (run + 1), .opts = {
              .enable_native_libtorch = true,
              .input_size = 512,
              .hidden_size = 512,
              .is_continuous = false,
              .num_threads_env = 2 + run,
              .num_threads_batch = 2,
              .num_gpu_batches = 4,
              .bptt_horizon = 4 + run
          }
      };
      c_setup_pufferoptions(&vec_env, MAX_NUM_ACTIONS, /* num_logits_per_action*/ 2,
          512, 512, false, vec_env.opts.num_gpu_batches);
      SetupVecEnv(vec_env, vec_env.opts.num_threads_env, full_obs_torch, full_rewards_torch, full_terminals_torch);
      TLOG(LOG_INFO, "Train run # %d Player: Loading config from %s ", (run + 1), configFilePath.c_str());
      int num_weights = modelConfig.GetInt("", "num_weights", 0);
      auto path = TrainedModelPath();
      TLOG(LOG_INFO, "Model path %s, # weights %d / num frame skips %d", path.c_str(), num_weights);

      auto weights_puffertorch = load_weights(path.c_str(), num_weights);
      c_vecinit(&vec_env);
      PufferTrainOpts train_opts = {
          .config = {
              {"adam_beta1", "0.9125589147271548"},
              {"adam_beta2", "0.9949125084366474"},
              {"adam_eps", "1.0288779830345268e-8"},
              {"anneal_lr", "true"},
              {"bptt_horizon", "64"},
              {"checkpoint_interval", "200"},
              {"clip_coef", "0.3476977282484254"},
              {"ent_coef", "0.011639885746246309"},
              {"gae_lambda", "0.999"},
              {"gamma", "0.999"},
              {"learning_rate", "0.015000000000000005"},
              {"max_grad_norm", "5"},
              {"max_minibatch_size", "32768"},
              {"minibatch_size", "65536"},
              {"optimizer", "muon"},
              {"prio_alpha", "0.8092077761120502"},
              {"prio_beta0", "0.99"},
              {"total_timesteps", "767524748.17480797"},
              {"update_epochs", "1"},
              {"vf_clip_coef", "0.10368561352859726"},
              {"vf_coef", "3.186630879635838"},
              {"vtrace_c_clip", "3.22476668024972"},
              {"vtrace_rho_clip", "3.612823695802223"},
          }
      };
      c_init_torch_train_lstm(((uintptr_t)&vec_env), train_opts);
      auto torch = vec_env.puff_torch;
      torch_load_weights(torch, weights_puffertorch);
      Tensor encoder_linear_w;
      Tensor encoder_linear_b;
      Tensor decoder_linear_w;
      Tensor decoder_linear_b;
      Tensor value_w;
      Tensor value_b;
      Tensor weight_ih;
      Tensor weight_hh;
      Tensor bias_ih;
      Tensor bias_hh;
      const auto& opts = vec_env.opts;
      const int H = opts.bptt_horizon;
      const int N = vec_env.num_envs;
      Tensor obs_out = torch::zeros({N, H, opts.obs_size}, puff_device);
      Tensor actions_out = torch::zeros({N, H, opts.num_actions}, puff_device).to(torch::kInt32).contiguous();
      Tensor logprobs_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor rewards_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor terminals_out = torch::zeros({N, H}, puff_device).contiguous();
      Tensor values_out = torch::zeros({N, H}, puff_device).contiguous();

      torch::NoGradGuard no_grad;
      weights_to_linear(weights_puffertorch, opts.obs_size, opts.hidden_size, encoder_linear_w,
          encoder_linear_b);
      weights_to_linear(weights_puffertorch, opts.hidden_size, opts.num_atns,
          decoder_linear_w, decoder_linear_b);
      weights_to_linear(weights_puffertorch, opts.hidden_size, 1, value_w, value_b);
      weights_to_tensor(weights_puffertorch,
          static_cast<uint64_t>(opts.hidden_size) * static_cast<uint64_t>(opts.input_size) * 4,
          weight_ih, opts.input_size * 4, opts.hidden_size);
      weights_to_tensor(weights_puffertorch,
          static_cast<uint64_t>(opts.hidden_size) * static_cast<uint64_t>(opts.input_size) * 4,
          weight_hh, opts.input_size * 4, opts.hidden_size);
      weights_to_tensor(weights_puffertorch, static_cast<uint64_t>(opts.hidden_size) * 4, bias_ih);
      weights_to_tensor(weights_puffertorch, static_cast<uint64_t>(opts.hidden_size) * 4, bias_hh);
      PUFFER_ASSERT(weights_puffertorch->idx == weights_puffertorch->size, "Must have used all weights exactly.");
      int total_epochs = 10;
      for (int epoch = 0; epoch < total_epochs; epoch++)
      {
        ++global_syllabus_index; // 
        c_torch_start_eval_lstm(((uintptr_t)&vec_env), full_obs_torch.squeeze().pin_memory(),
            full_rewards_torch.squeeze().pin_memory(), full_terminals_torch.squeeze().pin_memory(),
            encoder_linear_w, encoder_linear_b, decoder_linear_w, decoder_linear_b, value_w,
            value_b, weight_ih, weight_hh, bias_ih, bias_hh,
            obs_out, actions_out, logprobs_out, rewards_out, terminals_out, values_out);
        c_torch_run_fulleval(uintptr_t(&vec_env));
        EXPECT_EQ(vec_env.threading_batch->work_batches.size(), 0) << "Not all batch tasks completed.";
        EXPECT_EQ(vec_env.threading_env->work_batches.size(), 0) << "Not all env tasks completed.";
        auto results = c_torch_finish_eval_lstm(uintptr_t(&vec_env));
        for (auto& perf : results.perf_stats)
        {
          std::cout << "LSTM eval segment perf (ms): "
              << perf.name << ": " << (perf.total_duration_ms / double(perf.num_batches)) << "ms\n";
        }

        c_torch_train_lstm(uintptr_t(&vec_env), epoch, total_epochs, vec_env.num_envs, 16, 4, 1,
            obs_out, actions_out, logprobs_out, rewards_out, terminals_out, values_out);
        num_total_horizon_steps++;
      }
      free(weights_puffertorch);
      CleanupVecEnv(vec_env, true);
      c_cleanup_pufferoptions(&vec_env);
    }
  }
  END_LIBTORCH_CATCH
  RestoreGlobalState();
}

#include "rl_test.cpp"
#include "thread_test.cpp"
// To filter any test here --gtest_filter=PufferLibTest.* (replace with the test method/name or use wildcard).
#include <gtest/gtest.h>
#include <raylib_utils.h>

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
