#!/bin/bash
# Run all scripts from game/ directory.
# Debug train with pydbg (debugpy)
# Usage: bash start_pydbg_train.sh
# Then attach VSCode debugger to localhost:5678
cd thirdparty/PufferLib/
echo "Starting training with debugpy (use RELEASE build ; or if you build DEBUG version make sure to pass LD_PRELOAD=$(gcc -print-file-name=libasan.so)..."
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl train rlplays  --train.device cuda 
