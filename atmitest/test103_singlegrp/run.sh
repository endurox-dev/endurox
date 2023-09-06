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

# This gives:
# 3 sec check time for exsinglesv
# 20 sec lock take over time
export NDRX_SGREFRESH=10

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
    #export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
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
    #export NDRX_DEBUG_CONF=$TESTDIR/debug-dom2.conf
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

function validate_OK1_lock_loss {

    # have some debug:
    xadmin ppm
    xadmin psg

    CNT=`xadmin ppm | grep atmi.sv1 | grep 'wait  runok' | wc | awk '{print $1}'`
    if [ "$CNT" -ne "11" ]; then
        echo "Expected 11 atmi.sv103 processes in wait state, got [$CNT] (after the lock lost)"
        go_out -1
    fi

    # validate the xadmin pc output
    CMD="xadmin pc"
    echo "$CMD"
    OUT=`$CMD 2>&1`

    PATTERN="TAG1/SUBSECTION1 - waiting on process group 1 lock.*
TAG2/SUBSECTION2 - waiting on process group 1 lock.*
TAG3/- - running pid [0-9]+ .*"

    echo "got output [$OUT]"

    if ! [[ "$OUT" =~ $PATTERN ]]; then
        echo "Expected to wait on group lock by the clients."
        go_out -1
    fi

}

#
# Test recover after lock loss
#
function validate_OK1_recovery {

    # the lock is not returned immediately, but instead of 20 sec...
    echo "Wait 5 to check locked_wait"
    sleep 5
    xadmin ppm
    xadmin psg

    # we shall still wait on lock, as exsignlesv was restarted
    CNT=`xadmin ppm | grep atmi.sv1 | grep 'wait  runok' | wc | awk '{print $1}'`
    if [ "$CNT" -ne "11" ]; then
        echo "Expected 11 atmi.sv103 processes in wait state, got [$CNT] (after the lock lost)"
        go_out -1
    fi

    echo "Wait 30 (lock regain + wait) check locked_wait + boot order booted all processes..."
    sleep 30
    xadmin ppm
    xadmin psg

    CNT=`xadmin ppm | grep atmi.sv1 | grep 'runok runok' | wc | awk '{print $1}'`
    if [ "$CNT" -ne "11" ]; then
        echo "Expected 11 atmi.sv103 processes in runok state, got [$CNT] (after the lock lost)"
        go_out -1
    fi

    # validate the xadmin pc output
    CMD="xadmin pc"
    echo "$CMD"
    OUT=`$CMD 2>&1`

    PATTERN="TAG1/SUBSECTION1 - running pid [0-9]+ .*
TAG2/SUBSECTION2 - running pid [0-9]+ .*
TAG3/- - running pid [0-9]+ .*"

    echo "got output [$OUT]"

    if ! [[ "$OUT" =~ $PATTERN ]]; then
        echo "Expected to wait on group lock by the clients."
        go_out -1
    fi
}

# clean up some old stuff
rm *.log 2>/dev/null
rm lock_* 2>/dev/null
rm ULOG* 2>/dev/null
rm -f *.status 2>/dev/null

# Any bridges that are live must be killed!
xadmin killall tpbridge

#set_dom1;
#xadmin down -y
#xadmin start -y || go_out 1

# lets start with Domain 2. While the Domain 1 is in cold state...

################################################################################
echo ">>> Basic tests of no lock at the boot"
################################################################################

set_dom2;
xadmin down -y
CMD="xadmin start -y 2>&1"
echo "$CMD"
OUT=`$CMD 2>&1`

#
# Validate that we wait on group lock. And group 2 starts OK
#
PATTERN="exec atmi.sv103 -k nZ22K8K7kewKo -i 50 -e .*\/test103_singlegrp\/atmisv-dom2.log -r -- -w10 --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 100 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 101 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 102 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 103 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 104 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 105 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 106 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 107 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 108 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec atmi.sv103 -k nZ22K8K7kewKo -i 109 -e .*\/atmitest\/test103_singlegrp\/atmisv-dom2.log -r  --  :
[[:space:]]process id=0 ... Waiting on group lock.
exec tpbridge -k nZ22K8K7kewKo -i 2300 -e .*\/atmitest\/test103_singlegrp\/bridge-dom2.log -r -- -f -n1 -r -i 0.0.0.0 -p 20003 -tP -z30 -P0 :
[[:space:]]process id=[0-9]+ ... Started.
exec exsinglesv -k nZ22K8K7kewKo -i 3000 -e .*\/atmitest\/test103_singlegrp\/exsinglesv2-dom2.log -r --  :
[[:space:]]process id=[0-9]+ ... Started.
exec atmi103_v2 -k nZ22K8K7kewKo -i 4000 -e .*\/atmitest\/test103_singlegrp\/atmi103_v2-dom2.log -r  --  :
[[:space:]]process id=[0-9]+ ... Started.
exec atmi103_v2 -k nZ22K8K7kewKo -i 4001 -e .*\/atmitest\/test103_singlegrp\/atmi103_v2-dom2.log -r  --  :
[[:space:]]process id=[0-9]+ ... Started.
exec atmi103_v2 -k nZ22K8K7kewKo -i 4002 -e .*\/atmitest\/test103_singlegrp\/atmi103_v2-dom2.log -r  --  :
[[:space:]]process id=[0-9]+ ... Started.
exec atmi103_v2 -k nZ22K8K7kewKo -i 4003 -e .*\/atmitest\/test103_singlegrp\/atmi103_v2-dom2.log -r  --  :
[[:space:]]process id=[0-9]+ ... Started.
exec atmi103_v2 -k nZ22K8K7kewKo -i 4004 -e .*\/atmitest\/test103_singlegrp\/atmi103_v2-dom2.log -r  --  :
[[:space:]]process id=[0-9]+ ... Started.
exec cpmsrv -k nZ22K8K7kewKo -i 9999 -e .*\/atmitest\/test103_singlegrp\/cpmsrv-dom2.log -r -- -k3 -i1 --  :
[[:space:]]process id=[0-9]+ ... Started.
Startup finished. 8 processes started."

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Unexpected output from xadmin start -y"
    go_out -1
fi

xadmin psc
xadmin ppm

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

# check the bootlock flag
if [ ! -f ${TESTDIR}/exec_on_bootlocked.status ]; then
    echo "Expected exec_on_bootlocked.status file"
    go_out -1
fi

if [ -f ${TESTDIR}/exec_on_locked.status ]; then
    echo "Not expected exec_on_locked.status file"
    go_out -1
fi

# Ensure that PSG is locked but server & client are not booted
CMD="xadmin psg"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="SGID LCKD MMON SBOOT CBOOT LPSRVID    LPPID LPPROCNM          REFRESH RSN FLAGS
---- ---- ---- ----- ----- ------- -------- ---------------- -------- --- -----
   1 Y    N    N     N          10[[:space:]]+[0-9]+ exsinglesv[[:space:]]+.*   0 i"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected group to be locked, but not client/servers booted"
    go_out -1
fi

CMD="xadmin pc"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="TAG1/SUBSECTION1 - waiting on process group 1 lock.*
TAG2/SUBSECTION2 - waiting on process group 1 lock.*
TAG3/- - running pid [0-9]+ .*"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected to wait on group lock by the clients."
    go_out -1
fi

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
   1 Y    N    Y     Y          10[[:space:]]+[0-9]+ exsinglesv[[:space:]]+.*   0 i"

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

################################################################################
echo ">>> Lock loss: ping failure"
################################################################################

atmiclt103 lock_file ${TESTDIR}/lock_OK1_2 &

# the exsinglesv interval is 3 sec, so ping shall detect that it cannot lock
# anymore, and group will be unlocked and processes would get killed
# and would result in waiting for lock again
sleep 5
validate_OK1_lock_loss;
xadmin killall atmiclt103
validate_OK1_recovery;

if [ ! -f ${TESTDIR}/exec_on_locked.status ]; then
    echo "Expected exec_on_locked.status file"
    go_out -1
fi

################################################################################
echo ">>> Lock loss: due to exsinglesv crash"
################################################################################

LOCK_PID=`xadmin ps -a "exsinglesv -k nZ22K8K7kewKo -i 10" -p`
kill -9 $LOCK_PID

# let ndrxd to collect the fact
sleep 5
validate_OK1_lock_loss;
validate_OK1_recovery;

################################################################################
echo ">>> Lock loss: exsinglesv freeze (lock loss)"
################################################################################

LOCK_PID=`xadmin ps -a "exsinglesv -k nZ22K8K7kewKo -i 10" -p`
kill -SIGSTOP $LOCK_PID
# wait for loss detect:
# -> group timeout => 10 sec
# -> killall processes -> 1sec
# -> try boot processes -> 1 sec
# -> + 2 sec buffer
sleep 15
validate_OK1_lock_loss;
kill -SIGCONT $LOCK_PID
validate_OK1_recovery;

################################################################################
echo ">>> Node 1 boot -> groups not locked"
################################################################################

set_dom1;
xadmin down -y
xadmin start -y || go_out 1
xadmin ppm
xadmin pc
xadmin psg

# let CPM to cycle on...
sleep 5

# verify that no group is booted...
CNT=`xadmin ppm | grep atmi | grep 'wait  runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "16" ]; then
    echo "Expected 16 atmi.sv103/atmi103_v2 processes in wait state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin pc | grep 'waiting on process group' | wc | awk '{print $1}'`
if [ "$CNT" -ne "3" ]; then
    echo "Expected 3 clients in waiting state, got [$CNT]"
    go_out -1
fi

################################################################################
echo ">>> Test maintenance mode (no lock takover)"
################################################################################

xadmin mmon

# node2 goes down, first does not take over as in maintenace mode...
set_dom2;
xadmin stop -y

set_dom1;
# let exsinglesv to cycle..
sleep 5

xadmin ppm 
xadmin pc
xadmin psg

CNT=`xadmin ppm | grep atmi | grep 'wait  runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "16" ]; then
    echo "Expected 16 atmi.sv103/atmi103_v2 processes in wait state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin pc | grep 'waiting on process group' | wc | awk '{print $1}'`
if [ "$CNT" -ne "3" ]; then
    echo "Expected 3 clients in waiting state, got [$CNT]"
    go_out -1
fi

# release maintenance mode
xadmin mmoff
################################################################################
echo ">>> Node 2 shutdown: exsinglesv waits for reporting lock"
################################################################################

echo "sleep 5, check that we do not report the lock yet"
sleep 5
xadmin ppm
xadmin pc
xadmin psg

CNT=`xadmin ppm | grep atmi. | grep 'wait  runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "16" ]; then
    echo "Expected 16 atmi.sv103/atmi103_v2 processes in wait state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin pc | grep 'waiting on process group' | wc | awk '{print $1}'`
if [ "$CNT" -ne "3" ]; then
    echo "Expected 3 clients in waiting state, got [$CNT]"
    go_out -1
fi

################################################################################
echo ">>> Node 2 shutdown: no-order group wait long wait server booted fully"
################################################################################

# wait for locks to establish
sleep 20

# all process shall be running, expect that long booting, shall be still in start
# state

xadmin ppm
xadmin psg
xadmin pc

CNT=`xadmin ppm | grep atmi.sv1 | grep 'runok runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "10" ]; then
    echo "Expected 10 atmi.sv103 processes in runok state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin ppm | grep atmi.sv1 | grep 'start runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "1" ]; then
    echo "Expected 1 atmi.sv103 processes in start state, got [$CNT]"
    go_out -1
fi

CNT=`xadmin pc | grep 'running pid' | wc | awk '{print $1}'`
if [ "$CNT" -ne "3" ]; then
    echo "Expected 3 clients in running state, got [$CNT]"
    go_out -1
fi

# Ensure that PSG is locked but server & client are not booted
CMD="xadmin psg"
echo "$CMD"
OUT=`$CMD 2>&1`

PATTERN="SGID LCKD MMON SBOOT CBOOT LPSRVID    LPPID LPPROCNM          REFRESH RSN FLAGS
---- ---- ---- ----- ----- ------- -------- ---------------- -------- --- -----
   1 Y    N    Y     Y          10[[:space:]]+[0-9]+ exsinglesv[[:space:]]+.*   0 ni[[:space:]]*
   2 Y    N    Y     Y        3000[[:space:]]+[0-9]+ exsinglesv[[:space:]]+.*   0 ni[[:space:]]*"

echo "got output [$OUT]"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected both groups locked..."
    go_out -1
fi


################################################################################
echo ">>> bootlock script not working..."
################################################################################

chmod 000 $TESTDIR/exec_on_bootlocked.status
chmod 000 $TESTDIR/exec_on_locked.status

xadmin stop -y
xadmin start -y

# let cpmsrv to gather stats..
sleep 5

# let the lock to fail...
xadmin ppm
xadmin psg
xadmin pc

CNT=`xadmin ppm | grep atmi.sv1 | grep 'wait  runok' | wc | awk '{print $1}'`
if [ "$CNT" -ne "11" ]; then
    echo "Expected 11 atmi.sv103 processes in wait state, got [$CNT]"
    go_out -1
fi

# wait by cpmsrv too...
CNT=`xadmin pc | grep 'waiting on process group' | wc | awk '{print $1}'`
if [ "$CNT" -ne "2" ]; then
    echo "Expected 2 clients in waiting state, got [$CNT]"
    go_out -1
fi

# we are done!

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

