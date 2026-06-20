#!/bin/bash
set -e

# --- System update ---
apt-get update && apt-get install -y --no-install-recommends \
    wget curl ca-certificates gnupg lsb-release \
    python3 python3-pip python3-venv python3-dev \
    build-essential git tmux

# NOTE: CUDA is expected to already be installed (e.g. via the nvidia/cuda Docker base image).
# If running outside Docker without CUDA, install it manually first:
#   https://developer.nvidia.com/cuda-downloads

# --- Clean up ---
apt-get clean && rm -rf /var/lib/apt/lists/*

# --- Python packages ---
pip3 install --upgrade pip uv ninja
cd /home/rlplays/rlplays/game/thirdparty/
uv venv puffer
source puffer/bin/activate
uv pip install --index-url https://download.pytorch.org/whl/cu128 torch torchvision torchaudio

