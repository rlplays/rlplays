#!/bin/bash
echo "First pull/build for env $1"
DEBUG=1 NO_ASAN=1 bash rlplays/build_pull_pyenv.sh $*
echo "Starting debugger for env $1"
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl train puffer_$1  --train.device cuda 
