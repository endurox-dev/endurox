#!/bin/bash
## 
## @brief @(#) Test036 - TP Recovery testing
##
## @file run.sh
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

export TESTNO="036"
export TESTNAME_SHORT="tprecover"
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
    xadmin killall atmiclt36

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 60

set_dom1;

echo "Before ndrxd kill - dom1"
xadmin ppm

set_dom2;
echo "Before ndrxd kill - dom2"
xadmin ppm

xadmin killall ndrxd

# Let the restore services + send bridge updates
sleep 60

echo "Before ndrxd kill - dom2"
echo "We should be ready back online"
xadmin ppm
xadmin psc

OUT=`xadmin ppm | grep BrC`

if [[ "X$OUT" == "X" ]]; then
    echo "TESTERROR, the domain2 does not contain connected bridge!"
    go_out 1
fi


set_dom1;

echo "Before ndrxd kill - dom1"
echo "We should be ready back online"
xadmin ppm
xadmin psc

OUT=`xadmin ppm | grep BrC`

if [[ "X$OUT" == "X" ]]; then
    echo "TESTERROR, the domain1 does not contain connected bridge!"
    go_out 2
fi

# well we should test here that bridged service is SVC36 is available

OUT=`xadmin psc | grep SVC36`


if [[ "X$OUT" == "X" ]]; then
    echo "TESTERROR, the domain1 does not contain other domain's services, missing: SVC36!"
    go_out 3
fi

RET=0

go_out $RET



