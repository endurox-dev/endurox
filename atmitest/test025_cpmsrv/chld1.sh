#!/bin/bash

echo "Starting chld1.sh"

trap 'echo Ignoring SIGINT' SIGINT
trap 'echo Ignoring SIGTERM' SIGTERM

# Run off some child
chld2.sh


#
# Sleep for ever... - this is due to fact that forks causes at some points
# to show two processes, thus mes up accounting during run.sh tests
# we cannot sleep for ever with sleep, because then signals are ignored.
# thus read from pipe where nothing comes in
#
while [[ 1 == 1 ]]; do
	read < ./test.fifo
done
