#!/bin/bash

echo "Starting chld2.sh"

#while [[ 1 == 1 ]]; do
#        sleep 1
#done

#
# this will exit once parent is dead - avoid forking
#
#
# Forkless sleep
#
read < ./test25_sleep.fifo
