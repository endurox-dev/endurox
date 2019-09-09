#!/bin/bash
##
## @brief @(#) Test18 Launcher
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
## See LICENSE file for full text.
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
TESTNAME="test018_tpacalltout"
TOUT=4

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test018_tpacalltout
fi;

. ../testenv.sh
# client timeout
export NDRX_TOUT=4

ulimit -c unlimited

export NDRX_DEBUG_CONF=`pwd`/debug.conf
echo "Debug config set to: [$NDRX_DEBUG_CONF]"
xadmin killall atmi.sv18 2>/dev/null
xadmin killall atmiclt18
# remove process leftovers...
xadmin down -y
rm *.log

# We need nr of copies as nr of tpacalltout, 
# as if on thread is doing timout testing
# The other thread will get time-out as well and could be 
# in not expected place..
(./atmi.sv18 -t10 -i 125 2>&1) > ./atmisv18.log &
sleep 1
(./atmiclt18 2>&1) > ./atmiclt18.log 
RET=$?


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmi.sv18 2>/dev/null
xadmin killall atmiclt18
# remove process leftovers...
xadmin down -y

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
