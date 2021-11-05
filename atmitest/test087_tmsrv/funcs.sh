#!/bin/bash
##
## @brief Support functions for tmsrv scripting
##
## @file run_funcs.sh
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


. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
export NDRX_ULOG=$TESTDIR
export NDRX_TOUT=10
export NDRX_CCONFIG=$TESTDIR/test.ini
export NDRX_SILENT=Y
NDRX_EXT=so
if [ "$(uname)" == "Darwin" ]; then
    NDRX_EXT=dylib
fi
export NDRX_XA_DRIVERLIB=../../xadrv/tms/libndrxxatmsx.$NDRX_EXT


UNAME=`uname`

#
# Get the crash lib...
#
case $UNAME in

  Darwin)
    export NDRX_PLUGINS=libt86_lcf.dylib
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$TESTDIR/../test086_tmqlimit
    ;;

  AIX)
    export NDRX_PLUGINS=libt86_lcf.so
    export LIBPATH=$LIBPATH:$TESTDIR/../test086_tmqlimit
    ;;

  *)
    export NDRX_PLUGINS=libt86_lcf.so
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TESTDIR/../test086_tmqlimit
    ;;
esac

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
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y 2>/dev/null

    popd 2>/dev/null
    exit $1
}

#
#  cleanup tmsrv logfiles
#
function clean_logs {
    #
    # Cleanup test logs...
    #
    rm -rf ./log1 2>/dev/null; mkdir ./log1
    rm -rf ./log2 2>/dev/null; mkdir ./log2
}

#
# Prior test, clean ulog
#
function clean_ulog {

    rm ULOG*
    > tmsrv_lib1.log
    > tmsrv_lib2.log
}

#
# Verify ULOG ops
#
function verify_ulog {

    rmid=$1
    operation=$2
    count=$3

    cnt=`grep "$rmid: $operation" ULOG* | wc -l|  awk '{print $1}'`

    if [ "X$cnt" != "X$count" ]; then
        echo "$rmid expected $operation $count times, got $cnt"
        go_out -1
    fi
}

#
# Verify number of log files
#
function verify_logfiles {

    rmlog=$1
    count=$2

    cnt=`ls -1 ./$rmlog/TRN* 2>/dev/null | wc -l | awk '{print $1}'`

    if [[ "X$cnt" != "X$count" ]]; then
        echo "Expected $rmlog to have $count logs but got $cnt"
        go_out -1
    fi
}

#
# Verify that debug log contains 
#
function have_output {

    cmd=$1
    cmd="$cmd | wc -l | awk '{print \$1}'"
    cnt=`eval $cmd`
    
    if [ "X$cnt" == "X0" ]; then
        echo "Expected >0 lines of output from [$cmd] got [$cnt]"
        go_out -1
    fi
}

#
# Build programs
#
function buildprograms {

    LIBMODE=$1

    ################################################################################
    echo "Building programs..."
    ################################################################################

    echo ""
    echo "************************************************************************"
    echo "Building tmsrv libl87_1$LIBMODE ..."
    echo "************************************************************************"
    buildtms -o tmsrv_lib1 -rlibl87_1$LIBMODE -v

    RET=$?

    if [ "X$RET" != "X0" ]; then
        echo "Failed to build tmstest: $RET"
        go_out 1
    fi

    echo ""
    echo "************************************************************************"
    echo "Building tmsrv libl87_2$LIBMODE ..."
    echo "************************************************************************"
    buildtms -o tmsrv_lib2 -rlibl87_2$LIBMODE -v

    RET=$?

    if [ "X$RET" != "X0" ]; then
        echo "Failed to build tmstest: $RET"
        go_out 1
    fi

    echo ""
    echo "************************************************************************"
    echo "Building atmiclt87 NULL ..."
    echo "************************************************************************"
    buildclient -o atmiclt87 -rlibl87_1$LIBMODE -l atmiclt87.c -v
    RET=$?

    if [ "X$RET" != "X0" ]; then
        echo "Build atmiclt87 failed"
        go_out 1
    fi

    echo ""
    echo "************************************************************************"
    echo "Building atmisv87 NULL ..."
    echo "************************************************************************"
    buildserver -o atmisv87_1 -rlibl87_1$LIBMODE -l atmisv87_1.c -sTESTSV1 -sTESTSVE1 -sTESTSVE1_RET -sTESTSVE1_NORET -sTEST1_PART -v
    RET=$?

    if [ "X$RET" != "X0" ]; then
        echo "Build atmiclt87 failed"
        go_out 1
    fi

    echo ""
    echo "************************************************************************"
    echo "Building atmisv87 NULL ..."
    echo "************************************************************************"
    buildserver -o atmisv87_2 -rlibl87_2$LIBMODE -l atmisv87_2.c -sTESTSV2 -sTESTSVE2 -v
    RET=$?

    if [ "X$RET" != "X0" ]; then
        echo "Build atmiclt87 failed"
        go_out 1
    fi

}

#
# We will work only in dom1
#
set_dom1;

################################################################################
echo "Configure build environment..."
################################################################################

# Additional, application specific
COMPFLAGS=""
#
# export the library path.
#
case $UNAME in

  AIX)
    # check compiler, we have a set of things required for each compiler to actually build the binary
    $CC -qversion 2>/dev/null
    RET=$?
    export OBJECT_MODE=64

    if [ "X$RET" == "X0" ]; then
	echo "Xlc compiler..."
        COMPFLAGS="-brtl -qtls -q64"
    else
	echo "Default to gcc..."
        COMPFLAGS="-Wl,-brtl -maix64"
    fi

    ;;
  SunOS)
        COMPFLAGS="-m64"
    ;;

  *)
    # default..
    ;;
esac

# Support sanitizer
if [ "X`xadmin pmode 2>/dev/null | grep '#define NDRX_SANITIZE'`" != "X" ]; then
    COMPFLAGS="$COMPFLAGS -fsanitize=address"
fi

export NDRX_HOME=.
export PATH=$PATH:$PWD/../../buildtools
export CFLAGS="-I../../include -L../../libatmi -L../../libatmiclt -L../../libubf -L../../tmsrv -L../../libatmisrv -L../../libexuuid -L../../libexthpool -L../../libnstd $COMPFLAGS"

clean_logs;
rm *.log
rm ULOG*


# vim: set ts=4 sw=4 et smartindent:

