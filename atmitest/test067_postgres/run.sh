#!/bin/bash
##
## @brief Postgresql distributed transaction test ECPG/PQ - test launcher
##  Additionally we test common commit queue + database including automatic
##  forward Bug #421
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## AGPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU Affero General Public License, version 3 as published
## by the Free Software Foundation;
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
## for more details.
##
## You should have received a copy of the GNU Affero General Public License along 
## with this program; if not, write to the Free Software Foundation, Inc., 
## 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

export TESTNAME="test067_postgres"

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

export NDRX_TOUT=10

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf

    # XA SECTION
    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR="{ \"url\":\"tcp:postgresql://${EX_PG_HOST}/${EX_PG_DB}\", \"user\":\"${EX_PG_USER}\", \"password\":\"${EX_PG_PASS}\"}"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
    export NDRX_XA_DRIVERLIB=libndrxxapq.so
    export NDRX_XA_RMLIB=-
    export NDRX_XA_LAZY_INIT=1
    # TODO: Add both test modes... 
    #export NDRX_XA_FLAGS="NOJOIN"
    # XA SECTION, END

}

export NDRX_LIBEXT="so"

if [ "$(uname)" == "Darwin" ]; then
    export NDRX_XA_RMLIB=libndrxxaqdisk.dylib
    export NDRX_LIBEXT="dylib"
fi

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y


    # If some alive stuff left...
    xadmin killall atmiclt67

    popd 2>/dev/null
    exit $1
}

# Where to store TM logs
rm -rf ./RM1
mkdir RM1

rm -rf ./RM2
mkdir RM2

# Where to store Q messages (QSPACE1)
rm -rf ./QSPACE1
mkdir QSPACE1

rm *.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

# remove any prep transactions

xadmin recoverlocal -p
xadmin abortlocal -y


RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;
(./atmiclt67 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt67 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

################################################################################
# Queue test...
################################################################################
echo "Queue tests..."
(./atmiclt67 testq 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


################################################################################
# Timeout test... Bug #417
################################################################################
echo "Timeout tests..."

(./atmiclt67 tout 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

################################################################################
# Test that we get error at xa_end
################################################################################
echo "ENDPREPFAIL test..."
export NDRX_TESTMODE=ENDPREPFAIL

(./atmiclt67 endfail 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

################################################################################
# Perform tests on transaction recovery...
################################################################################

echo "TMSNOCOMMIT/local commands test..."
export NDRX_TESTMODE=TMSNOCOMMIT

xadmin stop -y
xadmin start -y

# remove any left over
xadmin abortlocal -y


echo "Do insert..."
(./atmiclt67 doinsert 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

xadmin recoverlocal
CNT=`xadmin recoverlocal | wc | awk '{print $1}'`


if [ "X$CNT" != "X1" ]; then

    echo "Expected 1 transaction, got: $CNT"
    goto_out -10

fi

xadmin commitlocal -y


echo "Check 50"
(./atmiclt67 ck50 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


echo "Do insert2..."
(./atmiclt67 insert2 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

xadmin abortlocal -y -p

echo "Check 0"
(./atmiclt67 ck0 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


################################################################################
# Common queue tests
# 1. Shared insert + abort (non auto queue)
# Try to get count from Q -> no records
# Try to get from Db -> 0 records
# the same with commit, must have records for both..
################################################################################

unset NDRX_TESTMODE

xadmin stop -y
xadmin start -y

echo "Queue Forward transactional tests..."

(./atmiclt67 enqok 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Sleep 5 - wait for forward"
sleep 5

echo "There should be 1 msg in table"
(./atmiclt67 ck1 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Invalid count in table!"
    go_out $RET
fi

echo "There should be no messages in queue"

xadmin mqlm -s "MYSPACE" -q "OKQ1"
OUT=`xadmin mqlm -s "MYSPACE" -q "OKQ1"`

if [ "X$OUT" != "X" ]; then
    echo "No output expected for Q!"
    go_out 100
fi

echo "There should be no prepared transactions"

xadmin recoverlocal

OUT=`xadmin recoverlocal`

if [ "X$OUT" != "X" ]; then
    echo "No output expected for transactions!"
    go_out 101
fi


echo "Queue dead queue"

(./atmiclt67 enqfail 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Sleep 5 - wait for forward / sill msg in Q"
sleep 5

xadmin mqlm -s "MYSPACE" -q "BADQ1"
OUT=`xadmin mqlm -s "MYSPACE" -q "BADQ1"`

if [ "X$OUT" == "X" ]; then
    echo "Expected message BADQ1!"
    go_out 100
fi

echo "Wait 15 to finish Q off"
sleep 15

echo "There should be 0 msg in table"
(./atmiclt67 ck0 2>&1) >> ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Invalid count in table, must be 0!"
    go_out $RET
fi

echo "There should be no messages in queue (as expired & removed)"

xadmin mqlm -s "MYSPACE" -q "BADQ1"
OUT=`xadmin mqlm -s "MYSPACE" -q "BADQ1"`

if [ "X$OUT" != "X" ]; then
    echo "No output expected for Q!"
    go_out 100
fi

echo "There should be no prepared transactions"

xadmin recoverlocal

OUT=`xadmin recoverlocal`

if [ "X$OUT" != "X" ]; then
    echo "No output expected for transactions!"
    go_out 101
fi

################################################################################
# 2. We enqueue message to auto Q, the service for Q fails with TPFAIL.
# the service must test that transaction was started.
# after certain time the shell shall wait and see that there is no message in
# queue. Also we shall ensure that there is no prepared transactions. Also
# ensure that record count in db is 0.
################################################################################


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

