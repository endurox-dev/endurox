#!/bin/bash
##
## @brief DDR functionality tests - test launcher
##
## @file run.sh
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

export TESTNAME="test084_ddr"

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
export NDRX_ULOG=$TESTDIR
export NDRX_TOUT=10
export NDRX_SILENT=Y

#
# Domain 1 - here client will live
#
set_dom1() {
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
set_dom2() {
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
    set +x
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y


    set_dom2;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt84

    popd 2>/dev/null
    exit $1
}


export NDRX_SVCMAX=43

rm *.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

# start with long
cp ndrxconfig-dom1-long.xml ndrxconfig-dom1.xml

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

# Have some wait for ndrxd goes in service - wait for connection establishment.
echo "Wait 10 for conn"
sleep 10
RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;

################################################################################
echo "*** Check routes... (long type)"
################################################################################

set -x

./atmiclt84 -STESTSV -l-200 -gTESTSV@DOM1 -e0 || go_out 1

./atmiclt84 -STESTSV -l-15 -gTESTSV@DOM2 -e0 || go_out 1

# no routing range:
./atmiclt84 -STESTSV -l-11 -e12 || go_out 1

./atmiclt84 -STESTSV -l1 -gTESTSV@DOM1 -e0 || go_out 2

# still in DOM1
./atmiclt84 -STESTSV -l5 -gTESTSV@DOM1 -e0 || go_out 3

# DOM2
./atmiclt84 -STESTSV -l6 -gTESTSV@DOM2 -e0 || go_out 4

# DOM2
./atmiclt84 -STESTSV -l7 -gTESTSV@DOM2 -e0 || go_out 5

# DEFAULT GRP
./atmiclt84 -STESTSV -l8 -gTESTSV -e0 || go_out 6

# DEFAULT GRP
./atmiclt84 -STESTSV -l99999 -gTESTSV -e0 || go_out 7

# Prev match -> no service
./atmiclt84 -ST2 -l200 -gT2@DOM3 -e6 || go_out 8

# DEFAULT MATCH
./atmiclt84 -ST2 -l201 -gT2 -e0 || go_out 9


################################################################################
echo "*** Check routes... (double type)"
################################################################################

# check with double
cp ndrxconfig-dom1-double.xml ndrxconfig-dom1.xml
xadmin reload
echo "Echo wait 5 for DDR update to apply..."
sleep 5

./atmiclt84 -STESTSV -d-11 -gTESTSV@DOM1 -e0 || go_out 1

./atmiclt84 -STESTSV -d-1 -e12 || go_out 1

./atmiclt84 -STESTSV -d-1.1 -e0 -gTESTSV@DOM2 || go_out 1

./atmiclt84 -STESTSV -d1.2 -e0 -gTESTSV@DOM1 || go_out 1

./atmiclt84 -STESTSV -d1.011 -e0 -gTESTSV@DOM2 || go_out 1

./atmiclt84 -STESTSV -d9999.555 -e0 -gTESTSV || go_out 1


################################################################################
echo "Check routes... (string type)"
################################################################################

cp ndrxconfig-dom1-string.xml ndrxconfig-dom1.xml
xadmin reload
# start G8 server
xadmin start -y
echo "Echo wait 5 for DDR update to apply..."
sleep 5

xadmin psc

./atmiclt84 -STESTSV -s0 -gTESTSV@DOM1 -e0 || go_out 1
./atmiclt84 -STESTSV -sA -gTESTSV@DOM1 -e0 || go_out 1
./atmiclt84 -STESTSV -sCC -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -STESTSV -sD -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -STESTSV -sFF -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -STESTSV -sMAX -gTESTSV@DOM3 -e0 || go_out 1
./atmiclt84 -STESTSV -sN -gTESTSV -e0 || go_out 1
./atmiclt84 -STESTSV -sO -gTESTSV -e0 || go_out 1
./atmiclt84 -STESTSV -sMIN -gTESTSV@DOM4 -e0 || go_out 1

################################################################################
echo "Forward tests"
################################################################################

./atmiclt84 -SFWDSV -s0 -gTESTSV@DOM1 -e0 || go_out 1
./atmiclt84 -SFWDSV -sA -gTESTSV@DOM1 -e0 || go_out 1
./atmiclt84 -SFWDSV -sCC -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -SFWDSV -sD -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -SFWDSV -sFF -gTESTSV@DOM2 -e0 || go_out 1
./atmiclt84 -SFWDSV -sMAX -gTESTSV@DOM3 -e0 || go_out 1
./atmiclt84 -SFWDSV -sN -gTESTSV -e0 || go_out 1
./atmiclt84 -SFWDSV -sO -gTESTSV -e0 || go_out 1
./atmiclt84 -SFWDSV -sMIN -gTESTSV@DOM4 -e0 || go_out 1


################################################################################
echo "Connect tests"
################################################################################

./atmiclt84 -STESTSV -s0 -gTESTSV@DOM1 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sA -gTESTSV@DOM1 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sCC -gTESTSV@DOM2 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sD -gTESTSV@DOM2 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sFF -gTESTSV@DOM2 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sMAX -gTESTSV@DOM3 -e0 -C || go_out 1
./atmiclt84 -STESTSV -sN -gTESTSV -e0 -C || go_out 1
./atmiclt84 -STESTSV -sO -gTESTSV -e0 -C || go_out 1
./atmiclt84 -STESTSV -sMIN -gTESTSV@DOM4 -e0 -C || go_out 1

set +x

################################################################################
echo "Advertise checks"
################################################################################
# tpadvertise/unadvertise some service at atmi84sv ->  it must not appear
# perform dynamic advertise + unadvertise -> via atmicl83 call?
# 

CNT=`xadmin psc | grep UNASV | wc -l`
if [[ "X$CNT" != "X0" ]]; then
    echo "Expected missing UNASV but got $CNT"
    go_out 1
fi

echo "Testing dynamic advertise of the groupp:"
#
# Needs to test atomic -> needs to have both
# or needs to have 0
#
for ((n=0;n<50;n++)); do

    SVCNM="EXSV$n"
    echo "About to advertise: $SVCNM"

    #We accept either error 11
    ./atmiclt84 -SDADV -s$SVCNM -gDADV -e11 || go_out 1

    num_adv=`xadmin psc | grep $SVCNM | wc -l`

    # for lowers it is mandatory to have 2 as slot count is enough
    if [[ "$n" -lt "9" && "$num_adv" -ne "2" ]]; then
        echo "Expected for $n to have 2 service of $SVCNM but got: $num_adv"
        go_out 1
    fi

    # for uppers, either 0 or 2
    if [[ "$num_adv" -ne "0" && "$num_adv" -ne "2" ]]; then
        echo "Expected for $n to have 0 or 2 service of $SVCNM but got: $num_adv"
        go_out 1
    fi

done


echo "Testing dynamic un-advertise of the groups"

#
# Needs to test atomic -> needs to have both
# or needs to have 0
#
for ((n=0;n<50;n++)); do

    SVCNM="EXSV$n"
    echo "About to un-advertise: $SVCNM"

    #We accept either error 11
    ./atmiclt84 -SDUNA -s$SVCNM -gDUNA -e11 || go_out 1

    num_adv=`xadmin psc | grep $SVCNM | wc -l`

    # for lowers it is mandatory to have 2 as slot count is enough
    if [[ "$num_adv" -ne "0" ]]; then
        echo "Expected for $n to have 0 service of $SVCNM but got: $num_adv"
        go_out 1
    fi

done


echo "Check xadmin unadvertise (not working in groups):"
num_adv=`xadmin psc | grep DADV | wc -l`

echo "num_adv=$num_adv"
if [[ "X$num_adv" -ne  "X2" ]]; then
    echo "Expected 2 DADV got: $num_adv"
    go_out 1
fi

xadmin unadv -i 420 -s DADV
num_adv=`xadmin psc | grep DADV | wc -l`
echo "num_adv=$num_adv"
if [[ "X$num_adv" -ne  "X1" ]]; then
    echo "Expected 1 DADV got: $num_adv"
    go_out 1
fi

xadmin unadv -i 420 -s DADV@DOM4
num_adv=`xadmin psc | grep DADV | wc -l`
echo "num_adv=$num_adv"
if [[ "X$num_adv" -ne  "X0" ]]; then
    echo "Expected 0 DADV got: $num_adv"
    go_out 1
fi

################################################################################
# Reload tests during high-load
# Create generic tool -> exbench ? (basic version) call exbenchsv in N threads
# for X minutes. Meanwhile in background perform intensive reload of ddr.
# Also ... give some acceptable error codes like TPESYSTEM (and write some log)
# we could also create at test point like ndrx_G_testpoint_ddr_sleep X seconds
# thus several versions of DDR could change and then we could get TPESYSTEM
# ... the other option would be to leave as is and assume that exbenchcl with 
# out tpesystem shall cover all things
################################################################################


# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

