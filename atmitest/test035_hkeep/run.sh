#!/bin/bash
## 
## @(#) Test035 - housekeep routine tests
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
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
## A commercial use license is available from ATR Baltic, SIA
## contact@atrbaltic.com
## -----------------------------------------------------------------------------
##

export TESTNO="035"
export TESTNAME_SHORT="hkeep"
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
    xadmin killall atmiclt35

    popd 2>/dev/null
    exit $1
}

rm *dom*.log

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 60

# Go to domain 1
set_dom1;

RET=0

# Run the client test...
echo "Soon will issue client calls:"
xadmin psc
xadmin psvc
xadmin ppm

echo "Before test start"
xadmin pqa -a

# This will call local and remove conv servers...
(./atmiclt35 2>&1) > ./atmiclt-dom1.log &

echo "after start"
xadmin pqa -a

# Wait threads start-up
sleep 5

xadmin killall atmiclt35 atmisv35 tpbridge

echo "after kill"

xadmin pqa -a

# Let house keep to run...
sleep 20

echo "after sleep 20"

xadmin pqa -a

# Test the queues, must be something like this
#0          /dom2,sys,bg,ndrxd
#0          /dom2,clt,reply,ndrxd,14591,0
#0          /dom1,sys,bg,xadmin,14629
#0          /dom1,sys,bg,ndrxd
#0          /dom1,clt,reply,ndrxd,14573,0

CNT=`xadmin pqa -a | wc | awk '{print $1}'`

if [[ $CNT -gt 5 ]]; then
    echo "Queue count $CNT, must be 5 or less"
    go_out 3
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

go_out $RET

