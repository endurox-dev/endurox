#!/bin/bash

if [ "X$HELLO1" != "XYTHIS IS 1" ]; then
    echo "TESTERROR ! Invalid value for HELLO1: [$HELLO1]"
fi

if [ "X$HELLO2" != "XYTHIS IS 2" ]; then
    echo "TESTERROR ! Invalid value for HELLO2: [$HELLO2]"
fi

if [ "X$HELLO11" != "XYTHIS IS 11" ]; then
    echo "TESTERROR ! Invalid value for HELLO11: [$HELLO11]"
fi

echo "test55-2.sh - done"
