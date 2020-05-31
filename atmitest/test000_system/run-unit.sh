#!/bin/bash
##
## @brief @(#) Unit tests for ATMI package
##
## @file run-unit.sh
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
TESTNAME="test000_system"

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
export NDRX_DEBUG_CONF=$TESTDIR/debug.conf
export NDRX_ULOG=$TESTDIR
export NDRX_SILENT=Y

# clean up the env for processing...
xadmin down -y
xadmin qrmall test000
rm *.log 2>/dev/null

# cleanup any running clients...
xadmin killall atmiunit0 2>/dev/null

RET=0

# Run unit test
#(./atmiunit0 2>&1) > ./atmiunit0.log

./atmiunit0

TMP=$?
if [ $TMP != 0 ]; then
    echo "./atmiunit0 failed"
    RET=-2
fi

xadmin killall atmiunit0 2>/dev/null

popd 2>/dev/null

exit $RET

# vim: set ts=4 sw=4 et smartindent:
