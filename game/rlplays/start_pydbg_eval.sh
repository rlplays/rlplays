#!/bin/bash
# Run all scripts from game/ directory.
# Debug eval with pydbg (debugpy)
# Usage: bash start_pydbg_eval.sh
# Then attach VSCode debugger to localhost:5678
export DISPLAY=:10.0
cd thirdparty/PufferLib/
echo "If you get a GLFW error, ensure $DISPLAY matches your \$DISPLAY from a non-ssh/screen terminal window."
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl eval rlplays  --train.device cuda --load-model-path latest 

