# Run using source start_python_env.sh
command -v uv >/dev/null 2>&1 || pip install --break-system-packages uv
[ -d "puffer" ] || uv venv puffer
source puffer/bin/activate
export CUDA_HOME="${CUDA_HOME:-/usr/local/cuda}"
export PATH="${CUDA_HOME}/bin:${PATH}"
export LD_LIBRARY_PATH="${CUDA_HOME}/lib64:${LD_LIBRARY_PATH:-}"
cd PufferLib

