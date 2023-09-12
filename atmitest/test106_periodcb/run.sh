#!/bin/bash
##
## @brief Verify time accurrancy of the tpext_addperiodcb() callbacks - test launcher (Bug #811)
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

TESTNAME="test106_periodcb"

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test106_periodcb
fi;

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
export NDRX_ULOG=$TESTDIR
export NDRX_SCANUNIT=10

xadmin killall atmi.sv106 2>/dev/null
xadmin killall atmiclt106 2>/dev/null

# client timeout
export NDRX_TOUT=10
export NDRX_ULOG=$TESTDIR
export NDRX_DEBUG_CONF=`pwd`/debug.conf

function go_out {
    echo "Test exiting with: $1"
    xadmin killall atmi.sv106 2>/dev/null
    xadmin killall atmiclt106 2>/dev/null
    popd 2>/dev/null
    exit $1
}

rm *.log 2>/dev/null

(./atmi.sv106 -i123 2>&1) > ./atmisv106.log &
sleep 1
./atmiclt106

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out -2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
