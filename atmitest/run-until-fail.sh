#!/bin/bash

#
# @(#) Run test until it fails...
# shall be run in test dir assuming that it starts with run.sh
#

DONE=0

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <script_name>"
    exit -1
fi

echo "Running up..."

while $1; do
    DONE=$((DONE+1))
    echo "DONE: $DONE loops"
done

echo "Failed at loop: $DONE"
