#!/bin/bash
##
## @brief @(#) See README. Domain duplicate records
##
## @file 14_run_domdups.sh
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

source ./test-func-include.sh

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export TESTDIR_DB=$TESTDIR
    export TESTDIR_SHM=$TESTDIR
    export NDRX_CCTAG=dom1
}

set_dom2() {
    echo "Setting domain 2"
    . ../dom2.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export TESTDIR_DB=$TESTDIR/dom2
    export TESTDIR_SHM=$TESTDIR/dom2
    export NDRX_CCTAG=dom2
}

set_dom3() {
    echo "Setting domain 3"
    . ../dom3.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom3.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom3.log
    export NDRX_LOG=$TESTDIR/ndrx-dom3.log
    export TESTDIR_DB=$TESTDIR/dom3
    export TESTDIR_SHM=$TESTDIR/dom3
    export NDRX_CCTAG=dom3
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

    set_dom3;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt48

    popd 2>/dev/null
    exit $1
}

rm *.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

echo "Booting domain 1"
set_dom1;
xadmin down -y
xadmin start -y || go_out 1

echo "Booting domain 2"
set_dom2;
xadmin down -y
xadmin start -y || go_out 1

echo "Booting domain 3"
set_dom3;
xadmin down -y
xadmin start -y || go_out 1

echo "Let clients to boot & links to establish..."
sleep 60

RET=0

echo "Domain 1 info"
set_dom1;
xadmin psc
xadmin ppm
xadmin sc -t CACHED
xadmin pc

echo "Domain 2 info"
set_dom2;
xadmin psc
xadmin ppm
xadmin sc -t CACHED
xadmin pc

echo "Domain 3 info"
set_dom3;
xadmin psc
xadmin ppm
xadmin sc -t CACHED
xadmin pc


echo "Running off client on domain 1"
set_dom1;

(time ./testtool48 -sTESTSV14 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1"}' \
    -cY -n1 -fY -d 2>&1) > ./14_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

sleep 5

echo "Running off client on domain 2"
set_dom2;


(time ./testtool48 -sTESTSV14 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -cY -n1 -fY -d 2>&1) >> ./14_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi

#
# Also output on both caches must be equal
#

echo "Printing dom 1 keys"
set_dom1;
xadmin cs db14
ensure_keys db14 2

CS1=`xadmin cs db14`

echo "Printing dom 2 keys"
set_dom2;
xadmin cs db14
ensure_keys db14 2
CS2=`xadmin cs db14`

if [[ "X$CS1" != "X$CS2" ]]; then

        echo "Cache1 must be equal to cache2, but got [$CS1] vs [$CS2]"
        go_out 2
fi

echo "Run second time with cached results, should clean up the db..."


set_dom1;

(time ./testtool48 -sTESTSV14 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -cY -n100 -fN 2>&1) >> ./14_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi

ensure_keys db14 1

CS1=`xadmin cs db14`


echo "Local cleanup shall not be broadcasted"
set_dom2;
ensure_keys db14 2

(time ./testtool48 -sTESTSV14 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -cY -n100 -fN 2>&1) >> ./14_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

ensure_keys db14 1

CS2=`xadmin cs db14`


if [[ "X$CS1" != "X$CS2" ]]; then

        echo "(2) Cache1 must be equal to cache2, but got [$CS1] vs [$CS2]"
        go_out 2
fi

echo "Now test with duplicate removal of tpcached - domain 3 shall have two keys now"

set_dom3;

xadmin cs db14

ensure_keys db14 2

echo "starting tpcached..."

xadmin bc -t CACHED

echo "waiting for duplicate scan... 10 sec"

sleep 10

ensure_keys db14 1

(time ./testtool48 -sTESTSV14 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM2"}' \
    -cY -n100 -fN 2>&1) >> ./14_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi



go_out $RET



# vim: set ts=4 sw=4 et smartindent:
