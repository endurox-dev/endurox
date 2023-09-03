#!/bin/bash
##
## @brief Singleton group tests - test launcher
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

export TESTNAME="test103_singlegrp"

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
    export NDRX_CCONFIG=$TESTDIR/app-dom1.ini
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
    export NDRX_CCONFIG=$TESTDIR/app-dom2.ini
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
    xadmin killall atmiclt103

    popd 2>/dev/null
    exit $1
}

rm *dom*.log
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

################################################################################
echo ">>> Basic tests of no lock at the boot"
################################################################################
#
set_dom2;
xadmin down -y
xadmin start -y || go_out 2

xadmin psc
xadmin ppm
#echo "Running off client"
#

# TODO: validate startup (wait on grou plock)
# validated PPM, "wait" state.

################################################################################
echo ">>> Basic tests of singleton groups -> group start, check order"
################################################################################

xadmin psg -a
xadmin start -i  10
# Let processes to lock and boot
sleep 7

CMD="xadmin ppm"
echo "$CMD"
OUT=`$CMD 2>&1`

echo "got output [$OUT]"

CNT=`xadmin ppm | grep atmi.sv1 | grep 'start runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "1" ]; then
    echo "Expected 1 in start state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin ppm | grep atmi.sv1 | grep 'wait  runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "10" ]; then
    echo "Expected 10 atmi.sv103 processes in wait state, got [$CNT]"
    go_out -1
fi

xadmin ps -a atmi.sv103
CNT=`xadmin ps -a atmi.sv103 -b nZ22K8K7kewKo | wc | awk '{print $1}'`
if [ "$CNT" -ne "1" ]; then
    echo "Expected 1 atmi.sv103 running, got [$CNT]"
    go_out -1
fi

# Ensure that PSG is locked but server & client are not booted
CMD="xadmin psg"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="SGID LCKD MMON SBOOT CBOOT LPSRVID    LPPID LPPROCNM          REFRESH RSN FLAGS
---- ---- ---- ----- ----- ------- -------- ---------------- -------- --- -----
   1 Y    N    N     N          10    [0-9]+ exsinglesv             .*   0 i"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected group to be locked, but not client/servers booted"
    go_out -1
fi

CMD="xadmin pc"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="TAG1/SUBSECTION1 - waiting on process group 1 lock.*
TAG2/SUBSECTION2 - waiting on process group 1 lock.*"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected to wait on group lock by the clients."
    go_out -1
fi

# Server 50 shall be starting, all other shall be still in wait.
echo ">>> wait 25 for full boot..."
sleep 25
xadmin ppm

CNT=`xadmin ppm | grep atmi.sv1 | grep 'runok runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "11" ]; then
    echo "Expected 11 atmi.sv103 processes in start state, got [$CNT]"
    go_out -1
fi

xadmin ps -a atmi.sv103
CNT=`xadmin ps -a atmi.sv103 -b nZ22K8K7kewKo | wc | awk '{print $1}'`
if [ "$CNT" -ne "11" ]; then
    echo "Expected 11 atmi.sv103 running, got [$CNT]"
    go_out -1
fi

# Ensure that group is fully booted.
CMD="xadmin psg"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="SGID LCKD MMON SBOOT CBOOT LPSRVID    LPPID LPPROCNM          REFRESH RSN FLAGS
---- ---- ---- ----- ----- ------- -------- ---------------- -------- --- -----
   1 Y    N    Y     Y          10    [0-9]+ exsinglesv             .*   0 i"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected group to be locked, but not client/servers booted"
    go_out -1
fi

CMD="xadmin pc"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="TAG1/SUBSECTION1 - running pid .*
TAG2/SUBSECTION2 - running pid .*"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected TAG1/SUBSECTION1 and TAG2/SUBSECTION2 to be running"
    go_out -1
fi

# TODO: Check no-order startup sequence
# TODO: Check normal startup with immediate lock
# TODO: Check failover from dom2 -> dom1 (with lock delay)
# TODO: Check lock broken (locked by some other process) -> shall unlock immediately

RET=0

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

