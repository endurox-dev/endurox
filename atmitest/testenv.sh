#!/bin/bash
##
## @file testenv.sh
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
export NDRX_QPREFIX=/mccp

# Load UBF, we assume that we are inside of test dir!
. ../../sampleconfig/setndrx
. ../../ubftest/setenv
unset NDRX_DEBUG_CONF
export FLDTBLDIR=../../ubftest/ubftab
export PATH=$PATH:../../xadmin

export PSCMD="ps -efww"
if [ "$(uname)" == "FreeBSD" ]; then
    export PSCMD="ps -auwwx"
elif [ "$(uname)" == "SunOS" ]; then
    export PSCMD="ps -ef"
fi

# this is due to fact that for SystemV reasons we
# open the shared memory segments by clients too.
xadmin down -y >/dev/null 2>&1

# Clean up q Space (remove all queues matching the symbol)
xadmin qrmall , > /dev/null 2>&1

# any left overs from previous tests...
xadmin killall ndrxd > /dev/null 2>&1

if [[ `xadmin poller 2>/dev/null` == "SystemV" ]]; then
    xadmin udown -y > /dev/null 2>&1
fi

# vim: set ts=4 sw=4 et smartindent:
