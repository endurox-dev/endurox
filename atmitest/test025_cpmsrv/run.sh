#!/bin/bash
## 
## @(#) Test025, Client Process Monitor tests
## Currently will test for
## - Auto startup
## - Manual startup
## - Automatic restart
## - Shutdown
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 525 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

export TESTNO="025"
export TESTNAME_SHORT="cpmsrv"
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
# Configure the runtime - override stuff here!
export NDRX_CONFIG=$TESTDIR/ndrxconfig.xml
export NDRX_DMNLOG=$TESTDIR/ndrxd.log
export NDRX_LOG=$TESTDIR/ndrx.log
export NDRX_DEBUG_CONF=$TESTDIR/debug.conf
# Override timeout!
export NDRX_TOUT=13
# Test process count
PROC_COUNT=100
#
# Having some bash forks...
#
PROC_COUNT_DIFFALLOW=103

PSCMD="ps -ef"
if [ "$(uname)" == "FreeBSD" ]; then
	PSCMD="ps -auwwx"
fi

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin stop -y
    xadmin down -y
    xadmin killall chld1.sh chld2.sh chld3.sh chld4.sh chld5.sh chld6.sh

    popd 2>/dev/null
    exit $1
}

#
# Test process count with given name
#
function test_proc_cnt {
    proc=$1
    cnt=$2
    go=$3

    echo ">>> Testing $proc to be $cnt of count. If fail, error $go"
    $PSCMD | grep $proc | grep -v grep
    CNT=`$PSCMD | grep $proc | grep -v grep | wc | awk '{print $1}'`
    XPROC_COUNT=$cnt
    echo ">>> $PSCMD procs: $CNT"
    if [[ "$CNT" -ne "$XPROC_COUNT" ]]; then 
        echo "TESTERROR! $XPROC_COUNT $proc not booted (according to $PSCMD )!"
        go_out $go
    fi

    echo "$proc ok cnt $cnt ok"

}  

rm *.log

#
# Kill the children test processes if any
#
xadmin killall chld1.sh chld2.sh chld3.sh chld4.sh chld5.sh chld6.sh ndrxbatchmode

xadmin down -y
xadmin start -y || go_out 1

################################################################################
echo "Run some tests of the batch mode"
################################################################################

test_proc_cnt "ndrxbatchmode.sh" "0" "29"

echo "Batch start"
xadmin bc -t BATCH% -s% -w 10000

xadmin pc

test_proc_cnt "ndrxbatchmode.sh" "3" "30"

echo "Batch stop (no subsect)"
xadmin sc -t BATCH%

test_proc_cnt "ndrxbatchmode.sh" "2" "31"

echo "with subsect"
xadmin sc -t BATCH% -sB%
test_proc_cnt "ndrxbatchmode.sh" "0" "32"

echo "Batch start"
xadmin bc -t BATCH% -s%
sleep 10

test_proc_cnt "ndrxbatchmode.sh" "3" "33"


echo "Testing batch reload"
OUT1=`xadmin pc`

echo "Before reload [$OUT1]"

xadmin rc -t BATCH% -s%
sleep 10

test_proc_cnt "ndrxbatchmode.sh" "3" "34"
OUT2=`xadmin pc`

echo "After reload [$OUT2]"

if [ "X$OUT1" == "X$OUT2" ]; then
    echo "TESTERROR! [$OUT1]==[$OUT2] -> FAIL, pid must be changed..."
    go_out 35
fi

xadmin bc -t TESTENV
xadmin bc -t TESTENV2

sleep 10

xadmin pc


################################################################################
# Child cleanup... testing
################################################################################

# At this point we should have all child processes

test_proc_cnt "chld1.sh" "1" "21"
test_proc_cnt "chld2.sh" "1" "22"
test_proc_cnt "chld3.sh" "1" "23"
test_proc_cnt "chld4.sh" "1" "24"
test_proc_cnt "chld5.sh" "1" "25"
test_proc_cnt "chld6.sh" "1" "26"

echo "Stopping CHLD 1,3,5 ..."

xadmin sc -t CHLD1
test_proc_cnt "chld1.sh" "0" "27"
test_proc_cnt "chld2.sh" "0" "28"

xadmin sc -t CHLD3
test_proc_cnt "chld3.sh" "0" "23"
test_proc_cnt "chld4.sh" "0" "24"

#
# here "chld3.log" should contain some messages about shutdown...
#
OUT=`grep "Doing exit of chld3" chld3.log`

if [[ "X$OUT" == "X" ]]; then
    echo "chld3.log does not contain the message!!!!"
    go_out 25
fi

xadmin sc -t CHLD5
test_proc_cnt "chld5.sh", "0", "26"
test_proc_cnt "chld6.sh", "1", "27"

################################################################################
# Child cleanup... testing, end
################################################################################

# Test for section/subsection passing
OUT=`grep '\-t TAG1 \-s SUBSECTION1' testbin1-1.log`
if [[ "X$OUT" == "X" ]]; then
        echo "TESTERROR! invalid testbin1-1.log content!"
        go_out 1
fi

OUT=`grep '\-t TAG2 \-s SUBSECTION2' testbin1-2.log`
if [[ "X$OUT" == "X" ]]; then
        echo "TESTERROR! invalid testbin1-2.log content!"
        go_out 2
fi

# Test for env override processing
OUT=`grep ___ENV_OVER__1  env1.log`
if [[ "X$OUT" == "X" ]]; then
        echo "TESTERROR! invalid env1.log content!"
        go_out 3
fi

OUT=`grep ___ENV_OVER__2  env2.log`
if [[ "X$OUT" == "X" ]]; then
        echo "TESTERROR! invalid env2.log content!"
        go_out 4
fi

# Test for automatic restarts...
CNT=`wc testbin1-1.log | awk '{print $1}'`
echo "Process restarts: $CNT"
if [[ "$CNT" -lt "2" ]]; then 
        echo "TESTERROR! Automatic process reboots does not work!"
        go_out 5
fi

# Start some tons of processes
#for i in `seq 1 $PROC_COUNT`; do
for ((i=1;i<=100;i++)); do
        xadmin bc -t WHILE -s $i
done

sleep 20

# have some sync (wait for startup to complete, print the output)
xadmin pc

CNT=`xadmin pc | grep "WHILE" | grep "running pid" | wc | awk '{print $1}'`
echo "xadmin procs: $CNT"
if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
        echo "TESTERROR! $PROC_COUNT procs not booted (according to xadmin pc)!"
        go_out 6
fi

echo "Processes in system: "
$PSCMD

#CNT=`$PSCMD | grep whileproc.sh | grep -v grep | wc | awk '{print $1}'`
#echo "$PSCMD procs: $CNT"
#if [[ "$CNT" -ne "$PROC_COUNT" ]]; then 
        #echo "TESTERROR! $PROC_COUNT procs not booted (according to $PSCMD )!"
        #go_out 7
#fi

#
# Having some issues when bash is doing forks inside the test script -> whileproc.sh
# Thus filter by cpmsrv pid in ps line...
#
CPM_PID=0
if [ "$(uname)" == "FreeBSD" ]; then
        CPM_PID=`ps -auwwx| grep $USER | grep $NDRX_RNDK | grep cpmsrv | awk '{print $2}'`
else
        CPM_PID=`ps -ef | grep $USER | grep $NDRX_RNDK | grep cpmsrv | awk '{print $2}'`
fi

CNT=0
if [ "$(uname)" == "Linux" ]; then
	while read -r line ; do
    		echo "Processing [$line]"
    		# your code goes here
    		MATCH=`echo $line | grep $CPM_PID |grep whileproc.sh`
    
    		if [ "X$MATCH" != "X" ]; then
        		echo "MATCH: [$MATCH]"
        		CNT=$((CNT+1))
    		else
        		echo "NOT MATCH: [$MATCH]"
    		fi
	done < <($PSCMD)

	PROC_COUNT_DIFFALLOW=$PROC_COUNT

else

	CNT=`$PSCMD | grep whileproc.sh | grep -v grep | wc | awk '{print $1}'`
fi

echo "$PSCMD procs: $CNT"

if [ "$CNT" -lt "$PROC_COUNT" ] || [ "$CNT" -gt "$PROC_COUNT_DIFFALLOW"  ]; then 
        echo "TESTERROR! $PROC_COUNT procs not booted (according to $PSCMD )!"
        go_out 7
fi

# test signle binary shutdown 
xadmin sc -t IGNORE
# We should have a binary which does not react on kill signals
OUT=`grep SIGINT ignore.log`
if [[ "X$OUT" == "X" ]]; then
        echo "WARNING! no SIGINT in ignore.log!"
fi

#boot it back for next test
> ignore.log

xadmin bc -t IGNORE
sleep 10

# Stop the cpmsrv (will do automatic process shutdown)...
xadmin stop -s cpmsrv

# We should have 0 now
CNT=`$PSCMD | grep whileproc.sh | grep -v grep | wc | awk '{print $1}'`
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "0" ]]; then 
        echo "TESTERROR! not all whileproc.sh stopped!"
        go_out 8
fi

# We should have a binary which does not react on kill signals
OUT=`grep SIGINT ignore.log`
if [[ "X$OUT" == "X" ]]; then
        echo "WARNING! no SIGINT in ignore.log!"
fi

OUT=`grep SIGTERM ignore.log`
if [[ "X$OUT" == "X" ]]; then
        echo "WARNING! no SIGTERM in ignore.log!"
fi

# process must be killed
CNT=`$PSCMD | grep ignore.sh | grep -v grep | wc | awk '{print $1}'`
echo "$PSCMD procs: $CNT"
if [[ "$CNT" -ne "0" ]]; then 
        echo "TESTERROR! ignore.sh not stopped!"
        go_out 11
fi

go_out 0

