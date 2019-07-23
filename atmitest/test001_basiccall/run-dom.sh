#!/bin/bash
##
## @brief @(#) Test001 - clusterised version
##
## @file run-dom.sh
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

export TESTNO="001"
export TESTNAME_SHORT="basiccall"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"
export NDRX_SILENT=Y

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
export PATH=$PATH:$TESTDIR
# Override timeout!
export NDRX_TOUT=3

#
# Domain 1 - here client will live
#
function set_dom1 {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

#
# Domain 2 - here server will live
#
function set_dom2 {
    echo "Setting domain 2"
    . ../dom2.sh    
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom2.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom2.log
    export NDRX_LOG=$TESTDIR/ndrx-dom2.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    set_dom2;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt1

    popd 2>/dev/null
    exit $1
}

#
# Print domain statuses...
#
function print_domains {

    set_dom1;
    xadmin ppm
    xadmin psvc
    xadmin psc

    set_dom2;
    xadmin ppm
    xadmin psvc
    xadmin psc

}


rm *dom*.log

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

#exit 0
###############################################################################
echo "Test restart command (shall not stall...)"
###############################################################################

xadmin r -y
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Restart fails!"
    go_out -100
fi

###############################################################################
echo "Have some wait for ndrxd goes in service - wait for connection establishment."
echo "Test Connection recovery... (bug #250)"
###############################################################################
sleep 20
print_domains;

# Go to domain 1
set_dom1;

# test Bug #250 - test conn recovery

echo ">>> DOM1 network shutdown..."
xadmin stop -s tpbridge
# links must be lost
print_domains;
set_dom1;
xadmin start -s tpbridge


sleep 20
echo ">>> After DOM1 network shutdown..."
print_domains;


# Go to domain 2
set_dom2;

echo ">>> DOM2 network shutdown..."
xadmin stop -s tpbridge
print_domains;
set_dom2;
xadmin start -s tpbridge

###############################################################################
echo "Now continue with standard tests.."
###############################################################################
sleep 20
echo "PRINT DOMAINS"
print_domains;
set_dom1;
xadmin ppm

# check that services are blacklisted (others will be tested by tpcalls)

# UNIXINFO and UNIX2 must be missing

echo "DOM1 PSC"
xadmin psc 

if [ "X`xadmin psc | grep UNIXINFO`" != "X" ]; then
	echo "UNIXINFO is present in domain1!";
fi

if [ "X`xadmin psc | grep UNIX2`" != "X" ]; then
	echo "UNIX2 is present in domain1!";
fi

# Run the client test...
echo "Will issue calls to clients:"
(./atmiclt1 2>&1) > ./atmiclt-dom1.log
RET=$?

grep "Performance" atmiclt-dom1.log

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
