#include "rlplays.h"
#define Env RLPlaysEnv
using namespace std;
#include "../thirdparty/PufferLib/pufferlib/ocean/env_binding.h"

#ifndef UNPACK
#define UNPACK(a) env->a = unpack(kwargs, #a)
#endif

#ifndef ASSIGN_LOG_DICT
#define ASSIGN_LOG_DICT(a) assign_to_dict(dict, #a, log->a)
#endif

//#ifdef PUFFERLIB_MULTI_THREADED_ENV
//#define VEC_ENV
//#include "rl_thread.h"
//#endif
static int my_init(Env* env, PyObject* args, PyObject* kwargs)
{
  UNPACK(num_obs);
  UNPACK(num_actions);
  UNPACK(width);
  UNPACK(height);
  UNPACK(num_frame_skips);
  UNPACK(randomize_player_pos);
  init(env);
  return 0;
}

static int my_log(PyObject* dict, Log* log)
{
  ASSIGN_LOG_DICT(perf);
  ASSIGN_LOG_DICT(score);
  ASSIGN_LOG_DICT(episode_return);
  ASSIGN_LOG_DICT(episode_length);
  ASSIGN_LOG_DICT(syllabus_index);
  ASSIGN_LOG_DICT(current_self_play_count);
  ASSIGN_LOG_DICT(num_trains_for_file);
  ASSIGN_LOG_DICT(total_self_play_prev_rewards);
  ASSIGN_LOG_DICT(total_reward_count);
  ASSIGN_LOG_DICT(current_obs_count);
  ASSIGN_LOG_DICT(last_game_type);
  ASSIGN_LOG_DICT(total_self_play_prev_time);
  ASSIGN_LOG_DICT(total_self_play_count);
  ASSIGN_LOG_DICT(total_self_play_success_count);
  return 0;
}

