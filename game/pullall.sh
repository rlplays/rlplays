#!/bin/bash
cd "$(dirname "$0")"
git pull origin
cd thirdparty/PufferLib
git pull origin

echo "Done pulling all repos"
git status
echo "-----------------"
