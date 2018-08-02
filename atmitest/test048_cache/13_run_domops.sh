#!/bin/bash
## 
## @brief @(#) See README. Basic domain operations
##
## @file 13_run_domops.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
xadmin pc

echo "Domain 2 info"
set_dom2;
xadmin psc
xadmin ppm
xadmin pc

echo "Domain 3 info"
set_dom3;
xadmin psc
xadmin ppm
xadmin pc


echo "Running off client on domain 1"

set_dom1;

(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY1"}' \
    -m '{"T_STRING_FLD":"KEY1"}' \
    -cY -n100 -fY 2>&1) > ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY2"}' \
    -m '{"T_STRING_FLD":"KEY2"}' \
    -cY -n91 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY3"}' \
    -m '{"T_STRING_FLD":"KEY3"}' \
    -cY -n92 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY4"}' \
    -m '{"T_STRING_FLD":"KEY4"}' \
    -cY -n93 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY5"}' \
    -m '{"T_STRING_FLD":"KEY5"}' \
    -cY -n94 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY6"}' \
    -m '{"T_STRING_FLD":"KEY6"}' \
    -cY -n95 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (6)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY7"}' \
    -m '{"T_STRING_FLD":"KEY7"}' \
    -cY -n96 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (7)"
    go_out 1
fi

(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY8"}' \
    -m '{"T_STRING_FLD":"KEY8"}' \
    -cY -n97 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (8)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY9"}' \
    -m '{"T_STRING_FLD":"KEY9"}' \
    -cY -n98 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (9)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY10"}' \
    -m '{"T_STRING_FLD":"KEY10"}' \
    -cY -n99 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (10)"
    go_out 1
fi


(time ./testtool48 -sTESTSV13 -b '{"T_STRING_FLD":"KEY11"}' \
    -m '{"T_STRING_FLD":"KEY11"}' \
    -cY -n100 -fY 2>&1) >> ./13_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (11)"
    go_out 1
fi

echo "Checking keys in domain 1..."
set_dom1;

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 1
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 1
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1
ensure_field db13 SV13KEY8 T_STRING_FLD KEY8 1
ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 1
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1
ensure_field db13 SV13KEY4 T_STRING_FLD KEY4 1
ensure_field db13 SV13KEY3 T_STRING_FLD KEY3 1
ensure_field db13 SV13KEY2 T_STRING_FLD KEY2 1
ensure_field db13 SV13KEY1 T_STRING_FLD KEY1 1

#
# Let events to process if any queues..
#
sleep 5

echo "Checking keys in domain 2..."
set_dom2;

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 1
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 1
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1
ensure_field db13 SV13KEY8 T_STRING_FLD KEY8 1
ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 1
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1
ensure_field db13 SV13KEY4 T_STRING_FLD KEY4 1
ensure_field db13 SV13KEY3 T_STRING_FLD KEY3 1
ensure_field db13 SV13KEY2 T_STRING_FLD KEY2 1
ensure_field db13 SV13KEY1 T_STRING_FLD KEY1 1

echo "Checking keys in domain 3..."
set_dom3;

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 1
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 1
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1
ensure_field db13 SV13KEY8 T_STRING_FLD KEY8 1
ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 1
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1
ensure_field db13 SV13KEY4 T_STRING_FLD KEY4 1
ensure_field db13 SV13KEY3 T_STRING_FLD KEY3 1
ensure_field db13 SV13KEY2 T_STRING_FLD KEY2 1
ensure_field db13 SV13KEY1 T_STRING_FLD KEY1 1


echo "*************************************************************************"
echo "Testing delete broadcast... (single record)"
echo "*************************************************************************"

echo "Domain 1"
set_dom1;

xadmin ci -d db13 -k SV13KEY6

ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 0
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1

echo "Domain 2"
set_dom2;

ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 0
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1

echo "Domain 3"
set_dom3;

ensure_field db13 SV13KEY7 T_STRING_FLD KEY7 1
ensure_field db13 SV13KEY6 T_STRING_FLD KEY6 0
ensure_field db13 SV13KEY5 T_STRING_FLD KEY5 1


echo "*************************************************************************"
echo "Testing delete broadcast... (regexp)"
echo "*************************************************************************"

echo "Domain 1"
set_dom1;

xadmin ci -d db13 -k SV13KEY1. -r

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 0
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 0
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1

echo "Domain 2"
set_dom2;

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 0
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 0
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1

echo "Domain 3"
set_dom3;

ensure_field db13 SV13KEY11 T_STRING_FLD KEY11 0
ensure_field db13 SV13KEY10 T_STRING_FLD KEY10 0
ensure_field db13 SV13KEY9 T_STRING_FLD KEY9 1



go_out $RET

