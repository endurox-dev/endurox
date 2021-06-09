#!/bin/bash
##
## @brief @(#) Test086 - testing on MQ limitations
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

export TESTNO="086"
export TESTNAME_SHORT="tmqlimit"
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
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt86

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

#
# export the library path.
#
case $UNAME in

  Darwin)
    export NDRX_PLUGINS=libt86_lcf.dylib
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$TESTDIR
    ;;

  AIX)
    export NDRX_PLUGINS=libt86_lcf.so
    export LIBPATH=$LIBPATH:$TESTDIR
    ;;

  *)
    export NDRX_PLUGINS=libt86_lcf.so
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TESTDIR
    ;;
esac


# Where to store TM logs
rm -rf ./RM1
mkdir RM1

rm -rf ./RM2
mkdir RM2

# Where to store Q messages (QSPACE1)
rm -rf ./QSPACE1
mkdir QSPACE1

set_dom1;
# clean up anything left from prevoius tests...
xadmin down -y
# let ndrxd to finish
sleep 2
xadmin start -y || go_out 1

# Run the client test...
xadmin psc
xadmin psvc
xadmin ppm
clean_logs;
rm ULOG*

echo "Testing crashloop"
(./atmiclt86 crashloop 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    xadmin psc
    go_out $RET
fi

# print what's left in q...
xadmin mqlq
clean_logs;
rm ULOG*

echo "Testing errorq"
(./atmiclt86 errorq 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    xadmin psc
    go_out $RET
fi

echo "Testing abortrules"
(./atmiclt86 abortrules 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing loadprep"
(./atmiclt86 loadprep 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# after loadprep log folders shall be cleaned up...
xadmin stop -y

# Where to store TM logs
rm -rf ./RM1
mkdir RM1

rm -rf ./RM2
mkdir RM2

# Where to store Q messages (QSPACE1)
rm -rf ./QSPACE1
mkdir QSPACE1

xadmin start -y

echo "Testing diskfull"
clean_logs;
xadmin help lcf 

(./atmiclt86 diskfull 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing tmsrvdiskerr"
(./atmiclt86 tmsrvdiskerr 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing commit_crash"
(./atmiclt86 commit_crash 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing badmsg"
(./atmiclt86 badmsg 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing qfull"
(./atmiclt86 qfull 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing tmsrvrestart"
(./atmiclt86 tmsrvrestart 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing commit_shut"
(./atmiclt86 commit_shut 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing deqwriteerr"
(./atmiclt86  deqwriteerr 2>&1) >> ./atmiclt-dom1.log
RET=$?
if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "Testing corrupted file houskeeping"

echo "Creating 1KB block... of zeros"
dd if=/dev/zero of=./QSPACE1/active/zero_block count=1024 bs=1
touch ./QSPACE1/active/empty_file
touch ./QSPACE1/prepared/empty_file

xadmin stop -s tmqueue
xadmin start -s tmqueue

# files must exist as 30 sec is not passed...
if [[ ! -f ./QSPACE1/active/zero_block ]]; then
    echo "./QSPACE1/active/zero_block must exist (as not yet expired)!"
    go_out -1
fi

if [[ ! -f ./QSPACE1/active/empty_file ]]; then
    echo "./QSPACE1/active/empty_file must exist (as not yet expired)!"
    go_out -1
fi

if [[ ! -f ./QSPACE1/prepared/empty_file ]]; then
    echo "./QSPACE1/prepared/empty_file must exist (no expiry)!"
    go_out -1
fi

echo "Sleep 33.."
sleep 33

xadmin stop -s tmqueue
xadmin start -s tmqueue

# files must be removed as 30 sec passed
if [[ -f ./QSPACE1/active/zero_block ]]; then
    echo "./QSPACE1/active/zero_block must be removed (expired)!"
    go_out -1
fi

if [[ -f ./QSPACE1/active/empty_file ]]; then
    echo "./QSPACE1/active/empty_file must be removed (expired)!"
    go_out -1
fi

# only active folder is housekeeped
if [[ ! -f ./QSPACE1/prepared/empty_file ]]; then
    echo "./QSPACE1/prepared/empty_file must exist (no expiry)!"
    go_out -1
fi

# cleanup...
rm ./QSPACE1/prepared/empty_file

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
