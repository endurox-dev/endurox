#!/bin/bash
## 
## @brief @(#) See README. Test record expiry
##
## @file 04_run.sh
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

#
# First record
#
(time ./testtool48 -sTESTSV04 -b '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -m '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -cY -n100 -fY 2>&1) > ./04_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

#
# Existing record
#
(time ./testtool48 -sTESTSV04 -b '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -m '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -cY -n100 -fN 2>&1) > ./04_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 1
fi


#
# First record, 2
#
(time ./testtool48 -sTESTSV04 -b '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -m '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -cY -n100 -fY 2>&1) >> ./04_testtool48.log


if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 3
fi

#
# Existing record, 2
#
(time ./testtool48 -sTESTSV04 -b '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -m '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -cY -n100 -fN 2>&1) >> ./04_testtool48.log


if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 3
fi


#
# Must have some 2 records
#
xadmin cs -d db04

if [ $? -ne 0 ]; then
    echo "xadmin cs failed"
    go_out 2
fi

ensure_keys db04 2

echo "Waiting for records to expire... (40s)"
sleep 40

#
# now we shall get cached results...
#

ensure_keys db04 0

echo "Cache after sleep (expiry)"

#
# First record
#
(time ./testtool48 -sTESTSV04 -b '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -m '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -cY -n100 -fY 2>&1) > ./04_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi

go_out $RET

