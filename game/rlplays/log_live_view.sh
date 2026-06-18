FILE="$(ls -t ./private/logs/*.json.txt | head -n1)"
echo "Viewing: $FILE"
tail -f $FILE