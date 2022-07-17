#!/bin/bash
##
## @brief System V mode housekeeping in case if first instance crashed (for other pollers must work ok too) - test launcher
##
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

export TESTNAME="test098_svqhkeep"

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
export NDRX_ULOG=$TESTDIR
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
    xadmin killall atmiclt98

    popd 2>/dev/null
    exit $1
}

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

rm *.log

set_dom1;
xadmin down -y
xadmin idle
xadmin lcf advcrash -A 1 -a -n
xadmin start -y || go_out 1

echo "Let housekeep to fix SHM"
# let housekeep to proceed.
xadmin lcf advcrash -A 0 -a -n
sleep 20

xadmin start -y

RET=0

xadmin psvc -r
echo "Running off client"

set_dom1;
(./atmiclt98 2>&1) > ./atmiclt-dom1.log
#(valgrind --leak-check=full --log-file="v.out" -v ./atmiclt98 2>&1) > ./atmiclt-dom1.log

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi


go_out $RET

# vim: set ts=4 sw=4 et smartindent:

