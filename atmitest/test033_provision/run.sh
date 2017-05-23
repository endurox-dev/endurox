#!/bin/bash
## 
## @(#) Test33 Test provision script
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
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


