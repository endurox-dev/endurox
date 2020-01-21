#!/bin/bash
##
## @brief Test buildserver, buildclient and buildtms - test launcher
##  needs to think what would happen if in the same group we run null switch
##  and then server will join to real tran.
## @file run.sh
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

export TESTNAME="test071_buildtools"

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

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR

export NDRX_TOUT=10

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf

    NDRX_EXT=so
    if [ "$(uname)" == "Darwin" ]; then
        NDRX_EXT=dylib
    fi

    # Configure XA Driver
    # Driver library will be 
    # this will be default 1 group...
    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR=-
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
    export NDRX_XA_DRIVERLIB=../../xadrv/tms/libndrxxatmsx.$NDRX_EXT
    export NDRX_XA_RMLIB=-

    echo "Driver: $NDRX_XA_DRIVERLIB"
    echo "RM: $NDRX_XA_RMLIB"

    export NDRX_XA_LAZY_INIT=0

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
    xadmin killall atmiclt71

    popd 2>/dev/null
    exit $1
}

rm *.log 2>/dev/null

###############################################################################
echo "Firstly lets build the processes"
###############################################################################

export NDRX_SILENT=Y
export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
export NDRX_HOME=.
export PATH=$PATH:$PWD/../../buildtools
export CFLAGS="-I../../include -L../../libatmi -L../../libubf -L../../tmsrv -L../../libatmisrv -L../../libexuuid -L../../libexthpool -L../../libnstd"


echo "Building tms..."
buildtms -o tmstest -rTestSw -v

RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Failed to build tmstest: $RET"
    go_out 1
fi

echo "Build server..."
buildserver -o atmi.sv71 -rTestSw -a atmisv71_1.c -l atmisv71_2.c -v \
    -s A,B,C:TESTSV -sECHOSV -s:TESTSV -sZ:ECHOSV -f atmisv71_3.c -l atmisv71_4.c \
    -s @advertise_file.txt

RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Failed to build atmisv71: $RET"
    go_out 2
fi

echo "Build server... (compile fail)"
buildserver -o atmi.sv71err -rTestSw -f atmisv71_X.c -l atmisv71_2.c -v \
    -s A,B,C:TESTSV -sECHOSV -s:TESTSV -sZ:ECHOSV -f atmisv71_3.c -l atmisv71_4.c

RET=$?

if [ "X$RET" == "X0" ]; then
    echo "atmi.sv71err shall fail (no source): $RET"
    go_out 2
fi


echo "Build server... (invalid compiler)"
CC_SAVED=$CC
export CC=no_such_compiler
buildserver -o atmi.sv71err2 -rTestSw -f atmisv71_1.c -l atmisv71_2.c -v \
    -s A,B,C:TESTSV -sECHOSV -s:TESTSV -sZ:ECHOSV -f atmisv71_3.c -l atmisv71_4.c

RET=$?

if [ "X$RET" == "X0" ]; then
    echo "atmi.sv71err2 shall fail (no compiler): $RET"
    go_out 2
fi

export CC=$CC_SAVED
export CFLAGS=""
unset NDRX_HOME
# test the other option...
export NDRX_RMFILE=./udataobj/RM

echo "Build client..., No switch..."
buildclient -o atmiclt71err -rerrorsw -a atmiclt71_1.c -l atmiclt71_2.c -v \
    -l atmiclt71_3.c -f atmiclt71_4.c \
    -f "-I../../include -L../../libatmi -L../../libubf -L../../tmsrv -L../../libatmisrv -L../../libnstd"
RET=$?

if [ "X$RET" == "X0" ]; then
    echo "Build atmiclt71err shall fail, but was OK"
    go_out 2
fi

echo "Build client..., Build OK"
buildclient -o atmiclt71 -rnullsw -f atmiclt71_1.c -l atmiclt71_2.c -v \
    -l atmiclt71_3.c -f atmiclt71_4.c \
    -f "-I../../include -L../../libatmi -L../../libubf -L../../tmsrv -L../../libatmisrv -L../../libnstd"
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Failed to build atmiclt71: $RET"
    go_out 2
fi

echo "Build client default sw..., Build OK"
buildclient -o atmiclt71dflt -f atmiclt71_1.c -l atmiclt71_2.c -v \
    -l atmiclt71_3.c -f atmiclt71_4.c \
    -f "-I../../include -L../../libatmi -L../../libubf -L../../libnstd -L ../../libatmiclt"
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Failed to build atmiclt71: $RET"
    go_out 2
fi

echo "Build client, working switch, Build OK"
buildclient -o atmiclt71_txn -a atmiclt71_txn.c -v -r TestSw -k \
    -f "-I../../include -L../../libatmi -L../../libubf -L../../libnstd -L ../../libatmiclt"
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Failed to build atmiclt71_txn: $RET"
    go_out 2
fi

###############################################################################
echo "Now execute them..."
###############################################################################

SETLIBPATH="$PWD/../../libatmi:$PWD/../../libubf:$PWD/../../tmsrv:$PWD/../../libatmisrv:$PWD/../../libnstd:$PWD/../../libatmiclt:$PWD/../test021_xafull"
UNAME=`uname -s`

#
# export the library path.
#
case $UNAME in

  Darwin)
    export DYLD_LIBRARY_PATH=$SETLIBPATH
    ;;

  AIX)
    export LIBPATH=$LIBPATH:$SETLIBPATH
    # check compiler, we have a set of things required for each compiler to actually build the binary
    ;;

  *)
    export LD_LIBRARY_PATH=$SETLIBPATH
    echo "LIBPATH: [$LD_LIBRARY_PATH]"
    ;;
esac


# prepare folders
rm -rf $TESTDIR/RM1 2>/dev/null
rm -rf $TESTDIR/RM2 2>/dev/null

#
# Note RM1 is NULL group
# RM2 is test switch
#

mkdir $TESTDIR/RM1
mkdir $TESTDIR/RM2

mkdir $TESTDIR/RM2/active
mkdir $TESTDIR/RM2/prepared
mkdir $TESTDIR/RM2/committed
mkdir $TESTDIR/RM2/aborted
export NDRX_TEST_RM_DIR=$TESTDIR/RM2

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm

CNT=`xadmin psc | grep ECHO2SV | wc | awk '{print $1}'`
if [ $CNT -ne 1 ]; then
    echo "Too many ECHO2SV!: $CNT"
    go_out -10
fi

echo Run off binaries...""

(./atmiclt71 2>&1) > ./atmiclt-dom1.log
RET=$?

echo "Execute atmiclt71..."
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Execute atmiclt71dflt..."
(./atmiclt71dflt 2>&1) > ./atmiclt-dom1_dlft.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Execute atmiclt71_txn..."

# start in 2nd group
(NDRX_XA_RES_ID=2 ./atmiclt71_txn 2>&1) > ./atmiclt71_txn.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "Failed to run atmiclt71_txn: $RET"
    go_out $RET
fi

echo "Checking the committed record count..."
CNT=`ls -1 RM2/committed/ | wc | awk '{print $1}'`

if [ $CNT -ne 1633 ]; then
    echo "1633 transactions must be committed, but got: $CNT"
    go_out -10
fi

# 33 of them shall be clients...
CNT=`grep 'CLT' RM2/committed/* | wc | awk '{print $1}'`

if [ $CNT -ne 33 ]; then
    echo "CLT 33 not found: $CNT"
    go_out -1
fi

if [ -f ./RM2/TRN-* ]; then
    echo "Transaction must be completed!"
    go_out -11
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-20
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
