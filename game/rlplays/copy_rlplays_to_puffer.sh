#!/bin/bash
# Run all scripts from game/ directory.
mkdir -p thirdparty/PufferLib/rlplays
cp -f -L -r rlplays/* thirdparty/PufferLib/rlplays/
cp -L rlplays/rlplays.ini thirdparty/PufferLib/pufferlib/config/rlplays.ini
echo "-----Copied extension+Python+config for rlplays to PufferLib-----"
