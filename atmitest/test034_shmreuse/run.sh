#!/bin/bash
##
## @brief @(#) Test034 - test the config server
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

export TESTNO="034"
export TESTNAME_SHORT="shmreuse"
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
# Override timeout!
export NDRX_TOUT=10
export NDRX_SILENT=Y
export NDRX_SVCMAX=45

#
# Domain 1 - here client will live
#
function set_dom1 {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd.log
    export NDRX_LOG=$TESTDIR/ndrx.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug.conf
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
    xadmin killall atmiclt34

    popd 2>/dev/null
    exit $1
}

rm *.log

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

xadmin psc
xadmin psvc
xadmin ppm

echo "Run readv"
(./atmiclt34 readv 2>&1) > ./atmiclt.log
RETTMP=$?

if [ "X$RETTMP" != "X0" ]; then

    echo "*** PSVC ***"
    xadmin psvc

    echo "***PSC***"
    xadmin psc

    go_out -1
fi

xadmin stop -y
xadmin down -y
xadmin start -y || go_out 1

echo "Run chkfull 45 - shm limit"
(./atmiclt34 chkfull 45 2>&1) > ./atmiclt_chkfull45.log
RETTMP=$?

if [ "X$RETTMP" != "X0" ]; then

    echo "*** PSVC ***"
    xadmin psvc

    echo "***PSC***"
    xadmin psc

    go_out -2
fi

xadmin stop -y

#
# Note 2 queues are reserved for admin and reply q.
#
echo "Run chkfull 48 - per svc limit"
export NDRX_SVCMAX=100

xadmin down -y
xadmin start -y || go_out 1

(./atmiclt34 chkfull 48 2>&1) > ./atmiclt_chkfull48.log
RETTMP=$?

if [ "X$RETTMP" != "X0" ]; then

    echo "*** PSVC ***"
    xadmin psvc

    echo "***PSC***"
    xadmin psc

    go_out -3
fi

echo "Direct SHM tests.."
xadmin stop -y
xadmin down -y

export NDRX_SVCMAX=200
xadmin idle
xadmin ldcf
xadmin appconfig sanity 99999
sleep 2

(./atmiclt34_2 2>&1) > ./atmiclt_2.log
RETTMP=$?

if [ "X$RETTMP" != "X0" ]; then

    echo "*** PSVC ***"
    xadmin psvc

    echo "***PSC***"
    xadmin psc

    go_out -4
fi

echo "Testing static advertise limits"

xadmin stop -y
xadmin down -y

export NDRX_SVCMAX=20
xadmin idle
xadmin ldcf
xadmin start -i 201

echo "***PSC After startup***"
xadmin psc

echo "***PSVC After startup (shall miss services above 19) ***"
xadmin psvc

echo "***pqa -a After startup***"
xadmin pqa -a


echo "Check that we can call some service..."
(./atmiclt34 call19 2>&1) > ./atmiclt_call19.log
RETTMP=$?

if [ "X$RETTMP" != "X0" ]; then

    echo "*** PSVC ***"
    xadmin psvc

    echo "***PSC***"
    xadmin psc

    go_out -3
fi

# stop the service and see queues...
xadmin stop -i 201

echo "***PSC After shutdown***"
xadmin psc

echo "***PSVC After shutdown ***"
xadmin psvc

echo "***pqa -a After shutdown***"
xadmin pqa -a


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out -99
fi

go_out 0

# vim: set ts=4 sw=4 et smartindent:
