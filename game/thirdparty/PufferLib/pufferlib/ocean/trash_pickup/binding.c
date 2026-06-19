#include "trash_pickup.h"

#define Env CTrashPickupEnv
#include "../env_binding.h"

static int my_init(Env* env, PyObject* args, PyObject* kwargs) {
    env->num_agents = unpack(kwargs, "num_agents");
    env->grid_size = unpack(kwargs, "grid_size");
    env->num_trash = unpack(kwargs, "num_trash");
    env->num_bins = unpack(kwargs, "num_bins");
    env->max_steps = unpack(kwargs, "max_steps");
    env->agent_sight_range = unpack(kwargs, "agent_sight_range");
    initialize_env(env);
    return 0;
}

static int my_log(PyObject* dict, Log* log) {
    assign_to_dict(dict, "perf", log->perf);
    assign_to_dict(dict, "score", log->score);
    assign_to_dict(dict, "episode_return", log->episode_return);
    assign_to_dict(dict, "episode_length", log->episode_length);
    assign_to_dict(dict, "trash_collected", log->trash_collected);
    return 0;
}
