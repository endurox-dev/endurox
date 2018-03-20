#!/bin/bash
## 
## @(#) See README. Limited cache, recently used lives...
##
## @file 09_run_lru.sh
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
## Place, Suite 330, Boston, MA 02111-1309 USA
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

echo "Running off client"

(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY1"}' \
    -m '{"T_STRING_FLD":"KEY1"}' \
    -cY -n100 -fY 2>&1) > ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY2"}' \
    -m '{"T_STRING_FLD":"KEY2"}' \
    -cY -n100 -fY 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY3"}' \
    -m '{"T_STRING_FLD":"KEY3"}' \
    -cY -n100 -fY 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi


(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY4"}' \
    -m '{"T_STRING_FLD":"KEY4"}' \
    -cY -n100 -fY 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi


echo "Run key 1 again, should live in cache"
(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY1"}' \
    -m '{"T_STRING_FLD":"KEY1"}' \
    -cY -n100 -fN 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi


(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY5"}' \
    -m '{"T_STRING_FLD":"KEY5"}' \
    -cY -n100 -fY 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (6)"
    go_out 1
fi


(time ./testtool48 -sTESTSV09 -b '{"T_STRING_FLD":"KEY6"}' \
    -m '{"T_STRING_FLD":"KEY6"}' \
    -cY -n100 -fY 2>&1) >> ./09_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (7)"
    go_out 1
fi



echo "wait for tpcached to complete scanning... (every 5 sec)"

sleep 7

echo "There must be 5 keys"
ensure_keys db09 5

echo "There check they keys, should be 1,3,4,5,6"

xadmin cs db09

ensure_field db09 SV9KEY1 T_STRING_FLD KEY1 1
ensure_field db09 SV9KEY2 T_STRING_FLD KEY2 0
ensure_field db09 SV9KEY3 T_STRING_FLD KEY3 1
ensure_field db09 SV9KEY4 T_STRING_FLD KEY4 1
ensure_field db09 SV9KEY5 T_STRING_FLD KEY5 1
ensure_field db09 SV9KEY6 T_STRING_FLD KEY6 1



go_out $RET

