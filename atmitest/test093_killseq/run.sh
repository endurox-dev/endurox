#!/bin/bash
##
## @brief Test kill signal sequences - test launcher
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

export TESTNAME="test093_killseq"

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

################################################################################
echo ">>> Checking defaults..."
################################################################################
xadmin down -y
xadmin start -y || go_out 1
xadmin ppm

echo "Stopping..."
xadmin stop -y

################################################################################
echo ">>> Checking server setting..."
################################################################################
export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1_sv.xml
xadmin down -y
xadmin start -y || go_out 1
xadmin ppm

echo "Stopping..."
xadmin stop -y


################################################################################
echo ">>> Checking stock settings..."
################################################################################
export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1_stock.xml
xadmin down -y
xadmin start -y || go_out 1
xadmin ppm

echo "Stopping..."
xadmin stop -y

CNT=`grep "ALL SIGNALS OK" atmisv-dom1.log | wc | awk '{print($1)}'`
echo "CNT=$CNT"

if [ "X$CNT" != "X2" ]; then
    echo "[ALL SIGNALS OK] expected 2x times in atmisv-dom1.log"
    go_out -1
fi

CNT=`grep "STOCK SIGNALS OK" atmisv-dom1.log | wc | awk '{print($1)}'`
echo "CNT=$CNT"

if [ "X$CNT" != "X1" ]; then
    echo "[STOCK SIGNALS OK] expected 1x times in atmisv-dom1.log"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out -1
fi

go_out 0


# vim: set ts=4 sw=4 et smartindent:

