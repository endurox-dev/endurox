#!/bin/bash
##
## @brief @(#) Test038 - TP Notify tests
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

export TESTNO="038"
export TESTNAME_SHORT="tpnotify"
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
# We do not need timeout, we will kill procs...
export NDRX_TOUT=9999

#
# Domain 1 - here client will live
#
function set_dom1 {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

#
# Domain 2 - here server will live
#
function set_dom2 {
    echo "Setting domain 2"
    . ../dom2.sh    
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    set_dom2;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt38

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 30
RET=0
MAX_CALLS=10000 # Same as in atmiclt38.c constant...

echo "Running off client"
set_dom1;
(./atmiclt38 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt38 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

#
# Check the presence of numbers in logs...
# In ATMI client:
# We need to have BB0100 - 10000
# We need to have BB0200 - 10000

# We need to have CC0100 - 10000
# We need to have CC0200 - 10000

CNT=`grep BB0100 atmiclt-dom1.log | wc | awk '{print $1}'`
echo "BB0100 count: $CNT"
if [[ $CNT -ne $MAX_CALLS ]]; then
        echo "Actual BB0100 $CNT != $MAX_CALLS! (1)"
        go_out 1        
fi

CNT=`grep BB0200 atmiclt-dom1.log | wc | awk '{print $1}'`
echo "BB0200 count: $CNT"
if [[ $CNT -ne $MAX_CALLS ]]; then
        echo "Actual BB0200 $CNT != $MAX_CALLS! (2)"
        go_out 2
fi

CNT=`grep CC0100 atmiclt-dom1.log | wc | awk '{print $1}'`
echo "CC0100 count: $CNT"
if [[ $CNT -ne $MAX_CALLS ]]; then
        echo "Actual CC0100 $CNT != $MAX_CALLS! (3)"
        go_out 3
fi

CNT=`grep CC0200 atmiclt-dom1.log | wc | awk '{print $1}'`
echo "CC0200 count: $CNT"
if [[ $CNT -ne $MAX_CALLS ]]; then
        echo "Actual CC0200 $CNT != $MAX_CALLS! (4)"
        go_out 4
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi


go_out $RET



