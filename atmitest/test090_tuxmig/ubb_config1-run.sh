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

#
# Enduor/X not yet booted
#
function go_out_silent {
    echo "Test exiting with: $1"

    popd 2>/dev/null
    exit $1
}

################################################################################
echo ">>> Testing ubb_config1 -> E/X convert"
################################################################################

#
# Cleanup by rndkey, maybe random...
#

xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}
xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}
xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}

export NDRX_SILENT=Y
rm -rf ./runtime tmp1 tmp2 2>/dev/null
ubb2ex ubb_config1 -P ./runtime

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out_silent $RET
fi

#
# put down some bins
#
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_2
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_3
ln -s $TESTDIR/atmi.sv90 runtime/user90/bin/atmi.sv90_4
ln -s $TESTDIR/atmiclt90 runtime/user90/bin/atmiclt90

# Really not needed: shall be in dist path:
#ln -s $TESTDIR/../../exbench/exbenchsv runtime/user90/bin/exbenchsv
#ln -s $TESTDIR/../../exbench/exbenchcl runtime/user90/bin/exbenchcl

# Start the runtime

pushd .

cd runtime/user90/conf
. settest1

# cleanup shms...
xadmin down -y
xadmin start -y

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

xadmin pc

################################################################################
echo ">>> Checking DDR..."
################################################################################

exbenchcl -n1 -P -t9999 -b '{"T_STRING_10_FLD":"2"}' -f EX_DATA -S1024 -R500
RET=$?
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

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# Enqueue To Q space, wait for notification back...
exbenchcl -n1 -P -t9999 -b '{}' -f EX_DATA -S1024 -R1000 -sQGRP1_2 -QQSPB -I -E

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# Enqueue To Q space, wait for notification back...
exbenchcl -n1 -P -t9999 -b '{}' -f EX_DATA -S1024 -R1000 -sQGRP1_2 -QQSPC -I -E

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

# List the Qs...
xadmin mqlq


################################################################################
echo ">>> Checking Events..."
################################################################################

exbenchcl -n5 -P -t9999 -b '{"T_STRING_10_FLD":"0"}' -f EX_DATA -S1024 -R1000 -sTESTEV -e
RET=$?
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

OUT=`xadmin psc | grep "TESTSV       TESTSV       atmi.sv9+   500"`

if [ "X$OUT" != "X" ]; then
    echo "TESTSV must not be advertised by srvid=500"
    go_out -1
fi

################################################################################
echo ">>> Compare outputs of the XML"
################################################################################

OUT=`diff $TESTDIR/ubb_config1.xml $TESTDIR/runtime/user90/conf/ndrxconfig.test1.xml`

echo $OUT

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

if [ "X$OUT" != "X" ]; then
    echo "ubb_config1.xml!=runtime/user90/conf/ndrxconfig.test1.xml"
    go_out -1
fi

################################################################################
#echo ">>> Compare outputs of the INI" - TODO figrue out 
################################################################################

# prepare ini files, strip the dynamic parts
cat $TESTDIR/ubb_config1.ini        | \
    grep -v NDRX_XA_DRIVERLIB       | \
    grep -v NDRX_XA_RMLIB           | \
    grep -v NDRX_RNDK               | \
    grep -v NDRX_QPATH > tmp1

cat $TESTDIR/runtime/user90/conf/app.test1.ini  | \
    grep -v NDRX_XA_DRIVERLIB                   | \
    grep -v NDRX_XA_RMLIB                       | \
    grep -v NDRX_RNDK                           | \
    grep -v NDRX_QPATH  > tmp2

OUT=`diff tmp1 tmp2`

echo $OUT

if [ "X$OUT" != "X" ]; then
    echo "ubb_config1.ini!=runtime/user90/conf/app.test1.ini"
    go_out -1
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

