#!/bin/bash
##
## @brief @(#) Test004 Launcher
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
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
TESTNAME="test004_basicevent"

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

export NDRX_DEBUG_CONF=`pwd`/debug.conf

xadmin killall atmisv4_1ST 2>/dev/null
xadmin killall atmisv4_2ND 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

xadmin qrmall /

rm *.log

# Start event server
#(valgrind --track-origins=yes --leak-check=full ../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
# NOTE: WE HAVE MEM LEAK HERE:
(../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
sleep 2
# Start subscribers
(./atmisv4_1ST -t 4 -i 100 2>&1) > ./atmisv4_1ST.log &
#(./atmisv4_1ST -t 4 -i 120 2>&1) > ./atmisv4_1ST-2.log &
#(./atmisv4_1ST -t 4 -i 121 2>&1) > ./atmisv4_1ST-3.log &
# Start subscribers
(./atmisv4_2ND -i 110 2>&1) > ./atmisv4_2ND.log &
(./atmisv4_2ND -i 130 2>&1) > ./atmisv4_2ND-2.log &
#(valgrind --track-origins=yes --leak-check=full ./atmisv4_2ND -i 131 2>&1) > ./atmisv4_2ND-3.log &
(./atmisv4_2ND -i 131 2>&1) > ./atmisv4_2ND-3.log &
sleep 2
# Post the event
(./atmiclt4 2>&1) > ./atmiclt4.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv4_1ST 2>/dev/null
xadmin killall atmisv4_2ND 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
