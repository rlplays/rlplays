#!/bin/bash
# NOTE: Make sure pwd is game/.
# Profile with pydbg (debugpy) 
# e.g. usage: bash start_profile_env.sh puffer_go,puffer_breakout,puffer_squared --profile.name multiproc --profile.train 0
# Usage: bash start_pydbg_eval.sh
# Then attach VSCode debugger to localhost:5678
export DISPLAY=:10.0
cd thirdparty/PufferLib/
echo "If you get a GLFW error, ensure $DISPLAY matches your \$DISPLAY from a non-ssh/screen terminal window."

IFS=',' read -ra envs <<< "$1"
for env in "${envs[@]}"; do
   echo "Using env: $env"
   echo "python -m pufferlib.pufferl profile $env --train.device cuda ${@:2}"
   # To debug uncomment this instead:
   # python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl profile $env --train.device cuda ${@:2}
   python -m pufferlib.pufferl profile "$env" --train.device cuda "${@:2}"

done