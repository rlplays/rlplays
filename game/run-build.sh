#!/bin/bash
# To run DEBUG build, pass in DEBUG as an argument
EXE_NAME="rlplays_game"
ADDITIONAL_ARGS=""
MORE_ARGS=""
COMPILE_ARGS=""
for arg in "$@"
do
  case $arg in
    CONVERTER)
      EXE_NAME="rlplays_converter"
      ;;
    TEST)
      EXE_NAME="rlplays_test"
      ADDITIONAL_ARGS="--gtest_output=xml:build/report.xml"
      echo "Test filter: --gtest_filter='*TestMethod*'"
      ;;
    PUFFER_TEST)
      EXE_NAME="pufferlib_test"
      ADDITIONAL_ARGS="--gtest_output=xml:build/report.xml"
      MORE_ARGS="$MORE_ARGS PUFFER_TEST"
      echo "Test filter: --gtest_filter='*TestMethod*'"
      ;;
     *)
      # Anything not matched above will be forwarded to the final EXE
      if [ -z "$MORE_ARGS" ]; then
        MORE_ARGS="$arg"
      else
        MORE_ARGS="$MORE_ARGS $arg"
      fi
      ;;
  esac
done

sh full-build.sh $*
if [ $? -ne 0 ]; then
  echo "Build failed"
  exit 1
fi

echo "..............................."
echo "Running ./build/$EXE_NAME/$EXE_NAME $ADDITIONAL_ARGS $MORE_ARGS..."
echo "..............................."
./build/$EXE_NAME/$EXE_NAME $ADDITIONAL_ARGS $MORE_ARGS
if [ $? -ne 0 ]; then
  echo "$EXE_NAME failed"
  exit 1
fi

