#!/bin/bash
# Run all scripts from game/ directory.
PYVER=$(python3 -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')")
echo "-----Copying extension+Python+config for rlplays to PufferLib-----"
mkdir -p thirdparty/PufferLib/build/lib.linux-x86_64-cpython-${PYVER}/thirdparty/PufferLib/pufferlib
cp -L -f build/lib.linux-x86_64-cpython-${PYVER}/thirdparty/PufferLib/pufferlib/* thirdparty/PufferLib/build/lib.linux-x86_64-cpython-${PYVER}/thirdparty/PufferLib/pufferlib/
bash rlplays/copy_rlplays_to_puffer.sh
if [ $? -ne 0 ]; then
  echo "Failed to copy stuff to PufferLib"
  exit 1
fi

echo "Running training"
cd thirdparty/PufferLib/
echo "****************************************"
echo "***Current Config: ***"
cat config/rlplays.ini
echo "****************************************"
bash ../../../../sites/send-mail.sh "RL Training Started" "Started RL training at $(date)."
echo "python -m pufferlib.pufferl train rlplays --train.device cuda $*"
python -m pufferlib.pufferl train rlplays --train.device cuda $*

if [ $? -ne 0 ]; then
  echo "Failed to train - check logs in experiments/rlplays.log"
  exit 1
fi

bash ../../../../sites/send-mail.sh "RL Training Finished" "Finished RL training at $(date)."

cd ../../
bash ./rlplays/export_weights.sh