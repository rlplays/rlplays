"""A simple sample environment. Use this as a template for your own envs."""

import gymnasium
import numpy as np

import pufferlib
from rlplays import binding


class RLPlays(pufferlib.PufferEnv):
    def __init__(
        self,
        num_envs=1,
        num_obs=3179,
        num_actions=7,
        width=3072,
        height=1728,
        num_frame_skips=10,
        render_mode=None,
        log_interval=128,
        randomize_player_pos=1,
        size=11,
        buf=None,
        seed=0,
        max_num_threads=0,
    ):
        self.single_observation_space = gymnasium.spaces.Box(
            low=-1,
            high=1,
            shape=((num_obs),),
            dtype=np.float32,
        )

        # self.single_action_space = gymnasium.spaces.Discrete(num_actions)
        self.single_action_space = gymnasium.spaces.MultiDiscrete([2] * num_actions)
        self.num_agents = num_envs

        self.render_mode = render_mode
        self.log_interval = log_interval

        super().__init__(buf, binding=binding, max_num_threads=max_num_threads)
        c_envs = []
        for i in range(num_envs):
            c_env = binding.env_init(
                self.observations[i : (i + 1)],
                self.actions[i : (i + 1)],
                self.rewards[i : (i + 1)],
                self.terminals[i : (i + 1)],
                self.truncations[i : (i + 1)],
                seed,
                width=width,
                height=height,
                num_actions=num_actions,
                num_obs=num_obs,
                num_frame_skips=num_frame_skips,
                randomize_player_pos=randomize_player_pos,
            )
            c_envs.append(c_env)
        
        self.c_envs = binding.vectorize(*c_envs)

    def reset(self, seed=0):
        binding.vec_reset(self.c_envs, seed)
        self.tick = 0
        return self.observations, []
    def step(self, actions):
        self.tick += 1
        self.actions[:] = actions
        binding.vec_step(self.c_envs)

        info = []
        if self.tick % self.log_interval == 0:
            log = binding.vec_log(self.c_envs)
            if log:
                info.append(log)

        return (self.observations, self.rewards, self.terminals, self.truncations, info)

    def render(self):
        binding.vec_render(self.c_envs, 0)

    def close(self):
        binding.vec_close(self.c_envs)


if __name__ == "__main__":
    N = 512
    CACHE = 1024
    print("Actions setup")
    actions = np.random.randint([7], size=(CACHE, 1))

    print("Starting RLPlays Benchmark")
    env = RLPlays()

    print("Env reset")
    env.reset()
    steps = 0

    i = 0
    import time

    start = time.time()
    print(f"Starting time {start}")
    while time.time() - start < 10:
        # if steps % 1000 == 0:
        #     print (f"Step #{steps}")
        env.step(actions[i % CACHE])
        steps += 1
        i += 1

    print(f"Endingtime {time.time()}")
    print("RLPlays SPS:", int(steps / (time.time() - start)))
