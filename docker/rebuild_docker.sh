#!/bin/bash
# Usage: bash rebuild_docker.sh [NOCACHE]
#   NOCACHE - optional. When passed, rebuild the image from scratch (docker build --no-cache).

BUILD_ARGS=""
if [ "$1" = "NOCACHE" ]; then
  echo "Building without cache"
  BUILD_ARGS="--no-cache"
fi

docker rm -f rlplays_rdp
docker rmi rlplays:linux
docker build $BUILD_ARGS -t rlplays:linux .
docker run -d -p 3399:3389 -p 2222:22 --gpus all --name rlplays_rdp rlplays:linux

echo "Built and running docker:"
docker ps




