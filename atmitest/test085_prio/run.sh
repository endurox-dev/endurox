#!/bin/bash
##
## @brief Test tpsprio, tpgprio - test launcher
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

export TESTNAME="test085_prio"

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
    xadmin killall atmiclt85

    popd 2>/dev/null
    exit $1
}

rm *.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;
(./atmiclt85 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt85 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


# if poller is epoll very that priority actually works

POLLER=`xadmin poller`

if [[ "$POLLER" == "epoll" ]]; then

    export NDRX_BENCH_FILE="bench.50.log"
    export NDRX_BENCH_CONFIGNAME="results"
    exbenchcl -n5 -P  -B "UBF" -t20 -b "{\"T_LONG_FLD\":5}" -f T_CARRAY_FLD -S1024 -p 50 &
    BENCHPID50_PID=$!

    # event this is started later, it shall fill take a priority over...
    export NDRX_BENCH_FILE="bench.70.log"
    export NDRX_BENCH_CONFIGNAME="results"
    exbenchcl -n5 -P  -B "UBF" -t20 -b "{\"T_LONG_FLD\":5}" -f T_CARRAY_FLD -S1024 -p 70 &
    BENCHPID70_PID=$!

    # wait for pids...
    wait $BENCHPID50_PID
    wait $BENCHPID70_PID

    RES50=`cat bench.50.log | grep results | awk '{print $3}'`
    RES70=`cat bench.70.log | grep results | awk '{print $3}'`

    echo "[$RES50] vs [$RES70]"
    if [[ "$RES50" -ge "$RES70" ]]; then
        echo "Priority does not work prio 50 processed $RES50 prio 70 processed $RES70"
        go_out 1
    fi

fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi


go_out $RET


# vim: set ts=4 sw=4 et smartindent:

