#!/bin/bash
##
## @brief tmsrv houskeeping extended tests - test launcher
##  test will have two parts. 1) check that corrupted logs are collected by
##  log try-load count processign. 2) check that tries are expired, however
##  the log is finally collected.
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

export TESTNAME="test107_tmsrvhkeep"

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

if [ "$(uname)" == "Darwin" ]; then
    export NDRX_LIBEXT="dylib"
else
    export NDRX_LIBEXT="so"
fi

#
# Common configuration between the domains
#
export NDRX_CCONFIG=$TESTDIR/app-common.ini

#
# Domain 1 - trasnaction is collected by tries counter (expiry check at attempt)
#
set_dom1_tries() {
    echo "Setting domain 1 (tries)"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1-tries.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
}

#
# Doamin 1 - transaction is collected by log expiry
#
set_dom1_exp() {
    echo "Setting domain 1 (tries)"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1-exp.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"

    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt107

    popd 2>/dev/null
    exit $1
}

rm *.log
rm -rf RM2 qdata 2>/dev/null
# where to store the data
mkdir RM2 qdata

for test in 1 2; do

    echo ">>> Test number [$test]"
    rm ULOG* 2>/dev/null

    if [ $test -eq 1 ]; then
        set_dom1_tries;
    else
        set_dom1_exp;
    fi
    # cleanup old session
    xadmin stop -y
    xadmin down -y
    xadmin start -y

    # load single message, sequence 0
    NDRX_CCTAG=RM2TMQ ./atmiclt107 enq 0

    RET=$?
    if [[ "X$RET" != "X0" ]]; then
        echo "Failed to enqueue message!"
        go_out $RET
    fi

    # generate transaction entry (incomplete)
    NDRX_CCTAG=RM2TMQ ./atmiclt107 deq 1 Y

    RET=$?
    if [[ "X$RET" != "X0" ]]; then
        echo "Failed to dequeue (broken, locked, dequeue not committed)!"
        go_out $RET
    fi

    # now corrupt the transaction log
    find $TESTDIR/RM2 -type f -exec truncate -s 0 {} \;

    # make any active tmqueue transactions as prepared...
    # so that tmrecoversv can collect them...
    mv $TESTDIR/qdata/active/* $TESTDIR/qdata/prepared

    # reload tmqueue (to stuck the transaction)
    xadmin sreload tmqueue

    # reload tmsrv (to capture broken txn)
    xadmin sreload tmsrv

    # let some sanity cycle to run / recover (shall not recover yet...)
    sleep 4

    # try dequeue, it shall fail, as  msg is locked...
    NDRX_CCTAG=RM2TMQ ./atmiclt107 deq 1 N

    RET=$?
    if [[ "X$RET" == "X0" ]]; then
        echo "Dequeue must fail!"
        go_out $RET
    fi

    # wait for tries or expiry to kick in
    sleep 20

    NDRX_CCTAG=RM2TMQ ./atmiclt107 deq 1 N

    RET=$?
    if [[ "X$RET" != "X0" ]]; then
        echo "Dequeue must succeed, but failed (after expiry)!"
        go_out $RET
    fi

    # verify that tmsrv did not crash...
    if [ "X`grep "Server process \[tmsrv\]" ULOG* | grep died `" != "X" ]; then
        echo "tmsrv died during the testing!"
        go_out -1
    fi

    # ensure that log directory is empty..
    ls -l $TESTDIR/RM2

    CNT=`ls -1 $TESTDIR/RM2 | wc -l | awk '{print $1}'`
    if [ $CNT -ne 0 ]; then
        echo "Expected empty log directory, but got $CNT files"
        go_out -1
    fi

    # verify number of attempts done for loading... (scan the tmsrv log)
    ATTEMPTS=`grep "info record - not loading" ULOG*  | wc -l | awk '{print $1}'`

    if [ $test -eq 1 ]; then
        if [ $ATTEMPTS -lt 10 ]; then
            echo "Expected load attempts atleast 10, but got $ATTEMPTS"
            go_out -1
        fi
    else
        if [ $ATTEMPTS -ne 1 ]; then
            echo "Expected 1x load attempt, but got $ATTEMPTS"
            go_out -1
        fi
    fi

done

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET
# vim: set ts=4 sw=4 et smartindent:

