#!/bin/bash
cd thirdparty/PufferLib/
echo "Using $1 as env"
python -m pufferlib.pufferl train $1  --train.device cuda 
