#!/bin/bash
##
## @brief @(#) Test2 Launcher
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
TESTNAME="test002_basicforward"

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

echo "-------------  RUNNING DOWN -------------"
xadmin down -y
echo "------------ SYSTEM AFTER DOWN ----------"
ipcs
xadmin ps | grep atmi
xadmin ps | grep ndrx
xadmin ps | grep tp
xadmin ps | grep xa
echo "-----------------------------------------"

rm *.log
export NDRX_DEBUG_CONF=`pwd`/debug.conf

(./atmisv2FIRST -t 4 -i 1 2>&1) > ./atmisv2FIRST.log &
(./atmisv2SECOND -i 1 2>&1) > ./atmisv2SECOND.log &
sleep 2
(./atmiclt2 2>&1) > ./atmiclt2.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv2FIRST 2>/dev/null
xadmin killall atmisv2SECOND 2>/dev/null

#killall atmiclt1

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
