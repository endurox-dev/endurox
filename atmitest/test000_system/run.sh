#!/bin/bash
##
## @brief @(#) Test000 Launcher - JSON typed buffer test
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
TESTNAME="test000_system"

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
xadmin qrmall test000
rm *.log 2>/dev/null

# Post the event
(./atmiclt0 2>&1) > ./atmiclt0.log

RET=$?

# run queue tests...
./atmiclt0_mqsv &
TMP=$?
if [ $TMP != 0 ]; then
    echo "Failed to start atmiclt0_mqsv"
    RET=-3
fi
sleep 1

# run off the client
./atmiclt0_mqcl
TMP=$?
if [ $TMP != 0 ]; then
    echo "Failed to start atmiclt0_mqcl"
    RET=-4
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv0 2>/dev/null
xadmin killall atmiclt0 2>/dev/null
# kill both client & server
xadmin killall atmiclt0_ 2>/dev/null

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
