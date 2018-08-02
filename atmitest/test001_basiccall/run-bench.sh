#!/bin/bash
## 
## @brief @(#) Test1 Launcher
##
## @file run-bench.sh
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
TESTNAME="test001_basiccall"
TOUT=2

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test001_basiccall
fi;

. ../testenv.sh
# client timeout
export NDRX_TOUT=2

ulimit -c unlimited

export NDRX_DEBUG_CONF=`pwd`/debug-bench.conf
echo "Debug config set to: [$NDRX_DEBUG_CONF]"
rm *.log

# We need nr of copies as nr of basiccall, 
# as if on thread is doing timout testing
# The other thread will get time-out as well and could be 
# in not expected place..
(./atmi.sv1 -t10 -i 123 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 124 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
(./atmi.sv1 -t10 -i 125 2>&1) > ./atmisv1.log &
sleep 1
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_1.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_2.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_3.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_4.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_5.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_6.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_7.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_8.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_9.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_10.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_11.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_12.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_13.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_14.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_15.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_16.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_17.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_18.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_19.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_20.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_21.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_22.log &
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_23.log &
ps -ef | grep atmiclt1 | grep -v grep | wc

# Run off the tests...
echo 1 > sync.log

# keep the process for total sync...
(./atmiclt1 2>&1 s :1:8:) > ./atmiclt1_24.log 

sleep 5 # let other to complete... (sync..)
RETP=`ps -ef | grep atmiclt1 | grep -v grep`
while [[ "X$RETP" != "X" ]]; do
    RETP=`ps -ef | grep atmiclt1 | grep -v grep`
done 

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

killall atmi.sv1 2>/dev/null

killall atmiclt1

popd 2>/dev/null

exit $RET

