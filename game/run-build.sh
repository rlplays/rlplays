#!/bin/bash
# To run DEBUG build, pass in DEBUG as an argument

EXE_NAME="rlplays_game"
ADDITIONAL_ARGS=""
RUN_COMMIT_CHANGES=0
MORE_ARGS=""
COMPILE_ARGS=""
for arg in "$@"
do
  case $arg in
    PULL)
      echo "Pulling first"
      git pull
      cd thirdparty/PufferLib
      git pull
      cd ../../
      ;;
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
    COMMIT_CHANGES)
      RUN_COMMIT_CHANGES=1
      echo "Content changes will be committed"
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


if [ $RUN_COMMIT_CHANGES -ne 0 ]; then
  echo "Committing content changes"
  git status
  git add data/
  git add gameui/src/resources/
  git commit -m "Content update"
  git push origin node
  echo "Content changes committed"
fi