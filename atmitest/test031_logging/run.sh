#!/bin/bash
##
## @brief @(#) Test31 Launcher
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
TESTNAME="test031_logging"

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

if [ "X`grep 'Thread 1 logs to main' clt-tp.log`" == "X" ]; then
        echo "error in clt-tp.log (missing Thread 1 logs to main in main)!"
	RET=-2
fi

if [ "X`grep 'Thread 2 logs to main' clt-tp.log`" == "X" ]; then
        echo "error in clt-tp.log (missing Thread 2 logs to main in main)!"
	RET=-2
fi

# There shall be 1000 files in log directory
FILES=` ls -1 ./logs/*.log | wc | awk '{print $1}'`

echo "Got request files: [$FILES]"
if [ "X$FILES" != "X1000" ]; then
        echo "Invalid files count [$FILES] should be 1000!"
	RET=-2
fi

################################################################################
# there shall be in each log file:
# - Hello from SETREQFILE
# - Hello from atmicl31
# - Hello from TEST31_2ND
################################################################################

# Test all 1000 files

for ((i=1;i<=100;i++)); do
echo "Testing sequence: $i"

    if [ "X`grep 'Hello from SETREQFILE' ./logs/request_$i.log`" == "X" ]; then
            echo "Missing 'Hello from SETREQFILE' file $i"
            RET=-2
    fi

    if [ "X`grep 'Hello from atmicl31' ./logs/request_$i.log`" == "X" ]; then
            echo "Missing 'Hello from atmicl31' file $i"
            RET=-2
    fi

    if [ "X`grep 'Hello from TEST31_2ND' ./logs/request_$i.log`" == "X" ]; then
            echo "Missing 'Hello from TEST31_2ND' file $i"
            RET=-2
    fi
done

if [ "X`grep 'Finishing off' ./clt-tp.log`" == "X" ]; then
        echo "Missing 'Finishing off'"
        RET=-2
fi

popd 2>/dev/null

exit $RET

