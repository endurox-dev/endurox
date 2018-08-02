#!/bin/bash
## 
## @brief @(#) Test013, Test process auto restart at no response (long startup, no ping rsp, or long shutdown)
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
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

export TESTNO="013"
export TESTNAME_SHORT="procnorsp"
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
export NDRX_TOUT=4
export NDRX_CMDWAIT=1

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

rm *.log

# Run some extra cleanup
xadmin down -y
xadmin killall ndrxd
xadmin down -y

# Will do stall servers
echo 1 > $TESTDIR/case_type

# No ping
xadmin start -i 126 || go_out 1
# Long stop
xadmin start -i 127 || go_out 1
# Will run in background, long start
xadmin start -i 125 &
sleep 2
#tail -n200 ndrxd.log

#### Capture current PIDs of all 3x processes #####
LONG_START_PID=""
if [ "$(uname)" == "FreeBSD" ]; then
	LONG_START_PID=`ps -auwwx | grep "\-i 125" | grep -v grep | awk '{print $2}'`;
else
	LONG_START_PID=`ps -ef | grep "\-i 125" | grep -v grep | awk '{print $2}'`;
fi
echo "long_start pid: $LONG_START_PID"

NO_PING_PROCESS_PID=""
if [ "$(uname)" == "FreeBSD" ]; then
	NO_PING_PROCESS_PID=`ps -auwwx| grep "\-i 126" | grep -v grep | awk '{print $2}'`;
else
	NO_PING_PROCESS_PID=`ps -ef | grep "\-i 126" | grep -v grep | awk '{print $2}'`;
fi
echo "no_ping_process pid: $NO_PING_PROCESS_PID"

LONG_STOP_PID=""
if [ "$(uname)" == "FreeBSD" ]; then
	LONG_STOP_PID=`ps -auwwx| grep "\-i 127" | grep -v grep | awk '{print $2}'`;
else
	LONG_STOP_PID=`ps -ef | grep "\-i 127" | grep -v grep | awk '{print $2}'`;
fi
echo "long_stop pid: $LONG_STOP_PID"
sleep 2

# No server stalling
echo 2 > $TESTDIR/case_type

# will stop the process so that PING will hang
kill -9 $NO_PING_PROCESS_PID
xadmin stop -i 127 &

#### Sleep some 10 sec
sleep 20
ps -ef | grep atmi
#### All those processes now should be restarted, so get new PIDs
LONG_START_PID2=""
if [ "$(uname)" == "FreeBSD" ]; then
	LONG_START_PID2=`ps -auwwx | grep "\-i 125" | grep -v grep |awk '{print $2}'`;
else
	LONG_START_PID2=`ps -ef | grep "\-i 125" | grep -v grep |awk '{print $2}'`;
fi
echo "long_start pid2: $LONG_START_PID2"

NO_PING_PROCESS_PID2=""
if [ "$(uname)" == "FreeBSD" ]; then
	NO_PING_PROCESS_PID2=`ps -auwwx | grep "\-i 126" | grep -v grep | awk '{print $2}'`;
else
	NO_PING_PROCESS_PID2=`ps -ef | grep "\-i 126" | grep -v grep | awk '{print $2}'`;
fi

echo "no_ping_process pid2: $NO_PING_PROCESS_PID2"
LONG_STOP_PID2=""
if [ "$(uname)" == "FreeBSD" ]; then
	LONG_STOP_PID2=`ps -auwwx | grep "\-i 127" | grep -v grep | awk '{print $2}'`;
else
	LONG_STOP_PID2=`ps -ef | grep "\-i 127" | grep -v grep | awk '{print $2}'`;
fi
echo "long_stop pid2: $LONG_STOP_PID2"


#
# Test long start
#
if [ "X$LONG_START_PID2" == "X$LONG_START_PID" ]; then
    echo "long start probl: old PID: $LONG_START_PID new PID: $LONG_START_PID2"
    go_out 1
fi

if [ "X$LONG_START_PID2" == "X" ]; then
    echo "long start probl: process not re-spawned!!!"
    go_out 2
fi

#
# Test ping...
#
if [ "X$NO_PING_PROCESS_PID2" == "X$NO_PING_PROCESS_PID" ]; then
    echo "no_ping_process probl: old PID: $NO_PING_PROCESS_PID new PID: $NO_PING_PROCESS_PID2"
    go_out 3
fi

if [ "X$NO_PING_PROCESS_PID2" == "X" ]; then
    echo "no_ping_process probl: process not re-spawned!!!"
    go_out 4
fi

#
# Test long shutdown
#
if [ "X$LONG_STOP_PID2" == "X$LONG_STOP_PID" ]; then
    echo "long stop probl: old PID: $LONG_STOP_PID new PID: $LONG_STOP_PID2"
    go_out 5
fi

# It was requested to shutdown, so should not be re-started!
if [ "X$LONG_STOP_PID2" != "X" ]; then
    echo "long stop probl: process not re-spawned!!!"
    go_out 6
fi

# Print process model
xadmin ppm

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out 100
fi

go_out 0

