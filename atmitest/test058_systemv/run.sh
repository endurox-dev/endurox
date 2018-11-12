#!/bin/bash
##
## @brief System V polling tests - test launcher
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

export TESTNAME="test058_systemv"

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

export NDRX_TOUT=20

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
    xadmin killall atmiclt58

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
xadmin psvc
#
# Check psvc that there is two queues according to regexp map
#
RES=`xadmin psvc | egrep 'RES\(2\): [0-9]+\(2\) [0-9]+\(2\)'`

if [ "X$RES" == "X" ]; then
    echo "Invalid service count (1)!"
    go_out -10
fi

#
# Stop one service the map should be changed
#
xadmin stop -i 10

RES=`xadmin psvc | egrep 'RES\(2\): [0-9]+\(1\) [0-9]+\(2\)'`

if [ "X$RES" == "X" ]; then
    echo "Invalid service count (2)!"
    go_out -11
fi


#
# Stop another service from rqaddr map should be changed once again
#

xadmin stop -i 11

RES=`xadmin psvc | egrep 'RES\(1\): [0-9]+\(2\) '`

if [ "X$RES" == "X" ]; then
    echo "Invalid service count (3)!"
    go_out -12
fi


#
# start servers back, services should be back as in first test
#

xadmin start -y

RES=`xadmin psvc | egrep 'RES\(2\): [0-9]+\(2\) [0-9]+\(2\)'`

if [ "X$RES" == "X" ]; then
    echo "Invalid service count (4)!"
    go_out -13
fi

echo "Running off client"

set_dom1;

# Test scenario for round robins
# do some call
(./atmiclt58 GpriQQMLVdaBg 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt58 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi

#
# Above performs 4x calls to sleepy services, thus with round robin all
# of the must got the 1x calls, now validate it.
#
echo "Waiting for servers to finish..."
sleep 6

xadmin psc

RES=`xadmin psc | egrep "atmi\.sv58[ \\t\\r\\n\\v\\f]+10[ \\t\\r\\n\\v\\f]+1[ \\t\\r\\n\\v\\f]+0"`

if [ "X$RES" == "X" ]; then
    echo "Processed entries missing (5)!"
    go_out -14
fi

RES=`xadmin psc | egrep "atmi\.sv58[ \\t\\r\\n\\v\\f]+11[ \\t\\r\\n\\v\\f]+1[ \\t\\r\\n\\v\\f]+0"`

if [ "X$RES" == "X" ]; then
    echo "Processed entries missing (6)!"
    go_out -14
fi

RES=`xadmin psc | egrep "atmi\.sv58[ \\t\\r\\n\\v\\f]+100[ \\t\\r\\n\\v\\f]+1[ \\t\\r\\n\\v\\f]+0"`

if [ "X$RES" == "X" ]; then
    echo "Processed entries missing (7)!"
    go_out -15
fi

RES=`xadmin psc | egrep "atmi\.sv58[ \\t\\r\\n\\v\\f]+101[ \\t\\r\\n\\v\\f]+1[ \\t\\r\\n\\v\\f]+0"`

if [ "X$RES" == "X" ]; then
    echo "Processed entries missing (8)!"
    go_out -15
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

