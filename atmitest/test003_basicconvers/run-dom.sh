#!/bin/bash
##
## @brief @(#) Test003 - clusterised version
##
## @file run-dom.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

export TESTNO="003"
export TESTNAME_SHORT="basicconvers"
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
export NDRX_TOUT=20

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
    xadmin killall atmiclt3

    popd 2>/dev/null
    exit $1
}

rm *dom*.log

set_dom1;
xadmin down -y
xadmin start -y || go_out 1
echo "PSVC DOM 1"
xadmin psvc

set_dom2;
xadmin down -y
xadmin start -y || go_out 2
echo "PSVC DOM 2"
xadmin psvc

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 60

# Go to domain 1
set_dom1;

echo "PSVC DOM 1"
xadmin psvc
echo "PPM DOM 1"
xadmin ppm
echo "PSC DOM 1"
xadmin psc

# Run the client test...
echo "Will issue client calls:"
(./atmiclt3 normal 2>&1) > ./atmiclt-dom1.log
RET=$?

if [ "X$RET" !=  "X0" ]; then
    echo "normal case failed"
    go_out -6
fi

# echo loop
(./atmiclt3 echoloop 2>&1) >> ./atmiclt3.log
RET=$?

if [ "X$RET" !=  "X0" ]; then
    echo "echoloop case failed"
    go_out -7
fi

echo "PSVC DOM 1 (AFTER RUN)"
xadmin psvc

set_dom2;
echo "PSVC DOM 2 (AFTER RUN)"
xadmin psvc

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
