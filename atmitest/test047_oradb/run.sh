#!/bin/bash
##
## @brief @(#) Test server connection to Oracle DB from C - test launcher
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

export TESTNAME="test047_oradb"

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
export NDRX_TOUT=20
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

    # We need DB Config here too...

    # XA SECTION
    export NDRX_XA_RES_ID=1
    export NDRX_XA_OPEN_STR="ORACLE_XA+SqlNet=$EX_ORA_SID+ACC=P/$EX_ORA_USER/$EX_ORA_PASS+SesTM=180+LogDir=./+nolocal=f+Threads=true+Loose_Coupling=false"
    export NDRX_XA_CLOSE_STR=$NDRX_XA_OPEN_STR
    export NDRX_XA_RMLIB=$EX_ORA_OCILIB
    export NDRX_XA_LAZY_INIT=1
    export NDRX_XA_FLAGS="RECON:*:3:100"
    # XA SECTION, END
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
    xadmin killall atmiclt47

    popd 2>/dev/null
    exit $1
}

set_dom1;

#
# Do static switch / dynamic switch tests
#
swmode=0
while [ $swmode -lt 2 ]; do

    if [ $swmode -eq 0 ]; then
        echo ">>> Static Switch testing"
        export NDRX_XA_DRIVERLIB=libndrxxaoras.so
    else
        echo ">>> Dynamic Switch testing"
        export NDRX_XA_DRIVERLIB=libndrxxaorad.so
    fi

    echo "Going down..."
    xadmin down -y
    #
    # assuming sudos are configured
    #
    if type "tcpkill" > /dev/null; then
        sudo LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH` PATH=`echo $PATH` `which xadmin` killall tcpkill
    fi

    rm *.log 2>/dev/null
    j=0
    while [ $j -lt 2 ]; do

        if [ $j -eq 1 ]; then
            echo ">>>> No Join, tight branching test"
            export NDRX_XA_FLAGS="$NDRX_XA_FLAGS;NOJOIN;BTIGHT"
        else
            echo ">>>> With JOIN"
        fi

        xadmin down -y
        xadmin start -y || go_out 1

        RET=0

        xadmin psc
        xadmin ppm
        echo "Running off client"

        xadmin forgetlocal -y
        xadmin abortlocal -y
        xadmin recoverlocal

        (./atmiclt47 2>&1) >> ./atmiclt-dom1.log

        RET=$?

        if [ "X$RET" != "X0" ]; then
            go_out $RET
        fi

        # Catch is there is test error!!!
        if [ "X`grep TESTERROR *.log`" != "X" ]; then
            echo "Test error detected!"
            RET=-2
            go_out -2
        fi

        j=$(( j + 1 ))
    done

    #
    # Currently this is linux only test!
    #
    # only if have tcpkill 
    # test recon engine, when connections randomly breaks up
    #
    # For Ora tests  required sudo configuration, adjust the paths accordingly to the OS
    # ----------------------
    # user1   ALL=(ALL)   NOPASSWD: /sbin/tcpkill,/bin/kill,/bin/xadmin,/home/user1/endurox/dist/bin/xadmin
    # ----------------------
    # 
    # To recover from broken connections, enable tcp keepalive
    # ----------------------
    # # cat << EOF >> /etc/sysctl.conf
    #
    # net.ipv4.tcp_keepalive_time=15
    # net.ipv4.tcp_keepalive_intvl=15
    # net.ipv4.tcp_keepalive_probes=3
    # 
    # EOF
    # # sysctl -f /etc/sysctl.conf
    # ----------------------
    # 
    # Enable broken connections on tnsname.ora
    # ----------------------
    # XASVC =
    # (DESCRIPTION=
    # (FAILOVER=on)
    # (ENABLE=broken)
    # (ADDRESS=(PROTOCOL=tcp)(HOST=rac1-vip)(PORT=1521))
    # (ADDRESS=(PROTOCOL=tcp)(HOST=rac2-vip)(PORT=1521))
    # (CONNECT_DATA=
    # (SERVICE_NAME=XASVC)
    # (FAILOVER_MODE=
    # (TYPE=SESSION)
    # (METHOD=BASIC)
    # (RETRIES=10)
    # (DELAY=15)
    # )
    # )
    # )
    # ----------------------

    if type "tcpkill" > /dev/null; then
        echo ">>>> Loop testing of recon (if available)"
        # use common incl normal use
        #export NDRX_XA_FLAGS="RECON:*:3:100:-3,-7"
        # reset btight flag...
        export NDRX_XA_FLAGS="RECON:*:3:100"
        export NDRX_TOUT=800
        export NDRX_DEBUG_CONF=$TESTDIR/debug_loop-dom1.conf
        export NDRX_TEST047_KILL=1
        sudo LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH` PATH=`echo $PATH` `which xadmin` killall tcpkill >/dev/null 2>&1
        xadmin stop -y
        xadmin start -y
        (./atmiclt47_loop 2>&1) > ./atmiclt_loop-dom1.log &
        LOOP_PID=$!
        echo "Wait for startup..."
        sleep 5
        # while binary is working kill connections periodically
        # and expect binary to complete in the time
        while kill -0 $LOOP_PID >/dev/null 2>&1
        do

            # kill connections for 5 sec
            sudo tcpkill -i $EX_ORA_IF port $EX_ORA_PORT >/dev/null  2>&1 & 
            TCPKILL_PID=$!
            echo "Wait 5 (tcpkill work)"
            sleep 5
            sudo LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH` PATH=`echo $PATH` `which xadmin` killall tcpkill >/dev/null 2>&1 

            # let binary 
            echo "Wait 220 (OK work)"
            sleep 220

        done

        sudo LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH` PATH=`echo $PATH` `which xadmin` killall tcpkill >/dev/null 2>&1 
    fi

    # Catch is there is test error!!!
    if [ "X`grep TESTERROR *.log`" != "X" ]; then
        echo "Test error detected!"
        RET=-3
    fi

    if [ "X`grep -i "timed out" *.log | grep aborting`" != "X" ]; then
        echo "There must be no timed-out transactions!"
        RET=-4
    fi

    swmode=$(( swmode + 1 ))
done


go_out $RET

# vim: set ts=4 sw=4 et smartindent:
