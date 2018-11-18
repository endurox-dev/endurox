#!/bin/bash
##
## @brief @(#) Test028 - Trnsactional Message Queue tests (with static & dynamic XA drivers)
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
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

export TESTNO="028"
export TESTNAME_SHORT="tmq"
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

#
# Dynamic tests
#
echo "Dynamic XA driver tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdiskd.so

if [ "$(uname)" == "Darwin" ]; then
	export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdiskd.dylib
fi

./run-dom.sh
RET=$?

if [[ "X$RET" != "X0" ]]; then
    exit 1
fi


#
# Static tests
#
echo "Static XA driver tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdisks.so

if [ "$(uname)" == "Darwin" ]; then
	export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdisks.dylib
fi

./run-dom.sh
RET=$?

if [[ "X$RET" != "X0" ]]; then
    exit 1
fi

# vim: set ts=4 sw=4 et smartindent:
