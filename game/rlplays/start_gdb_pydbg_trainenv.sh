#!/bin/bash
# Run all scripts from game/ directory.
# Debug train with pydbg (debugpy)
# Usage: bash start_pydbg_train.sh
# Then attach VSCode debugger to localhost:5678
cd thirdparty/PufferLib/
echo "Ensure you use DEBUG=1 NO_ASAN=1 python setup.py build_ext --inplace --force first"
echo "Using $1 as env"
# gdb --args python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl train $1  --train.device cuda 
# LD_PRELOAD=$(gcc -print-file-name=libasan.so) gdb --args python  -m pufferlib.pufferl train $1  --train.device cuda 
#gdb  --args python  -m pufferlib.pufferl train $1  --train.device cuda 
gdbserver :2345 python  -m pufferlib.pufferl train $1  --train.device cuda 
