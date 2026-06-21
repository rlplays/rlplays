
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
