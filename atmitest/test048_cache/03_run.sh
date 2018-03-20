#!/bin/bash
## 
## @(#) See README. Service cached results must be removed after server shutdown.
##
## @file 03_run.sh
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



#
# Test cache dump (cd) command when record does not exists, we should get error
# something like .*not.*
#

xadmin cd -d db03 -k NON_EXISTING_KEY -i

OUT=`(xadmin cd -d db03 -k NON_EXISTING_KEY -i 2>&1) | grep EDB_NOTFOUND`

if [[ "X$OUT" == "X" ]]; then
    echo "Missing EDB_NOTFOUND in dump invalid key output...!"
    go_out -1
fi

echo "Running off client"

(time ./testtool48 -sTESTSV03 -b '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -m '{"T_STRING_FLD":"KEY1","T_SHORT_FLD":123,"T_CHAR_FLD":"A","T_DOUBLE_FLD":4.4}' \
    -cY -n100 -fY 2>&1) > ./03_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (1)"
    go_out 1
fi


echo "Show cache... "

xadmin cs -d db03

if [ $? -ne 0 ]; then
    echo "xadmin cs failed"
    go_out 2
fi

echo "ensure 1"
ensure_keys db03 1

(time ./testtool48 -sTESTSV03 -b '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -m '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.2","T_SHORT_FLD":44,"T_CHAR_FLD":"B"}' \
    -cY -n100 -fY 2>&1) >> ./03_testtool48.log


if [ $? -ne 0 ]; then
    echo "testtool48 failed (2)"
    go_out 3
fi


xadmin cd -d db03 -k SV3KEY2 -i

#
# Test the message dump for saved fields
#
ensure_field db03 SV3KEY2 T_STRING_FLD KEY2 1
ensure_field db03 SV3KEY2 T_SHORT_FLD 44 1
ensure_field db03 SV3KEY2 T_CHAR_FLD B 1
ensure_field db03 SV3KEY2 T_FLOAT_FLD 1.2 0

#
# Must have some 2 records
#
xadmin cs -d db03

if [ $? -ne 0 ]; then
    echo "xadmin cs failed"
    go_out 2
fi

echo "ensure 2"
ensure_keys db03 2

xadmin stop -s atmi.sv48


xadmin psc

#
# Let tpcached clean up the results
#
sleep 10

xadmin start -s atmi.sv48


echo "ensure 3"
ensure_keys db03 0

#
# now we shall get cached results...
#

(time ./testtool48 -sTESTSV03 -b '{"T_STRING_FLD":"KEY1","T_FLOAT_FLD":"1.4","T_CHAR_FLD":"D"}' \
    -m '{"T_STRING_FLD":"KEY1","T_FLOAT_FLD":"1.4","T_CHAR_FLD":"D"}' \
    -cY -n100 -fY 2>&1) >> ./03_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (3)"
    go_out 6
fi


(time ./testtool48 -sTESTSV03 -b '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.5","T_CHAR_FLD":"E"}' \
    -m '{"T_STRING_FLD":"KEY2","T_FLOAT_FLD":"1.5","T_CHAR_FLD":"E"}' \
    -cY -n100 -fY 2>&1) >> ./03_testtool48.log

if [ $? -ne 0 ]; then
    echo "testtool48 failed (4)"
    go_out 7
fi


go_out $RET

