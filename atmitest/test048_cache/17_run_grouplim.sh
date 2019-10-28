#!/bin/bash
##
## @brief @(#) See README. Keygroup tests, limited by keys and by group..
##
## @file 17_run_grouplim.sh
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
#xadmin sc -t CACHED
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

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"1"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"1"}' \
    -cY -n50 -fY 2>&1) > ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"2"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"4"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"5"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"5"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (5)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"6"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"6"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (6)"
    go_out 1
fi

echo "Letting tpcached to zap the keys..."
sleep 7

echo "DOM 1 testing"
xadmin cs k@db17
ensure_keys k@db17 5

ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV171 0
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV172 1
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV176 1


echo "DOM 2 testing"
set_dom2;
xadmin cs k@db17
ensure_keys k@db17 5

ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV171 0
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV172 1
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV176 1

echo "DOM 3 testing"
set_dom3;
xadmin cs k@db17
ensure_keys k@db17 5

ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV171 0
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV172 1
ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV176 1

echo "Testing group fifo zapping..."
set_dom1;


(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (7)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY3","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY3","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (8)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY4","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY4","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (9)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY5","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY5","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (10)"
    go_out 1
fi

(time ./testtool48 -sTESTSV17 -b '{"T_STRING_FLD":"KEY6","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -m '{"T_STRING_FLD":"KEY6","T_STRING_2_FLD":"DOM1","T_SHORT_FLD":"3"}' \
    -cY -n50 -fY 2>&1) >> ./17_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (10)"
    go_out 1
fi


echo "Letting tpcached to zap the keys... (2)"
sleep 7


echo "OK so first group must be zapped, we have 5x groups now and 5x keys in total"

echo "DOM 1 testing"
xadmin cs k@db17
xadmin cs g@db17

ensure_keys k@db17 5
ensure_keys g@db17 5

ensure_field g@db17 SV17KEY1 EX_CACHE_OPEXPR SV17KEY1-SV173 0
ensure_field g@db17 SV17KEY2 EX_CACHE_OPEXPR SV17KEY2-SV173 1
ensure_field g@db17 SV17KEY3 EX_CACHE_OPEXPR SV17KEY3-SV173 1
ensure_field g@db17 SV17KEY4 EX_CACHE_OPEXPR SV17KEY4-SV173 1
ensure_field g@db17 SV17KEY5 EX_CACHE_OPEXPR SV17KEY5-SV173 1
ensure_field g@db17 SV17KEY6 EX_CACHE_OPEXPR SV17KEY6-SV173 1
ensure_field g@db17 SV17KEY6 EX_CACHE_OPEXPR SV17KEY7-SV173 0

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
