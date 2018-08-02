#!/bin/bash
## 
## @brief @(#) Test022 Launcher - STRING typed buffer test
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
TESTNAME="test022_typedstring"

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

xadmin killall atmisv22 2>/dev/null
xadmin killall atmiclt22 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

# Start event server
#(valgrind --track-origins=yes --leak-check=full ../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
# NOTE: WE HAVE MEM LEAK HERE:
(../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
sleep 10
# Start subscribers
(./atmisv22 -t 4 -i 100 2>&1) > ./atmisv22.log &
sleep 10
# Post the event
(./atmiclt22 2>&1) > ./atmiclt22.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv22 2>/dev/null
xadmin killall atmiclt22 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

#killall atmiclt1

popd 2>/dev/null

exit $RET

