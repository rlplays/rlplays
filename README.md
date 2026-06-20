# RLPlays Game Environment - Explore RL using a 2D game

RLPlays game env allows you to build 2D games - pixel platformers, shooters etc - that you can then train using PufferLib.

It supports optional advanced features such as self-play, curriculum learning and so on.

The game is designed around (a) an editor (b) create characters/blocks.

Here is what the game looks with a fully trained RL agent with a bunch of enemies/reward/goal/blocks like:

![RLPlays gameplay clip](docs/rl_clip1.gif)


# Setup / Building

Use the Ubuntu GUI Docker container to setup as Python deps + nvcc/cuda + C++ combo is hard to track; plus isolation helps you avoid messing up your machine / supply-chain attacks.


```
git clone https://github.com/rlplays/rlplays
cd rlplays/docker
docker build -t rlplays:linux .
docker run -d -p 3399:3389 -p 2222:22 --name rlplays_rdp rlplays:linux
# Connect via RDP or SSH (Change the password after connecting!)
# ssh rlplays@localhost -p 2222 (password c)
# or rdp via localhost:3399 rlplays / c
```


> NOTE: You need a machine with NVidia+CUDA to train the RL environment.

<details>
<summary>Detailed steps to setup via Docker or via local Linux</summary>

> I highly discourage using WSL2 due to a variety of issues [that might hopefully be fixed in the future](https://x.com/craigaloewen/status/2061956765646979091). File I/O and importantly CUDA->dxgkrnl latency is too high to train on WSL2. Just use a Docker container *from within an actual Ubuntu pyhsical machine* to bypass Windows shenanigans.


#### Option A: Use Docker + dockerfile to build/launch the game

```
cd rlplays/docker
docker build -t rlplays:linux .
docker run -d -p 3399:3389 -p 2222:22 --name rlplays_rdp rlplays:linux
# Connect via RDP or SSH
# ssh rlplays@localhost -p 2222 (password c)
# or rdp via localhost:3399 rlplays / c
```


#### Option B:

Manually run parts [`docker/install_deps.sh`](./docker/install_deps.sh) to install the deps for both Linux and Python manually. This is automatically installed if you use Docker instead and is safer too!

> Note: Use python venv when using the script above - Docker uses a root user as it's already isolated.
> Note: You need CUDA 12.8 (not 13) for both nvidia and torch. Check with `nvcc --version`.


## Windows/WSL2/Mac

> NOTE: I haven't trained the RL env on Windows yet, so YMMV. Use Ubuntu+NVIDIA when in doubt.

On non-Linux machines, you can run the game with the trained weights, but it's not an ideal environment to perform RL training.

For Windows: Use VS 2026 (Community) edition, open CMake project and use the target `rlplays_game` to run locally.
For Mac: The Bash script *should* work fine to launch the game, but haven't tested it much.

On Windows, use `run-build.cmd` similar to `full-build.sh`.


</details>

------

Next:

### Build/launch the game with pretrained RL weights + a sample map:

```
# In docker, cd /home/rlplays
cd game/
bash full-build.sh DEBUG EDITOR RUN
```

You should see a default map open up. Check out the [editor section below](#useful-editor-commands)




# Editor/Gameplay mode

> Press CTRL+T to toggle between the editor mode and the gameplay mode.

> Press C to switch between the RL Agent and your Player. (On Round 2 and above, you can play *against* the RL agent!)

## Useful editor commands

Start the editor/gameplay program using `bash full-build.sh DEBUG EDITOR RUN`

```
--Shortcuts--

CTRL+T to open/close the editor mode, check out the various files, edit the level etc and go back to playing it.

**Editor Mode**
Right click to select any block
 - Edit the block using the Block window
 - Use the Worlds window to load a map/level/world
 - Use the Blocks to add blocks, save/load world
 - Change the World-level parameters in the World window.
 
SHIFT+Left-click/drag to move blocks
Change any property and you can preview with CTRL+T
Important: Make sure to Save World in the Blocks Window to save the file, otherwise, you will lose the changes!

```

TODO add gifs for editor / llm use

## Using an LLM to create characters/blocks

I use Opus/Sonnet 4.x as well as GPT 5.x to create the blocks/characters - it's a lot of fun to use an LLM as it's very well suited to this task as well as adding things like confetti or scene transitions etc.

I added [`AGENTS.md`](AGENTS.md) to help with this. It's fairly easy to create these blocks manually but editing the various headers to add a new block has many manual steps - LLMs are way better at it and they produce quick/decent UIs.


# RL Training Mode: Train the game using pufferlib

> Note: I have a [forked version of pufferlib (3.0) with my own native multithreading code](https://github.com/rlplays/PufferLib) in `game/thirdparty/PufferLib/`. I haven't ported to 4.0 yet.


```
cd rlplays/game

# First time, use INSTALL_DEPS to install Python deps, process all the levels and then train:
bash rlplays/build_rl_train.sh CLEAN INSTALL_DEPS RELEASE BUILD CONVERTER TRAIN

# To just retrain:
bash rlplays/build_rl_train.sh RELEASE BUILD CONVERTER TRAIN

# To use WANDB (https://wandb.ai)
bash rlplays/build_rl_train.sh RELEASE WANDB BUILD CONVERTER TRAIN
```

# Tests



## Assets

I have included the minimal assets from Kenney.nl as this repo is strictly to understand/train using RL - you can download the full assets from [www.kenney.nl](https://www.kenney.nl).

Please consider donating/buying the full asset pack if you find it useful.


# Thirdparty code

I have included the actual code instead of a complicated `git submodule` setup which is finicky.

Here are the actual thirdparty code in `game/thirdparty/`:
- PufferLib - Main RL training+eval - Using my forked 3.0 branch native multithreading here https://github.com/rlplays/PufferLib
- Raylib - for all the graphics, input and tons of amazing utils. https://raylib.com
- Dear ImGui - for the editor UI, integrated via `raylib_imgui` - https://github.com/ocornut/imgui/
- Kenney.nl - minimal pixel platformer assets included (CC0) - https://kenney.nl/assets
- json - serialization library - https://github.com/nlohmann/json
- emsdk - for the web build - https://github.com/emscripten-core/emsdk

Please check [`THIRDPARTY.md`](THIRDPARTY.md) for the specific licenses.

Although PufferLib is in thirdparty, it's mostly to isolate the github fork as a submodule. It's a core part of the env though.





### NOTES

Add converter notes
Editor notes
RL Training notes
 - Increase num threads/gpu batches/bptt_horizon etc