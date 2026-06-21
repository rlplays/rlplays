
# Setup / Building

Use the prebuilt docker image for a quick start:

```

docker pull rlplays/rlplays:latest
docker run -d -p 3399:3389 -p 2222:22 --gpus all --name rlplays_rdp rlplays/rlplays:latest

# Connect via RDP or SSH (Change the password after connecting!)
# ssh rlplays@localhost -p 2222 (password c)
# or rdp via localhost:3399 rlplays / c
```



## Detailed steps to setup via Docker or via local Linux

> I highly discourage using WSL2 due to a variety of issues [that might hopefully be fixed in the future](https://x.com/craigaloewen/status/2061956765646979091). File I/O and importantly CUDA->dxgkrnl latency is too high to train on WSL2. Just use a Docker container *from within an actual Ubuntu pyhsical machine* to bypass Windows shenanigans.


#### Option A: Use Docker + dockerfile to build/launch the game

```
cd rlplays/docker
docker build -t rlplays:latest .
docker run -d -p 3399:3389 -p 2222:22 --name rlplays_rdp rlplays:latest
# Connect via RDP or SSH
# ssh rlplays@localhost -p 2222 (password c)
# or rdp via localhost:3399 rlplays / c
```


#### Option B:

Manually run parts of [`docker/install_deps.sh`](../docker/install_deps.sh) to install the deps for both Linux and Python manually. This is automatically installed if you use Docker instead and is safer too!

> Note: Use python venv when using the script above - Docker uses a root user as it's already isolated.
> Note: You need CUDA 12.8 (not 13) for both nvidia and torch. Check with `nvcc --version`.


## Windows/WSL2/Mac

> NOTE: I haven't trained the RL env on Windows yet, so YMMV. Use Ubuntu+NVIDIA when in doubt.

On non-Linux machines, you can run the game with the trained weights, but it's not an ideal environment to perform RL training.

For Windows: Use VS 2026 (Community) edition, open folder -> `game/` and use the target `rlplays_game` to run locally (it takes a while as it looks at all raylib/imgui/etc targets as well).
For Mac: The Bash script *should* work fine to launch the game, but haven't tested it much.




------



### Build/launch the game with pretrained RL weights + a sample map:

```
# In docker, cd /home/rlplays/rlplays
cd game/

# Get the latest changes in case the docker image is stale:
git pull

# Build the C++ code and run the GUI env. Requires RDP or some Ubuntu GUI to view the env.
bash full-build.sh DEBUG EDITOR RUN
```

You should see a default map open up. 

![Game Window](../docs/game_window.png)



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

CTRL+Click/drag to resize the block 
SHIFT+Left-click/drag to move blocks
CTRL+D to delete a block
Change any property and you can preview with CTRL+T
Important: Make sure to Save World in the Blocks Window to save the file, otherwise, you will lose the changes!

DEBUG VIEW: CTRL+1 to show the blocks, move vectors, and grid collisions.

```

![Editor View](../docs/rlplays_editor.gif)

See the full docs [in the root README](../README.md).