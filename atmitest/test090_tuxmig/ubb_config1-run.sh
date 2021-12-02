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

rm -rf ./runtime 2>/dev/null
../../migration/tuxedo/tmloadcf ubb_config1 -P ./runtime

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

# Start the runtime

pushd .

cd runtime/user90/conf
. set.test1

xadmin start -y

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

xadmin pc


go_out $RET

# vim: set ts=4 sw=4 et smartindent:

