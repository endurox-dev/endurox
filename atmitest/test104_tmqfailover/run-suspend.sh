#!/bin/bash
##
## @brief Test is as run-local.sh, excep we test here situations what might
##  happen if in cluster of virtual machine some suspend / failover / resume
##  happens.
##
## @file run-suspend.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

export TESTNAME="test104_tmqfailover"

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
export NDRX_SGREFRESH=10

################################################################################
# 6 gives:
# lock expire if not refreshed in 6 seconds
# lock take over by other node if file unlocked: 12 sec
# exsinglesv periodic scans / locks 2 sec
export NDRX_SGREFRESH=30
################################################################################

if [ "$(uname)" == "Darwin" ]; then
    export NDRX_LIBEXT="dylib"
else
    export NDRX_LIBEXT="so"
fi

UNAME=`uname`

#
# Get the crash lib...
#
case $UNAME in

  Darwin)
    export NDRX_PLUGINS=libt86_lcf.dylib
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$TESTDIR/../test086_tmqlimit
    ;;

  AIX)
    export NDRX_PLUGINS=libt86_lcf.so
    export LIBPATH=$LIBPATH:$TESTDIR/../test086_tmqlimit
    ;;

  *)
    export NDRX_PLUGINS=libt86_lcf.so
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TESTDIR/../test086_tmqlimit
    ;;
esac

#
# Common configuration between the domains
#
export NDRX_CCONFIG1=$TESTDIR/app-common.ini

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1-sus.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_CCONFIG=$TESTDIR/app-dom1-sus.ini
    #export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

#
# Domain 2 - here server will live
#
set_dom2() {
    echo "Setting domain 2"
    . ../dom2.sh    
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2-sus.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export NDRX_CCONFIG=$TESTDIR/app-dom2-sus.ini
    #export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf
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
    xadmin killall atmiclt104

    popd 2>/dev/null
    exit $1
}

# clean up some old stuff
rm *.log 2>/dev/null
rm lock_* 2>/dev/null
rm ULOG* 2>/dev/null
rm -rf RM1 RM2 qdata 2>/dev/null

# where to store the data
mkdir RM1 RM2 qdata

# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

echo "Sleep 15 for link"
sleep 15

set_dom1;
xadmin psg
xadmin psc

################################################################################
echo ">>> Loop enqueue + crash"
################################################################################
NUM=400

counter=0
while [ $counter -lt $NUM ]
do
    echo "Loop [$counter]"

    # enq single msg...
    ./atmiclt104 enq $counter
    RET=$?

    if [[ "X$RET" != "X0" ]]; then
        echo "./atmiclt104 enq $counter failed"
        go_out $RET
    fi

    # plock loss simulation
    if [ "$(($counter % 40))" == "0" ]; then

        echo "Node freeze test...."
        # for active node, we will suspend tmsrv...
        # that shall cause failover...

        set_dom1;
        DOM_NUM=2

        #if [[ "X`xadmin ppm | grep 'wait  runok'`" != "X" ]]; then
        #    echo "domain 2 is active"
        #    set_dom2;
        #    xadmin stop -s exsingleckr
        #else
        #    echo "domain 1 is active"
        #    xadmin stop -s exsingleckr
        #    DOM_NUM=1
        #fi

        set_dom1;
        xadmin lcf lockloss -A5 -a
        xadmin lcf
        xadmin stop -s exsingleckr
        xadmin dsleep 15

        set_dom2;
        xadmin lcf lockloss -A5 -a
        xadmin lcf
        xadmin stop -s exsingleckr
        xadmin dsleep 15

        echo "Let to failover tmsrvs... (sleep 15)"
        sleep 15
        
        # restore original domain...
        #if [[ "$DOM_NUM" == "2" ]]; then
        #    echo "domain 2 is active"
        #    set_dom2;
        #    xadmin start -s exsingleckr
        #else
        #    echo "domain 1 is active"
        #    xadmin start -s exsingleckr
        #fi

        # let system to clear up
        #xadmin lcf lockloss -A0 -a
        #
        
        set_dom1;
        xadmin lcf lockloss -A0 -a
        xadmin lcf
        xadmin start -s exsingleckr

        set_dom2;
        xadmin lcf lockloss -A0 -a
        xadmin lcf
        xadmin start -s exsingleckr
        
        sleep 15
        xadmin psc
        xadmin ppm
        set_dom1;
        xadmin psc
        xadmin ppm

    fi

    ((counter++))

done

################################################################################
echo ">>> Validate $NUM messages"
################################################################################

# disable auto from Qs....
xadmin mqch -n2 -i 200 -qQ1,autoq=n
xadmin mqch -n2 -i 200 -qQ2,autoq=n
sleep 5
# lets Q to complete...
xadmin mqlc
xadmin mqlq

# validate that all messages are in place
# enq single msg...
./atmiclt104 deq $NUM
RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "./atmiclt104 deq $counter failed"
    go_out $RET
fi

################################################################################
echo ">>> Corrupted ping file -> all groups down"
################################################################################
# the 0 content would fail the CRC32 test thus all groups of all nodes shall go down...
dd if=/dev/zero of=$TESTDIR/lock_GRP2_2 bs=1 count=2048
# let the exsinglesv to detect the situation

set_dom1;
xadmin stop -s exsingleckr
set_dom2;
xadmin stop -s exsingleckr
sleep 5

set_dom1;
# avoid exsinglesv feed from services:
xadmin ppm
CNT=`xadmin ppm | grep tmqueue | grep 'runok runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "0" ]; then
    echo "Expected tmqueue down (0) on dom1, but got $CNT"
    go_out -1
fi

set_dom2;
xadmin ppm
CNT=`xadmin ppm | grep tmqueue | grep 'runok runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "0" ]; then
    echo "Expected tmqueue down (0) on dom2, but got $CNT"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

