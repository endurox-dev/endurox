#!/bin/bash

#
# @(#) Run test until it fails...
# shall be run in test dir assuming that it starts with run.sh
#

DONE=0

echo "Running up..."

while ./run.sh; do
	DONE=$((DONE+1))
	echo "DONE: $DONE loops"
done

echo "Failed at loop: $DONE"
