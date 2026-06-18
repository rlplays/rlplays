cmake -S ./tests/ -B ./tests/build/ -DFETCHCONTENT_SOURCE_DIR_RAYLIB=./thirdparty/raylib/ -DCMAKE_BUILD_TYPE=Debug
if [ $? -ne 0 ]; then
  echo "Tests Build generation failed"
  exit 1
fi
cmake --build ./tests/build/
if [ $? -ne 0 ]; then
  echo "Tests Build failure"
  exit 1
fi
cd ./tests/build/
./gametests
if [ $? -ne 0 ]; then
  echo "Tests failed"
  exit 1
fi