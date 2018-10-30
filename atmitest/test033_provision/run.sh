#!/bin/bash
##
## @brief @(#) Test33 Test provision script
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
TESTNAME="test033_provision"

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

#
# Test for service existance
# Returns 1 if ok
# returns 0 if fail
#
function testSvc {

    
    OUT=`xadmin psc | grep $1`

    echo "Got output: [$OUT]"

    if [ "X$OUT" == "X" ]; then

        echo "$1 service missing!"

        return 0
    fi

    return 1
}

pushd .

rm -rf runtime 2>/dev/null

mkdir runtime

xadmin killall ndrxd

cd runtime

xadmin provision -d

cd conf

. settest1

xadmin down -y

xadmin start -y || exit 1

xadmin psc


# Run some tests for service existance 

RET=0

if testSvc cconfsrv; then
    RET=1
fi

if testSvc tpevsrv; then
    RET=1
fi

if testSvc tmsrv; then
    RET=1
fi

if testSvc tmqueue; then
    RET=1
fi

if testSvc tpbridge; then
    RET=1
fi

if testSvc cpmsrv; then
    RET=1
fi

xadmin stop -c -y

popd 

echo "Test exists with $RET"

exit $RET


# vim: set ts=4 sw=4 et smartindent:
