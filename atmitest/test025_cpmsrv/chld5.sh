#!/bin/bash

echo "Starting chld5.sh"

trap 'echo Ignoring SIGINT' SIGINT
trap 'echo Ignoring SIGTERM' SIGTERM

# Run off some child
chld6.sh

#while [[ 1 == 1 ]]; do
#        sleep 1
#done

#
# Forkless sleep
#
while [[ 1 == 1 ]]; do
	read < ./test25_sleep.fifo
done

