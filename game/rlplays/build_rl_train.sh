#!/bin/bash
# Run all scripts from game/ directory.

# Builds the C/C++ code with Python extension and places them in thirdparty/PufferLib/rlplays/_C.so
# Must be run from the parent dir: bash rlplays/build_rl_train.sh
# Example usage:
# rlplays/game$ bash rlplays/build_rl_train.sh CLEAN PULL RELEASE TEST CONVERTER BUILD TRAIN

# Check if PULL argument is provided to run git pull
TRAIN_ARGS=""
BUILD_ARGS="fast"
DEBUG=0  # Initialize DEBUG flag to 0 (false)
cd thirdparty
source start_python_env.sh
cd ../../
pwd
export SELF_PLAY=1

for arg in "$@"
do
  if [ $? -ne 0 ]; then
    echo "Error - exiting."
    exit 1
  fi

  case $arg in
    CLEAN)
      echo "Cleaning first"
      sh clean-build.sh
      ;;
    DEBUG)
      BUILD_ARGS="local"
      export DEBUG=1
      export NO_ASAN=1
      
      ;;
    BUILD_MAIN_ONLY)
      bash rlplays/build_main_program.sh $BUILD_ARGS
      if [ $? -ne 0 ]; then
        echo "Failed to build main program"
        exit 1
      fi
      ;;
    BUILD)
      bash rlplays/build_main_program.sh $BUILD_ARGS
      if [ $? -ne 0 ]; then
        echo "Failed to build main program"
        exit 1
      fi

      export DEBUG=$DEBUG
      bash rlplays/build_py_ext.sh 
      if [ $? -ne 0 ]; then
        echo "Failed to build C++/Python extension"
        exit 1
      fi
      ;;
    PULL)
      echo "Pulling first"
      git pull
      cd thirdparty/PufferLib
      git pull
      cd ../../
      ;;
    WANDB)
      echo "Enabling Weights & Biases logging"
      TRAIN_ARGS="$TRAIN_ARGS --wandb --tag rlplays --no-model-upload"
      ;;
    LOAD_MODEL)
      echo "Load model fro previous training"
      TRAIN_ARGS="$TRAIN_ARGS --load-model-path latest"
      ;;
    TRAIN)
      echo "RL Training mode enabled with args: $TRAIN_ARGS"

      # export PYTORCH_CUDA_ALLOC_CONF="expandable_segments:True,max_split_size_mb:512"
      # export PYTORCH_ALLOC_CONF="expandable_segments:True,max_split_size_mb:512"
      if [ $DEBUG -eq 1 ]; then
        echo "Debug mode: using debug training script"
        echo "Remember to attach to the process as it won't proceed until you do so"
        bash rlplays/start_pydbg_train.sh $TRAIN_ARGS
      else
        bash rlplays/run_train.sh $TRAIN_ARGS
      fi
      ;;
    SWEEP)
      echo "RL Sweep/Training mode enabled with args: $TRAIN_ARGS"
      bash rlplays/run_sweep.sh $TRAIN_ARGS
      ;;
    EVAL)
      echo "RL Eval mode enabled"

      bash rlplays/run_eval.sh
      ;;
    CONVERTER)
      echo "Converter mode enabled"
      sh run-build.sh CLEAN RELEASE CONVERTER
      ;;
    TEST)
      echo "Converter + Test+Debug mode enabled"
      sh run-build.sh CLEAN RELEASE TEST DEBUG
      ;;
    RUN)
      echo "Running trained model"
      ./rlplays/out/rlplays trained
      ;;
    RUN_GDB)
      echo "Running trained model (with gdb)"
      echo "Use 'run trained' inside gdb to start"
      gdb ./rlplays/out/rlplays 
      ;;
    # Add other flags as needed
    *)
      echo "Unknown argument: $arg"
      ;;
  esac
done



echo "-----Building rlplays complete-----"

