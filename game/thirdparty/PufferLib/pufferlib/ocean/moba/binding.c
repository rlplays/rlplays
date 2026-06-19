#include "moba.h"

#define Env MOBA
#define MY_SHARED
#include "../env_binding.h"

static PyObject* my_shared(PyObject* self, PyObject* args, PyObject* kwargs) {
    unsigned char* game_map_npy = read_file("resources/moba/game_map.npy");
    int* ai_path_buffer = calloc(3*8*128*128, sizeof(int));
    unsigned char* ai_paths = calloc(128*128*128*128, sizeof(unsigned char));
    for (int i = 0; i < 128*128*128*128; i++) {
        ai_paths[i] = 255;
    }

    PyObject* ai_path_buffer_handle = PyLong_FromVoidPtr(ai_path_buffer);
    PyObject* ai_paths_handle = PyLong_FromVoidPtr(ai_paths);
    PyObject* game_map_handle = PyLong_FromVoidPtr(game_map_npy);
    PyObject* state = PyDict_New();
    PyDict_SetItemString(state, "ai_path_buffer", ai_path_buffer_handle);
    PyDict_SetItemString(state, "ai_paths", ai_paths_handle);
    PyDict_SetItemString(state, "game_map", game_map_handle);
    return PyLong_FromVoidPtr(state);
}

static int my_init(Env* env, PyObject* args, PyObject* kwargs) {
    env->vision_range = unpack(kwargs, "vision_range");
    env->agent_speed = unpack(kwargs, "agent_speed");
    env->discretize = unpack(kwargs, "discretize");
    env->reward_death = unpack(kwargs, "reward_death");
    env->reward_xp = unpack(kwargs, "reward_xp");
    env->reward_distance = unpack(kwargs, "reward_distance");
    env->reward_tower = unpack(kwargs, "reward_tower");
    env->script_opponents = unpack(kwargs, "script_opponents");

    PyObject* handle_obj = PyDict_GetItemString(kwargs, "state");
    if (handle_obj == NULL) {
        PyErr_SetString(PyExc_KeyError, "Key 'state' not found in kwargs");
        return 1;
    }

    // Check if handle_obj is a PyLong
    if (!PyLong_Check(handle_obj)) {
        PyErr_SetString(PyExc_TypeError, "state handle must be an integer");
        return 1;
    }

    // Convert PyLong to PyObject* (state dictionary)
    PyObject* state_dict = (PyObject*)PyLong_AsVoidPtr(handle_obj);
    if (state_dict == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid state dictionary pointer");
        return 1;
    }

    // Verify it’s a dictionary
    if (!PyDict_Check(state_dict)) {
        PyErr_SetString(PyExc_TypeError, "State pointer does not point to a dictionary");
        return 1;
    }

    // Basic validation: check reference count
    if (state_dict->ob_refcnt <= 0) {
        PyErr_SetString(PyExc_RuntimeError, "State dictionary has invalid reference count");
        return 1;
    }

    // Extract ai_path_buffer
    PyObject* ai_path_buffer_obj = PyDict_GetItemString(state_dict, "ai_path_buffer");
    if (ai_path_buffer_obj == NULL) {
        PyErr_SetString(PyExc_KeyError, "Key 'ai_path_buffer' not found in state");
        return 1;
    }
    if (!PyLong_Check(ai_path_buffer_obj)) {
        PyErr_SetString(PyExc_TypeError, "ai_path_buffer must be an integer");
        return 1;
    }
    env->ai_path_buffer = (int*)PyLong_AsVoidPtr(ai_path_buffer_obj);
    if (env->ai_path_buffer == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid ai_path_buffer pointer");
        return 1;
    }

    // Extract ai_paths
    PyObject* ai_paths_obj = PyDict_GetItemString(state_dict, "ai_paths");
    if (ai_paths_obj == NULL) {
        PyErr_SetString(PyExc_KeyError, "Key 'ai_paths' not found in state");
        return 1;
    }
    if (!PyLong_Check(ai_paths_obj)) {
        PyErr_SetString(PyExc_TypeError, "ai_paths must be an integer");
        return 1;
    }
    env->ai_paths = (unsigned char*)PyLong_AsVoidPtr(ai_paths_obj);
    if (env->ai_paths == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid ai_paths pointer");
        return 1;
    }

    // Extract game_map
    PyObject* game_map_obj = PyDict_GetItemString(state_dict, "game_map");
    if (game_map_obj == NULL) {
        PyErr_SetString(PyExc_KeyError, "Key 'game_map' not found in state");
        return 1;
    }
    if (!PyLong_Check(game_map_obj)) {
        PyErr_SetString(PyExc_TypeError, "game_map must be an integer");
        return 1;
    }
    unsigned char* game_map = (unsigned char*)PyLong_AsVoidPtr(game_map_obj);
    if (game_map == NULL) {
        PyErr_SetString(PyExc_ValueError, "Invalid game_map pointer");
        return 1;
    }

    init_moba(env, game_map);
    return 0;
}

static int my_log(PyObject* dict, Log* log) {
    assign_to_dict(dict, "perf", log->perf);
    assign_to_dict(dict, "score", log->score);
    assign_to_dict(dict, "episode_return", log->episode_return);
    assign_to_dict(dict, "episode_length", log->episode_length);
    assign_to_dict(dict, "radiant_victory", log->radiant_victory);
    assign_to_dict(dict, "dire_victory", log->dire_victory);
    assign_to_dict(dict, "radiant_level", log->radiant_level);
    assign_to_dict(dict, "dire_level", log->dire_level);
    assign_to_dict(dict, "radiant_towers_alive", log->radiant_towers_alive);
    assign_to_dict(dict, "dire_towers_alive", log->dire_towers_alive);

    assign_to_dict(dict, "radiant_support_episode_return", log->radiant_support_episode_return);
    assign_to_dict(dict, "radiant_support_reward_death", log->radiant_support_reward_death);
    assign_to_dict(dict, "radiant_support_reward_xp", log->radiant_support_reward_xp);
    assign_to_dict(dict, "radiant_support_reward_distance", log->radiant_support_reward_distance);
    assign_to_dict(dict, "radiant_support_reward_tower", log->radiant_support_reward_tower);
    assign_to_dict(dict, "radiant_support_level", log->radiant_support_level);
    assign_to_dict(dict, "radiant_support_kills", log->radiant_support_kills);
    assign_to_dict(dict, "radiant_support_deaths", log->radiant_support_deaths);
    assign_to_dict(dict, "radiant_support_damage_dealt", log->radiant_support_damage_dealt);
    assign_to_dict(dict, "radiant_support_damage_received", log->radiant_support_damage_received);
    assign_to_dict(dict, "radiant_support_healing_dealt", log->radiant_support_healing_dealt);
    assign_to_dict(dict, "radiant_support_healing_received", log->radiant_support_healing_received);
    assign_to_dict(dict, "radiant_support_creeps_killed", log->radiant_support_creeps_killed);
    assign_to_dict(dict, "radiant_support_neutrals_killed", log->radiant_support_neutrals_killed);
    assign_to_dict(dict, "radiant_support_towers_killed", log->radiant_support_towers_killed);
    assign_to_dict(dict, "radiant_support_usage_auto", log->radiant_support_usage_auto);
    assign_to_dict(dict, "radiant_support_usage_q", log->radiant_support_usage_q);
    assign_to_dict(dict, "radiant_support_usage_w", log->radiant_support_usage_w);
    assign_to_dict(dict, "radiant_support_usage_e", log->radiant_support_usage_e);
    return 0;
}
