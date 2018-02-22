#!/bin/bash
## 
## @(#) Basic cache calls
##
## @file 01_run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
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

export TESTNAME="test048_cache"

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

export NDRX_CCONFIG=`pwd`
. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
export NDRX_TOUT=10
export NDRX_ULOG=$TESTDIR
export TESTDIR_SHM=$TESTDIR
#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
#    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

source ./test-func-include.sh

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y



    # If some alive stuff left...
    xadmin killall atmiclt48

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
(time ./01_atmiclt48 2>&1) > ./01_atmiclt.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt48 2>&1) > ./atmiclt-dom1.log

RET=$?

echo "Show cache... "

xadmin cs -d db01

TMP=$?

echo "xadmin ret $TMP"

if [ $TMP -ne 0 ]; then
    echo "xadmin failed"
    go_out 1
fi

ensure_keys db01 40


#
# Test with out -d...
#

xadmin cs db01

TMP=$?

echo "xadmin ret $TMP"

if [ $TMP -ne 0 ]; then
    echo "xadmin failed"
    go_out 2
fi


#
# Dump record
#
xadmin cd -d db01 -k SV1HELLO-1 -i

TMP=$?

echo "xadmin ret $TMP"

if [ $TMP -ne 0 ]; then
    echo "xadmin failed dump record failed"
    go_out 3
fi


#
# Delete (invalidate) by regexp, all keys containing 2
#

xadmin ci -d db01 -k '.*2.*' -r

TMP=$?

echo "xadmin ret $TMP"

if [ $TMP -ne 0 ]; then
    echo "xadmin failed dump record failed"
    go_out 4
fi

#
# Validate record count
#

echo "After delete: "

xadmin cs -d db01

ensure_keys db01 27

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out 5
fi


go_out $RET

