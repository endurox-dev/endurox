#!/bin/bash
## 
## @brief @(#) Test007, advertise & unadvertise
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
export TESTNAME="test007_advertise"

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


#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin down -y
    popd 2>/dev/null
    exit $1
}

rm *.log

xadmin down -y
xadmin start -y || go_out 1

# Have some wait for ndrxd goes in service
sleep 2

# Stuff should not be advertised

if [ "X`xadmin psc | grep TESTSVFN`" != "X" ]; then
	echo "TESTSVFN should not be advertised"
	go_out 2
fi

echo "Sending DOADV to sv"
atmiclt_007 DOADV || go_out 4

if [ "X`xadmin psc | grep TESTSVFN`" == "X" ]; then
	echo "TESTSVFN must be appeared"
	go_out 2
fi

echo "Sending TEST to sv"
atmiclt_007 TEST || go_out 5


echo "Sending UNADV to sv"
atmiclt_007 UNADV || go_out 6

if [ "X`xadmin psc | grep TESTSVFN`" != "X" ]; then
	echo "TESTSVFN must be dissapeared"
	go_out 7
fi

echo "Testing again client TEST call, service dissapeared, should be bad call"
atmiclt_007 TEST

if [ $? -eq 0 ]; then
	echo "For some reason test call after UNADV succeeded!!!"
	go_out 8
fi

echo "Sending DOADV to sv"
atmiclt_007 DOADV || go_out 9
sleep 3

if [ "X`xadmin psc | grep TESTSVFN`" == "X" ]; then
	echo "TESTSVFN must be appeared"
	go_out 10
fi

echo "Testing again client TEST call, service dissapeared, should be ok call"
atmiclt_007 TEST || go_out 11


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out 12
fi



go_out 0

