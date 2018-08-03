#!/bin/bash
##
## @brief @(#) Bug #236 - two pass config (global section reference to global) and process cfg reload - test launcher
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

export TESTNAME="test046_twopasscfg"

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

# We will make some tests with config
export NDRX_CCONFIG=$TESTDIR
export NDRX_CCTAG="2/A"

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
    xadmin killall atmiclt46

    popd 2>/dev/null
    exit $1
}

set_dom1;

rm *.log

# Any bridges that are live must be killed!
xadmin killall tpbridge
xadmin down -y


#
# Make some configuration...
# So basically if we resolve by xadmin pe or by process then variables shall
# be fully resolved
#

echo "[@global]" > $NDRX_CCONFIG/app.ini
echo "VTEST1=HELLO" >> $NDRX_CCONFIG/app.ini

#
# There should be this log open because [@debug] also can reference globals
#
echo "TEST_DEBUG_F=common.log" >> $NDRX_CCONFIG/app.ini
#
# This shall be resolved as "HELLO WORLD"
#
echo 'VTEST2=${VTEST1} WORLD' >> $NDRX_CCONFIG/app.ini
#
# This shall be resolved as "Enduro/X is middleware"
#
echo 'VTEST3=Enduro/X is ${VTEST4}' >> $NDRX_CCONFIG/app.ini

echo "[@global/1]" >> $NDRX_CCONFIG/app.ini
echo "VTEST4=app server" >> $NDRX_CCONFIG/app.ini

echo "[@global/2]" >> $NDRX_CCONFIG/app.ini
echo "VTEST4=middleware" >> $NDRX_CCONFIG/app.ini

xadmin start -y || go_out 1

#
# Test the variables
#

xadmin pe

VTEST2=`xadmin pe | grep VTEST2`

echo "echo got VTEST2=[$VTEST2]"

if [ "X$VTEST2" != "XVTEST2=HELLO WORLD" ]; then
    echo "Expected [VTEST2=HELLO WORLD] but got [$VTEST2]"
    go_out -1
fi


VTEST3=`xadmin pe | grep VTEST3`

echo "echo got VTEST3=[$VTEST3]"

if [ "X$VTEST3" != "XVTEST3=Enduro/X is middleware" ]; then
    echo "Expected [VTEST3=Enduro/X is middleware] but got [$VTEST3]"
    go_out -2
fi


#
# Test what variables sees ATMI46 server
#

SVCOUT=`cat << EOF | ud | grep T_STRING_FLD | cut -f2
SRVCNM	TESTSV
EOF
`

echo "Service out: [$SVCOUT]"

if [ "X$SVCOUT" !=  "XEnduro/X is middleware" ]; then
    echo "Invalid output from service: [$SVCOUT]"
    go_out  -3
fi

echo "Update config and check that we have new value"


echo "[@global/A]" >> $NDRX_CCONFIG/app.ini
echo "VTEST3=this is changed" >> $NDRX_CCONFIG/app.ini

xadmin sr -s atmi.sv46

#
# Update config global section
# and check what server sees now. The values must be changed..
#

SVCOUT=`cat << EOF | ud | grep T_STRING_FLD | cut -f2
SRVCNM	TESTSV
EOF
`

echo "Service out: [$SVCOUT]"

if [ "X$SVCOUT" !=  "Xthis is changed" ]; then
    echo "Invalid output from service: [$SVCOUT]"
    go_out  -4
fi

RET=0

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi


go_out $RET

