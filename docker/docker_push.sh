docker build -t rlplays/rlplays:latest .

docker login                       # prompts for username + access token
docker push rlplays/rlplays:latest

# To run
# docker pull rlplays/rlplays:latest
# docker run -d -p 3399:3389 -p 2222:22 --gpus all --name rlplays_rdp rlplays/rlplays:latest