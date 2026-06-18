#!/bin/bash
# Run all scripts from game/ directory.

echo "Running eval"
cd thirdparty/PufferLib/
echo "python -m pufferlib.pufferl eval rlplays --train.device cuda --load-model-path latest"
python -m pufferlib.pufferl eval rlplays --train.device cuda --load-model-path latest  
if [ $? -ne 0 ]; then
  echo "Failed to eval - check logs in check logs in experiments/rlplays.log"
  exit 1
fi
cd ../../

echo "Evaluation complete"
echo "Check logs in experiments/rlplays.log for details"