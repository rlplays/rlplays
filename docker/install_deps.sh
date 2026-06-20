#!/bin/bash
set -euo pipefail

# Repo / project layout (matches the clone path used in the Dockerfile).
REPO_ROOT="${REPO_ROOT:-/home/rlplays/rlplays}"
GAME_DIR="${REPO_ROOT}/game"
THIRDPARTY_DIR="${GAME_DIR}/thirdparty"
PUFFER_DIR="${THIRDPARTY_DIR}/PufferLib"
VENV_DIR="${THIRDPARTY_DIR}/puffer"

# CUDA arches to build the PufferLib CUDA extension for (V100..H100).
TORCH_CUDA_ARCH_LIST="${TORCH_CUDA_ARCH_LIST:-7.0;7.5;8.0;8.6;8.9;9.0}"

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

# --- CUDA environment (needed so setup.py builds the CUDA extension) ---
export CUDA_HOME="${CUDA_HOME:-/usr/local/cuda}"
export PATH="${CUDA_HOME}/bin:${PATH}"
export LD_LIBRARY_PATH="${CUDA_HOME}/lib64:${LD_LIBRARY_PATH:-}"

if [ ! -d "${CUDA_HOME}" ]; then
    echo "WARNING: CUDA_HOME (${CUDA_HOME}) not found. The PufferLib CUDA extension will"
    echo "         fall back to a CPU-only build and 'build_rl_train.sh ... TRAIN' (which"
    echo "         uses --train.device cuda) will fail at runtime."
fi

# --- Python tooling ---
pip3 install --upgrade pip uv ninja

# --- Create / activate the venv used by all rlplays training scripts ---
cd "${THIRDPARTY_DIR}"
[ -d "${VENV_DIR}" ] || uv venv puffer
# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

# --- Install PyTorch (CUDA 12.8 build to match the nvidia/cuda:12.8.0 base image) ---
uv pip install --index-url https://download.pytorch.org/whl/cu128 \
    torch torchvision torchaudio

# --- Build-time dependencies that PufferLib's setup.py imports at module load.
#     These must live in the venv so the editable build can run without isolation. ---
uv pip install setuptools wheel Cython "numpy<2.0" pybind11 ninja

# --- Install PufferLib (editable) together with all of its runtime dependencies.
#     --no-build-isolation makes the build reuse the venv's torch/numpy/pybind11
#     (matching PufferLib's [tool.uv] no-build-isolation-package = ["torch"]),
#     so the CUDA extension links against the cu128 torch installed above. ---
cd "${PUFFER_DIR}"
TORCH_CUDA_ARCH_LIST="${TORCH_CUDA_ARCH_LIST}" \
    uv pip install --no-build-isolation -e .

echo "------------------------------------------------------------"
echo "Dependencies installed into venv: ${VENV_DIR}"
echo "PufferLib installed (editable) from: ${PUFFER_DIR}"
echo "You can now train with:"
echo "  cd ${GAME_DIR} && bash rlplays/build_rl_train.sh CLEAN BUILD TRAIN"
echo "------------------------------------------------------------"

