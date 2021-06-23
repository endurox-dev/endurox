#!/bin/bash
##
## @brief @(#) Test089 - tmrecover testing
##
## @file run-dom.sh
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

export TESTNO="089"
export TESTNAME_SHORT="tmrecover"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"

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
# Override timeout!
export NDRX_TOUT=90
export NDRX_LIBEXT="so"
export NDRX_ULOG=$TESTDIR
export NDRX_SILENT=Y

#
# Domain 1 - here client will live
#
function set_dom1 {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf

# XA config, mandatory for TMQ:
    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR="./QSPACE1"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
# Used from parent
    export NDRX_XA_DRIVERLIB=$NDRX_XA_DRIVERLIB_FILENAME

    export NDRX_XA_RMLIB=libndrxxaqdisk.so
if [ "$(uname)" == "Darwin" ]; then
    export NDRX_XA_RMLIB=libndrxxaqdisk.dylib
    export NDRX_LIBEXT="dylib"
fi
    export NDRX_XA_LAZY_INIT=0
}

#
# Domain 3 - here client will live
#
function set_dom2 {
    echo "Setting domain 2"
    . ../dom2.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf

# XA config, mandatory for TMQ:
    export NDRX_XA_RES_ID=2
    export NDRX_XA_OPEN_STR="./QSPACE2"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
# Used from parent
    export NDRX_XA_DRIVERLIB=$NDRX_XA_DRIVERLIB_FILENAME

    export NDRX_XA_RMLIB=libndrxxaqdisk.so
if [ "$(uname)" == "Darwin" ]; then
    export NDRX_XA_RMLIB=libndrxxaqdisk.dylib
    export NDRX_LIBEXT="dylib"
fi
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

    set_dom2;
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

function clean_logs {

    # clean-up the logs for debbuging at the error.
    for f in `ls *.log`; do
         echo > $f
    done

}

UNAME=`uname`

# Where to store TM logs
rm -rf ./RM1
mkdir RM1

rm -rf ./RM2
mkdir RM2

# Where to store Q messages (QSPACE1)
rm -rf ./QSPACE1
mkdir QSPACE1

rm -rf ./QSPACE2
mkdir QSPACE2

set_dom1;
xadmin start -y || go_out 1

set_dom2;
xadmin start -y || go_out 2

set_dom1;

echo "Wait for connection..."
sleep 10

# Run the client test...
xadmin psc
xadmin psvc
xadmin ppm
clean_logs;
rm ULOG*

echo "load 1100 msgs..."
exbenchcl -n1 -P -t1000 -b "{}" -f EX_DATA -S1024 -QMYSPACE -sTEST1 -E -R1100 || go_out 3
echo "Move to prepared..."

# get the transaction names (so that we do not have to generate ones...)
# get the base names of the transactions, with out the number
grep 'Writing command fil' *.log | cut -d '[' -f2 | cut -d ']' -f1 | cut -d '/' -f 4 | cut -d '-' -f 1 | while read -r line ; do
    MSG_FILE=`ls -1 QSPACE1/committed | tail -1`
    echo "Injecting transaction: [mv QSPACE1/committed/$MSG_FILE QSPACE1/prepared/${line}-001]"

    # make file to appear twice... (append 1)
    cp QSPACE1/committed/$MSG_FILE QSPACE1/prepared/${line}-001 || go_out 4
    mv QSPACE1/committed/$MSG_FILE QSPACE1/prepared/${line}-002 || go_out 5

done

# test remove recover...
set_dom2;

echo "Recovering..."
xadmin recoverlocal 2>/dev/null
CNT=`xadmin recoverlocal | wc | awk '{print $1}'`

echo "Got txns: $CNT"
if [[ "X$CNT" != "X1100" ]]; then
    echo "Expecting X1100, got X$CNT"
    go_out 6
fi

echo "Aborting..."
# count number aborted...
CNT=`xadmin abortlocal -y | wc | awk '{print $1}'`

echo "Got processed recs: $CNT"
if [[ "X$CNT" != "X2200" ]]; then
    echo "Expecting X2200, got X$CNT (abortlocal processing)"
    go_out 6
fi

# all must be aborted...
echo "Check..."
CNT=`xadmin recoverlocal | wc | awk '{print $1}'`
echo "Got txns: $CNT"
if [[ "X$CNT" != "X0" ]]; then
    echo "Expecting X0, got X$CNT"
    go_out 7
fi

echo "All ok"
go_out 0

# vim: set ts=4 sw=4 et smartindent:
