#!/bin/bash
##
## @brief Test process groups - test launcher
##  Validate process group operations. Invluding configuration checking
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

export TESTNAME="test102_procgrp"

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
# Generic exit function
#
function go_out {
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt102

    popd 2>/dev/null
    exit $1
}

#
# Check ndrxconfig.xml for errors
#
function validate_invalid_config {

    cfg_file=$1
    code=$2
    message=$3
    
    export NDRX_CONFIG=$TESTDIR/$cfg_file
    
    xadmin stop -y
    xadmin ldcf
    OUT=`xadmin ldcf 2>&1`
    echo "YOPT [$OUT]"
    
    if [[ $OUT != *"$code"* ]]; then
        echo "Missing error code [$code] in ldcf output for $cfg_file"
        go_out -1
    fi
    
    if [[ $OUT != *"$message"* ]]; then
        echo "Missing message code [$message] in ldcf output for $cfg_file"
        go_out -1
    fi
    
}

#
# Check ndrxconfig.xml for errors
#
function validate_invalid_config_reload {

    code=$1
    message=$2
    
    xadmin reload
    OUT=`xadmin reload 2>&1`

    if [[ $OUT != *"$code"* ]]; then
        echo "Missing error code [$code] in ldcf output for $cfg_file"
        go_out -1
    fi
    
    if [[ $OUT != *"$message"* ]]; then
        echo "Missing message code [$message] in ldcf output for $cfg_file"
        go_out -1
    fi
    
}

rm *.log 2>/dev/null
rm ULOG* 2>/dev/null
rm -f lock_* 2>/dev/null
# Any bridges that are live must be killed!
#xadmin killall tpbridge

set_dom1;

# grpno outside 1..64 range:


# Running LP changed group...
# reload test:
export NDRX_CCONFIG="$TESTDIR"
export NDRX_CONFIG="ndrxconfig-dom1-lp_tmp.xml.tmp"
cp ndrxconfig-dom1-lp_ok.xml ndrxconfig-dom1-lp_tmp.xml.tmp
xadmin start -y
cp ndrxconfig-dom1-lp_changed.xml ndrxconfig-dom1-lp_tmp.xml.tmp
validate_invalid_config_reload "NDRXD_EREBBINARYRUN" "Lock provider [exsinglesv]/10 must be shutdown prior changing locking group (from 12 to 13)"

# load config tests:
validate_invalid_config "ndrxconfig-dom1-dup_lp.xml" "NDRXD_ECFGINVLD" "Lock provider [exsinglesv]/11 duplicate for process group [OK]. Lock already provided by srvid 10"
validate_invalid_config "ndrxconfig-dom1-singlegrp_inval1.xml" "NDRXD_EINVAL" "Invalid \`singleton' setting [X] in <procgroup>"
validate_invalid_config "ndrxconfig-dom1-noorder_inval.xml" "NDRXD_EINVAL" "Invalid \`noorder' setting [X] in <procgroup>"
validate_invalid_config "ndrxconfig-dom1-no-name.xml" "NDRXD_ECFGINVLD" "\`name' not set in <procgroup> section"
validate_invalid_config "ndrxconfig-dom1-no-id.xml" "NDRXD_ECFGINVLD" "\`grpno' not set in <procgroup> section"
validate_invalid_config "ndrxconfig-dom1-lp_missing.xml" "NDRXD_ECFGINVLD" "Singleton process group [OK] does not have lock provider defined"
validate_invalid_config "ndrxconfig-dom1-inval_procgrp_lp.xml" "NDRXD_ENOENT" "Failed to resolve procgrp_lp"
validate_invalid_config "ndrxconfig-dom1-inval_procgrp.xml" "NDRXD_ENOENT" "Failed to resolve procgrp"
validate_invalid_config "ndrxconfig-dom1-defaults-dup_no.xml" "NDRXD_EINVAL" "is duplicate in <procgroup> section"
validate_invalid_config "ndrxconfig-dom1-defaults-dup_name.xml" "NDRXD_EINVAL" "is duplicate in <procgroup> section"
validate_invalid_config "ndrxconfig-dom1-bad-no.xml" "NDRXD_EINVAL" "Invalid \`grpno'"
validate_invalid_config "ndrxconfig-dom1-bad-name.xml" "NDRXD_EINVAL" "invalid characters used in"
# in this case config is OK, config was loaded
validate_invalid_config "ndrxconfig-dom1-ok-name.xml" "NDRXD_ECFGLDED" "NDRXD_ECFGLDED"

# test server group operations:
export NDRX_CONFIG="ndrxconfig-dom1-procgroups.xml"
xadmin stop -y
xadmin start -y
# let clients to boot:
sleep 6
xadmin pc

################################################################################
CMD="xadmin ppm -3"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"
# check that 5x atmi.sv102 processs are running
# shall have in output 
PATTERN="BINARY[[:space:]]*SRVID[[:space:]]*PID[[:space:]]*SVPID[[:space:]]*PROCGRP[[:space:]]*PGNO[[:space:]]*PROCGRPL[[:space:]]*PGLNO[[:space:]]*PGNLA
--------[[:space:]]*-----[[:space:]]*--------[[:space:]]*--------[[:space:]]*--------[[:space:]]*-----[[:space:]]*--------[[:space:]]*-----[[:space:]]*-----
atmi.sv1[[:space:]]*10[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK[[:space:]]*28[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*11[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK[[:space:]]*28[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*12[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK[[:space:]]*28[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*100[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK2[[:space:]]*1[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*101[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK2[[:space:]]*1[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*102[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK2[[:space:]]*1[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*103[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK2[[:space:]]*1[[:space:]]*-[[:space:]]*0[[:space:]]*0
atmi.sv1[[:space:]]*104[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*OK2[[:space:]]*1[[:space:]]*-[[:space:]]*0[[:space:]]*0
exsingle[[:space:]]*200[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*-[[:space:]]*0[[:space:]]*_this_I\+[[:space:]]*64[[:space:]]*64
cpmsrv[[:space:]]*1000[[:space:]]*[0-9]+[[:space:]]*[0-9]+[[:space:]]*-[[:space:]]*0[[:space:]]*-[[:space:]]*0[[:space:]]*0"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "ppm -3 page invalid"
    go_out -1
fi

################################################################################
CMD="xadmin stop -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"
# check that 5x atmi.sv102 processs are running
# shall have in output 
PATTERN="Server executable = atmi.sv102	Id = 12 :	Shutdown succeeded.
Server executable = atmi.sv102	Id = 11 :	Shutdown succeeded.
Server executable = atmi.sv102	Id = 10 :	Shutdown succeeded.
Shutdown finished. 3 processes stopped."

if [[ $OUT != *"$PATTERN"* ]]; then
    echo "Expected 3x servers to be stopped"
    go_out -1
fi

# check that we have 5x servers running
xadmin ps -a atmi.sv102
CNT=`xadmin ps -a atmi.sv102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "5" ]; then
    echo "Expected 5 servers after group shutdown, got [$CNT]"
    go_out -1
fi

################################################################################
CMD="xadmin start -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="exec atmi.sv102 -k nre38Kff1kz -i 10 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
exec atmi.sv102 -k nre38Kff1kz -i 11 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
exec atmi.sv102 -k nre38Kff1kz -i 12 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
Startup finished. 3 processes started."

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected 3x servers to be started"
    go_out -1
fi

# check that we have 8x servers running
xadmin ps -a atmi.sv102
CNT=`xadmin ps -a atmi.sv102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "8" ]; then
    echo "Expected 8 servers after group shutdown, got [$CNT]"
    go_out -1
fi

################################################################################
CMD="xadmin sreload -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="Server executable = atmi.sv102	Id = 10 :	Shutdown succeeded.
exec atmi.sv102 -k nre38Kff1kz -i 10 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
Server executable = atmi.sv102	Id = 11 :	Shutdown succeeded.
exec atmi.sv102 -k nre38Kff1kz -i 11 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
Server executable = atmi.sv102	Id = 12 :	Shutdown succeeded.
exec atmi.sv102 -k nre38Kff1kz -i 12 -e .*/test102_procgrp/atmisv-dom1.log -r --  :
	process id=[0-9]+ ... Started.
Reload finished. 3 processes reloaded."

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected 3x servers to be started"
    go_out -1
fi

# check that we have 8x servers running
xadmin ps -a atmi.sv102
CNT=`xadmin ps -a atmi.sv102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "8" ]; then
    echo "Expected 8 servers after group shutdown, got [$CNT]"
    go_out -1
fi

################################################################################
CMD="xadmin stop -g OK -s JJJ"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="-i, -s and -g cannot be combined!"

if [[ "$OUT" != *"$PATTERN"* ]]; then
    echo "Pattern [$PATTERN] not found in output"
    go_out -1
fi

################################################################################
CMD="xadmin stop -g OK -i 500"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="-i, -s and -g cannot be combined!"

if [[ "$OUT" != *"$PATTERN"* ]]; then
    echo "Pattern [$PATTERN] not found in output"
    go_out -1
fi

################################################################################
CMD="xadmin stop -g KK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="NDRXD_ENOENT.*Process group \[KK\] is not defined"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "pattern [$PATTERN] not found in output"
    go_out -1
fi

################################################################################
CMD="xadmin sc -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`
echo "got output [$OUT]"

PATTERN="Client process CLT1/- stopped
Client process CLT2/- stopped
Client process CLT3/- stopped
3 client(s) stopped."

if [[ "$OUT" != *"$PATTERN"* ]]; then
    echo "pattern [$PATTERN] not found in output"
    go_out -1
fi

# check that we have 4x clients running
xadmin ps -a atmiclt102
CNT=`xadmin ps -a atmiclt102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "4" ]; then
    echo "Expected 4x atmiclt102, got [$CNT]"
    go_out -1
fi


################################################################################
CMD="xadmin bc -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`
echo "got output [$OUT]"

PATTERN="Client process CLT1/- marked for start
Client process CLT2/- marked for start
Client process CLT3/- marked for start
3 client(s) marked for start."

# let clients to boot..
sleep 7

if [[ "$OUT" != *"$PATTERN"* ]]; then
    echo "pattern [$PATTERN] not found in output"
    go_out -1
fi

# check that we have 7x clients running
xadmin ps -a atmiclt102
CNT=`xadmin ps -a atmiclt102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "7" ]; then
    echo "Expected 7x atmiclt102, got [$CNT]"
    go_out -1
fi

################################################################################
CMD="xadmin rc -g OK"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`
echo "got output [$OUT]"

PATTERN="Client process CLT1/- stopped
Client process CLT1/- marked for start
Client process CLT2/- stopped
Client process CLT2/- marked for start
Client process CLT3/- stopped
Client process CLT3/- marked for start
3 client(s) restarted."

# let clients to boot..
sleep 7

if [[ "$OUT" != *"$PATTERN"* ]]; then
    echo "pattern [$PATTERN] not found in output"
    go_out -1
fi

# check that we have 7x clients running
xadmin ps -a atmiclt102
CNT=`xadmin ps -a atmiclt102 | wc | awk '{print $1}'`
if [ "$CNT" -ne "7" ]; then
    echo "Expected 7x atmiclt102, got [$CNT]"
    go_out -1
fi


################################################################################
CMD="xadmin pc"
echo "$CMD"
################################################################################

OUT=`$CMD 2>&1`

echo "got output [$OUT]"

PATTERN="CLT1/- - running pid.*
CLT2/- - running pid.*
CLT3/- - running pid.*
CLT4/- - running pid.*
CLT5/- - running pid.*
CLT6/- - running pid.*
CLT7/- - running pid.*"

if ! [[ "$OUT" =~ $PATTERN ]]; then
    echo "Expected 3x servers to be started"
    go_out -1
fi

RET=$?

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

