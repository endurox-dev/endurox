#!/bin/bash
## 
## @(#) Test31 Launcher
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
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
## A commercial use license is available from ATR Baltic, SIA
## contact@atrbaltic.com
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

rm *.log

# Clean up log dir
rm -rf ./logs
mkdir ./logs

(./atmisv31FIRST -t 4 -i 1 2>&1) > ./atmisv31FIRST.log &
(./atmisv31SECOND -i 1 2>&1) > ./atmisv31SECOND.log &
sleep 2
(./atmiclt31 2>&1) > ./atmiclt31.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmisv31FIRST 2>/dev/null
xadmin killall atmisv31SECOND 2>/dev/null

#killall atmiclt1


# Check the log files
if [ "X`grep 'Hello from NDRX' clt-endurox.log`" == "X" ]; then
        echo "error in clt-endurox.log!"
	RET=-2
fi

if [ "X`grep 'Hello from tp' clt-tp.log`" == "X" ]; then
        echo "error in clt-tp.log!"
	RET=-2
fi

if [ "X`grep 'hello from thread 1' clt-tp-th1.log`" == "X" ]; then
        echo "error in clt-tp-th1.log!"
	RET=-2
fi

if [ "X`grep 'hello from thread 2' clt-tp-th2.log`" == "X" ]; then
        echo "error in clt-tp-th2.log!"
	RET=-2
fi

if [ "X`grep 'hello from main thread' clt-tp.log`" == "X" ]; then
        echo "error in clt-tp.log!"
	RET=-2
fi

popd 2>/dev/null

exit $RET

