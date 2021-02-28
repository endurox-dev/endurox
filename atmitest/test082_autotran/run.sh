#!/bin/bash
##
## @brief Test auto-transaction functionality - test launcher
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
## See LICENSE file for full text.
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

export TESTNAME="test082_autotran"

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
export NDRX_ULOG=$TESTDIR
export NDRX_TOUT=10
export NDRX_SILENT=Y

export NDRX_LIBEXT="so"
if [ "$(uname)" == "Darwin" ]; then
    export NDRX_XA_RMLIB=libndrxxaqdisk.dylib
    export NDRX_LIBEXT="dylib"
fi

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
}

#
# Domain 2 - here client will live
#
set_dom2() {
    echo "Setting domain 2"
    . ../dom2.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    set_dom2;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt82

    popd 2>/dev/null
    exit $1
}


rm -rf ${TESTDIR}/RM1 2>/dev/null
mkdir ${TESTDIR}/RM1

rm -rf ${TESTDIR}/RM2 2>/dev/null
mkdir ${TESTDIR}/RM2

rm -rf ${TESTDIR}/QSPACE1 2>/dev/null
mkdir ${TESTDIR}/QSPACE1

rm *.log
rm ULOG*


for TEST_MODE in 2 1
do

# Clean up the stuff
xadmin killall tpbridge
set_dom2;
xadmin down -y

xadmin killall tpbridge
set_dom1;
xadmin down -y
xadmin start -y || go_out 1


if [ $TEST_MODE -eq 2 ]; then

    echo "Testing domain mode..."
    set_dom2;
    xadmin start -y
    echo "Wait for connection..."
    sleep 30
    set_dom1;

    # do not use server 2 locally.., only remote
    xadmin stop -s atmi.sv82_2

fi

xadmin psc

RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;


################################################################################
echo "*** Running OK case..."

./atmiclt82 OK

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "./atmiclt82 OK failed"
    go_out $RET
fi

echo "Checking OK case..."
# count the results
CNT=`./atmiclt82 COUNT | grep OK | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X2" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

################################################################################
echo "*** Running OK2 case..."

./atmiclt82 OK2

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "./atmiclt82 OK2 failed"
    go_out $RET
fi

echo "Checking OK2 case..."
# count the results
CNT=`./atmiclt82 COUNT | grep OK | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X2" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

################################################################################
echo "*** Checking tmsrv down..."

xadmin stop -s tmsrv

OUTERR=`./atmiclt82 OK3`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 OK3 must fail, but did not"
    go_out $RET
fi
RET=0

if [[ $OUTERR != *"TPETRAN"* ]]; then
    echo "TPETRAN shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "Checking OK3 case..."
# count the results
CNT=`./atmiclt82 COUNT | grep OK | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

xadmin start -s tmsrv

################################################################################
echo "*** Checking commit fails transation timeout..."

OUTERR=`./atmiclt82 SLEEP`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 OK4 must fail, but did not"
    go_out $RET
fi
RET=0

if [[ $OUTERR != *"TPESVCERR"* ]]; then
    echo "TPESVCERR shall be provided as cannot commit tran, but was [$OUTERR]"
    go_out -3
fi

echo "Checking SLEEP case..."
# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

xadmin start -s tmsrv

################################################################################
echo "*** Normal fail test"

OUTERR=`./atmiclt82 FAIL`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 FAIL must fail, but did not"
    go_out $RET
fi
RET=0

if [[ $OUTERR != *"TPESVCFAIL"* ]]; then
    echo "TPESVCFAIL shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "Checking FAIL case..."
# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT, expected 0"
    go_out -1
fi

################################################################################
echo "*** No transaction used.. get fail msgs stand alone commit in tmqueue"

export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1-notran.xml
xadmin stop -y
xadmin start -y


if [ $TEST_MODE -eq 2 ]; then
    # do not use server 2 locally.., only remote
    xadmin stop -s atmi.sv82_2
    echo "Testing domain mode... wait for connection..."

    set_dom2;

    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2-notran.xml
    xadmin stop -y
    xadmin start -y
    
    set_dom1;
    
    sleep 30
fi

xadmin psc

echo "*** Normal fail test"

OUTERR=`./atmiclt82 FAIL`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 FAIL must fail, but did not"
    go_out $RET
fi
RET=0

if [[ $OUTERR != *"TPESVCFAIL"* ]]; then
    echo "TPESVCFAIL shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "Checking FAIL case..."
# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X2" ]]; then
    echo "Got invalid count: $CNT, expected 2"
    go_out -1
fi

################################################################################
echo "*** CONV OK"

export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
xadmin stop -y
xadmin start -y

if [ $TEST_MODE -eq 2 ]; then
    # do not use server 2 locally.., only remote
    xadmin stop -s atmi.sv82_2
    echo "Testing domain mode... wait for connection..."

    set_dom2;
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    xadmin stop -y
    xadmin start -y

    set_dom1;

    sleep 30
fi

xadmin psc

./atmiclt82 OK C

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "./atmiclt82 OK C failed"
    go_out $RET
fi

echo "Checking OK case..."
# count the results
CNT=`./atmiclt82 COUNT | grep OK | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X1" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

################################################################################
echo "*** CONV start fail"

xadmin stop -s tmsrv

if [ $TEST_MODE -eq 2 ]; then
    set_dom2;
    xadmin stop -s tmsrv
    set_dom1;
fi

OUTERR=`./atmiclt82 OK C`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 OK2 C shall fail, but didn't"
    go_out -1
fi

RET=0

echo "Output: [$OUTERR]"

if [[ $OUTERR != *"TPETRAN"* ]]; then
    echo "TPETRAN shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "CONV start fail check count 0"
# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

# print all queues after the failed to connect...
echo "QUEUES:"
xadmin pqa
echo "QUEUES, END"

if [ $TEST_MODE -eq 2 ]; then
    set_dom2;
    xadmin start -s tmsrv
    set_dom1;
fi

xadmin start -s tmsrv

################################################################################
if [ $TEST_MODE -eq 2 ]; then
    echo "*** CONV start fail (TMSRV1 out of order TMSRV2 OK)"

    xadmin stop -s tmsrv

    OUTERR=`./atmiclt82 OK C`

    RET=$?

    if [[ "X$RET" == "X0" ]]; then
        echo "./atmiclt82 OK2 C shall fail, but didn't"
        go_out -1
    fi

    RET=0

    echo "Output: [$OUTERR]"

    if [[ $OUTERR != *"TPEV_SVCERR"* ]]; then
        echo "TPEV_SVCERR shall be provided as cannot start tran, but was [$OUTERR]"
        go_out -3
    fi

    echo "CONV start fail check count 0"
    # count the results
    CNT=`./atmiclt82 COUNT | wc -l`

    RET=$?

    if [[ "X$RET" != "X0" ]]; then
        echo "Failed to run COUNT"
        go_out $RET
    fi

    if [[ "X$CNT" != "X0" ]]; then
        echo "Got invalid count: $CNT"
        go_out -1
    fi

    xadmin start -s tmsrv

    if [ $TEST_MODE -eq 2 ]; then
        set_dom2;
        xadmin start -s tmsrv
        set_dom1;
    fi

fi
################################################################################

################################################################################
# common stats
################################################################################
# print all queues after the failed to connect...
echo "QUEUES:"
xadmin pqa
echo "QUEUES, END"

################################################################################
echo "*** CONV fails to commit.. (timeout)"

OUTERR=`./atmiclt82 SLEEP C`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 SLEEP C shall fail, but didn't"
    go_out -1
fi

RET=0

echo "Output: [$OUTERR]"

if [[ $OUTERR != *"TPEV_SVCERR"* ]]; then
    echo "TPEV_SVCERR shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "CONV start fail check count 0"

# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

################################################################################
echo "*** CONV fails to commit.. (timeout) using tpsend() as event generator"

OUTERR=`./atmiclt82 SLEEP S`

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "./atmiclt82 SLEEP C S shall fail, but didn't"
    go_out -1
fi

RET=0

echo "Output: [$OUTERR]"

if [[ $OUTERR != *"TPEV_SVCERR"* ]]; then
    echo "TPEV_SVCERR shall be provided as cannot start tran, but was [$OUTERR]"
    go_out -3
fi

echo "CONV start fail check count 0"

# count the results
CNT=`./atmiclt82 COUNT | wc -l`

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run COUNT"
    go_out $RET
fi

if [[ "X$CNT" != "X0" ]]; then
    echo "Got invalid count: $CNT"
    go_out -1
fi

# master loop 2x configs local and remote...
done

################################################################################
# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi


go_out $RET


# vim: set ts=4 sw=4 et smartindent:

