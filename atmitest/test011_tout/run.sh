#!/bin/bash
## 
## @(#) Test011, Testing the case when server dies & queue is being removed.
## The svcerr reply is passed back to client!#
## THIS CURRENLTY COVERS THE CASE WHEN SHUTDOWN IS REQUESTED, SERVER EXISTS, BUT
## MESSAGES ARE LEFT IN QUEUE, THEY WILL GOT SVCERR. BUT IN VERY CLEAN IMPLEMENTATION
## WE MIGHT SET THE FLAG IN SHARED MEMORY THAT SERVICE IS NOT AVAILABLE, AND LEAVE ENQUEUED
## ONES FOR PROCESSING.
## But for current implementation SVCERR will be OK too, at last is is not timeout!
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

export TESTNO="011"
export TESTNAME_SHORT="tout"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"

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

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
# Configure the runtime - override stuff here!
export NDRX_CONFIG=$TESTDIR/ndrxconfig.xml
export NDRX_DMNLOG=$TESTDIR/ndrxd.log
export NDRX_LOG=$TESTDIR/ndrx.log
export NDRX_DEBUG_CONF=$TESTDIR/debug.conf
# Override timeout!
export NDRX_TOUT=5

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    killall -9 atmiclt_$TESTNO

    popd 2>/dev/null
    exit $1
}

rm *.log

xadmin down -y
xadmin start -y || go_out 1

# Have some wait for ndrxd goes in service
sleep 1

atmiclt_$TESTNO &
atmiclt_$TESTNO &
sleep 7

# Now test log files, we should contain
# [TESTSVFN TIMEOUT @1@]
if [ "X`grep 'TESTSVFN TIMEOUT @1@' *.log`" == "X" ]; then
	echo "Test error detected - no entry of [TESTSVFN TIMEOUT @1@]!"
	go_out 2
fi

# [TESTSVFN TIMEOUT @2@] - should not contain, as message should be discarded!
if [ "X`grep 'TESTSVFN TIMEOUT @2@' *.log`" != "X" ]; then
	echo "Test error detected - logs contain [TESTSVFN TIMEOUT @2@]!"
	go_out 3
fi

# Now run stuff with NOTIME flag..! 
atmiclt_$TESTNO N&
atmiclt_$TESTNO N&
sleep 13

# Now test log files, we should contain
# [TESTSVFN TIMEOUT @1@]
if [ "X`grep 'TESTSVFN NOTOUT @1@' *.log`" == "X" ]; then
	echo "Test error detected - no entry of [TESTSVFN NOTOUT @1@]!"
	go_out 4
fi

# [TESTSVFN TIMEOUT @2@] - should not contain, as message should be discarded!
if [ "X`grep 'TESTSVFN NOTOUT @2@' *.log`" == "X" ]; then
	echo "Test error detected - no entry of [TESTSVFN NOTOUT @2@]!"
	go_out 5
fi


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out 127
fi

go_out 0

