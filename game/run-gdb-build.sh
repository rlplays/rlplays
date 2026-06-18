sh full-build.sh $*
cd gameui/src

echo "Type set environment ASAN_OPTIONS=abort_on_error=1"
echo "Then run"

gdb ../../build/gameui/rlplays_game


