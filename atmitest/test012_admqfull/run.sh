#!/bin/bash
## 
## @(#) Test012, Test semaphore for startup, so that if process is killed other processes can continue to work!
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
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

# So plan is following:
# 1. we start auto deadly process with xadmin start -y
# 2. we kill deadly process  & wait for background ndrxd will respawn it.
# 3. we start manually good process inbackground - now it cannot compelete the init, because bad process have semaphore
# 4. we test that OK service is not advertised
# 5. we kill the bad service, system releases sempahore, good process gets advertised & we test that
# .... this is the test ....
#


export TESTNO="012"
export TESTNAME_SHORT="admqfull"
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
export NDRX_MSGMAX=50
export NDRX_DQMAX=50


# Default process count for Q full tests
export TEST_PROCS=400

# OSX hangs at 500 procs running, thus reduce test to 200
if [ "$(uname)" == "Darwin" ]; then
    export TEST_PROCS=200
fi

echo "Running tests with: $TEST_PROCS processes...."
##################################

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin killall sleep
# Might cause respawn for that one process...
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

###################################
# Time-out processing.
###################################
#Time to wait for stuck processes before killing them                           
export ALARMTIME=900

PARENTPID=$$

exit_timeout() {
    echo "Alarm signal received - TEST FAILED!"
    go_out 128

}

#Prepare to catch SIGALRM, call exit_timeout
trap exit_timeout SIGALRM
(sleep $ALARMTIME && kill -ALRM $PARENTPID) &
##################################

rm *.log

xadmin killall atmisv
xadmin killall ndrxd
xadmin down -y
xadmin start -y
# Why?
#xadmin start -y &

#### WHY?
# Enter the both servers in respawn state
# sleep 30
#####

# ALARM: THIS SHOULD NOT HANG!
xadmin psc
CNT=`xadmin psc | wc | awk '{print $1}'`
echo "Process count: $CNT"
if [[ $CNT -ne $TEST_PROCS ]]; then
	echo "Service count != $TEST_PROCS! (1)"
	go_out 1	
fi
# Count should be around 1K
echo 
echo "Before running kill -9"
date
xadmin psc

xadmin killall atmisv_$TESTNO

echo "After runing kill -9"
date
xadmin psc

# Wait for respawn, now it should be respawned...
sleep 300

echo "After sleeping of kill -9"
date
xadmin psc

# This should raise the fail!
# sleep 30
# ALARM: THIS SHOULD NOT HANG & should be all servers booted.
CNT=`xadmin psc | wc | awk '{print $1}'`
echo "Process count: $CNT"
if [[ $CNT -ne $TEST_PROCS ]]; then
	echo "Service count != 500! (2)"
	go_out 2
fi
# Count should be around 1K

go_out 0

