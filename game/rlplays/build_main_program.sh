#!/bin/bash
# Run all scripts from game/ directory.
echo "First building all core rlplays libraries"
sh full-build.sh CLEAN RL_TRAIN RELEASE
