#!/bin/bash
## 
## @(#) Test encryption - test launcher
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware PlatfoRm for Distributed Transaction Processing
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
TESTNAME="test043_encrypt"

PWD=`pwd`
if [ `echo $PWD | grep $TESTNAME ` ]; then
	# Do nothing 
	echo > /dev/null
else
	# started from parent folder
	pushd .
	echo "Doing cd"
	cd test043_encrypt
fi;

. ../testenv.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR

xadmin killall atmi.sv43 2>/dev/null
xadmin killall atmiclt43 2>/dev/null

# client timeout
export NDRX_TOUT=10
export NDRX_DEBUG_CONF=`pwd`/debug.conf

function go_out {
    echo "Test exiting with: $1"
    xadmin killall atmi.sv43 2>/dev/null
    xadmin killall atmiclt43 2>/dev/null
    
    popd 2>/dev/null
    exit $1
}

rm *.log


ENCSTR=""

#
# Test standard encryption
# We test internal key with external plugin key
#
for i in 1 2
do
    if [ "X$i" == "X2" ]; then
        export NDRX_PLUGINS=libcryptohost.so

	if [ "$(uname)" == "Darwin" ]; then
        	echo "Darwin host"
        	export NDRX_PLUGINS=libcryptohost.dylib
	fi

    fi

    CLR="hello Test $i"

    ENCSTR=`exencrypt "$CLR"`
    RET=$?

    if [ "$RET" -ne "0" ]; then
            echo "Failed to encrypt..."
            go_out -3
    fi

    echo "Encrypted string: [$ENCSTR]"

    if [ "Xhello Test" == "X$ENCSTR" ]; then
            echo "ENCSTR must not be equal to non encrypted value!!!"
            go_out -4
    fi


    DECSTR=`exdecrypt $ENCSTR`
    RET=$?

    if [ "$RET" -ne "0" ]; then
            echo "Failed to decrypt..."
            go_out -5
    fi

    echo "Decrypted string: [$DECSTR] (clear: [$CLR])"

    if [ "Xhello Test $i" != "X$DECSTR" ]; then
            echo "DECSTR Must be equal to clear value, but got [$DECSTR]"
            go_out -6
    fi
done

unset NDRX_PLUGINS

#
# Now if we try to decryp the ENCSTR it should fail or value shall not be equal
# to clear string, because we are not using cryptohost but default built in
# crypto key function.
#

DECSTR=`exdecrypt $ENCSTR`
RET=$?

echo "Decrypted value (with different key): [$DECSTR]"

if [ "$RET" -eq "0" ] && [ "X$DECSTR" == "Xhello Test 2" ]; then
    echo "ERROR ! value shall not be decrypted successfully with different key!"
    go_out -7
fi

#
# Export plugin again. Now we will export encrypted string to the global
# section of the ini file. And this this env variable shall be readable ok
# from the client process
#
export NDRX_PLUGINS=libcryptohost.so

if [ "$(uname)" == "Darwin" ]; then
	echo "Darwin host"
	export NDRX_PLUGINS=libcryptohost.dylib
fi

echo "[@global]" > test.conf
echo "TEST_ENV_PARAM=\${dec=$ENCSTR}" >> test.conf

#
# Export CC Config
#
export NDRX_CCONFIG=`pwd`/test.conf


(./atmiclt43 2>&1) > ./atmiclt43.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	go_out -2
fi

go_out $RET

