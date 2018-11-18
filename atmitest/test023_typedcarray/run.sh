#!/bin/bash
##
## @brief @(#) Test023 Launcher - CARRAY typed buffer test
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
TESTNAME="test023_typedcarray"

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

xadmin killall atmisv23 2>/dev/null
xadmin killall atmiclt23 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

# Start event server
#(valgrind --track-origins=yes --leak-check=full ../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
# NOTE: WE HAVE MEM LEAK HERE:
(../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
sleep 30
# Start subscribers
(./atmisv23 -t 4 -i 100 2>&1) > ./atmisv23.log &
sleep 30
# Post the event
(./atmiclt23 2>&1) > ./atmiclt23.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv23 2>/dev/null
xadmin killall atmiclt23 2>/dev/null
xadmin killall tpevsrv 2>/dev/null

#killall atmiclt1

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
