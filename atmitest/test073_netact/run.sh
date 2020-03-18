#!/bin/bash
##
## @brief Network activity testing - test launcher
##  Check that inactive connections are reset
##  Check that zero length messages keeps the traffic going / thus no restart.
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

export TESTNAME="test073_netact"

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
export NDRX_ULOG=$TESTDIR
export NDRX_TOUT=10
export NDRX_SILENT=Y

#
# Domain 1 - here client will live
#
set_dom1() {
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
set_dom2() {
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
    xadmin killall atmiclt73

    popd 2>/dev/null
    exit $1
}

rm *.log
rm ULOG*

# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom2;
xadmin down -y
xadmin start -y || go_out 1

#
# OK, firstly boot non activity bridge...
#
set_dom1;
xadmin down -y
xadmin start -y || go_out 1
xadmin start -i 100


echo "Wait for connection reset... about 30 sec.. (connect + check)"
sleep 30


# now check ULOG...
RESULT=`grep 'No data received in' ULOG*`

echo "Result: [$RESULT]"

if [ "X$RESULT" == "X" ]; then
    echo "Connection not restarted!"
    go_out -10
fi

xadmin stop -i 100
#
# Let processes to fully disconnect 
# and clean-up old logs..
#
sleep 1
rm ULOG*
xadmin start -i 200

echo "Let connection to establish... + non reset (zero msgs should work...)"
sleep 30

# now check ULOG...
RESULT=`grep 'No data received in' ULOG*`

echo "Result: [$RESULT]"

if [ "X$RESULT" != "X" ]; then
    echo "Connection restarted when not expected (zero msgs should be feeding the activity)!"
    go_out -20
fi

#
# Check that connection is OK
#
xadmin ppm

RESULT=`xadmin ppm | grep 200 | grep BC`

echo "Result: [$RESULT]"

if [ "X$RESULT" == "X" ]; then
    echo "No open connection!"
    go_out -30
fi


echo "All OK"

go_out 0

# vim: set ts=4 sw=4 et smartindent:
