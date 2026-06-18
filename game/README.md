# VS configuration

Open Folder, select rlplays/game
Set root CMake project -> gameui/CMakeLists.txt
Choose rlplays_game.exe as target (unselect everything else).

# To build on Linux

Install CMake (ask Copilot how to). Easiest way:

```sh
sudo apt install cmake libxrandr-dev xorg-dev libxinerama-dev
# Also install RL stuff (see PufferLib ocean_swimming.md for more details)
sudo apt-get install python3-dev build-essential  nvidia-cuda-toolkit 
```



Next, build debug+editor:

```sh
sh run-build.sh DEBUG EDITOR

```


## Web Build notes

- Using emscripten to compile/run on WASM/HTML5.

Following [this guide](https://dev.to/marcosplusplus/how-to-install-raylib-with-web-support-l71)

```
sudo apt install libasound2-dev mesa-common-dev libx11-dev libxrandr-dev \
libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev \
build-essential cmake make g++ \
freeglut3-dev libglfw3 libglfw3-dev
```

Also needed:

```
sudo apt install emscripten
```

Next:

```
bash web-build.sh
```

Just works. Places the files in sites under the blog ../../sites/perumaal/astroblog/public/game/.