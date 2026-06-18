#!/bin/bash
# Run all scripts from game/ directory.
echo "Check logs in experiments/rlplays.log for details"

EXPORT_ARGS="latest"
if [ "$1" != "" ]; then
  EXPORT_ARGS="$1"
fi
cd thirdparty/PufferLib
echo "Exporting weights..."
python -m pufferlib.pufferl export rlplays --load-model-path $EXPORT_ARGS
if [ $? -ne 0 ]; then
  echo "Export failed"
  exit 1
fi

DATE=$(date +%Y_%m_%d)
echo "Copying to ../../editor/alldata/models/"
mkdir -p ../../editor/alldata/models/
cp -f -L rlplays_weights.bin ../../editor/alldata/models/rlplays_weights.bin
cp -f -L rlplays_weights.bin ../../editor/alldata/models/rlplays_weights_${DATE}.bin
cp -f -L rlplays_config.ini ../../editor/alldata/models/rlplays_config.ini
cp -f -L rlplays_config.ini ../../editor/alldata/models/rlplays_config_${DATE}.ini
echo "...Done"

echo "Updating worlds.json with new weights entry..."

WORLDS_JSON="../../editor/alldata/worlds/worlds.json"
python3 - <<EOF
import json
date = "${DATE}"
entry = {
    "Comment": "Weights from ${DATE}.",
    "Config": "models/rlplays_config_${DATE}.ini",
    "Filename": "models/rlplays_weights_${DATE}.bin"
}
with open("${WORLDS_JSON}", 'r') as f:
    data = json.load(f)
weights = data['RLTrain']['Weights']
if not any(w.get('Filename') == entry['Filename'] for w in weights):
    weights.append(entry)
    print("Added dated weights entry to worlds.json")
else:
    print("Dated weights entry already exists in worlds.json")
data['RLTrain']['PreferredWeight'] = entry
with open("${WORLDS_JSON}", 'w') as f:
    json.dump(data, f, indent=2)
print(f"Set PreferredWeight to {entry['Filename']}")
EOF

echo "...Done"

cp -f -L rlplays_weights.bin ../../data/models/rlplays_weights.bin
cp -f -L rlplays_config.ini ../../data/models/rlplays_config.ini

cp -f -L rlplays_weights.bin ../../gameui/src/resources/models/rlplays_weights.bin
cp -f -L rlplays_config.ini ../../gameui/src/resources/models/rlplays_config.ini

echo "********************************************************************"
echo "Export complete."
echo "Weights + config copied to editor/alldata/models/"
echo "For reference, latest experiment @ "
date
echo "...last 10 files:"
ls -lt experiments/rlplays*  | head -n 10
echo " Internal logs @ data/temp/file_counts.txt"
#cat ../../data/temp/file_counts.txt | tail -n 1
#echo " Internal logs @ data/temp/file_counts.txt"
echo "********************************************************************"