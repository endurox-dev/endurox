#!/bin/bash
## 
## @brief @(#) See README. Run refresh rule
##
## @file 06_run_bootreset.sh
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


echo "Running off client (warm up the caches)"

echo "Clean up databases..."

xadmin ci -d db06_1

if [ $? -ne 0 ]; then
    echo "xadmin ci -d db06_1 failed (1)"
    go_out -1
fi

xadmin ci -d db06_2

if [ $? -ne 0 ]; then
    echo "xadmin ci -d db06_2 failed (2)"
    go_out -2
fi


#
# First db
#
(time ./testtool48 -sTESTSV06 -b '{"T_STRING_FLD":"KEY1","T_STRING_2_FLD":"CACHE1"}' \
    -m '{"T_STRING_FLD":"KEY1"}' \
    -cY -n100 -fY 2>&1) > ./06_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 1
fi

#
# Second db
#
(time ./testtool48 -sTESTSV06 -b '{"T_STRING_FLD":"KEY2","T_STRING_2_FLD":"CACHE2"}' \
    -m '{"T_STRING_FLD":"KEY2"}' \
    -cY -n100 -fY 2>&1) >> ./06_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 1
fi

ensure_keys db06_1 1
ensure_keys db06_2 1

xadmin r -s tpcachebtsv

echo "Ensure that no cache changes are made for single binary restart (not full boot)"
ensure_keys db06_1 1
ensure_keys db06_2 1


#
# Restart 
#
xadmin stop -y
xadmin start -y

ensure_keys db06_1 1

#
# Boot reset will clear this...
#
ensure_keys db06_2 0


go_out $RET
