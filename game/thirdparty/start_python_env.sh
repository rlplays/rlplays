# Run using source start_python_env.sh
command -v uv >/dev/null 2>&1 || pip install --break-system-packages uv
[ -d "puffer" ] || uv venv puffer
source puffer/bin/activate
cd PufferLib

