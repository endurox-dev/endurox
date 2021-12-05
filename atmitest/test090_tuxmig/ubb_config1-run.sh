#!/bin/bash
##
## @brief Perform UBB config test. This one uses routing, persistent queues,
##  null switches.
##
## @file ubb_config1-run.sh
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

################################################################################
echo "Testing ubb_config1"
################################################################################

export NDRX_SILENT=Y
rm -rf ./runtime 2>/dev/null
../../migration/tuxedo/ubb2ex ubb_config1 -P ./runtime

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

#
# put down some bins
#
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_2
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_3
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_4
ln -s $TESTDIR/atmiclt90 runtime/user90/bin/atmiclt90
ln -s $TESTDIR/../../exbench/exbenchsv runtime/user90/bin/exbenchsv
ln -s $TESTDIR/../../exbench/exbenchcl runtime/user90/bin/exbenchcl

# Start the runtime

pushd .

cd runtime/user90/conf
. set.test1

xadmin start -y

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

xadmin pc

################################################################################
echo ">>> Checking DDR..."
################################################################################

exbenchcl -n1 -P -t9999 -b '{"T_STRING_10_FLD":"2"}' -f EX_DATA -S1024 -R500
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

#xadmin psc

DDR1=`xadmin psc | grep "EXBENCH@DDR1 EXBENCHSV    exbenchsv   600     0     0"`

if [ "X$DDR1" == "X" ]; then
    echo "DDR routing not working (1)"
    go_out -1
fi

DDR2=`xadmin psc | grep "EXBENCH@DDR2 EXBENCHSV    exbenchsv   700   500     0"`

if [ "X$DDR2" == "X" ]; then
    echo "DDR routing not working (2)"
    go_out -1
fi

################################################################################
echo ">>> Checking /Q..."
################################################################################

# Enqueue To Q space, wait for notification back...
exbenchcl -n1 -P -t9999 -b '{}' -f EX_DATA -S1024 -R1000 -sQGRP1_2 -QQSPA -I -E

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# Enqueue To Q space, wait for notification back...
exbenchcl -n1 -P -t9999 -b '{}' -f EX_DATA -S1024 -R1000 -sQGRP1_2 -QQSPB -I -E

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# Enqueue To Q space, wait for notification back...
exbenchcl -n1 -P -t9999 -b '{}' -f EX_DATA -S1024 -R1000 -sQGRP1_2 -QQSPC -I -E

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# List the Qs...
xadmin mqlq


################################################################################
echo ">>> Checking Events..."
################################################################################

exbenchcl -n5 -P -t9999 -b '{"T_STRING_10_FLD":"0"}' -f EX_DATA -S1024 -R1000 -sTESTEV -e

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

################################################################################
echo ">>> Checking escaping..."
################################################################################

if [ "X`grep 'Arg c OK' $TESTDIR/runtime/user90/log/atmi.sv90_2.200.log`" == "X" ]; then
    echo "Missing arg c check!"
    RET=-2
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR $TESTDIR/runtime/user90/log/*.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

################################################################################
echo ">>> Checking missing -A -> -N..."
################################################################################

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

