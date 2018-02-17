#!/bin/bash

#
# @(#) Cache tests, master runner
#

> ./test.out
# Have some terminal output...
tail -f test.out &

(

M_tests=0
M_ok=0
M_fail=0

run_test () {

        test=$1
        M_tests=$((M_tests + 1))
        echo "*** RUNNING [$test]"

        ./$test.sh
        ret=$?
        
        echo "*** RESULT [$test] $ret"
        
        if [[ $ret -eq 0 ]]; then
                M_ok=$((M_ok + 1))
        else
                M_fail=$((M_fail + 1))
        fi
}

run_test "01_run"
run_test "02_run"

echo "*** SUMMARY $M_tests tests executed. $M_ok passes, $M_fail failures"

xadmin killall tail

exit $M_fail

) > test.out 2>&1

