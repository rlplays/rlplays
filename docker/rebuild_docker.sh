docker rm -f rlplays_rdp
docker rmi rlplays:linux
docker build -t rlplays:linux .
docker run -d -p 3399:3389 -p 2222:22 --gpus all --name rlplays_rdp rlplays:linux

echo "Built and running docker:"
docker ps




