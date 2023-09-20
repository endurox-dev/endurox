#!/bin/bash
##
## @brief Test the failover of TMSRV+TMQ with the singleton groups between two domains - test launcher
##  Testing strategy would be following:
##    - Have two domains, both use the same data folder for tmq/srv and virtual node id (-n2)
##    - tmsrv+tmq in both domains are in "GRP1" lock group, whith shared locks.
##    - tmsrv+tmq has the same SRVIDs
##    - Two automatic queues are configured: Q1 and Q2, when QFWD1 and QFWD2 servers to send msgs between the
##     the queues.
##  During the test, client keeps loading the Q1 with messages, periodically (at random points)
##  for active domain the exsinglesv is shutdown + sleep (of other node exsinglesv try to lock) + and started back
##  the client waits for "locked wait" and for Q bring up... and the enqueues again.
##  when all this run for some 5 minutes. queues are re-configured to manual, and full number of enqueued messages
##  is read.
##  Additionally, the exsinglesv lock scripts removes transactinos logs which are not prepared i.e.
##  If not having this “:S:50:” In transaction, then transaction log is removed, the tmrecoversv shall scan
##  for prepared lost transactions and shall recover them.
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

export TESTNAME="test104_tmqfailover"

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
export NDRX_TOUT=25
export NDRX_SILENT=Y

################################################################################
# 6 gives:
# lock expire if not refreshed in 6 seconds
# lock take over by other node if file unlocked: 12 sec
# exsinglesv periodic scans / locks 2 sec
export NDRX_SGREFRESH=6
################################################################################

if [ "$(uname)" == "Darwin" ]; then
    export NDRX_LIBEXT="dylib"
else
    export NDRX_LIBEXT="so"
fi

#
# Common configuration between the domains
#
export NDRX_CCONFIG1=$TESTDIR/app-common.ini

#
# Domain 1 - here client will live
#
set_dom1() {
    echo "Setting domain 1"
    . ../dom1.sh
    export NDRX_CONFIG=$TESTDIR/ndrxconfig-dom1.xml
    export NDRX_DMNLOG=$TESTDIR/ndrxd-dom1.log
    export NDRX_LOG=$TESTDIR/ndrx-dom1.log
    export NDRX_CCONFIG=$TESTDIR/app-dom1.ini
    #export NDRX_DEBUG_CONF=$TESTDIR/debug-dom1.conf
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
    export NDRX_CCONFIG=$TESTDIR/app-dom2.ini
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
    xadmin killall atmiclt104

    popd 2>/dev/null
    exit $1
}

# clean up some old stuff
rm *.log 2>/dev/null
rm lock_* 2>/dev/null
rm ULOG* 2>/dev/null
rm -rf RM1 RM2 qdata 2>/dev/null

# where to store the data
mkdir RM1 RM2 qdata

# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

set_dom2;
xadmin down -y
xadmin start -y || go_out 2

echo "Sleep 15 for link"
sleep 15

set_dom1;
xadmin psg
xadmin psc

################################################################################
echo ">>> Loop enqueue + crash"
################################################################################
NUM=60

counter=0
while [ $counter -lt $NUM ]
do
    echo "Loop [$counter]"

    # enq single msg...
    ./atmiclt104 enq $counter
    RET=$?

    if [[ "X$RET" != "X0" ]]; then
        echo "./atmiclt104 enq $counter failed"
        go_out $RET
    fi

    if [ "$(($counter % 5))" == "0" ]; then
        echo "Doing failover..."
        # failover the groups...
        # whoever will first get the lock that will win...
        xadmin killall exsinglesv
        
        # let ndrxd to collect, otherwise we might remove some msg where
        # some TM already writes the message to disk...
        # remove transaction logs which are not
        sleep 5
        # prepared...
        if [ "$(($counter % 3))" == "0" ]; then
            grep -L ":S:50:" $TESTDIR/RM1/* | xargs rm
            grep -L ":S:50:" $TESTDIR/RM2/* | xargs rm
        fi

        echo "Sleep 15... to bring processes back... (with some buffer for msg read)"
        sleep 15

        set_dom1;
        xadmin psg
        xadmin mqlq
        xadmin ppm

        set_dom2;
        xadmin psg
        xadmin mqlq
        xadmin ppm
    else
        echo "Echo let processes to run for 1 sec"
        sleep 1
    fi

    # swap the domains of attempts...
    if [ "$(($counter % 2))" == "0" ]; then
        set_dom1;
        xadmin psg
        xadmin ppm
        xadmin mqlq
    else
        set_dom2;
        xadmin psg
        xadmin ppm
        xadmin mqlq
    fi

    # periodically freeze the active tmsrv (this will
    # simulate the case when )
    if [ "$(($counter % 6))" == "0" ]; then

        echo "Node freeze test...."
        # for active node, we will suspend tmsrv...
        # that shall cause failover...

        set_dom1;

        if [[ "X`xadmin ppm | grep 'wait  runok'`" != "X" ]]; then
            echo "domain 2 is active"
            set_dom2;
        else
            echo "domain 1 is active"
        fi

        ########################################################################
        # this point we will freeze the tmsrv and will remove the logs (if
        # transaction is not comitting). As aborts will be collected
        # by tmrecoversv.
        # Simualte the case if node does freeze (suspend) and
        # does continue, transaction logs must be in place
        ########################################################################

        xadmin ps -a tmsrv -p | xargs kill -SIGSTOP

        grep -L ":S:50:" $TESTDIR/RM1/* | xargs rm
        grep -L ":S:50:" $TESTDIR/RM2/* | xargs rm

        # let tmsrv's to detect the situation...
        xadmin ps -a tmsrv -p | xargs kill -SIGCONT

        echo "Let to failover tmsrvs... (sleep 15)"
        sleep 15

    fi

    ((counter++))

done



################################################################################
echo ">>> Validate $NUM messages"
################################################################################

# disable auto from Qs....
xadmin mqch -n2 -i 200 -qQ1,autoq=n
xadmin mqch -n2 -i 200 -qQ2,autoq=n
sleep 5
# lets Q to complete...
xadmin mqlc
xadmin mqlq

# validate that all messages are in place
# enq single msg...
./atmiclt104 deq $NUM
RET=$?

if [[ "X$RET" != "X0" ]]; then
    echo "./atmiclt104 deq $counter failed"
    go_out $RET
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

