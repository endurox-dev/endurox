#!/bin/bash

if [ "X$HELLO1" != "XYTHIS IS 1" ]; then
    echo "TESTERROR ! Invalid value for HELLO1: [$HELLO1]"
fi

if [ "X$HELLO2" != "XYTHIS IS 2" ]; then
    echo "TESTERROR ! Invalid value for HELLO2: [$HELLO2]"
fi

if [ "X$HELLO6" != "X" ]; then
    echo "TESTERROR ! Invalid value for HELLO6: [$HELLO6]"
fi

if [ "X$HELLO7" != "XYTHIS IS 6" ]; then
    echo "TESTERROR ! Invalid value for HELLO7: [$HELLO7]"
fi

if [ "X$HELLO10" != "XYTHIS IS 10" ]; then
    echo "TESTERROR ! Invalid value for HELLO10: [$HELLO10]"
fi


echo "test55.sh - done"
