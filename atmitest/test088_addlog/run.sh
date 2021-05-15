#!/bin/bash
##
## @brief Thread logging and check for stderr/stdout server logrotate - test launcher
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

export TESTNAME="test088_addlog"

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
export NDRX_SILENT=Y

. ../dom1.sh
export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
export NDRX_LOG=$TESTDIR/ndrx-dom1.log
export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf


#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

rm *.log 2>/dev/null
rm ULOG* 2>/dev/null

xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm
echo "Running off client"
(./atmiclt88 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# verify that logs have gone to atmisv
LOGTXT=`grep 'HELLO WORLD SV' atmisv*.log `
if [[ "X$LOGTXT" == "X" ]]; then
    echo "Missing log entries in atmisv*.log!"
    go_out -3
fi

echo "STDERR/STDOUT logrotate test"
export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1-std.conf
rm *.log

xadmin stop -y
xadmin start -y

echo "Running off client"
(./atmiclt88 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
        go_out $RET
fi

LOGTXT=`grep 'HELLO STDERR 1' atmisv-dom1.log`
if [[ "X$LOGTXT" == "X" ]]; then
    echo "Missing log entry: HELLO STDERR 1!"
    go_out -3
fi

LOGTXT=`grep 'HELLO STDOUT 1' atmisv-dom1.log`
if [[ "X$LOGTXT" == "X" ]]; then
    echo "Missing log entry: HELLO STDOUT 1!"
    go_out -3
fi

echo "Logrotate"
rm *.log
xadmin lcf logrotate

echo "Running off client"
(./atmiclt88 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
        go_out $RET
fi

LOGTXT=`grep 'HELLO STDERR 2' atmisv-dom1.log`
if [[ "X$LOGTXT" == "X" ]]; then
    echo "Missing log entry: HELLO STDERR 2!"
    go_out -3
fi

LOGTXT=`grep 'HELLO STDOUT 2' atmisv-dom1.log`
if [[ "X$LOGTXT" == "X" ]]; then
    echo "Missing log entry: HELLO STDOUT 2!"
    go_out -3
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

