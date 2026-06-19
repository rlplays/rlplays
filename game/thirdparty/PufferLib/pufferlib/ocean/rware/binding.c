#include "rware.h"

#define Env CRware
#include "../env_binding.h"

static int my_init(Env* env, PyObject* args, PyObject* kwargs) {
    env->width = unpack(kwargs, "width");
    env->height = unpack(kwargs, "height");
    env->map_choice = unpack(kwargs, "map_choice");
    env->num_agents = unpack(kwargs, "num_agents");
    env->num_requested_shelves = unpack(kwargs, "num_requested_shelves");
    env->grid_square_size = unpack(kwargs, "grid_square_size");
    env->human_agent_idx = unpack(kwargs, "human_agent_idx");
    init(env);
    return 0;
}

static int my_log(PyObject* dict, Log* log) {
    assign_to_dict(dict, "perf", log->perf);
    assign_to_dict(dict, "score", log->score);
    assign_to_dict(dict, "episode_return", log->episode_return);
    assign_to_dict(dict, "episode_length", log->episode_length);
    return 0;
}
