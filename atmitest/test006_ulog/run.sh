#!/bin/bash
##
## @brief @(#) Test006 Launcher
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
TESTNAME="test006_ulog"
TOUT=2

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test006_ulog
fi;

# client timeout
export NDRX_TOUT=2

. ../testenv.sh
# Remove existing logs
rm -rf ./log 2>/dev/null
mkdir ./log
export NDRX_ULOG="./log"

sleep 1
(./atmiclt006 2>&1) > ./atmiclt006.log

RET=$?
FILE=$NDRX_ULOG/ULOG.`date +%Y%m%d`

# Catch is there is test error!!!
if [ "X`grep 'HELLO WORLD FROM USER LOG' $FILE`" == "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
