#!/bin/bash

echo "Starting ignore.sh"

trap 'echo Ignoring SIGINT' SIGINT
trap 'echo Ignoring SIGTERM' SIGTERM

#while [[ 1 == 1 ]]; do
#        sleep 1
#done

#
# Forkless sleep
#
read < ./test25_sleep.fifo
