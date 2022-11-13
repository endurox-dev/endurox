#!/bin/bash
##
## @brief Server stdout test - test launcher
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

export TESTNAME="test100_svstdout"

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

    popd 2>/dev/null
    exit $1
}

rm *.log 2>/dev/null
rm ULOG* 2>/dev/null

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

xadmin psc
xadmin ppm

RET=0
# check logs...
if [ "X`grep 'Hello world from stdout' stdout.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -1
fi

if [ "X`grep 'Hello world from stderr' stderr.log`" == "X" ]; then
    echo "Missing stderr entry!"
    go_out -2
fi

if [ "X`grep 'tpsvrinit called' atmisv-dom1.log`" == "X" ]; then
    echo "Missing atmisv-dom1.log entry!"
    go_out -3
fi

if [ "X`grep 'Hello world from stdout' stderr2.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -4
fi

if [ "X`grep 'Hello world from stderr' stderr2.log`" == "X" ]; then
    echo "Missing stderr entry!"
    go_out -5
fi

if [ "X`grep 'Hello world from stdout' stderr3.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -6
fi

if [ "X`grep 'Hello world from stderr' stderr3.log`" == "X" ]; then
    echo "Missing stderr entry!"
    go_out -7
fi

if [ "X`grep 'Hello world from stdout' stdout4.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -8
fi

if [ "X`grep 'Hello world from stdout' stdout4.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -8
fi

# stuff goes to ndrxd.log
if [ "X`grep 'Hello world from stderr' stderr4.log`" != "X" ]; then
    echo "Stderr shall not be set!"
    go_out -9
fi

if [ "X`grep 'Hello world from stderr' ndrxd-dom1.log`" == "X" ]; then
    echo "ndrxd-dom1.log does not contain stderr output"
    go_out -10
fi

################################################################################
# Check the output logfile after rotate..
# default works together with -e thus managed by log sub-system
################################################################################
rm stderr3.log
xadmin lcf logrotate 
cat << EOF | ud32 
SRVCNM	WRITELOG_30
T_STRING_FLD	Hello world from stdout3
T_STRING_2_FLD	Hello world from stderr3
EOF

if [ "X`grep 'Hello world from stdout3' stderr3.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -11
fi

if [ "X`grep 'Hello world from stderr3' stderr3.log`" == "X" ]; then
    echo "Missing stderr entry!"
    go_out -12
fi

################################################################################
# Same name check
################################################################################
rm std50.log
xadmin lcf logrotate 
cat << EOF | ud32 
SRVCNM	WRITELOG_50
T_STRING_FLD	Hello world from stdout5
T_STRING_2_FLD	Hello world from stderr5
EOF

if [ "X`grep 'Hello world from stdout5' std50.log`" == "X" ]; then
    echo "Missing stdout entry!"
    go_out -13
fi

if [ "X`grep 'Hello world from stderr5' std50.log`" == "X" ]; then
    echo "Missing stderr entry!"
    go_out -14
fi

################################################################################
# Not managed by lcf (stdout)
################################################################################
rm stdout6.log
xadmin lcf logrotate 
cat << EOF | ud32 
SRVCNM	WRITELOG_60
T_STRING_FLD	Hello world from stdout6
T_STRING_2_FLD	Hello world from stderr6
EOF

if [ "X`grep 'Hello world from stdout6' stdout6.log`" != "X" ]; then
    echo "STDOUT shall not be restored!"
    go_out -15
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out -1
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

