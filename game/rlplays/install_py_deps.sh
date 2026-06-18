#!/bin/bash
# Run all scripts from game/ directory.
echo "Installing Python dependencies for RL Plays..."
cd thirdparty
source ./start_python_env.sh
if [ $? -ne 0 ]; then
  echo "Failed to build main program"
  exit 1
fi

pip install -e .
echo "There may be errors, but usually it is fine"
cd ../../
cd thirdparty/

