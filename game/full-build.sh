#!/bin/bash
# To run DEBUG build, pass in DEBUG as an argument
CMAKE_ARGS="-DFETCHCONTENT_SOURCE_DIR_RAYLIB=./thirdparty/raylib/  -DCMAKE_EXPORT_COMPILE_COMMANDS=1"
BUILD_OUT_DIR="build"
MAIN_DIR="./gameui/"
for arg in "$@"
do
  case $arg in
    CLEAN)
      echo "Cleaning first"
      sh clean-build.sh
      ;;
    DEBUG_TRACE)
      CMAKE_ARGS="$CMAKE_ARGS -DDEBUG=1 -DDEBUG_TRACE=1"
      echo "Debug trace mode enabled"
      ;;
    DEBUG)
      CMAKE_ARGS="$CMAKE_ARGS -DDEBUG=1"
      echo "Debug mode enabled"
      ;;
    RELEASE)
      CMAKE_ARGS="$CMAKE_ARGS -DRELEASE=1"
      echo "Release mode enabled"
      ;;
    EDITOR)
      CMAKE_ARGS="$CMAKE_ARGS -DRLPLAYS_EDITOR=1"
      echo "Editor mode enabled"
      ;;
    CONVERTER)
      CMAKE_ARGS="$CMAKE_ARGS -DRLPLAYS_CONVERTER=1"
      echo "Converter enabled"
      ;;
    RL_TRAIN)
      CMAKE_ARGS="$CMAKE_ARGS -DRLPLAYS_TRAIN=1 -DPUFFERLIB_SELFPLAY=1"
      echo "RL Training mode enabled"
      ;;
    TEST)
      export ASAN_OPTIONS="halt_on_error=1:abort_on_error=1:detect_leaks=1:print_stacktrace=1:log_path=asan.log"    
      CMAKE_ARGS="$CMAKE_ARGS -DRLPLAYS_TEST=1"
      echo "Test mode enabled with $CMAKE_ARGS"
      ;;
    PUFFER_CUDA)
      CMAKE_ARGS="$CMAKE_ARGS -DPUFFER_CUDA=1"
      ;;
    PUFFER_TEST)
      echo "NOT WORKING ON UBUNTU YET!"
      exit 1
      CMAKE_ARGS="$CMAKE_ARGS -DRLPLAYS_TRAIN=1 -DRLPLAYS_TEST=1 -DRLPLAYS_PUFFERLIB_TESTS=1 "
      MAIN_DIR="./rlplays/tests"
      #MAIN_DIR="./rlplays/tests/"
      #BUILD_OUT_DIR="build/pufferlib_tests"
      echo "Test (+pufferlib tests) mode enabled"
      ;;
    # Add other flags as needed
    *)
      echo "Unknown argument: $arg"
      ;;
  esac
done

echo "..............................."
echo "Building code using $CMAKE_ARGS"
echo "To clean: sh clean-build.sh"
echo "..............................."
cmake -S $MAIN_DIR -B $BUILD_OUT_DIR $CMAKE_ARGS
if [ $? -ne 0 ]; then
  echo "Build failed"
  exit 1
fi
cmake --build $BUILD_OUT_DIR -j
if [ $? -ne 0 ]; then
  echo "Build failed"
  exit 1
fi

