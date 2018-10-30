#!/bin/bash
##
## @brief @(#) Test ndrxd ping procedures - test launcher (Bug #255)
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

export TESTNAME="test044_ping"

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

export NDRX_TOUT=2

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
}


function get_pid {

    local retpid=""
    if [ "$(uname)" == "FreeBSD" ]; then
            retpid=`ps -auwwx | grep "\-i 3519" | grep -v grep | awk '{print $2}'`;
    else
            retpid=`ps -ef | grep "\-i 3519" | grep -v grep | awk '{print $2}'`;
    fi

    echo $retpid
}

#
# Generic exit function
#
go_out() {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt44

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

PROCPID=$(get_pid)   # or result=`myfunc`

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 30

# The case Bug #255 did restart server at particular configuration
# when pings are performed at less time points than sanity checks
# 
PROCPID2=$(get_pid)   # or result=`myfunc`

echo "PID [$PROCPID] vs [$PROCPID2]"

if [ "X$PROCPID" != "X$PROCPID2" ]; then

        echo "PID Have changed [$PROCPID] vs [$PROCPID2]"
        go_out -1
fi

echo "Stopping..."
cat << EOF | ud
SRVCNM	TESTSV

EOF

sleep 60

PROCPID2=$(get_pid)   # or result=`myfunc`

echo "PID [$PROCPID] vs [$PROCPID2]"

if [ "X$PROCPID" == "X$PROCPID2" ]; then

        echo "PID must be changed [$PROCPID] vs [$PROCPID2]!!"
        go_out -2
fi


RET=0

xadmin psc
xadmin ppm
echo "Performing test..."

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        go_out -3
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
