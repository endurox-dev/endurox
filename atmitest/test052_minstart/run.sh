#!/bin/bash
##
## @brief @(#) Test Bug #306 - xadmin start -s should start only min number servers - test launcher
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

export TESTNAME="test052_minstart"

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
    xadmin killall atmiclt52

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
echo "Running off client"

set_dom1;
#(./atmiclt52 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt52 2>&1) > ./atmiclt-dom1.log

echo "Testing service stop..."
xadmin stop -s atmi.sv52

CNT=0

if [ "$(uname)" == "FreeBSD" ]; then
    CNT=`ps -auwwx | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
else
    CNT=`ps -ef | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
fi

echo "Process count: $CNT"
if [[ $CNT -ne 0 ]]; then
	echo "Service count != 0! (1)"
	go_out 1        
fi


echo "Testing service start..."

xadmin start -s atmi.sv52

if [ "$(uname)" == "FreeBSD" ]; then
    CNT=`ps -auwwx | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
else
    CNT=`ps -ef | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
fi

echo "Process count: $CNT"
if [[ $CNT -ne 2 ]]; then
	echo "Service count != 2! (2)"
	go_out 1        
fi


echo "Testing service start -i..."
xadmin start -i 12

if [ "$(uname)" == "FreeBSD" ]; then
    CNT=`ps -auwwx | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
else
    CNT=`ps -ef | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
fi

echo "Process count: $CNT"
if [[ $CNT -ne 3 ]]; then
	echo "Service count != 3! (3)"
	go_out 1        
fi


echo "Testing service stop -s..."

xadmin stop -s atmi.sv52

if [ "$(uname)" == "FreeBSD" ]; then
    CNT=`ps -auwwx | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
else
    CNT=`ps -ef | grep atmi.sv52 | grep -v grep | wc | awk '{print $1}'`;
fi

echo "Process count: $CNT"
if [[ $CNT -ne 0 ]]; then
	echo "Service count != 0! (3)"
	go_out 1        
fi

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi


go_out $RET

# vim: set ts=4 sw=4 et smartindent:
