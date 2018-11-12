#!/bin/bash
##
## @brief @(#) Test010, Test semaphore for startup, so that if process is killed other processes can continue to work!
##   TODO: Only for epoll() version. For unix load balancer we will need different one..
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

# So plan is following:
# 1. we start auto deadly process with xadmin start -y
# 2. we kill deadly process  & wait for background ndrxd will respawn it.
# 3. we start manually good process inbackground - now it cannot compelete the init, because bad process have semaphore
# 4. we test that OK service is not advertised
# 5. we kill the bad service, system releases sempahore, good process gets advertised & we test that
# .... this is the test ....
#


export TESTNO="010"
export TESTNAME_SHORT="strtracecond"
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

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
# Might cause respawn for that one process...
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

rm *.log

xadmin down -y
xadmin start -y &

# Enter the both servers in respawn state
sleep 3

xadmin killall atmisv_$TESTNO
# Wait for respawn, now it should be respawned...
sleep 5

# try to start stuff in background
xadmin start -i 2411 & 
sleep 1


echo "*** PQA ***"
xadmin pqa

# Now we should not have queue with "SVCOK", as server is locked on bad server
if [[ "X`xadmin pqa | grep SVCOK`" != "X" ]]; then
    echo "SVCOK must not be advertised!!!"
    go_out 1
fi

echo "User: $USER"
echo "Login: $LOGNAME"
echo "RNDK: $NDRX_RNDK"

#
# Solaris 12, do not have $USER, but have $LOGNAME
#
if [[ "X$USER" == "X" ]]; then
	USER=$LOGNAME
fi

if [ "$(uname)" == "FreeBSD" ]; then

	echo "******* PS ***************"
	ps -auwwx
	echo "******* PS Grep'ped*******"
	ps -auwwx| grep $USER | grep $NDRX_RNDK | grep "\-i 1341" | awk '{print $2}'
	echo "**************************"

	BAD_PID=`ps -auwwx| grep $USER | grep $NDRX_RNDK | grep "\-i 1341" | awk '{print $2}'`
else
	BAD_PID=`ps -ef | grep $USER | grep $NDRX_RNDK | grep "\-i 1341" | awk '{print $2}'`
fi

echo "BAD_PID=$BAD_PID"
kill -9 $BAD_PID
sleep 1
# Put it in shutdown state!
xadmin stop -i 1341

# Now we should not have removed queue with "SVCOK", as server is locked on bad server
if [ "X`xadmin psc | grep SVCOK`" == "X" ]; then
	echo "SVCOK must be advertised!!!"
	go_out 2
fi

sleep 5 

xadmin pqa
xadmin psvc
# Now SVC OK must be advertized
if [[ "X`xadmin psvc | grep SVCOK`" == "X" ]]; then
    echo "SVC Must be advertised!"
    go_out 1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out 4
fi

go_out 0

# vim: set ts=4 sw=4 et smartindent:
