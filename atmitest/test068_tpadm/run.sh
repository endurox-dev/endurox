#!/bin/bash
##
## @brief tpadm .TMIB interface tests - test launcher
##
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

export TESTNAME="test068_tpadm"

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

export NDRX_SILENT=Y
export NDRX_TOUT=60
export NDRX_LDBAL=0
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
# Domain 2 - here server will live
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
    xadmin killall atmiclt68

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1


set_dom2;
xadmin down -y
xadmin start -y || go_out 2


# Have some wait for ndrxd goes in service - wait for connection establishment.
echo "Sleep 20 for domain to establish..."
sleep 20
RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom2;

# Run some calls
(./atmiclt68 call 2>&1) > ./atmiclt-dom2-call.log

# Make some failure
(./atmiclt68 fail 2>&1) >> ./atmiclt-dom2-fail.log

# open the conn
(./atmiclt68 conv 2>&1) >> ./atmiclt-dom2-conv.log &

# open the conn / wait in Q
(./atmiclt68 conv 2>&1) >> ./atmiclt-dom2-conv.log &


# stop some client

xadmin sc BINARY2/2

echo "Let queues to establish ...  (wait 3)"
sleep 3

#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt68 2>&1) > ./atmiclt-dom1.log


echo "*** T_CLIENT ***"
xadmin mibget -c T_CLIENT
xadmin mibget -c T_CLIENT -m

HAVE_BIN3_3=`xadmin mibget -c T_CLIENT -m | egrep '2\|2/BINARY3/3\|./test.sh\|ACT\|[1-9][0-9]*\|-1\|-1\|[1-9][0-9]*\|'`
if [ "X$HAVE_BIN3_3" == "X" ]; then
    echo "2/BINARY3/3 not found!"
    go_out -10
fi

ATMICLT68=`xadmin mibget -c T_CLIENT -m | egrep '2\|\/dom2,clt,reply,atmiclt68,[^\|]*\|atmiclt68\|ACT\|[1-9][0-9]*\|1\|1\|0\|'`
if [ "X$ATMICLT68" == "X" ]; then
    echo "ATMICLT68 not found!"
    go_out -11
fi

echo "*** T_DOMAIN ***"
xadmin mibget -c T_DOMAIN
xadmin mibget -c T_DOMAIN -m

POLLER=`xadmin poller`
DOMAINOUT=`xadmin mibget -c T_DOMAIN -m | egrep '2\|ACT\|[1-9][0-9]*\|[1-9][0-9]*\|[1-9][0-9]*\|'`

if [ "X$DOMAINOUT" == "X" ]; then
    echo "DOMAIN not found!"
    go_out -12
fi

echo "*** T_MACHINE ***"
xadmin mibget -c T_MACHINE
xadmin mibget -c T_MACHINE -m

NOD1=`xadmin mibget -c T_MACHINE -m | egrep '1\|-1\|-1\|-1\|ACT\|'`

if [ "X$NOD1" == "X" ]; then
    echo "NOD1 not found!"
    go_out -13
fi


NOD2=`xadmin mibget -c T_MACHINE -m | egrep '2\|[1-9][0-9]*\|[1-9][0-9]*\|2\|ACT\|'`

if [ "X$NOD2" == "X" ]; then
    echo "NOD2 not found!"
    go_out -14
fi

NOD3=`xadmin mibget -c T_MACHINE -m | egrep '3\|-1\|-1\|-1\|PEN\|'`

if [ "X$NOD3" == "X" ]; then
    echo "NOD3 not found!"
    go_out -15
fi

echo "*** T_QUEUE ***"
xadmin mibget -c T_QUEUE
xadmin mibget -c T_QUEUE -m

# there must be 1 msg enqueued...
MSGENQ=`xadmin mibget -c T_QUEUE -m | egrep 'ACT\|1\|'`

if [ "X$MSGENQ" == "X" ]; then
    echo "MSGENQ not found!"
    go_out -16
fi

# there must be client reply q
CLTREP=`xadmin mibget -c T_QUEUE -m | egrep '2\|.*,reply,atmiclt68,.*\|'`

if [ "X$CLTREP" == "X" ]; then
    echo "CLTREP not found!"
    go_out -17
fi

echo "*** T_SERVER ***"
xadmin mibget -c T_SERVER
xadmin mibget -c T_SERVER -m

SV68=`xadmin mibget -c T_SERVER -m | egrep '2\|10\|.*\|ACT\|[[0-9]*\|[1-9][0-9]*\|atmi.sv68\|atmi.sv68\|1\|'`

if [ "X$SV68" == "X" ]; then
    echo "SV68 not found!"
    go_out -18
fi


CPM=`xadmin mibget -c T_SERVER -m | egrep '2\|9999\|.*\|ACT\|[0-9]*\\|[1-9][0-9]*\\|cpmsrv\|cpmsrv\|1\|'`

if [ "X$CPM" == "X" ]; then
    echo "CPM not found!"
    go_out -19
fi


DUMSV=`xadmin mibget -c T_SERVER -m | egrep '2\|30\|.*\|.*\|[0-9]*\|[0-9]*\|dummysv\|dumcmdsv\|3\|'`

if [ "X$DUMSV" == "X" ]; then
    echo "DUMSV not found!"
    go_out -20
fi

echo "*** T_SERVICE ***"
xadmin mibget -c T_SERVICE
xadmin mibget -c T_SERVICE -m


# Test services:

FAILSVC=`xadmin mibget -c T_SERVICE -m | egrep 'FAILSV\|ACT\|'`

if [ "X$FAILSVC" == "X" ]; then
    echo "FAILSVC not found!"
    go_out -21
fi

MIBSVC=`xadmin mibget -c T_SERVICE -m | egrep '\.TMIB\|ACT\|'`

if [ "X$MIBSVC" == "X" ]; then
    echo "MIBSVC not found!"
    go_out -22
fi

MIBSVC2=`xadmin mibget -c T_SERVICE -m | egrep '\.TMIB-2-401\|ACT\|'`

if [ "X$MIBSVC2" == "X" ]; then
    echo "MIBSVC2 not found!"
    go_out -23
fi

echo "*** T_SVCGRP ***"
xadmin mibget -c T_SVCGRP
xadmin mibget -c T_SVCGRP -m

TESTSVCG=`xadmin mibget -c T_SVCGRP -m | egrep 'TESTSV\|2/10\|ACT\|2\|10\|TESTSV\|100\|100\|0\|[0-9]+\|[0-9]+\|[0-9]+\|'`

if [ "X$TESTSVCG" == "X" ]; then
    echo "TESTSVCG not found!"
    go_out -24
fi

FAILSVG=`xadmin mibget -c T_SVCGRP -m | egrep 'FAILSV\|2/10\|ACT\|2\|10\|FAILURESV\|1\|0\|1\|[0-9]+\|[0-9]+\|[0-9]+\|'`

if [ "X$FAILSVG" == "X" ]; then
    echo "FAILSVG not found!"
    go_out -25
fi

TESTSVC1G=`xadmin mibget -c T_SVCGRP -m | egrep 'TESTSV\|1/101\|ACT\|1\|101\|N/A\|-1\|-1\|-1\|-1\|-1\|-1\|'`

if [ "X$TESTSVC1G" == "X" ]; then
    echo "TESTSVC1G not found!"
    go_out -26
fi

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi


go_out $RET


# vim: set ts=4 sw=4 et smartindent:

