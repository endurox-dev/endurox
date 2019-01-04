#!/bin/bash

# Bash does not react on INT...
_term() { 
        exit 0
}

trap _term SIGTERM
trap _term SIGINT

while [[ 1 == 1 ]]; do
        #sleep 1;
	# do not fork... please while sleeping...
	read -t 1 <> ndrxconfig.xml
done
