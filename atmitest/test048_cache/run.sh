#!/bin/bash

#
# @(#) Cache tests, master runner
#

export TESTNAME="test048_cache"

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
    # Do nothing 
    echo > /dev/null
else
    # started from parent folder
    pushd .
    echo "Doing cd"
    cd $TESTNAME
fi;

rm *.log

> ./test.out
# Have some terminal output...
tail -f test.out &

(

M_tests=0
M_ok=0
M_fail=0
M_failstr=""


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
                M_failstr="$M_failstr $test.sh"
        fi
}

run_test "01_run"
run_test "02_run"
run_test "03_run"
run_test "04_run"
run_test "05_run_refresh"
run_test "06_run_bootreset"
run_test "07_run_failsvc"
run_test "08_run_invaltheir"

echo "*** SUMMARY $M_tests tests executed. $M_ok passes, $M_fail failures ($M_failstr)"

xadmin killall tail

exit $M_fail

) > test.out 2>&1

