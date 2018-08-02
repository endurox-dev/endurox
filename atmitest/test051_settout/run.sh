#!/bin/bash
## 
## @brief @(#) Test of tptoutset()/get() - test launcher (Bug #300)
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 3 of the License, or (at your option) any later
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

export TESTNAME="test051_settout"

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
    xadmin killall atmiclt51

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1
xadmin psc

set_dom2;
xadmin down -y
xadmin start -y || go_out 2
xadmin psc

echo "Let bridge to establish"
sleep 30

set_dom1;

RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;
(./atmiclt51 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt51 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        go_out -2
fi

echo "sleep 6"
sleep 6
echo "Check logs for expiry"

EXP=`grep "Received call already expired" atmisv-dom2.log`

if [[ "X$EXP" == "X" ]]; then
        echo "Missing [Received call already expired] in atmisv-dom2.log!!!"
        go_out -2
fi


set_dom2;
echo "Let xadmin to expire the call for service shutdown..."

export NDRX_XADMINTOUT=1
OUT=`xadmin stop -s atmi.sv51 2>&1`
RET=$?

echo "output: [$OUT]"

if [[ "X$RET" == "X0" ]]; then
    echo "xadmin must fail, but was ok (1)!"
    go_out -3
fi


if [[ ! "$OUT" =~ "timeout" ]]; then

    echo "expected output to contain [timeout] but got [$OUT]"
    go_out -4
fi


echo "Testing xadmin's timeout overrid -> set invalid value, xadmin must fail"

export NDRX_XADMINTOUT=0

xadmin echo OK

RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "xadmin must fail, but was ok (2)!"
    go_out -3
fi

unset NDRX_XADMINTOUT


echo "sleep 5 - attach to console session"
xadmin cat
#sleep 5

echo "Testing double shutdown... (shall not hang)..."
xadmin stop -y
xadmin stop -y


go_out 0

