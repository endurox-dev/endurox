#!/bin/bash
## 
## @(#) Test028 - clusterised version
##
## @file run-dom.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

export TESTNO="028"
export TESTNAME_SHORT="tmq"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"

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

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
# Override timeout!
export NDRX_TOUT=90

#
# Domain 1 - here client will live
#
function set_dom1 {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf

# XA config, mandatory for TMQ:
    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR="./QSPACE1"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
# Used from parent
    export NDRX_XA_DRIVERLIB=$NDRX_XA_DRIVERLIB_FILENAME

    export NDRX_XA_RMLIB=libndrxxaqdisk.so
if [ "$(uname)" == "Darwin" ]; then
    export NDRX_XA_RMLIB=libndrxxaqdisk.dylib
fi
    export NDRX_XA_LAZY_INIT=0
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt28

    popd 2>/dev/null
    exit $1
}


function clean_logs {

    # clean-up the logs for debbuging at the error.
    for f in `ls *.log`; do
         echo > $f
    done

}
#
# Test Q space for empty condition
#
function test_empty_qspace {
	echo "Testing Qspace empty"
    
	COUNT=`find ./QSPACE1 -type f | wc | awk '{print $1}'`

	if [[ "X$COUNT" != "X0" ]]; then
		echo "QSPACE1 MUST BE EMPTY AFTER TEST!!!!"
		go_out 2
	fi

    clean_logs;
}

rm *dom*.log

# Where to store TM logs
rm -rf ./RM1
mkdir RM1

# Where to store Q messages (QSPACE1)
rm -rf ./QSPACE1
mkdir QSPACE1

cp q.conf.tpl q.conf

set_dom1;
xadmin stop -y
xadmin start -y || go_out 1

# Go to domain 1
set_dom1;

# Run the client test...
xadmin psc
xadmin psvc
xadmin ppm

set | grep NDRX

echo "Running: basic test (enq + deq)"
(./atmiclt28 basic 2>&1) > ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: enqueue"
(./atmiclt28 enq 2>&1) > ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

#find ./QSPACE1 -type f

xadmin stop -y
xadmin start -y || go_out 1
clean_logs;

################################################################################
#
# CLI TESTS
#
################################################################################
#reload config

echo "Testing [xadmin mqrc]"

echo "BINQ2,svcnm=-,autoq=n,tries=10,waitinit=1,waitretry=1,waitretryinc=0,waitretrymax=1,mode=fifo" >> q.conf
xadmin mqrc
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`xadmin mqlc | grep BINQ2 | grep fifo`" ==  "X" ]; then
    echo "Missing BINQ2 not defined!"
    go_out 1
fi


# List q config
xadmin mqlc

echo "Testing [xadmin mqlc]"
#
# Test the config, we shall see "LTESTA" with lifo mode...
#
if [ "X`xadmin mqlc | grep LTESTA | grep lifo`" ==  "X" ]; then
    echo "Missing LTESTA lifo Q"
    go_out 1
fi

# List the queues (should be something..)
xadmin mqlq

echo "Testing [xadmin mqlq]"
if [ "X`xadmin mqlq | grep TESTB | grep 300`" ==  "X" ]; then
    echo "Missing TESTB and 300"
    go_out 1
fi

# List messages in q
xadmin mqlm -s MYSPACE -q TESTC 

#Dump the first message to stdout:

echo "Testing [xadmin mqlm]"

MSGID=`xadmin mqlm -s MYSPACE -q TESTC  | tail -1  | awk '{print($3)}'`

if [ "X$MSGID" ==  "X" ]; then
    echo "Missing message mqlm"
    go_out 1
fi

echo "Dumping message [$MSGID]"
xadmin mqdm -n 1 -i 100 -m $MSGID

echo "Testing [xadmin mqdm] UBF buffer"

if [ "X`xadmin mqdm -n 1 -i 100 -m $MSGID | grep 'TEST HELLO'`" ==  "X" ]; then
    echo "Missing 'TEST HELLO' in mqdm"
    go_out 1
fi

#
# Carray tests..
#

echo "Adding carray message to BINQ"
(./atmiclt28 carr 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

xadmin mqlm -s MYSPACE -q BINQ

MSGID=`xadmin mqlm -s MYSPACE -q BINQ  | tail -1  | awk '{print($3)}'`

echo "Testing [xadmin mqdm] CARRY buffer - $MSGID"
xadmin mqdm -n 1 -i 100 -m $MSGID

# Test the dump output in shell
if [ "X`xadmin mqdm -n 1 -i 100 -m $MSGID | grep '00 01 02 03 04 05 06 07'`" ==  "X" ]; then
    echo "Missing '00 01 02 03 04 05 06 07' in mqdm for CARRAY"
    go_out 1
fi

echo "Testing [xadmin mqrm]"

xadmin mqrm -n 1 -i 100 -m $MSGID

echo "****************************************"
xadmin mqlm -s MYSPACE -q BINQ
echo "****************************************"

xadmin mqlm -s MYSPACE -q BINQ | wc

if [ "X`xadmin mqlm -s MYSPACE -q BINQ | wc | awk '{print($1)}'`" !=  "X0" ]; then
    echo "Message not removed from BINQ"
    go_out 1
fi

echo "Testing [xadmin mqch]"
xadmin mqch -n 1 -i 100 -q MEMQ,svcnm=-,autoq=n,tries=10,waitinit=1,waitretry=1,waitretryinc=0,waitretrymax=1
xadmin mqlc

# List q config

if [ "X`xadmin mqlc | grep MEMQ`" ==  "X" ]; then
    echo "Missing 'MEMQ' in mqch output"
    go_out 1
fi

echo "Testing [xadmin mqmv]"

echo "Adding carray message to BINQ"
(./atmiclt28 carr 2>&1) >> ./atmiclt-dom1.log
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Move the message to another Q"
MSGID=`xadmin mqlm -s MYSPACE -q BINQ  | tail -1  | awk '{print($3)}'`

xadmin mqmv -n 1 -i 100 -m $MSGID -s MYSPACE -q BINQ2

MSGID=`xadmin mqlm -s MYSPACE -q BINQ2  | tail -1  | awk '{print($3)}'`

xadmin mqdm -n 1 -i 100 -m $MSGID

if [ "X`xadmin mqdm -n 1 -i 100 -m $MSGID | grep '00 01 02 03 04 05 06 07'`" ==  "X" ]; then
    echo "Missing '00 01 02 03 04 05 06 07' in mqdm for CARRAY"
    go_out 1
fi

if [ "X`xadmin mqdm -n 1 -i 100 -m $MSGID | grep 'TESTREPLY'`" ==  "X" ]; then
    echo "Missing 'TESTREPLY' in mqdm for mqmv"
    go_out 1
fi

if [ "X`xadmin mqdm -n 1 -i 100 -m $MSGID | grep 'TESTFAIL'`" ==  "X" ]; then
    echo "Missing 'TESTFAIL' in mqdm for mqmv"
    go_out 1
fi

# remove off the message...
xadmin mqrm -n 1 -i 100 -m $MSGID

################################################################################
#
# Continue with system tests...
#
################################################################################

echo "Running: dequeue (abort)"
(./atmiclt28 deqa 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

sleep 10

clean_logs;

echo "Running: dequeue (commit)"
(./atmiclt28 deqc 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: dequeue - empty"
(./atmiclt28 deqe 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: msgid tests"
(./atmiclt28 msgid 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: corid tests"
(./atmiclt28 corid 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: Auto queue ok + reply q"
(./atmiclt28 autoqok 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: Auto queue dead"
(./atmiclt28 autodeadq 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "Running: random fail for auto"
(./atmiclt28 rndfail 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

echo "LIFO tests..."

echo "Running: enqueue (LIFO)"
(./atmiclt28 lenq 2>&1) > ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

xadmin stop -y
xadmin start -y || go_out 1
clean_logs;

xadmin mqlc
xadmin mqlq

xadmin mqlm -s MYSPACE -q LTESTA

FIRST=`xadmin mqlm -s MYSPACE -q LTESTA  | head -1  | awk '{print($3)}'`

echo "First message in Q"
xadmin mqdm -n 1 -i 100 -m $FIRST


LAST=`xadmin mqlm -s MYSPACE -q LTESTA  | tail -1  | awk '{print($3)}'`

echo "Last message in Q"
xadmin mqdm -n 1 -i 100 -m $LAST

echo "Running: dequeue (abort) (LIFO)"
(./atmiclt28 ldeqa 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

sleep 10

echo "Running: dequeue (commit) (LIFO)"
(./atmiclt28 ldeqc 2>&1) >> ./atmiclt-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

test_empty_qspace;

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

if [ "X`grep TPETIME *.log`" != "X" ]; then
	echo "There must be no timeouts during tests!"
	RET=-2
fi



go_out $RET

