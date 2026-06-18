echo "-----Building Python extension for rlplays (DEBUG: $DEBUG)-----"
MAX_JOBS=16 python setup_env.py build_torch --inplace --force
MAX_JOBS=16 python setup_env.py build_ext --inplace --force
if [ $? -ne 0 ]; then
  echo "Failed to build Python extension"
  exit 1
fi
echo "*****Extension in thirdparty/PufferLib/rlplays/"
# Move the built extension to the correct location
echo "-----Copying extension+Python+config for rlplays to PufferLib-----"
bash rlplays/copy_rlplays_to_puffer.sh
