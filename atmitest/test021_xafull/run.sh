#!/bin/bash
##
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
export TESTNO="021"
export TESTNAME_SHORT="xafull"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"
export NDRX_TESTMODE=1

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


SUFFIX="so"

if [ "$(uname)" == "Darwin" ]; then
    SUFFIX="dylib"
fi

SYSTEM=`uname`
echo "SYSTEM: "$SYSTEM""

export TESTPING_DOM1="";
export TESTPING_DOM2="";
################################################################################
# Bug 160, xa_start fails due to closed connection, recon
################################################################################

# Seems TLS is not working for aix within the loaded driver
if [ "X$SYSTEM" != "XAIX" ]; then

echo ">>> Doing static registration tests... (Bug #160 - start fails at random...)"
echo ">>> #160: Firstly does retry, test case must succeed as flags set"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-startfail.$SUFFIX

#
# Recon on every xa_start error, 3xretries, sleep 10ms between retries.
#
export NDRX_XA_FLAGS="RECON:*:3:10"
export TEST160_FLAG=""

./run-dom.sh || exit $?

fi

################################################################################
# Bug 160, xa_start fails due to closed connection, recon, but only 2x times
# the test case engine needs 3x times...
################################################################################

# Seems TLS is not working for aix...
if [ "X$SYSTEM" != "XAIX" ]; then

echo ">>> Doing static registration tests... (Bug #160 - start fails at random...)"
echo ">>> #160: Secondly does retry, only 2x times, no success"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-startfail.$SUFFIX

#
# Recon on every xa_start error, 3xretries, sleep 10ms between retries.
#
export NDRX_XA_FLAGS="RECON:*:2:10"
export TEST160_FLAG="fail"

./run-dom.sh || exit $?

fi


################################################################################
# Bug 160, xa_start fails due to closed connection, no recon
################################################################################

# Seems TLS is not working for aix...
if [ "X$SYSTEM" != "XAIX" ]; then

echo ">>> Doing static registration tests... (Bug #160 - start fails at random...)"
echo ">>> #160: Third test -  no retries, test case must fail"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-startfail.$SUFFIX
export TEST160_FLAG="fail"
unset NDRX_XA_FLAGS

./run-dom.sh || exit $?

fi


################################################################################
# Test case 105, prepare fails
################################################################################

echo "Doing static registration tests... (Bug #105 - prepare ok, but proc abort)"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-105.$SUFFIX
./run-dom.sh || exit $?

echo "Doing static registration tests... (Bug #123 - try fail commit \
        manual complete (by xadmin))"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-tryfail.$SUFFIX
./run-dom.sh || exit $?

echo "Doing static registration tests... (Bug #123 - try fail, but recovers after awhile)"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-tryok.$SUFFIX
./run-dom.sh || exit $?

echo "Doing static registration tests... (Bug #105 - prepare ok, but proc abort)"
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s-105.$SUFFIX
./run-dom.sh || exit $?

################################################################################
# Normal tests
################################################################################

export NDRX_XA_FLAGS=RECON:*:3:100
# tests with xa_recover type pings
export TESTPING_DOM1="-P1 -R";

# tests with xa_start type pings
export TESTPING_DOM2="-P1";

echo "Doing static registration tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_s.$SUFFIX
./run-dom.sh || exit $?

echo "Doing dynamic registration tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libxadrv_d.$SUFFIX
./run-dom.sh || exit $?

# vim: set ts=4 sw=4 et smartindent:
