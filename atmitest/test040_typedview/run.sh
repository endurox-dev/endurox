#!/bin/bash
## 
## @brief @(#) Test040 Launcher - STRING typed buffer test
##
## @file run.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or Mavimax's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 3 of the License, or (at your option) any later
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
TESTNAME="test040_typedview"

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

#
# Set view tables
#
export VIEWDIR=.

xadmin killall atmisv40 2>/dev/null
xadmin killall atmiclt40 2>/dev/null

rm *.log

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"

    xadmin killall atmisv40 2>/dev/null
    xadmin killall atmiclt40 2>/dev/null

    popd 2>/dev/null
    exit $1
}


################################################################################
# Test view files
################################################################################
#
# Invalid size of the NULL value
#
TVIEWNAME=tbad_view_invl_size.v_in
../../viewc/viewc $TVIEWNAME


if [ $? == 0 ]; then
    echo "ERROR: $TVIEWNAME must not compile, but compiled OK!"
    go_out 1
fi

#
# Invalid L flag, set for long (accepted only for string & carray)
#
TVIEWNAME=tbad_invalid_L.v_in
../../viewc/viewc $TVIEWNAME

if [ $? == 0 ]; then
    echo "ERROR: $TVIEWNAME must not compile, but compiled OK!"
    go_out 2
fi

#
# Invalid L flag, set for long (accepted only for string & carray)
#

TVIEWNAME=tbad_invalid_size_no_type.v_in
../../viewc/viewc $TVIEWNAME

if [ $? == 0 ]; then
    echo "ERROR: $TVIEWNAME must not compile, but compiled OK!"
    go_out 3
fi

################################################################################
# Run unit tests
################################################################################
export VIEWFILES=t40.V,t40_2.V
./viewunit1 > viewunit1.out 2>&1

cat viewunit1.out

#RES=`grep '0 failures, 0 exceptions' viewunit1.out`
RES=`grep Completed viewunit1.out | grep -v '0 failures, 0 exceptions'`

echo "RES=> [$RES]"

if [ "X$RES" != "X" ]; then
    echo "ERROR: Unit test failed, have some exceptions or failures!"
    go_out 20
fi


export NDRX_DEBUG_CONF=`pwd`/debug.conf

# Start event server
#(valgrind --track-origins=yes --leak-check=full ../../tpevsrv/tpevsrv -i 10 2>&1) > ./tpevsrv.log &
# NOTE: WE HAVE MEM LEAK HERE:
# Start subscribers
(./atmisv40 -t 4 -i 100 -xTEST40_V2JSON:VIEW2JSON -xTEST40_JSON2V:JSON2VIEW 2>&1) > ./atmisv40.log &
sleep 5
# Post the event
(./atmiclt40 2>&1) > ./atmiclt40.log
#(valgrind --show-reachable=yes --leak-check=full ./atmiclt40 2>&1) > ./atmiclt40.log

RET=$?

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi


#killall atmiclt1

popd 2>/dev/null

go_out $RET


