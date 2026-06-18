# RLPlays Game Environment - Explore RL using a 2D game

RLPlays game env allows you to build 2D games - pixel platformers, shooters etc - that you can then train using PufferLib.

It supports optional advanced features such as self-play, curriculum learning and so on.

The game is design around (a) an editor (b) create characters/blocks.

Here is what the game looks with a fully trained RL agent with a bunch of enemies/reward/goal/blocks like:

![RLPlays gameplay clip](docs/rl_clip1.gif)

**Basic Structure**




# Setup / Building

## Ubuntu/WSL2/Mac

## Windows

Use VS 2026 (Community) edition, open CMake project and use the target `rlplays_game` to run locally.

## Web

# Editor: Create blocks and edit levels

## Using an LLM to create characters/blocks

I use Opus/Sonnet 4.x as well as GPT 5.x to create the blocks/characters - it's a lot of fun to use an LLM as it's very well suited to this task as well as adding things like confetti or scene transitions etc.

I added [`AGENTS.md`](AGENTS.md) to help with this. It's fairly easy to create these blocks manually but editing the various headers to add a new block has many manual steps - LLMs are way better at it and they produce quick/decent UIs.

## Editor Shortcuts

`CTRL+T`: Open/close editor mode (only on desktop)


# Tests


## Game tests

## RL tests


## Assets

I have included the minimal assets from Kenney.nl - you can download the full assets from [www.kenney.nl](https://www.kenney.nl).
Please consider donating/buying the full asset pack if you find it useful.

# Thirdparty code

- Raylib - for all the graphics, input and tons of amazing utils.
- Dear ImGui - for the editor UI, integrated with raylib_imgui
- Kenney.nl - minimal pixel platformer assets included (CC0)
- jsoncpp - serialization library
- emsdk - for the web build

Please check [`THIRDPARTY.md`](THIRDPARTY.md) for the specific licenses.

Although PufferLib is in thirdparty, it's mostly to isolate the github fork as a submodule. It's a core part of the env though.


