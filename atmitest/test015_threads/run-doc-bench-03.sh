#!/bin/bash
##
## @brief @(#) Test15 Launcher, for benchmark
##
## @file run-doc-bench-03.sh
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
TESTNAME="test015_threads"
TOUT=2

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test015_threads
fi;

. ../testenv.sh
# client timeout
export NDRX_TOUT=2

ulimit -c unlimited
export NDRX_DEBUG_CONF=`pwd`/debug.conf

# We need nr of copies as nr of threads, 
# as if on thread is doing timout testing
# The other thread will get time-out as well and could be 
# in not expected place..
(./atmi.sv15 -t10 -i 123 2>&1) > ./atmisv15.log &
(./atmi.sv15 -t10 -i 124 2>&1) > ./atmisv15_2.log &
(./atmi.sv15 -t10 -i 125 2>&1) > ./atmisv15_3.log &
(./atmi.sv15 -t10 -i 126 2>&1) > ./atmisv15_4.log &
(./atmi.sv15 -t10 -i 127 2>&1) > ./atmisv15_5.log &
sleep 1
(./atmiclt15 bench 2>&1) > ./atmiclt15.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

xadmin killall atmi.sv15 2>/dev/null

#killall atmiclt1

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
