#!/bin/bash

#
# @(#) Run test until it fails...
# shall be run in test dir assuming that it starts with run.sh
#

DONE=0

fail_out()
{
    echo "Failed at loop: $DONE"
    exit -1
}

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <script_name>"
    exit -1
fi

echo "Running up..."

while true; do

    for argument in "$@"
    do
        echo "Running command [$argument] @ loop $DONE"
        $argument || fail_out
    done

    DONE=$((DONE+1))
    echo "DONE: $DONE loops"
done

