#!/bin/bash
##
## @brief Test tplogprintubf masking plugin func - test launcher
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
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

TESTNAME="test070_ubfmask"

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test070_ubfmask
fi;

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR

UNAME=`uname -s`

#
# export the library path.
#
case $UNAME in

  Darwin)
    export NDRX_PLUGINS=libubfmasktest.dylib
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$TESTDIR
    ;;

  AIX)
    export NDRX_PLUGINS=libubfmasktest.so
    export LIBPATH=$LIBPATH:$TESTDIR
    ;;

  *)
    export NDRX_PLUGINS=libubfmasktest.so
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TESTDIR
    ;;
esac

if [ "$(uname)" == "Darwin" ]; then
        echo "Darwin host"
        export NDRX_PLUGINS=libubfmasktest.dylib
fi

xadmin killall atmiclt70 2>/dev/null

# client timeout
export NDRX_TOUT=10
export NDRX_DEBUG_CONF=`pwd`/debug.conf

function go_out {
    echo "Test exiting with: $1"
    xadmin killall atmiclt70 2>/dev/null
    
    popd 2>/dev/null
    exit $1
}


rm *.log

(./atmiclt70 2>&1) > ./atmiclt70.log

RET=$?

if [ "X0" != "X$RET" ]; then
    echo "Failed to run atmiclt70!"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out -2
fi

# check the number of hidden entries
XCNT=`grep "XXXXXXXXXXX" atmiclt70.log | wc | awk '{print($1)}'`
YCNT=`grep "YYYYY" atmiclt70.log | wc | awk '{print($1)}'`

echo "XCNT=$XCNT YCNT=$YCNT"

if [ "X$XCNT" != "X200" ]; then
    echo "XCNT=$XCNT expected 200"
    go_out -3
fi

if [ "X$YCNT" != "X200" ]; then
    echo "YCNT=$YCNT expected 200"
    go_out -4
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent: