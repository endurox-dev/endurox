#!/bin/bash
## 
## @brief @(#) Perform cache bechmark
##
## @file benchmark.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
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

export NDRX_CCONFIG=`pwd`
. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
export NDRX_TOUT=10
export NDRX_ULOG=$TESTDIR
export TESTDIR_DB=$TESTDIR
export TESTDIR_SHM=$TESTDIR
#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_CCTAG=dom1
}

source ./test-func-include.sh

if [ -d "/dev/shm" ]; then

    echo "Preparing ramdrive..."

    mkdir -rf /dev/shm/benchmark 2>/dev/null
    mkdir /dev/shm/benchmark

    mkdir -rf /dev/shm/db10 2>/dev/null
    mkdir /dev/shm/db10

    export TESTDIR_SHM="/dev/shm"
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
    xadmin killall atmiclt48

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm
echo "Running off benchmark..."

set_dom1;
(time ./atmiclt48_bench 2>&1) > ./benchmark.log

RET=$?
if [ $RET -ne 0 ]; then
    echo "benchmark failed"
    go_out 1
fi

echo "Show cache... "

xadmin cs -d benchmark

TMP=$?

echo "xadmin ret $TMP"

if [ $TMP -ne 0 ]; then
    echo "xadmin failed"
    go_out 1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out 5
fi


go_out $RET

