#!/bin/bash
echo "Pulling both repos "
git pull
cd thirdparty/PufferLib/
git pull 


IFS=',' read -ra envs <<< "$1"
for env in "${envs[@]}"; do
  echo "Building env: $env"
  python setup.py build_$env --inplace --force
done
cd ../../