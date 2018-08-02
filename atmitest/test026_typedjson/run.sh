#!/bin/bash
## 
## @brief @(#) Test026 Launcher - JSON typed buffer test
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
TESTNAME="test026_typedjson"

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
export NDRX_DEBUG_CONF=$TESTDIR/debug.conf

# clean up the env for processing...
xadmin down -y

# Start subscribers
(./atmisv26 -t 4 -i 100 -xTEST26_UBF2JSON:UBF2JSON -xTEST26_JSON2UBF:JSON2UBF -- >&1) > ./atmisv26.log &
sleep 1
# Post the event
(./atmiclt26 2>&1) > ./atmiclt26.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv26 2>/dev/null
xadmin killall atmiclt26 2>/dev/null

popd 2>/dev/null

exit $RET

