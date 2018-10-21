#!/bin/bash
##
## @brief @(#) Test037 - Test "xadmin down" - wipe out all Enduro/X runtime resources.
##   NOTE: We also test removal of client processes childs for cpm and standalone..
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

export TESTNO="037"
export TESTNAME_SHORT="down"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"

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
# We do not need timeout, we will kill procs...
export NDRX_TOUT=9999

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

#
# Domain 2 - here server will live
#
function set_dom2 {
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
    xadmin killall atmiclt37

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge
#xadmin killall sleep
xadmin killall nap.sh
xadmin killall nap2.sh
xadmin killall atmiclt
xadmin killall ndrxd
xadmin killall atmisrv

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Clean up the logs
rm xadmin-*.log

PSCMD="ps -ef"
if [ "$(uname)" == "FreeBSD" ]; then
    PSCMD="ps -auwwx"
fi

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 10

RET=0

set_dom1;

nohup ./atmiclt37 &
sleep 1

echo "DOM 1 atmiclts..."
$PSCMD | grep atmiclt37 | grep -v grep

set_dom2;

nohup ./atmiclt37 &
sleep 1

echo "After DOM 2 atmiclts..."
$PSCMD | grep atmiclt37 | grep -v grep


set_dom1;

echo "nap2 after boot"
$PSCMD | grep nap2.sh

$PSCMD | grep atmiclt37

################################################################################
# Test resources before kill
################################################################################
echo "Situation before downing..."
$PSCMD


echo "==== Details ===="

echo "cpms"
$PSCMD | grep cpmsrv

echo "ndrxd"
$PSCMD | grep ndrxd

echo "nap.sh"
$PSCMD | grep nap.sh

echo "nap2.sh"
$PSCMD | grep nap2.sh

echo "atmiclt37"
$PSCMD | grep atmiclt37

echo "sleep"
$PSCMD | grep sleep

echo "==== Details, end ===="


echo "dom1 client"

xadmin pc

#
# We should have 4x sleepy procs 2x for each dom
#
CNT=`$PSCMD | grep nap.sh | grep -v grep | wc | awk '{print $1}'`

PROC_COUNT="4"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT nap.sh not booted (according to $PSCMD )!"
    go_out 10
fi

$PSCMD | grep atmiclt37

CNT=`$PSCMD | grep atmiclt37 | grep -v grep | wc | awk '{print $1}'`

PROC_COUNT="8"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT atmiclt37 not booted (according to $PSCMD )!"
    go_out 11
fi

CNT=`$PSCMD | grep nap2.sh | grep -v grep | wc | awk '{print $1}'`

PROC_COUNT="4"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT nap2.sh not booted (according to $PSCMD )!"
    go_out 11
fi

################################################################################
# Down the dom1
################################################################################

echo "About to down dom1"
xadmin down -y

echo "After dom1 down"
$PSCMD
#
# The dom2 must not be killed, thus count their procs
#
CNT=`$PSCMD | grep nap.sh | grep -v grep | wc | awk '{print $1}'`
PROC_COUNT="2"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT nap.sh not left (according to $PSCMD )!"
    go_out 12
fi

#
# Children of the cpm clients must be killed too
#
CNT=`$PSCMD | grep nap2.sh | grep -v grep | wc | awk '{print $1}'`
PROC_COUNT="2"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT nap2.sh not left (according to $PSCMD )!"
    go_out 12
fi

#
# These how twice forked child... thus should have left 4
#
CNT=`$PSCMD | grep atmiclt37 | grep -v grep | wc | awk '{print $1}'`
PROC_COUNT="4"
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
    echo "TESTERROR! $PROC_COUNT atmiclt37 not left (according to $PSCMD )!"
    go_out 13
fi
    
#
# Test the other resources of dom1 & dom2
# Dom 1 queues must not exist
#
xadmin pqa
CNT=`xadmin pqa | wc | awk '{print $1}'`
echo "DOM1 Queues: $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Dom1 all queues must be killed except xadmin's!"
    go_out 14
fi

CNT=`$PSCMD | grep ndrxd | grep -v grep | wc | awk '{print $1}'`

echo "NDRXD daemons: $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Only one copy of ndrxd must be left!"
    go_out 15
fi

CNT=`$PSCMD | grep atmisv37 | grep -v grep |wc | awk '{print $1}'`
echo "atmisv37: $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Only one copy of atmisv37 must be left!"
    go_out 16
fi

CNT=`$PSCMD | grep atmisv_2_37 | grep -v grep |wc | awk '{print $1}'`
echo "atmisv_2_37 $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Only one copy of atmisv_2_37 must be left!"
    go_out 17
fi

CNT=`$PSCMD | grep tprecover | grep -v grep | wc | awk '{print $1}'`
echo "tprecover: $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Only one copy of tprecover must be left!"
    go_out 18
fi

#
# Test for shared memory to be removed
# looks like we can test this only on linux
#
if [ "$(uname)" == "Linux" ]; then


    echo "Shared mem objects: "

    ls -1 /dev/shm${NDRX_QPREFIX},*
    ls -1 /dev/shm${NDRX_QPREFIX},* | wc | awk '{print $1}'

    SHMS=`ls -1 /dev/shm${NDRX_QPREFIX},* | wc | awk '{print $1}'`

    echo "DOM1 Shared memories: $SHMS"

    if [[ `xadmin poller` == "SystemV" ]]; then

        if [[ "$SHMS" -ne "2" ]]; then 
            echo "TESTERROR! There must be 2 shared memory objs for dom1 after kill!"
            go_out 18
        fi
    else

        if [[ "$SHMS" -ne "0" ]]; then 
            echo "TESTERROR! There must be no shared memory for dom1 after kill!"
            go_out 18
        fi
    fi
fi

#
# Test for system v semaphore to be removed...
#
ipcs | grep $NDRX_IPCKEY
CNT=`ipcs | grep $NDRX_IPCKEY | wc | awk '{print $1}'`
echo "DOM1 Semaphores (guessed): $CNT"
if [[ "$CNT" -ne "0" ]]; then 
    echo "TESTERROR! The semaphore with key [$IPCKEY] must be removed!!"
    go_out 19
fi

################################################################################
# Down the dom2
################################################################################

set_dom2;
xadmin down -y
echo "After dom2 down..."
$PSCMD

xadmin pqa -a
CNT=`xadmin pqa -a | wc | awk '{print $1}'`
echo "DOM1 Queues: $CNT"
if [[ "$CNT" -ne "1" ]]; then 
    echo "TESTERROR! Dom1 & 2 all queues must be removed except 1 for xadmin!"
    go_out 30
fi

CNT=`$PSCMD | grep atmisv37 | grep -v grep | wc | awk '{print $1}'`
echo "Finally atmisv37: $CNT"
if [[ "$CNT" -ne "0" ]]; then 
    echo "TESTERROR! All atmisv37 must be killed!"
    go_out 31
fi

CNT=`$PSCMD | grep atmisv_2_37 | grep -v grep | wc | awk '{print $1}'`
echo "Finally atmisv_2_37 $CNT"
if [[ "$CNT" -ne "0" ]]; then 
    echo "TESTERROR! All atmisv_2_37 must be killed!"
    go_out 32
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
