#!/bin/bash
## 
## @brief @(#) See README. Keygroup tests
##
## @file 16_run_groupexp.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
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

(time ./testtool48 -sTESTSV16 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"1"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"1"}' \
    -cY -n50 -fY 2>&1) > ./16_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

(time ./testtool48 -sTESTSV16 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"2"}' \
    -cY -n50 -fY 2>&1) >> ./16_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


(time ./testtool48 -sTESTSV16 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./16_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi

(time ./testtool48 -sTESTSV16 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -cY -n50 -fY 2>&1) >> ./16_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

echo "sleeping 110 for next record"
sleep 110

(time ./testtool48 -sTESTSV16 -b '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -m '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -cY -n50 -fY 2>&1) >> ./16_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi

echo "List group..."
xadmin cs k@db16
ensure_keys g@db16 2
ensure_keys k@db16 5

xadmin bc -t CACHED
echo "Sleep 11, wait for cached to kill the records"
sleep 11

echo "Testing DOM 1"

ensure_keys g@db16 1
ensure_keys k@db16 1

xadmin cs g@db16
xadmin cs k@db16

xadmin cd -d g@db16 -k SV16KEY1 -i

ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV161 0
ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV164 0
ensure_field g@db16 SV16KEY2 EX_CACHE_OPEXPR SV16KEY2-SV164 1


echo "Testing DOM 2"
set_dom2;

ensure_keys g@db16 1
ensure_keys k@db16 1

xadmin cs g@db16
xadmin cs k@db16

xadmin cd -d g@db16 -k SV16KEY1 -i

ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV161 0
ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV164 0
ensure_field g@db16 SV16KEY2 EX_CACHE_OPEXPR SV16KEY2-SV164 1

echo "Testing DOM 3"
set_dom3;

ensure_keys g@db16 1
ensure_keys k@db16 1

xadmin cs g@db16
xadmin cs k@db16

xadmin cd -d g@db16 -k SV16KEY1 -i

ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV161 0
ensure_field g@db16 SV16KEY1 EX_CACHE_OPEXPR SV16KEY1-SV164 0
ensure_field g@db16 SV16KEY2 EX_CACHE_OPEXPR SV16KEY2-SV164 1

go_out $RET

