
# PufferLib + RL + RLPlays game

See [puffer docs](../thirdparty/PufferLib/docs/ocean_swimming.md) for full details on how to setup PufferLib.
Make sure to run `git submodule update --init --recursive` and ensure `git branch -v` shows the right branch inside `../thirdparty/PufferLib`.

Simplified setup for our needs:

```bash
# First install any deps as needed (especially if you get errors like "#include <Python.h><-- not found")
sudo apt-get update
sudo apt-get install python3-dev build-essential  nvidia-cuda-toolkit 

cd thirdparty/

# Start the local Puffer Python venv. Do this in a separate screen/terminal
source start_python_env.sh

cd ../

# In rlplays/game:
bash rlplays/build_rl_train.sh

# Verify raw C++ build works
./rlplays/out/rlplays

# Verify ext works
cd thirdparty/PufferLib
python -m rlplays.rlplays

#### DEBUG MODE:
DEBUG="1" bash rlplays/build_rl_train.sh
```

## Train/Eval using PufferLib

```bash
# Train the model
python -m pufferlib.pufferl train rlplays --train.device cuda

# Load latest model: 
python -m pufferlib.pufferl eval rlplays --train.device cuda --load-model-path latest
# ^^ also try out random weights by ignoring --load-model-path


# Debug training by outputting to a log
python -m pufferlib.pufferl train rlplays --train.device cuda --filelog_epoch 100
# cat experiments/rlplays.log

```


One-liner to pull, clean, build, test, train, eval:
```bash
bash rlplays/build_rl_train.sh CLEAN PULL BUILD TEST  TRAIN EVAL
```

## Model notes

- We set rnn Recurrent (LSTM)
- in `models.py`:
- Policy is `class LSTMWrapper(nn.Module):`

## Debugging Python + C++ issues


```
#### DEBUG MODE:
CUDA_VISIBLE_DEVICES=None LD_PRELOAD=$(gcc -print-file-name=libasan.so) python -m pufferlib.pufferl train rlplays --train.device cuda 
# Or: 
CUDA_VISIBLE_DEVICES=None LD_PRELOAD=$(gcc -print-file-name=libasan.so) gdb python

gdb python
set args -m rlplays.rlplays
# Or (also set filelog_epoch=1 under [train] in rlplays.ini)
set args -m pufferlib.pufferl train rlplays --train.device cuda 
run

# Backtrace

bt
info locals
```


## Track with WandB

```bash
pip install wandb -qU

# Help:  wandb --help
wandb login
# Paste API key  from https://wandb.ai/authorize?ref=models
#### Full message below
# wandb: Logging into wandb.ai. (Learn how to deploy a W&B server locally: https://wandb.me/wandb-server)
# wandb: You can find your API key in your browser here: https://wandb.ai/authorize?ref=models
# wandb: Paste an API key from your profile and hit enter, or press ctrl+c to quit:
# wandb: No netrc file found, creating one.
# wandb: Appending key for api.wandb.ai to your netrc file: /home/peru/.netrc
# wandb: Currently logged in as: rlplays (rlplays-com) to https://api.wandb.ai.
```


## Debug with Python debugger + VS Code:

To debug eval:

```
cd thirdparty/PufferLib
export DISPLAY=10.0
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl eval rlplays  --train.device cuda --load-model-path latest  
```

To debug train:

```
cd thirdparty/PufferLib
python -m debugpy --wait-for-client --listen 0.0.0.0:5678 -m pufferlib.pufferl train rlplays  --train.device cuda 
```

(Use `start_pydbg_*` scripts to debug)

And then 

In VS Code, go to the Run and Debug view (Ctrl+Shift+D)
Select the "Python: Remote Attach" configuration
Click the play button or press F5 to connect


## GPU info


Typically ~60C under training 70-100+W, ~35C (or room temp) 5W when idle (GTX 1080)

peru@rlplayspc:~$ nvidia-smi 
Wed Aug 13 09:34:09 2025       
+-----------------------------------------------------------------------------------------+
| NVIDIA-SMI 550.163.01             Driver Version: 550.163.01     CUDA Version: 12.4     |
|-----------------------------------------+------------------------+----------------------+
| GPU  Name                 Persistence-M | Bus-Id          Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf          Pwr:Usage/Cap |           Memory-Usage | GPU-Util  Compute M. |
|                                         |                        |               MIG M. |
|=========================================+========================+======================|
|   0  NVIDIA GeForce GTX 1080        Off |   00000000:01:00.0 Off |                  N/A |
| 42%   62C    P2             70W /  180W |    2863MiB /   8192MiB |     90%      Default |
|                                         |                        |                  N/A |
+-----------------------------------------+------------------------+----------------------+
                                                                                         
+-----------------------------------------------------------------------------------------+
| Processes:                                                                              |
|  GPU   GI   CI        PID   Type   Process name                              GPU Memory |
|        ID   ID                                                               Usage      |
|=========================================================================================|
|    0   N/A  N/A      4750      G   /usr/lib/xorg/Xorg                              4MiB |
|    0   N/A  N/A   2352512      C   python                                       2854MiB |
+-----------------------------------------------------------------------------------------+
peru@rlplayspc:~$ nvidia-smi 


```


Idle:
```
~$ nvidia-smi 
Wed Aug 13 09:36:40 2025       
+-----------------------------------------------------------------------------------------+
| NVIDIA-SMI 550.163.01             Driver Version: 550.163.01     CUDA Version: 12.4     |
|-----------------------------------------+------------------------+----------------------+
| GPU  Name                 Persistence-M | Bus-Id          Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf          Pwr:Usage/Cap |           Memory-Usage | GPU-Util  Compute M. |
|                                         |                        |               MIG M. |
|=========================================+========================+======================|
|   0  NVIDIA GeForce GTX 1080        Off |   00000000:01:00.0 Off |                  N/A |
| 33%   42C    P8              6W /  180W |       7MiB /   8192MiB |      0%      Default |
|                                         |                        |                  N/A |
+-----------------------------------------+------------------------+----------------------+
                                                                                         
+-----------------------------------------------------------------------------------------+
| Processes:                                                                              |
|  GPU   GI   CI        PID   Type   Process name                              GPU Memory |
|        ID   ID                                                               Usage      |
|=========================================================================================|
|    0   N/A  N/A      4750      G   /usr/lib/xorg/Xorg                              4MiB |
+-----------------------------------------------------------------------------------------+

```

Adjust GPU fan speed: [here](https://askubuntu.com/a/1372491/2175228)


