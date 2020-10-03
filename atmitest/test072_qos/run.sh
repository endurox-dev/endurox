#!/bin/bash
##
## @brief Both sites max send, avoid deadlock of full sockets - test launcher
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

export TESTNAME="test072_qos"

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
export NDRX_TOUT=310

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
    echo "Test exiting with: $1"
    
    set_dom1;
    xadmin stop -y
    xadmin down -y


    set_dom2;
    xadmin stop -y
    xadmin down -y

    # If some alive stuff left...
    xadmin killall atmiclt72

    popd 2>/dev/null
    exit $1
}

for action in 1 2 3
do

    export TEST_ACTION=$action
    echo "Testing drop action: $TEST_ACTION"

    rm *dom*.log ULOG*
    # Any bridges that are live must be killed!
    xadmin killall tpbridge atmiclt72

    set_dom1;
    xadmin down -y
    xadmin start -y || go_out 1


    set_dom2;
    xadmin down -y
    xadmin start -y || go_out 2

    # Have some wait for ndrxd goes in service - wait for connection establishment.
    sleep 20
    RET=0

    xadmin psc
    xadmin ppm
    echo "Running off client - dom2 background"

    set_dom2;
    (./atmiclt72 TEST1 GETINFOS1 2>&1) > ./atmiclt-dom2.log &
    (./atmiclt72 TEST11 GETINFOS11 2>&1) > ./atmiclt-dom2_1.log &

    echo "Running off client - dom1 foreground"

    set_dom1;
    (./atmiclt72 TEST22 GETINFOS22 2>&1) >> ./atmiclt-dom1_1.log &
    (./atmiclt72 TEST2 GETINFOS2 2>&1) >> ./atmiclt-dom1.log

    RET=$?

    if [[ "X$RET" != "X0" ]]; then

        go_out $RET
    fi

    # Catch is there is test error!!!
    if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-2
        go_out $RET
    fi

    # wait for finish results

    i=`grep "testcl finished" ULOG* | wc -l`

    while [ $i -lt 4 ] && [ $i -lt 65 ]
    do
        echo "Wait 5 finish..."
        sleep 5
        i=`grep "testcl finished" ULOG* | wc -l`
    done

    ok=`grep "testcl finished - OK" ULOG* | wc -l`
    bad=`grep "testcl finished - failed" ULOG* | wc -l`
    discard=`grep "Discarding message" ULOG* | wc -l`

    echo "ok=$ok bad=$bad discard=$discard"

    if [ $action -eq 1 ]; then

        if [ $ok -ne 4 ]; then
            echo "Expected OK clients 4 got: $ok bad: $fail"
            RET=-3
            go_out $RET
        fi

        if [ $discard -ne 0 ]; then
            echo "There must be no discarded messages, but got: $discard"
            RET=-4
            go_out $RET
        fi

    else

        if [ $bad -lt 1 ]; then
            
            echo "Expected 1 bad client got: $bad ok: $ok"
            RET=-3
            go_out $RET
        fi

        if [ $discard -eq 0 ]; then
            echo "There must be discarded messages, but got: $discard"
            RET=-4
            go_out $RET
        fi
    fi
done

go_out $RET

# vim: set ts=4 sw=4 et smartindent:
