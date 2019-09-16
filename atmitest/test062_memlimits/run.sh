#!/bin/bash
##
## @brief Test memory limit restarts - test launcher
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

export TESTNAME="test062_memlimits"

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
    xadmin stop -y
    xadmin down -y



    # If some alive stuff left...
    xadmin killall atmiclt62

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
xadmin ppm

echo "*************************************************************************"
echo "* Test RSS -> get PID of atmi.sv62"
echo "*************************************************************************"
SPID=`xadmin ps -p -a atmi.sv62`


echo "Current memory config: "
xadmin ps -m -p -a atmi.sv62 2>/dev/null

echo "Current server PID = $SPID"
./atmiclt62  srvrss> ./atmiclt-dom1.log 2>&1
RET=$?
if [[ "X$RET" != "X0" ]]; then
    echo "RSS Client failed"
    go_out 1
fi

echo "Wait for server respawn... wait 10 sec..."
for((i=1;i<=10;i+=1)); do 
    xadmin ps -m -p -a atmi.sv62 2>/dev/null
    sleep 1
done

NPID=`xadmin ps -p -a atmi.sv62`

if [[ "X$SPID" == "X$NPID" ]]; then

    echo "RSS limit restarts does not work...!"
    go_out 1
fi

echo "*************************************************************************"
echo "* Test VSZ -> get PID of atmi.sv62 (wait 5 for service start...)"
echo "*************************************************************************"

sleep 5
UNAME=`uname -s`

if [ "X$UNAME" == "XAIX" ]; then
    # the value des not incremenet
    echo "VSZ tests not available for AIX"
else
    SPID=`xadmin ps -p -a atmi.sv62`

    echo "Current memory config: "
    xadmin ps -m -p -a atmi.sv62 2>/dev/null

    echo "Current server PID = $SPID"
    ./atmiclt62  srvvsz > ./atmiclt-dom1.log 2>&1
    RET=$?
    if [[ "X$RET" != "X0" ]]; then
        echo "VSZ Client failed"
        go_out 2
    fi

    echo "Wait for server respawn... wait 10 sec..."
    for((i=1;i<=10;i+=1)); do 
        xadmin ps -m -p -a atmi.sv62 2>/dev/null
        sleep 1
    done

    NPID=`xadmin ps -p -a atmi.sv62`

    if [[ "X$SPID" == "X$NPID" ]]; then
        echo "VSZ limit restarts does not work...!"
        go_out 3
    fi
fi

echo "*************************************************************************"
echo "* Test RSS -> client run of atmiclt62"
echo "*************************************************************************"

xadmin bc -t ATMICLT62 -s RSS

echo "Let client to boot"
sleep 10

echo "**** XADMIN PC *****"
xadmin pc
echo "**** XADMIN PC, END *****"

echo "Grab the PID..."
SPID=`xadmin ps -p -a cltrss -b atmiclt62`
echo "Wait 30 sec... to see the results (client is sleeping for us to take pid and then cpm... to restart)"

for((i=1;i<=30;i+=1)); do 
    xadmin ps -m -p -a cltrss -b atmiclt62 2>/dev/null
    sleep 1
done

NPID=`xadmin ps -p -a cltrss -b atmiclt62`

if [[ "X$SPID" == "X$NPID" ]]; then

    echo "RSS limit restarts does not work for cpmsrv...!"
    go_out 4
fi


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

echo "*************************************************************************"
echo "* Test VSZ -> client run of atmiclt62"
echo "*************************************************************************"

if [ "X$UNAME" == "XAIX" ]; then
	echo "VSZ tests not available for AIX"
else

    xadmin sc -t ATMICLT62 -s RSS
    xadmin bc -t ATMICLT62 -s VSZ

    echo "Let client to boot"
    sleep 10
    echo "**** XADMIN PC *****"
    xadmin pc
    echo "**** XADMIN PC, END *****"

    echo "Grab the PID..."
    SPID=`xadmin ps -p -a cltvsz -b atmiclt62`
    echo "Wait 30 sec... to see the results (client is sleeping for us to take pid and then cpm... to restart)"

    for((i=1;i<=30;i+=1)); do 
        xadmin ps -m -p -a cltvsz -b atmiclt62 2>/dev/null
        sleep 1
    done

    NPID=`xadmin ps -p -a cltvsz -b atmiclt62`

    if [[ "X$SPID" == "X$NPID" ]]; then

        echo "VSZ limit restarts does not work for cpmsrv...!"
        go_out 5
    fi

    # Catch is there is test error!!!
    if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
    fi
fi

go_out 0


# vim: set ts=4 sw=4 et smartindent:
