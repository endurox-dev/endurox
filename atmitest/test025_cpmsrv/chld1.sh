#!/bin/bash

echo "Starting chld1.sh"

trap 'echo Ignoring SIGINT' SIGINT
trap 'echo Ignoring SIGTERM' SIGTERM

# Run off some child
chld2.sh

while [[ 1 == 1 ]]; do
        sleep 1
done

