#!/bin/bash
##
## @brief @(#) Test021 - XA testing
##
## @file run-dom.sh
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

export TESTNO="021"
export TESTNAME_SHORT="xafull"
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

# Include testing functions
source ./test-func-include.sh

export TESTDIR="$NDRX_APPHOME/atmitest/$TESTNAME"
export PATH=$PATH:$TESTDIR
# Override timeout!
export NDRX_TOUT=20

rm -rf $TESTDIR/RM1
rm -rf $TESTDIR/RM2

if [ ! -d "$TESTDIR/RM1" ]; then
    mkdir $TESTDIR/RM1
    mkdir $TESTDIR/RM1/active
    mkdir $TESTDIR/RM1/prepared
    mkdir $TESTDIR/RM1/committed
    mkdir $TESTDIR/RM1/aborted
fi

if [ ! -d "$TESTDIR/RM2" ]; then
    mkdir $TESTDIR/RM2
    mkdir $TESTDIR/RM2/active
    mkdir $TESTDIR/RM2/prepared
    mkdir $TESTDIR/RM2/committed
    mkdir $TESTDIR/RM2/aborted
fi

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
    export NDRX_TEST_RM_DIR=$TESTDIR/RM1

    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR="+"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
    export NDRX_XA_DRIVERLIB=$TESTDIR/$NDRX_XA_DRIVERLIB_FILENAME
    export NDRX_XA_RMLIB=$TESTDIR/libxadrv.so

    if [ "$(uname)" == "Darwin" ]; then
        export DYLD_FALLBACK_LIBRARY_PATH=$DYLD_FALLBACK_LIBRARY_PATH:`pwd`
        export NDRX_XA_RMLIB=$TESTDIR/libxadrv.dylib
    fi

    export NDRX_XA_LAZY_INIT=0

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
    export NDRX_TEST_RM_DIR=$TESTDIR/RM2

    export NDRX_XA_RES_ID=2
    export NDRX_XA_OPEN_STR="+"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
    export NDRX_XA_DRIVERLIB=$TESTDIR/$NDRX_XA_DRIVERLIB_FILENAME
    export NDRX_XA_RMLIB=$TESTDIR/libxadrv.so

    if [ "$(uname)" == "Darwin" ]; then
        export DYLD_FALLBACK_LIBRARY_PATH=$DYLD_FALLBACK_LIBRARY_PATH:`pwd`
        export NDRX_XA_RMLIB=$TESTDIR/libxadrv.dylib
    fi

    export NDRX_XA_LAZY_INIT=0

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
    xadmin killall atmiclt21

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


rm *.log

set_dom1;
xadmin down -y
# Let system to cleanup if any
sleep 2
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
# Let system to cleanup if any
sleep 2
xadmin start -y || go_out 2

#exit 0

# Have some wait for ndrxd goes in service - wait for connection establishment.
sleep 60

print_domains;

# Go to domain 1
set_dom1;


#
# Test case when tries exceeded, transaction not committed
# after manual commit from xadmin few times, in gets finally committed.
# the config is -m10 attempts each takes -r2x times
#
if [[ $NDRX_XA_DRIVERLIB_FILENAME == *"tryfail"* ]]; then
    echo ">>> TRYFAIL testing..."

    (./atmiclt21-try 2>&1) > ./atmiclt-try-dom1.log
    RET=$?

    sleep 30

    if [ $RET == 0 ]; then

        # test for transaction to be committed..
        # there should be no TRN- files at top level

        if [ ! -f ./RM1/TRN-* ]; then
                echo "Transaction must NOT be completed - thus \
                        intermediate file must exist!"
                RET=-2
        fi

        # try to manually commit with xadmin, 11 times, then it will approve
        TX=`xadmin pt | grep 'TM ref' | cut -d ':' -f2 | cut -d ')' -f1`

        cmd="xadmin commit $TX -y" 
        # 1x2 try for online, 10x2 for background, 2x4 retry, 
        for i in 1 2 3 4
        do
            echo "Running [$cmd"]
            eval $cmd
        done
        
        # Commit, should be ok
        eval $cmd || exit -10

        if [ -f ./RM1/TRN-* ]; then
                echo "Transaction must be completed!"
                RET=-3
        fi

        if [ ! -f ./RM1/committed/* ]; then
                echo "Transaction must be committed!"
                RET=-4
        fi

    fi
    go_out $RET
fi

#
# Test case when tries not exceeded, we get committed tran
#
echo ">>> LIB: $NDRX_XA_DRIVERLIB_FILENAME"

if [[ $NDRX_XA_DRIVERLIB_FILENAME == *"tryok"* ]]; then
    echo ">>> TRYOK testing..."

    (./atmiclt21-try 2>&1) > ./atmiclt-try-dom1.log
    RET=$?

    sleep 20

    if [ $RET == 0 ]; then

        # test for transaction to be committed..
        # there should be no TRN- files at top level

        if [ -f ./RM1/TRN-* ]; then
                echo "Transaction must be completed!"
                RET=-2
        fi

        if [ ! -f ./RM1/committed/* ]; then
                echo "Transaction must be committed!"
                RET=-3
        fi

    fi

    go_out $RET
    
fi

################################################################################
# Test case when xa_start_entry fails due to connection close (i.e. no 
# work done by tpopen() was closed).
# So it works for both cases when flag enabled end flag disabled
#
################################################################################

if [[ $NDRX_XA_DRIVERLIB_FILENAME == *"startfail"* ]]; then

	echo ">>> XA Start fail/retry, flags [$NDRX_XA_FLAGS]"

	(./atmiclt21-startfail $TEST160_FLAG 2>&1) > ./atmiclt-startfail-dom1.log
	RET=$?

	#
	# If all ok, test for transaction files.
	#
	if [ $RET == 0 ]; then
	
		# test for transaction to be aborted..
		# there should be no TRN- files at top level

		if [ -f ./RM1/TRN-* ]; then
			echo "Transaction must be completed!"
			RET=-2
		fi
		
		#if [ ! -f ./RM1/aborted/* ]; then
		#	echo "Transaction must be aborted!"
		#	RET=-3
		#fi
		
	fi

	go_out $RET
fi

################################################################################
# Test case for bug when resource manager prepares transaction, but
# tmsrv does not log it as prepared
# After fix it shall abort the transaction after tmsrv restart
################################################################################
if [[ $NDRX_XA_DRIVERLIB_FILENAME == *"105"* ]]; then

    echo "Testing bug #105 - prepare ok, but process terminates before writting log"

    (./atmiclt21-105 2>&1) > ./atmiclt-105-dom1.log
    RET=$?

    # let tmsrv boot back and abort transaction
    sleep 10
    #
    # If all ok, test for transaction files.
    #
    if [ $RET == 0 ]; then

        # test for transaction to be aborted..
        # there should be no TRN- files at top level

        if [ -f ./RM1/TRN-* ]; then
                echo "Transaction must be completed!"
                RET=-2
        fi

        if [ ! -f ./RM1/aborted/* ]; then
                echo "Transaction must be aborted!"
                RET=-3
        fi

    fi

    go_out $RET

fi

################################################################################
# Test that pings works...
################################################################################
PING_DOM1=`grep 'PING OK 0' TM1*.log`
echo "DOM1 PINGs by xa_recover: [$PING_DOM1]"

if [ "X$PING_DOM1" == "X" ]; then
    echo "DOM1 PINGs not working!"
    RET=-1
    go_out $RET
fi

echo "DOM1 pings OK"

PING_DOM2=`grep 'PING OK -4' TM2*.log`
echo "DOM2 PINGs by xa_start TMJOIN: [$PING_DOM2]"

if [ "X$PING_DOM2" == "X" ]; then
    echo "DOM2 PINGs not working!"
    RET=-1
    go_out $RET
fi

echo "DOM2 pings OK"

################################################################################
echo  "Test tpconvert()"
################################################################################
xadmin psc

(./atmiclt21-tpconvert 2>&1) > ./atmiclt21-tpconvert.log

RET=$?
if [ "X$RET" != "X0" ]; then
    echo "atmiclt21-tpconvert failed"
    
    go_out $RET
fi

################################################################################
# Normal tests
################################################################################
echo "Testing journal recovery..."
cp ./test_data/* ./RM1
xadmin restart -s tmsrv
sleep 5

# All must be completed
ensure_tran 0

# Run o-api tests
echo "O-api tests:"
#(valgrind --leak-check=full ./atmiclt21-oapi 2>&1) > ./atmiclt-oapi-dom1.log
(./atmiclt21-oapi 2>&1) > ./atmiclt-oapi-dom1.log
RET=$?

# Run the client test...
echo "Will issue calls to clients:"
(./atmiclt21 2>&1) > ./atmiclt-dom1.log
RET=$?

echo "Will run conversation tests:"
(./convclt21 2>&1) > ./convclt21-dom1.log
RET2=$?

echo "Cli tests..."
(./cli-tests.sh 2>&1) > ./cli-tests.sh.log
RET3=$?

if [[ $RET -eq 0 ]]; then
	RET=$RET2;
fi

if [[ $RET -eq 0 ]]; then
	RET=$RET3;
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
	echo "Test error detected!"
	RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
