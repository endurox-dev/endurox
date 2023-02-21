#!/bin/bash
##
## @brief Test that XATMI server reload happens only when original is fully shutdown (even if it freezed at shutdown) - test launcher
##
## @file run.sh
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

export TESTNAME="test101_dupsrv"

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
export NDRX_SILENT="Y"

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

set_dom1_no_respawn() {
    echo "Setting domain 1 (no respawn)"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1_no_respawn.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

set_dom1_pingkill() {
    echo "Setting domain 1 (pingkill)"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1_pingkill.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

set_dom1_reload() {
    echo "Setting domain 1 (reload)"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1_reload.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
}

#
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt101

    popd 2>/dev/null
    exit $1
}

# Call the service to lock-up
function run_call {

    (./atmiclt101 2>&1) >> ./atmiclt-dom1.log

    RET=$?

    if [[ "X$RET" != "X0" ]]; then
        go_out $RET
    fi
}

PID1=""
PID2=""

# Ensure that PIDs are not equal
# as process must restarted by Enduor/X
function get_pid_assert_not_equal {

    if [ "X$PID1" == "X$PID2" ]; then
        echo "PIDs are equal: [$PID1] == [$PID2]"
        go_out -1
    fi
}

function get_pid_assert_equal {

    if [ "X$PID1" != "X$PID2" ]; then
        echo "PIDs are not equal: [$PID1] == [$PID2]"
        go_out -1
    fi
}

function get_pid_before {
    ps -ef | grep atmi.sv101 | grep -v grep
    PID1=`xadmin ps -a atmi.sv101 | awk '{print $2}'`
}

function get_pid_after {
    ps -ef | grep atmi.sv101 | grep -v grep
    PID2=`xadmin ps -a atmi.sv101 | awk '{print $2}'`
    get_pid_assert_not_equal;
}

function get_pid_after_eq {
    ps -ef | grep atmi.sv101 | grep -v grep
    PID2=`xadmin ps -a atmi.sv101 | awk '{print $2}'`
    get_pid_assert_equal;
}

rm *.log 2>/dev/null
rm ULOG* 2>/dev/null
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y

# check config not loaded case for Support #448
# really small check, no need for new atmitest, add here:

NO_CONFIG_LOADED=`xadmin sreload atmi.sv101 2>&1`

if [[ "$NO_CONFIG_LOADED" == "fail, code: 25: NDRXD_ENOCFGLD (last error 25: No configuration loaded!)" ]]; then
    echo "Invalid error message in case if configuration not loaded and doing reload."
    go_out -1
fi

xadmin start -y || go_out 1

RET=0
xadmin psc
xadmin ppm

for i in 0 1
do

    if [ "$i" == "0" ]; then
        echo ">>> Testing with respawn enabled"
        #set_dom1; already in this mode
    else
        echo ">>> Testing with respawn disabled"
        set_dom1_no_respawn;
        xadmin start -y
    fi

    ################################################################################
    echo ">>>> xadmin sreload"
    ################################################################################
    get_pid_before;

    run_call;
    xadmin sreload atmi.sv101
    xadmin ppm
    sleep 30

    get_pid_after;

    CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
    # there shall be no duplicate servers...
    if [ "X$CNT" != "X1"  ]; then
        echo "Number of atmi.sv101 processes present: $CNT but must be 1"
        go_out -1
    fi

    ################################################################################
    echo ">>>> Reload On Change"
    ################################################################################
    get_pid_before;
    touch atmi.sv101
    sleep 20

    get_pid_after;

    CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
    # there shall be no duplicate servers...
    if [ "X$CNT" != "X1"  ]; then
        echo "Number of atmi.sv101 processes present: $CNT but must be 1"
        go_out -1
    fi

    ################################################################################
    echo ">>>> xadmin stop + start"
    ################################################################################

    get_pid_before;

    run_call;
    xadmin stop -s atmi.sv101
    xadmin start -s atmi.sv101
    sleep 30

    get_pid_after;

    CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
    # there shall be no duplicate servers...
    if [ "X$CNT" != "X1"  ]; then
        echo "Number of atmi.sv101 processes present: $CNT but must be 1"
        go_out -1
    fi

    ################################################################################
    echo ">>>> xadmin restart"
    ################################################################################

    get_pid_before;

    run_call;
    xadmin restart -s atmi.sv101
    sleep 30

    get_pid_after;

    CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
    # there shall be no duplicate servers...
    if [ "X$CNT" != "X1"  ]; then
        echo "Number of atmi.sv101 processes present: $CNT but must be 1"
        go_out -1
    fi

    ################################################################################
    echo ">>>> xadmin full shutdown (ensure no process left)"
    ################################################################################
    run_call;
    xadmin stop -y

    CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
    # there shall be no duplicate servers...
    if [ "X$CNT" != "X0"  ]; then
        echo "Number of atmi.sv101 processes present: $CNT but must be 0"
        go_out -1
    fi

done

################################################################################
echo ">>> Ensure that ping with respawn off does not boot server back..."
################################################################################
set_dom1_pingkill;
xadmin start -y
run_call;
sleep 15

CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
# there shall be no duplicate servers...
if [ "X$CNT" != "X0"  ]; then
    echo "Number of atmi.sv101 processes present: $CNT but must be 0"
    go_out -1
fi
xadmin stop -y

################################################################################
echo ">>> ROC - Reload only started copies (Bug #202)"
################################################################################

set_dom1_reload;
xadmin start -y

get_pid_before;
touch atmi.sv101
sleep 20
get_pid_after;

CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
# there shall be no duplicate servers...
if [ "X$CNT" != "X3"  ]; then
    echo "Number of atmi.sv101 processes present: $CNT but must be 3"
    go_out -1
fi

################################################################################
echo ">>> sreload - Reload only started copies (Bug #202)"
################################################################################

set_dom1_reload;
xadmin start -y

get_pid_before;
xadmin sreload atmi.sv101
get_pid_after;

CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
# there shall be no duplicate servers...
if [ "X$CNT" != "X3"  ]; then
    echo "Number of atmi.sv101 processes present: $CNT but must be 3"
    go_out -1
fi

################################################################################
echo ">>> sreload by instances..."
################################################################################

xadmin stop -s atmi.sv101
xadmin start -i 17
xadmin start -i 18
xadmin start -i 19

# Ensure that first instances are not reload as marked as not started
get_pid_before
xadmin sreload -i 10
# i.e. pid not changed
get_pid_after_eq

get_pid_before
xadmin sreload -i 11
get_pid_after_eq

# Ensure that each reloads and pid changes
get_pid_before
xadmin sreload -i 17
get_pid_after

get_pid_before
xadmin sreload -i 18
get_pid_after

get_pid_before
xadmin sreload -i 19
get_pid_after

CNT=`xadmin ps -a atmi.sv101 | wc | awk '{print($1)}'`
# there shall be no duplicate servers...
if [ "X$CNT" != "X3"  ]; then
    echo "Number of atmi.sv101 processes present: $CNT but must be 3"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

