#pragma once
#include <math.h>
#include <queue>
#include <string.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "main_game.h"
#include "base_block.h"
#include "world_fileset.h"
#include "tblock.h"
#include <atomic>
using namespace RLPlays;
using namespace std;

// Some of the code here is copied from PufferLib. 
// MIT License in rlplays/game/thirdparty/PufferLib/LICENSE

// Required struct. Only use floats!
struct Log
{
  // Follow C-style naming convention here.
  float perf;           // Recommended 0-1 normalized single real number perf metric
  float score;          // Recommended unnormalized single real number perf metric
  float episode_return; // Recommended metric: sum of agent rewards over episode
  float episode_length; // Recommended metric: number of steps of agent episode
  float syllabus_index; // For tracking curriculum progress.
  float current_self_play_count; // For tracking self-play training progress.
  float num_trains_for_file; // For tracking how many times we've trained on the current file.
  float total_self_play_prev_rewards; // For tracking rewards from the previous self-play training episode.
  float total_reward_count; // For tracking total rewards across episodes.
  float current_obs_count; // For tracking how many obs the agent has seen (across episodes).
  float last_game_type; // For tracking the game type of the last episode (cast to int in Python for readability).
  float total_self_play_prev_time; // For tracking time from the previous self-play training episode.
  float total_self_play_count; // For tracking how many self-play training episodes we've done.
  float total_self_play_success_count; // For tracking how many successful self-play training episodes we've done.
  // Any extra fields you add here may be exported to Python in binding.c
  float n; // Required as the last field
};


typedef struct {} Client;


struct Agent
{
  float x;
  float y;
};

struct Obs
{
  float TraitsOneHot[TBlockTraitsCount];
  float BlockType[TBlockTypeCount];
  Vector2 Pos;
  Vector2 PrevPos;
};


// This holds C++ things that we don't want to put in the main env struct, which gets zero'd out by the Python binding code. We can keep a shared_ptr to this struct in the main env struct.
struct TLiveData
{
  // Follow our normal code style guidelines.
  std::map<TBlockId, Vector2> PrevPositions;
  std::string TrainingForcedFilename = "";
  std::string CurrentTrainingFilename = "";
  std::string LogFilePrefix = "";
  std::stringstream LogStream;
  int TotalNumSteps = 0;
  int NumResets = 0;
  int LogResets = 0; // Tracks the number of resets we've logged (to avoid logging every reset).
  std::shared_ptr<TGameActions> Recorded = {};
  std::vector<TGameLoadInfo> LoadedGames = {};
  bool SelfPlayTraining = false;
};

struct RLPlaysEnv
{
  // Use C style naming convention for this struct alone.
  Log log; // Required field. Env binding code uses this to aggregate logs
  Client* client = nullptr;
  int num_obs = 0;                    // Number of float32 observations.
  float* observations = nullptr;      // Required. You can use any obs type, but make sure it matches in Python!
  int* actions = nullptr;             // Required. int* for discrete/multidiscrete, float* for box
  float* rewards = nullptr;           // Required
  unsigned char* terminals = nullptr; // Required. We don't yet have truncations as standard yet
  float total_rewards = 0;            // Total (per-episode) returns
  int num_actions = 0;                // Number of int32 actions.

  // Keep this first after the mandatory fields to track if we overrun the struct (when asan isn't available).
  int sentinel_first;
  int step_count;
  int max_steps;
  int max_rewards;
  int width;
  int height;
  float scale_factor;
  // Note: Don't use classes/structs with C++ stuff here, just use shared_ptr
  // because of how the env is initialized / zero'ed from Python.

  // Other game related info.
  std::shared_ptr<TGameInfo> game_info;

  // Contains a list of rl files with some training inputs to choose from during training.
  std::shared_ptr<TWorldFiles> rl_fileset;

  // EVERYTHING HERE WILL BE ZERO'D OUT BY THE PYTHON BINDING CODE.
  // DO NOT USE ANY NON-ZERO DEFAULT VALUES OR WORSE, C++ things like strings etc (shared_ptr is ok) HERE.
  
  float prev_rewards = 0;
  float prev_enemies = 0;
  int num_frame_skips = 0; // Will get init'ed correctly in allocate() or binding::init().
  int render_supported = 0;
  int agent_id = 0;
  TPlayerAction player_action = TPlayerAction::None; // is 0.
  // Track if the agent is sitting in one place over a period of time.
  Vector2 last_pos;
  int last_pos_timestep;
  int num_idle_timesteps;
  Vector2 current_agent_pos;
  std::shared_ptr<TLiveData> live_data;
  // Track steps perf
  double start_time_ms;
  double end_time_ms;
  // Persistent flag to track whether we are in next round (even after c_reset) - used by tests.
  bool proceed_to_next_round_for_test;
  // When done, should we proceed to the next round?
  bool proceed_to_next_round;
  // Randomize player pos during training once per c_reset.
  int randomize_player_pos = 0;
  // Controlled by RenderSupported and done exactly once per reset in c_step.
  bool randomize_player_pos_next = false;
  int current_obs_count = 0;
  int total_reward_count = 0;

  // Curriculum stuff
  int syllabus_index = 0;
  int current_successful_training_count = 0;
  int num_trains_for_file = 0;
  int current_failure_training_count = 0;
  int max_success_per_file_training_count = 0;
  int max_failure_per_file_training_count = 0;
  int current_self_play_count = 0;
  int total_self_play_count = 0;
  int total_self_play_success_count = 0;
  int total_self_play_prev_rewards = 0;
  int total_self_play_prev_time = 0;
  int current_round = 0;
  int max_rounds = 0;
  TGameType last_game_type = TGameType::SinglePlayer;
 
  //! @brief Starting player box (randomized or from stored level info).
  Rectangle last_player_box;
  atomic_int* total_step_count;

  // Keep this last to track if we overrun the struct (when asan isn't available).
  int sentinel_last;  
};

