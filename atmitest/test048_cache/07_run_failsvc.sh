#!/bin/bash
##
## @brief @(#) See README. Service failure result cache
##
## @file 07_run_failsvc.sh
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

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

#
# Let clients to boot
#
sleep 2

RET=0

set_dom1;
xadmin psc
xadmin ppm
xadmin pc


echo "Running off client"

echo "First db, this shall go to fail cache"

(time ./testtool48 -sTESTSV07 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"CACHE1"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"CACHE1"}' \
    -cY -n100 -fY -r0 -e11 2>&1) > ./07_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

ensure_keys db07_1 1


echo "Second call, succeed, does not go to cache"

(time ./testtool48 -sTESTSV07 -b '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"CACHE1","T_LONG_3_FLD":1}' \
    -m '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"CACHE1","T_LONG_3_FLD":1}' \
    -cN -n100 -fY 2>&1) >> ./07_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi

ensure_keys db07_1 1


echo "OK now put two results in cache 2"

(time ./testtool48 -sTESTSV07 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"CACHE2"}' \
    -m '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"CACHE2"}' \
    -cY -n100 -fY -r0 -e11 2>&1) >> ./07_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi

ensure_keys db07_2 1

(time ./testtool48 -sTESTSV07 -b '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"CACHE2","T_LONG_3_FLD":1}' \
    -m '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"CACHE2","T_LONG_3_FLD":1}' \
    -cY -n100 -fY 2>&1) >> ./07_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

ensure_keys db07_2 2

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
