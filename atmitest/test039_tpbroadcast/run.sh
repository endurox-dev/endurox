#!/bin/bash
## 
## @(#) Test039 - TP Broadcast tests
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

export TESTNO="039"
export TESTNAME_SHORT="tpbroadcast"
export TESTNAME="test${TESTNO}_${TESTNAME_SHORT}"

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
# We do not need timeout, we will kill procs...
export NDRX_TOUT=9999

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
    xadmin killall atmiclt39

    popd 2>/dev/null
    exit $1
}

#
# Check the lines of maching codes
#
function chk_lines {
    log_file=$1
    code=$2
    must_be=$3
    chk_num=$4

    CNT=`grep $code $log_file | grep T_STRING_FLD | wc | awk '{print $1}'`
    echo "$code count in $log_file: $CNT"

    if [[ $CNT -ne $must_be ]]; then
            echo "Actual $code $CNT != $must_be!"
            go_out $chk_num
    fi

}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 30
RET=0
MAX_CALLS=1000 # Same as in atmiclt39.c constant...

echo "Running off client"

#
# Dom 1 set
#
set_dom1;
(./atmicltA39 listen 2>&1 ) > ./atmicltA39-listen-dom1.log &
(./atmicltA39 mutted 2>&1 ) > ./atmicltA39-mutted-dom1.log  &

(./atmicltB39 listen 2>&1 ) > ./atmicltB39-listen-dom1.log  &
(./atmicltB39 mutted 2>&1 ) > ./atmicltB39-mutted-dom1.log  &

(./atmicltC39 listen 2>&1 ) > ./atmicltC39-listen-dom1.log  &
(./atmicltC39 mutted 2>&1 ) > ./atmicltC39-mutted-dom1.log  &

#
# Dom2
#
set_dom2;
(./atmicltA39 listen 2>&1 ) > ./atmicltA39-listen-dom2.log  &
(./atmicltA39 mutted 2>&1 ) > ./atmicltA39-mutted-dom2.log  &

(./atmicltB39 listen 2>&1 ) > ./atmicltB39-listen-dom2.log  &
(./atmicltB39 mutted 2>&1 ) > ./atmicltB39-mutted-dom2.log  &

(./atmicltC39 listen 2>&1 ) > ./atmicltC39-listen-dom2.log  &
(./atmicltC39 mutted 2>&1 ) > ./atmicltC39-mutted-dom2.log  &


# Let all listeners to wake up (start..)
sleep 5
#
# Broadcast from dom1
#
set_dom1;
# This our broadcaster...
(./atmicltA39 broadcast 2>&1) > ./atmicltA39-broadcast-dom1.log
RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# let the messages to completes it's processing...
sleep 10

#
# Broadcast of AA message to all clients
#

chk_lines "atmicltA39-broadcast-dom1.log" "AA01" "$MAX_CALLS" "1"

chk_lines "atmicltA39-listen-dom1.log" "AA01" "$MAX_CALLS" "2"
chk_lines "atmicltA39-mutted-dom1.log" "AA01" "0" "3"

chk_lines "atmicltB39-listen-dom1.log" "AA01" "$MAX_CALLS" "4"
chk_lines "atmicltB39-mutted-dom1.log" "AA01" "0" "5"

chk_lines "atmicltC39-listen-dom1.log" "AA01" "$MAX_CALLS" "6"
chk_lines "atmicltC39-mutted-dom1.log" "AA01" "0" "7"

chk_lines "atmicltA39-listen-dom2.log" "AA01" "$MAX_CALLS" "8"
chk_lines "atmicltA39-mutted-dom2.log" "AA01" "0" "9"

chk_lines "atmicltB39-listen-dom2.log" "AA01" "$MAX_CALLS" "10"
chk_lines "atmicltB39-mutted-dom2.log" "AA01" "0" "11"

chk_lines "atmicltC39-listen-dom2.log" "AA01" "$MAX_CALLS" "12"
chk_lines "atmicltC39-mutted-dom2.log" "AA01" "0" "13"


#
# Broadcast of BB message to .*B.* matching clients (regex)
#

chk_lines "atmicltA39-broadcast-dom1.log" "BB01" "0" "14"

chk_lines "atmicltA39-listen-dom1.log" "BB01" "0" "15"
chk_lines "atmicltA39-mutted-dom1.log" "BB01" "0" "16"

chk_lines "atmicltB39-listen-dom1.log" "BB01" "$MAX_CALLS" "17"
chk_lines "atmicltB39-mutted-dom1.log" "BB01" "0" "18"

chk_lines "atmicltC39-listen-dom1.log" "BB01" "0" "19"
chk_lines "atmicltC39-mutted-dom1.log" "BB01" "0" "20"

chk_lines "atmicltA39-listen-dom2.log" "BB01" "0" "21"
chk_lines "atmicltA39-mutted-dom2.log" "BB01" "0" "22"

chk_lines "atmicltB39-listen-dom2.log" "BB01" "$MAX_CALLS" "23"
chk_lines "atmicltB39-mutted-dom2.log" "BB01" "0" "24"

chk_lines "atmicltC39-listen-dom2.log" "BB01" "0" "25"
chk_lines "atmicltC39-mutted-dom2.log" "BB01" "0" "26"

#
# Broadcast of CC message to dom2 only
#
chk_lines "atmicltA39-broadcast-dom1.log" "CC01" "0" "27"

chk_lines "atmicltA39-listen-dom1.log" "CC01" "0" "28"
chk_lines "atmicltA39-mutted-dom1.log" "CC01" "0" "29"

chk_lines "atmicltB39-listen-dom1.log" "CC01" "0" "30"
chk_lines "atmicltB39-mutted-dom1.log" "CC01" "0" "31"

chk_lines "atmicltC39-listen-dom1.log" "CC01" "0" "32"
chk_lines "atmicltC39-mutted-dom1.log" "CC01" "0" "33"

chk_lines "atmicltA39-listen-dom2.log" "CC01" "$MAX_CALLS" "34"
chk_lines "atmicltA39-mutted-dom2.log" "CC01" "0" "35"

chk_lines "atmicltB39-listen-dom2.log" "CC01" "$MAX_CALLS" "36"
chk_lines "atmicltB39-mutted-dom2.log" "CC01" "0" "37"

chk_lines "atmicltC39-listen-dom2.log" "CC01" "$MAX_CALLS" "38"
chk_lines "atmicltC39-mutted-dom2.log" "CC01" "0" "39"


#
# Broadcast of DD message to atmicltC39 only
#
chk_lines "atmicltA39-broadcast-dom1.log" "DD01" "0" "40"

chk_lines "atmicltA39-listen-dom1.log" "DD01" "0" "41"
chk_lines "atmicltA39-mutted-dom1.log" "DD01" "0" "42"

chk_lines "atmicltB39-listen-dom1.log" "DD01" "0" "43"
chk_lines "atmicltB39-mutted-dom1.log" "DD01" "0" "44"

chk_lines "atmicltC39-listen-dom1.log" "DD01" "$MAX_CALLS" "45"
chk_lines "atmicltC39-mutted-dom1.log" "DD01" "0" "46"

chk_lines "atmicltA39-listen-dom2.log" "DD01" "0" "47"
chk_lines "atmicltA39-mutted-dom2.log" "DD01" "0" "48"

chk_lines "atmicltB39-listen-dom2.log" "DD01" "0" "49"
chk_lines "atmicltB39-mutted-dom2.log" "DD01" "0" "50"

chk_lines "atmicltC39-listen-dom2.log" "DD01" "$MAX_CALLS" "51"
chk_lines "atmicltC39-mutted-dom2.log" "DD01" "0" "52"

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
fi


go_out $RET



