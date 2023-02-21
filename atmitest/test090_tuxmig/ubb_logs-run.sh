#!/bin/bash
##
## @brief Test error output folder merging to NDRX_ULOG or NDRX_APPHOME
##
## @file ubb_logs-run.sh
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

(xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs kill -9) 2>/dev/null
(xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs kill -9) 2>/dev/null
(xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs kill -9) 2>/dev/null

export NDRX_SILENT=Y
rm -rf ./runtime tmp1 tmp2 2>/dev/null
ubb2ex -P ./runtime ubb_logs

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out_silent $RET
fi


# Start the runtime

pushd .

cd runtime/appdir/app/conf
. setsite1

popd

# cleanup shms...
xadmin down -y
xadmin start -y

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

xadmin psc
# We are done...
xadmin stop -y

################################################################################
echo ">>> Check file existence..."
################################################################################

if [ ! -f runtime/logs/log/a1.log ]; then
    echo "Missing runtime/logs/log/a1.log"
    go_out -1
fi

if [ ! -f runtime/logs/log/sub/a2.log ]; then
    echo "Missing runtime/logs/log/sub/a2.log"
    go_out -1
fi

if [ ! -f runtime/appdir/app/log/third.log ]; then
    echo "Missing runtime/appdir/app/log/third.log"
    go_out -1
fi

if [ ! -f runtime/appdir/app/dir/third2.log ]; then
    echo "Missing runtime/appdir/app/dir/third2.log"
    go_out -1
fi

if [ ! -f runtime/other/app/dir/forth.log ]; then
    echo "Missing runtime/other/app/dir/forth.log"
    go_out -1
fi

################################################################################
echo ">>> Compare outputs of the XML"
################################################################################

# prepare ini files, strip the dynamic parts
cat $TESTDIR/ubb_logs.xml                                   | \
    grep -v forth.log        > tmp1

cat $TESTDIR/runtime/appdir/app/conf/ndrxconfig.site1.xml   | \
    grep -v forth.log        > tmp2

OUT=`diff tmp1 tmp2`

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

echo $OUT

if [ "X$OUT" != "X" ]; then
    echo "$TESTDIR/ubb_logs.xml!=$TESTDIR/runtime/appdir/app/conf/ndrxconfig.site1.xml"
    go_out -1
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

