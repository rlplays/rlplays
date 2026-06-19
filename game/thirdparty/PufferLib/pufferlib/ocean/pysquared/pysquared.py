'''Pure python version of Squared, a simple single-agent sample environment.
   Use this as a template for your own envs.
'''

# We only use Gymnasium for their spaces API for compatibility with other libraries.
import gymnasium
import numpy as np

import pufferlib

NOOP = 0
DOWN = 1
UP = 2
LEFT = 3
RIGHT = 4

EMPTY = 0
AGENT = 1
TARGET = 2

# Inherit from PufferEnv
class PySquared(pufferlib.PufferEnv):
    # Required keyword arguments: render_mode, buf, seed
    def __init__(self, render_mode='ansi', size=11, buf=None, seed=0):
        # Required attributes below
        self.single_observation_space = gymnasium.spaces.Box(low=0, high=1,
            shape=(size*size,), dtype=np.uint8)
        self.single_action_space = gymnasium.spaces.Discrete(5)
        self.render_mode = render_mode
        self.num_agents = 1

        # Call super after initializing attributes
        super().__init__(buf)

        # Add anything else you want
        self.size = size

    # All methods below are required with the signatures shown
    def reset(self, seed=0):
        self.observations[0, :] = EMPTY
        self.observations[0, self.size*self.size//2] = AGENT
        self.r = self.size//2
        self.c = self.size//2
        self.tick = 0
        while True:
            target_r, target_c = np.random.randint(0, self.size, 2)
            if target_r != self.r or target_c != self.c:
                self.observations[0, target_r*self.size + target_c] = TARGET
                break

        # Observations are read from self. Don't create extra copies
        return self.observations, []

    def step(self, actions):
        atn = actions[0]

        # Note that terminals, rewards, etc. are updated in-place
        self.terminals[0] = False
        self.rewards[0] = 0

        self.observations[0, self.r*self.size + self.c] = EMPTY

        if atn == DOWN:
            self.r += 1
        elif atn == RIGHT:
            self.c += 1
        elif atn == UP:
            self.r -= 1
        elif atn == LEFT:
            self.c -= 1

        # Info is a list of dictionaries
        info = []
        pos = self.r*self.size + self.c
        if (self.tick > 3*self.size
                or self.r < 0
                or self.c < 0
                or self.r >= self.size
                or self.c >= self.size):
            self.terminals[0] = True
            self.rewards[0] = -1.0
            info = [{'reward': -1.0}]
            self.reset()
        elif self.observations[0, pos] == TARGET:
            self.terminals[0] = True
            self.rewards[0] = 1.0
            info = [{'reward': 1.0}]
            self.reset()
        else:
            self.observations[0, pos] = AGENT
            self.tick += 1

        # Return the in-place versions. Don't copy!
        return self.observations, self.rewards, self.terminals, self.truncations, info

    def render(self):
        # Quick ascii rendering. If you want a Python-based renderer,
        # we highly recommend Raylib over PyGame etc. If you use the
        # C-style Python API, it will be very easy to port to C native later.
        chars = []
        grid = self.observations.reshape(self.size, self.size)
        for row in grid:
            for val in row:
                if val == AGENT:
                    color = 94
                elif val == TARGET:
                    color = 91
                else:
                    color = 90
                chars.append(f'\033[{color}m██\033[0m')
            chars.append('\n')
        return ''.join(chars)

    def close(self):
        pass

if __name__ == '__main__':
    env = PySquared()
    env.reset()
    steps = 0

    CACHE = 1024
    actions = np.random.randint(0, 5, (CACHE, 1))

    import time
    start = time.time()
    while time.time() - start < 10:
        env.step(actions[steps % CACHE])
        steps += 1

    print('PySquared SPS:', int(steps / (time.time() - start)))
