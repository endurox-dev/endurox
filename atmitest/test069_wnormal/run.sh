#!/bin/bash
##
## @brief Wait for ndrxd normal state - test launcher
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

export TESTNAME="test069_wnormal"

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
export NDRX_SILENT=Y
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
# Do the lengthy shutdown
#
function back_stop {
     
    echo "Run shutdown procedure"
    xadmin start -y
    xadmin stop -y

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
    xadmin killall atmiclt69

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y & 

echo "let ndrxd to boot..."
sleep 2

echo "Start wait too short - fail"
export NDRX_NORMWAITMAX=5
xadmin start -y 
RET=$?
if [[ "X$RET" == "X0" ]]; then
    echo "Startup must be failed..."
    go_out -2
fi


echo "Start wait enough - ok"
export NDRX_NORMWAITMAX=20
xadmin start -y 
RET=$?
if [[ "X$RET" != "X0" ]]; then
    echo "Startup must be ok..."
    go_out -3
fi

#
# Run the full shutdown in background
#
echo "Wait for full shutdown..."
export NDRX_NORMWAITMAX=22
xadmin stop -y

echo "Start background..."
# OK lets run the background start + shutdown...
(back_stop) &
sleep 2

#
# Too short..
#
echo "Shutdown wait too short, exit fail..."
export NDRX_NORMWAITMAX=5
xadmin stop -y 
RET=$?

if [[ "X$RET" == "X0" ]]; then
    echo "Stop must fail because attempts exceeded"
    go_out -4
fi

#
# Now wait for pid...
#
echo "Forced stop, will wait until pid exits..."
export NDRX_NORMWAITMAX=1
xadmin stop -y -f
RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Forced stop must be OK."
    go_out -5
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent: