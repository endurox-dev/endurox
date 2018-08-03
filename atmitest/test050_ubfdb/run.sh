#!/bin/bash
##
## @brief @(#) UBF Field table database tests - test launcher
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

export TESTNAME="test050_ubfdb"

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

export NDRX_TOUT=10

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
    export NDRX_CCONFIG=$TESTDIR
    export NDRX_SILENT=Y
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y



    # If some alive stuff left...
    xadmin killall atmiclt50

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

# Have some wait for ndrxd goes in service - wait for connection establishment.
RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;
(./atmiclt50 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt50 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Running IPC setup"
(./atmiclt50ipc ubfdb_setup 2>&1) > ./atmicltipc-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


echo "Reboot atmi.sv50 to catch db which was reset"

xadmin sr atmi.sv50

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


echo "Running off IPC tests..."

(./atmiclt50ipc ubfdb_callsv 2>&1) >> ./atmicltipc-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


echo "Testing xadmin -> field listing feature"

xadmin pubfdb

echo "Tesging char field"
OUT=`xadmin pubfdb | grep T_HELLO_CHAR | grep char | grep 67111866 | grep 3002`
echo "got: [$OUT]"
if [ "X$OUT" == "X" ]; then
    echo "No char T_HELLO_CHAR found!"
    go_out 1
fi

echo "Tesging long field"
OUT=`xadmin pubfdb | grep 3003 | grep  33557435  | grep long  | grep T_HELLO_LONG`
echo "got: [$OUT]"
if [ "X$OUT" == "X" ]; then
    echo "No char T_HELLO_LONG found!"
    go_out 1
fi

echo "Tesging short field"
OUT=`xadmin pubfdb | grep 3001 | grep short | grep T_HELLO_SHORT`
echo "got: [$OUT]"
if [ "X$OUT" == "X" ]; then
    echo "No char T_HELLO_SHORT found!"
    go_out 1
fi

echo "Tesging string field"
OUT=`xadmin pubfdb | grep 3000 | grep 167775160 | grep string | grep T_HELLO_STR`
echo "got: [$OUT]"
if [ "X$OUT" == "X" ]; then
    echo "No char T_HELLO_STR found!"
    go_out 1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi


go_out $RET

