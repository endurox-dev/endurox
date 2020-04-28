#!/bin/bash
##
## @brief Test Enduro/X server dispatch threading - test launcher
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

export TESTNAME="test075_dispthread"

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
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y



    # If some alive stuff left...
    xadmin killall atmiclt75

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm

echo "Test tpacall from tpsvrthrinit() - there shall be 6 calls"
PSC_OUT=`xadmin psc | grep MULTI_INIT | grep 6`

if [ "X$PSC_OUT" == "X" ]; then
    echo "MULTI_INIT / 6 not found!"
    go_out -1
fi


echo "Running conversational + notification to server daemon thread test..."

(./atmiclt75_conv 2>&1) > ./atmiclt-dom1_conv.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt75 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "atmiclt75_conv failed..."
    go_out $RET
fi

echo "Running off client"

set_dom1;
(./atmiclt75 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt75 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi


#
# Check the out start of daemons
#
echo "Check that daemons are booted..."

PSC_OUT=`xadmin psc | grep DMNSV1 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV1 / BUSY not found!"
    go_out -1
fi

PSC_OUT=`xadmin psc | grep DMNSV2 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV2 / BUSY not found!"
    go_out -2
fi

echo "**** stop DMN 1 ****"
#
# Stop the daemon server...
#
cat << EOF | ud
SRVCNM	DMNSV_CTL
T_STRING_FLD	stop
T_SHORT_FLD	1
EOF
if [ "X$RET" != "X0" ]; then
    echo "ud failed"
    go_out -101
fi

# let thread to finish..
sleep 1
xadmin psc

PSC_OUT=`xadmin psc | grep DMNSV1 | grep AVAIL`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV1 / AVAIL not found!"
    go_out -3
fi

PSC_OUT=`xadmin psc | grep DMNSV2 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV2 / BUSY not found!"
    go_out -4
fi

echo "****  stop DMN 2 ****"

cat << EOF | ud
SRVCNM	DMNSV_CTL
T_STRING_FLD	stop
T_SHORT_FLD	2
EOF

RET=$?

if [ "X$RET" != "X0" ]; then
    echo "ud failed"
    go_out -104
fi

# let thread to finish..
sleep 1
xadmin psc

PSC_OUT=`xadmin psc | grep DMNSV1 | grep AVAIL`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV1 / AVAIL not found!"
    go_out -5
fi

PSC_OUT=`xadmin psc | grep DMNSV2 | grep AVAIL`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV2 / AVAIL not found!"
    go_out -6
fi

echo "****  start DMN 1 ****"

cat << EOF | ud
SRVCNM	DMNSV_CTL
T_STRING_FLD	start
T_SHORT_FLD	1
EOF

RET=$?

if [ "X$RET" != "X0" ]; then
    echo "ud failed"
    go_out -106
fi

# let thread to start..
sleep 1
xadmin psc

PSC_OUT=`xadmin psc | grep DMNSV1 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV1 / BUSY not found!"
    go_out -7
fi

PSC_OUT=`xadmin psc | grep DMNSV2 | grep AVAIL`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV2 / AVAIL not found!"
    go_out -8
fi


echo "****  start DMN 2 ****"

cat << EOF | ud
SRVCNM	DMNSV_CTL
T_STRING_FLD	start
T_SHORT_FLD	2
EOF

RET=$?

if [ "X$RET" != "X0" ]; then
    echo "ud failed"
    go_out -108
fi

# let thread to start..
sleep 1
xadmin psc

PSC_OUT=`xadmin psc | grep DMNSV1 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV1 / BUSY not found!"
    go_out -9
fi

PSC_OUT=`xadmin psc | grep DMNSV2 | grep BUSY`

if [ "X$PSC_OUT" == "X" ]; then
    echo "DMNSV2 / BUSY not found!"
    go_out -10
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

