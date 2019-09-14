#!/bin/bash
##
## @brief Buffer type switch + null - test launcher
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

export TESTNAME="test064_bufswitch"

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
    xadmin killall atmiclt64

    popd 2>/dev/null
    exit $1
}

#
# Generic exit function
#
function validate_type {

    xadmin stop -s atmi.sv64
    
    # Catch is there is test error!!!
    if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        go_out -4
    fi

    xadmin start -s atmi.sv64
}


rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

export VIEWDIR=.
export VIEWFILES=t64.V
set_dom1;
xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;

echo "NULL Tests..."
(./atmiclt64 NULL 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;

echo "JSON Tests..."
(./atmiclt64 JSON 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;

echo "STRING Tests..."
(./atmiclt64 STRING 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;


echo "CARRAY Tests..."
(./atmiclt64 CARRAY 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;


echo "VIEW Tests..."
(./atmiclt64 VIEW 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;

echo "UBF Tests..."
(./atmiclt64 UBF 2>&1) > ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

validate_type;


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi


go_out $RET


# vim: set ts=4 sw=4 et smartindent:
