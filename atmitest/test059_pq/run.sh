#!/bin/bash
##
## @brief Test print queue ops - test launcher. Also test unadv/readv xadmin
##  commands
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

export TESTNAME="test059_pq"

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
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    # avoid slow stop due to busy servers...
    xadmin killall atmi.sv59
    xadmin stop -y
    xadmin down -y



    # If some alive stuff left...
    xadmin killall atmiclt59

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin qrmall ,
xadmin qrmall some_hello_queue
xadmin start -y || go_out 1

# Have some wait for ndrxd goes in service - wait for connection establishment.
RET=0

echo "Configure servers..."

xadmin unadv -i 1892 -s TESTSV2
RET=$?
if [[ "X$RET" != "X0" ]]; then
    echo "Failed to undav 1892"
    go_out $RET
fi

xadmin unadv -i 1893 -s TESTSV1
RET=$?
if [[ "X$RET" != "X0" ]]; then
    echo "Failed to undav 1893"
    go_out $RET
fi

sleep 1

echo "Test that services are removed..."

xadmin psc | grep TESTSV2 | grep 1892
if [ "X`xadmin psc | grep TESTSV2 | grep 1892`" != "X" ]; then
    echo "TESTSV2/1892 must be missing"
    go_out -1
fi

xadmin psc | grep TESTSV1 | grep 1893
if [ "X`xadmin psc | grep TESTSV1 | grep 1893`" != "X" ]; then
    echo "TESTSV1/1893 must be missing"
    go_out -1
fi

echo "Ready to go - running off client"

xadmin psc
xadmin ppm

echo "TESTSV1 2"
(./atmiclt59 s TESTSV1 4 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "TESTSV2 3"
(./atmiclt59 s TESTSV2 5 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Arbitrary queue.."

(./atmiclt59 q "/some_hello_queue" 2 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Wait for some averges..."
sleep 5

echo "Printing services"
xadmin pq

echo "Printing domain queues"
xadmin pqa

echo "Printing all queues"
xadmin pqa -a
xadmin psc

echo "Testing busy services..."

if [ "X`xadmin psc | grep TESTSV1 | grep BUSY`" == "X" ]; then
    echo "TESTSV1 must be busy!"
    go_out -1
fi

if [ "X`xadmin psc | grep TESTSV2 | grep BUSY`" == "X" ]; then
    echo "TESTSV2 must be busy!"
    go_out -1
fi

echo "Testing service queues..."

if [ "X`xadmin pqa| egrep '^3'`" == "X" ]; then
    echo "There must be queue with 3x msgs enqueued...!"
    go_out -1
fi

if [ "X`xadmin pq| grep '3 3'`" == "X" ]; then
    echo "There must be stats with '3 3'!"
    go_out -1
fi

if [ "X`xadmin pqa| egrep '^4'`" == "X" ]; then
    echo "There must be queue with 4x msgs enqueued...!"
    go_out -1
fi

if [ "X`xadmin pq| grep '4 4'`" == "X" ]; then
    echo "There must be stats with '4 4'!"
    go_out -1
fi

echo "Test that custom queue is not seen in pqa but in pqa -a"

if [ "X`xadmin pqa| grep 'some_hello_queue'`" != "X" ]; then
    echo "[some_hello_queue] must not be seen in pqa!"
    go_out -1
fi

if [ "X`xadmin pqa -a| grep 'some_hello_queue'`" == "X" ]; then
    echo "[some_hello_queue] must be seen in pqa -a!"
    go_out -1
fi

echo "Test hello queue message counts..."

if [ "X`xadmin pqa -a| grep 'some_hello_queue' | egrep '^2'`" == "X" ]; then
    echo "[some_hello_queue] must have 2x messages enqueued!"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

