#!/bin/bash
## 
## @brief @(#) See README. Limited cache, records with more hits live longer, test linux shm
##
## @file 10_run_hits.sh
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
export TESTDIR_DB=$TESTDIR
export TESTDIR_SHM=$TESTDIR

source ./test-func-include.sh

if [ -d "/dev/shm" ]; then

    echo "Preparing ramdrive..."

    mkdir -rf /dev/shm/benchmark 2>/dev/null
    mkdir /dev/shm/benchmark

    mkdir -rf /dev/shm/db10 2>/dev/null
    mkdir /dev/shm/db10

    export TESTDIR_SHM="/dev/shm"
fi

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_CCTAG=dom1
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
    xadmin killall atmiclt48

    popd 2>/dev/null
    exit $1
}

rm *.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

#
# Let clients to boot
#
sleep 5

RET=0

set_dom1;
xadmin psc
xadmin ppm
xadmin pc


#
# Stop th daemon so that it does not interfere with in-progress calls
#
xadmin sc -t CACHED

echo "Running off client"

(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY1"}' \
    -m '{"T_STRING_FLD":"KEY1"}' \
    -cY -n100 -fY 2>&1) > ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY2"}' \
    -m '{"T_STRING_FLD":"KEY2"}' \
    -cY -n91 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY3"}' \
    -m '{"T_STRING_FLD":"KEY3"}' \
    -cY -n92 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY4"}' \
    -m '{"T_STRING_FLD":"KEY4"}' \
    -cY -n93 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY5"}' \
    -m '{"T_STRING_FLD":"KEY5"}' \
    -cY -n94 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY6"}' \
    -m '{"T_STRING_FLD":"KEY6"}' \
    -cY -n95 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (6)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY7"}' \
    -m '{"T_STRING_FLD":"KEY7"}' \
    -cY -n96 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (7)"
    go_out 1
fi

(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY8"}' \
    -m '{"T_STRING_FLD":"KEY8"}' \
    -cY -n97 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (8)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY9"}' \
    -m '{"T_STRING_FLD":"KEY9"}' \
    -cY -n98 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (9)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY10"}' \
    -m '{"T_STRING_FLD":"KEY10"}' \
    -cY -n99 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (10)"
    go_out 1
fi


(time ./testtool48 -sTESTSV10 -b '{"T_STRING_FLD":"KEY11"}' \
    -m '{"T_STRING_FLD":"KEY11"}' \
    -cY -n100 -fY 2>&1) >> ./10_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (11)"
    go_out 1
fi

xadmin bc -t CACHED

echo "let client to boot..."
sleep 5

echo "wait for tpcached to complete scanning... (every 5 sec)"

sleep 7

echo "There must be 5 keys"
ensure_keys db10 5

xadmin cs db10

ensure_field db10 SV10KEY11 T_STRING_FLD KEY11 1
ensure_field db10 SV10KEY10 T_STRING_FLD KEY10 1
ensure_field db10 SV10KEY9 T_STRING_FLD KEY9 1
ensure_field db10 SV10KEY8 T_STRING_FLD KEY8 1
ensure_field db10 SV10KEY7 T_STRING_FLD KEY7 0
ensure_field db10 SV10KEY6 T_STRING_FLD KEY6 0
ensure_field db10 SV10KEY5 T_STRING_FLD KEY5 0
ensure_field db10 SV10KEY4 T_STRING_FLD KEY4 0
ensure_field db10 SV10KEY3 T_STRING_FLD KEY3 0
ensure_field db10 SV10KEY2 T_STRING_FLD KEY2 0
ensure_field db10 SV10KEY1 T_STRING_FLD KEY1 1

go_out $RET
