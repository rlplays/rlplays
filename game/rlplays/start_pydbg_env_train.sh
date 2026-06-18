#!/bin/bash
# Run all scripts from game/ directory.
# Debug train with pydbg (debugpy)
# Usage: bash start_pydbg_train.sh
# Then attach VSCode debugger to localhost:5678
cd thirdparty/PufferLib/
echo "Using $1 as env with args: $2 $3 $4 $5 $6 $7 $8 $9"
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl train $1  --train.device cuda $2 $3 $4 $5 $6 $7 $8 $9
