#!/bin/bash
# Run all scripts from game/ directory.
echo "-----Copying extension+Python+config for rlplays to PufferLib-----"
bash rlplays/copy_rlplays_to_puffer.sh
if [ $? -ne 0 ]; then
  echo "Failed to copy stuff to PufferLib"
  exit 1
fi

echo "Running sweep/training"
cd thirdparty/PufferLib/
echo "****************************************"
echo "***Current Config: ***"
cat config/rlplays.ini
echo "****************************************"

echo "python -m pufferlib.pufferl sweep rlplays --train.device cuda $*"
python -m pufferlib.pufferl sweep rlplays --train.device cuda $*

if [ $? -ne 0 ]; then
  echo "Failed to sweep/train - check logs in experiments/rlplays.log"
  exit 1
fi



