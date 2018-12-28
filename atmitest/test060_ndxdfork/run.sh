#!/bin/bash
##
## @brief Test ndrxd forking child lockups - test launcher
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

export TESTNAME="test060_ndxdfork"

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
    xadmin killall ndrxd

    popd 2>/dev/null
    exit $1
}

rm *dom*.log

ulimit -c unlimited

# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1


# Have some wait for ndrxd goes in service - wait for connection establishment.
echo "Sleeping 90..."
sleep 60
RET=0

echo "Terminate sanity checks of ndrxd...."

xadmin appconfig sanity 99999

echo "Sleep 20... let last sanity check to finish up..."
sleep 30

echo "Print the list of ndrxd's running ... "

xadmin ps -a ndrxd -b $NDRX_RNDK
DPID=`xadmin dpid`

PROCS=`xadmin ps -a ndrxd -b $NDRX_RNDK | wc | awk '{print $1}'`

echo "Got number of ndrxd's: $PROCS"

if [[ "X$PROCS" != "X1" ]]; then
    echo "Number of 'ndrxd' daemons is not 1, but $PROCS - FAIL"

    echo "Making core dumps of any extra ndrxds..."

    IFS=$'\n'       # make newlines the only separator
    CORES=`xadmin ps -a ndrxd -b $NDRX_RNDK -p -x $DPID`
    for c in $CORES; do
        echo "Generate core for PID=$c"
        kill -11 $c
        break
    done

    go_out -1
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:
