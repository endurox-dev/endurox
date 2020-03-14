#!/bin/bash

# Bash does not react on INT...
_term() { 
        exit 0
}

trap _term SIGTERM
trap _term SIGINT

#while [[ 1 == 1 ]]; do
#        sleep 1;
#done

#
# Forkless sleep
#
read < ./test25_sleep.fifo

