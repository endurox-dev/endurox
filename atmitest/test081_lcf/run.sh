#!/bin/bash
##
## @brief LCF basic tests - test launcher
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

export TESTNAME="test081_lcf"

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
export NDRX_SILENT=y
export NDRX_LCFCMDEXP=15
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
    xadmin killall atmiclt81

    popd 2>/dev/null
    exit $1
}

rm *.log
rm ULOG*

UNAME=`uname`
#
# export the library path.
#
case $UNAME in

  Darwin)
    export NDRX_PLUGINS=libcustom_lcf.dylib
    export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$TESTDIR
    ;;

  AIX)
    export NDRX_PLUGINS=libcustom_lcf.so
    export LIBPATH=$LIBPATH:$TESTDIR
    ;;

  *)
    export NDRX_PLUGINS=libcustom_lcf.so
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TESTDIR
    ;;
esac

# Any bridges that are live must be killed!
xadmin killall tpbridge


set_dom1;
xadmin down -y
xadmin start -y || go_out 1
xadmin help
#
# Clean all LCFs
#
for i in {0..19}
do
   xadmin lcf disable -s $i
done

RET=0

xadmin psc
xadmin ppm
echo "Running off client"

set_dom1;
(./atmiclt81 1 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 1 EnduroX' atmisv-dom1*log`" == "X" ]; then
    echo "[atmisv-dom1.log] not found 1!"
    go_out 1
fi

echo "Check logrotate... function"

rm atmisv-dom1* 

# remove the logs & perform logrotate, the handle shall be re-open
xadmin lcf logrotate
xadmin lcf
(./atmiclt81 2 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 2 EnduroX' atmisv-dom1*log`" == "X" ]; then
    echo "[atmisv-dom1.log] not found 2!"
    go_out 1
fi

echo "Change log level to lower by process name"

xadmin lcf logchg -A "tp=4" -b "atmi.sv81"
xadmin lcf


(./atmiclt81 3 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 3 EnduroX' atmisv-dom1*log`" != "X" ]; then
    echo "[atmisv-dom1.log] found 3 (bellow log level)!"
    go_out 1
fi

echo "Change log level to lower by process name regexp -> back (not matching)"

xadmin lcf logchg -A "tp=5" -b "atmi.sv9." -r
xadmin lcf

(./atmiclt81 4 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 4 EnduroX' atmisv-dom1*log`" != "X" ]; then
    echo "[atmisv-dom1.log] found 4 -> not expected!"
    go_out 1
fi

echo "Change log level to lower by process name regexp -> back (matching)"

xadmin lcf logchg -A "tp=5" -b "atmi.sv8." -r
xadmin lcf

(./atmiclt81 5 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 5 EnduroX' atmisv-dom1*log`" == "X" ]; then
    echo "[atmisv-dom1.log] 5 not found!"
    go_out 1
fi

# let cpm to str...
sleep 10
xadmin pc

# Check CPM redirect of logs (have some loooper...)
if [ "X`grep 'Hello 5 EnduroX' atmisv-dom1*log`" == "X" ]; then
    echo "[atmisv-dom1.log] 5 not found!"
    go_out 1
fi

# thread logs shall not log the level 5
if [ "X`grep 'HELLO LEV 5' CLT1.log`" != "X" ]; then
    echo "[CLT1.log] HELLO LEV 5 not expected!"
    go_out 1
fi

if [ "X`grep 'HELLO LEV 5' main_thread.log`" == "X" ]; then
    echo "[main_thread.log] HELLO LEV 5 is expected but not found!"
    go_out 1
fi

echo "Change by pid, check the threads higher log level switch..."
# Now change the log level for the clt
CPID=`xadmin ps -p -a atmiclt81b`

# POST to PID... to change the level
xadmin lcf logchg -A "ndrx=5" -p $CPID
xadmin lcf

sleep 2

if [ "X`grep 'HELLO LEV 5' CLT1.log`" == "X" ]; then
    echo "[CLT1.log] HELLO LEV 5 is expected but not found!"
    go_out 1
fi

echo "Echo threads log-rotate target by threads... (not found)"
rm CLT*.log
rm main_thread.log

xadmin lcf logchg -A "ndrx=5" -p "X$CPID" -r
xadmin lcf

sleep 2

if test -f "./CLT1.log"; then
    echo "[CLT1.log] exit, but shall not regexp target..!"
    go_out 1
fi

echo "Echo threads log-rotate target by threads... (found OK)"
rm CLT*.log
rm main_thread.log

xadmin lcf logrotate -p "$CPID.*" -r
xadmin lcf

sleep 2

if [ "X`grep 'HELLO LEV 5' CLT1.log`" == "X" ]; then
    echo "[CLT1.log] HELLO LEV 5 is expected but not found (2) - logrotate shall restore files!"
    go_out 1
fi

if [ "X`grep 'HELLO LEV 5' CLT2.log`" == "X" ]; then
    echo "[CLT2.log] HELLO LEV 5 is expected but not found (3) - logrotate shall restore files!"
    go_out 1
fi

if [ "X`grep 'HELLO LEV 5' main_thread.log`" == "X" ]; then
    echo "[main_thread.log] HELLO LEV 5 is expected but not found (4) - logrotate shall restore files!"
    go_out 1
fi

# - have custom command registered...
# - provides some feedback
# - print all logs

# Have some plugin

echo "Posting the custom command ... with expiry (default)"
xadmin lcf customz -A "CUSTOM HELLO" -B "OKEY" -a
(./atmiclt81 5 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

echo "*******************************************"
grep 'CUSTOM HELLO' atmiclt-dom1.log
echo "*******************************************"

# atmiclt-dom1.log and atmisv-dom1.log should have the custom entries
if [ "X`grep 'CUSTOM HELLO' atmiclt-dom1.log`" == "X" ]; then
    echo "[atmiclt-dom1.log] CUSTOM HELLO expected 1!"
    go_out 1
fi

if [ "X`grep 'CUSTOM HELLO' atmisv-dom1.log`" == "X" ]; then
    echo "[atmisv-dom1.log] CUSTOM HELLO expected 2!"
    go_out 1
fi

echo "Wait command to expire..."
sleep 17

rm atmiclt-dom1.log
(./atmiclt81 5 2>&1) > ./atmiclt-dom1.log

echo "*******************************************"
grep 'CUSTOM HELLO' atmiclt-dom1.log
echo "*******************************************"

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'CUSTOM HELLO' atmiclt-dom1.log`" != "X" ]; then
    echo "[atmiclt-dom1.log] CUSTOM HELLO NOT expected 3!"
    go_out 1
fi

echo "Echo post with out expiry - always at startup... (-n) e.g. new process"
xadmin lcf customz -A "CUSTOM HELLO" -B "FAIL" -n -a -s5

sleep 16
(./atmiclt81 5 2>&1) > ./atmiclt-dom1.log

if [ "X`grep 'CUSTOM HELLO' atmiclt-dom1.log`" == "X" ]; then
    echo "[atmiclt-dom1.log] CUSTOM HELLO not found 4!"
    go_out 1
fi

echo "Test threaded logger re-opening the logs..."

rm atmi_sv81b.*log
xadmin lcf logrotate -a
xadmin lcf

rm atmiclt-dom1.log
(./atmiclt81 6 TEST2 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

if [ "X`grep 'Hello 6 EnduroX' atmi_sv81b.1.log`" == "X" ]; then
    echo "[atmi_sv81b.1.log] hello 6 not found"
    go_out 1
fi

xadmin lcf -1
xadmin lcf -2
xadmin lcf -3


echo "Testing stats..."

OKS=`xadmin lcf | grep cust | grep 987 | awk '{print $6}' `
FAILS=`xadmin lcf | grep cust | grep 741 |  awk '{print $7}'`

echo "custom stats: oks=$OKS fails=$FAILS"

if [ $OKS -eq 0 ]; then
    echo "Expected oks for custom"
    go_out 1
fi

if [ $FAILS -eq 0 ]; then
    echo "Expected failures for custom"
    go_out 1
fi

# check the tests...
if [ "X`xadmin lcf -1 | grep customz | grep 987`" == "X" ]; then
    echo "987 not found in page 1!"
    go_out 1
fi

if [ "X`xadmin lcf -3 | grep customz | grep OKEY`" == "X" ]; then
    echo "OKEY not found in page 3!"
    go_out 1
fi

if [ "X`xadmin lcf -3 | grep customz | grep FAIL`" == "X" ]; then
    echo "FAIL not found in page 3!"
    go_out 1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET


# vim: set ts=4 sw=4 et smartindent:

