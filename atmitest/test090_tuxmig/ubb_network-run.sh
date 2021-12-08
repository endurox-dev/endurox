#!/bin/bash
##
## @brief Execute networked UBB test
##
## @file ubb_network-run.sh
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

    . usr3_3/conf/set.site3
    xadmin stop -y
    xadmin down -y

    . usr4_3/conf/set.site4
    xadmin stop -y
    xadmin down -y

    . usr1_3/conf/set.site1
    xadmin stop -y
    xadmin down -y

    . usr2_3/conf/set.site2
    xadmin stop -y
    xadmin down -y

    . usr5_3/conf/set.site5
    xadmin stop -y
    xadmin down -y

    popd 2>/dev/null
    exit $1
}

#
# Enduro/X not yet booted
#
function go_out_silent {
    echo "Test exiting with: $1"

    popd 2>/dev/null
    exit $1
}

#
# Benchmark all node services 
#
function validate_links {

    exbenchcl -n5 -P -t5 -b '{}' -f EX_DATA -S1024 -s$1
    RET=$?
    if [ "X$RET" != "X0" ]; then
        echo "Failed to test service $1"
        go_out $RET
    fi
}

################################################################################
echo ">>> Testing ubb_network -> E/X convert"
################################################################################

#
# Cleanup by rndkey, maybe random...
#

xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}
xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}
xadmin ps -r "-k [a-zA-Z0-9]{8,8} -i" -p | xargs -i kill -9 {}

export NDRX_SILENT=Y
rm -rf ./runtime 2>/dev/null
../../migration/tuxedo/ubb2ex ubb_network -P ./runtime

RET=$?

if [ "X$RET" != "X0" ]; then
    go_out_silent $RET
fi

cd runtime
pushd .

echo ">>> Booting instances..."
. usr3_3/conf/set.site3
# cleanup shms...
xadmin down -y
xadmin start -y

RET=$?
if [ "X$RET" != "X0" ]; then
    echo "Failed to boot site3"
    go_out $RET
fi

. usr4_3/conf/set.site4
# cleanup shms...
xadmin down -y
xadmin start -y
RET=$?
if [ "X$RET" != "X0" ]; then
    echo "Failed to boot site4"
    go_out $RET
fi

. usr1_3/conf/set.site1
# cleanup shms...
xadmin down -y
xadmin start -y
RET=$?
if [ "X$RET" != "X0" ]; then
    echo "Failed to boot site1"
    go_out $RET
fi

. usr2_3/conf/set.site2
# cleanup shms...
xadmin down -y
xadmin start -y
RET=$?
if [ "X$RET" != "X0" ]; then
    echo "Failed to boot site2"
    go_out $RET
fi

. usr5_3/conf/set.site5
# cleanup shms...
xadmin down -y
xadmin start -y
RET=$?
if [ "X$RET" != "X0" ]; then
    echo "Failed to boot site5"
    go_out $RET
fi

echo ">>> Wait for connection"
sleep 60

echo ">>> Testing links from site1..."
. usr1_3/conf/set.site1
xadmin psc
validate_links "SERVER1"
validate_links "SERVER2"
validate_links "SERVER3"
validate_links "SERVER4"
validate_links "SERVER5"

echo ">>> Testing links from site2..."
. usr2_3/conf/set.site2
xadmin psc
validate_links "SERVER1"
validate_links "SERVER2"
validate_links "SERVER3"
validate_links "SERVER4"
validate_links "SERVER5"

echo ">>> Testing links from site3..."
. usr3_3/conf/set.site3
xadmin psc
validate_links "SERVER1"
validate_links "SERVER2"
validate_links "SERVER3"
validate_links "SERVER4"
validate_links "SERVER5"

echo ">>> Testing links from site4..."
. usr4_3/conf/set.site4
xadmin psc
validate_links "SERVER1"
validate_links "SERVER2"
validate_links "SERVER3"
validate_links "SERVER4"
validate_links "SERVER5"

echo ">>> Testing links from site5..."
. usr4_3/conf/set.site4
xadmin psc
validate_links "SERVER1"
validate_links "SERVER2"
validate_links "SERVER3"
validate_links "SERVER4"
validate_links "SERVER5"

echo ">>> Testing directories & files..."

if [ ! -f "usr1_4/log/ndrxd.log" ]; then
    echo "usr1_4/log/ndrxd.log does not exist."
    go_out -1
fi

if [ ! -f "usr2_4/log/ndrxd.log" ]; then
    echo "usr2_4/log/ndrxd.log does not exist."
    go_out -1
fi

if [ ! -f "usr3_4/log/ndrxd.log" ]; then
    echo "usr3_4/log/ndrxd.log does not exist."
    go_out -1
fi

if [ ! -f "usr4_4/log/ndrxd.log" ]; then
    echo "usr4_4/log/ndrxd.log does not exist."
    go_out -1
fi

if [ ! -d "usr4_1/app/tmlogs/rm4" ]; then
    echo "usr4_1/app/tmlogs/rm4 does not exist."
    go_out -1
fi

################################################################################
echo ">>> Compare outputs of the XML"
################################################################################

OUT=`diff $TESTDIR/ubb_network.xml $TESTDIR/runtime/usr4_3/conf/ndrxconfig.site4.xml`

echo $OUT

RET=$?
if [ "X$RET" != "X0" ]; then
    go_out $RET
fi

if [ "X$OUT" != "X" ]; then
    echo "ubb_network.xml!=runtime/usr4_3/conf/ndrxconfig.site4.xml"
    go_out -1
fi


go_out $RET

# vim: set ts=4 sw=4 et smartindent:

