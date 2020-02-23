#!/bin/bash
##
## @brief Ndrxd sanity scans / ulog entries - test launcher
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

export TESTNAME="test074_sanitulog"

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
    xadmin killall atmiclt74

    popd 2>/dev/null
    exit $1
}

manual_mode=0

if [ "$#" -gt 0 ]; then
    echo "We are running in hand test mode.."
    manual_mode=1
fi

rm *.log
rm ULOG*
# Any bridges that are live must be killed!
xadmin killall tpbridge

set_dom1;
xadmin down -y
xadmin start -y || go_out 1

RET=0

xadmin psc
xadmin ppm

echo "Running off client - 120s - wait for ULOG / no die in ULOG"

(./atmiclt74 2>&1) > ./atmiclt-dom1.log

RET=$?

if [[ "X$RET" != "X0" ]]; then
    go_out $RET
fi

ULOG=`cat ULOG*`

if [ "X$ULOG" != "X" ]; then
    echo "Found ulog, but shouldn't: [$ULOG]"
    go_out -1
fi

#
# Normal process die...
#
echo "Running client / normal die.."

(./atmiclt74 die 2>&1) > ./atmiclt-dom1.log

#
# Let ndrxd to zap..
#
sleep 5

ULOG=`cat ULOG*`

if [ "X$ULOG" == "X" ]; then
    echo "ULOG not found, but should!"
    go_out -2
fi

################################################################################
# Thread die, as thread race condition might not be found
# as main thread finishes out and only at next scan it sees thread queue
# and that will be treated as process died. Thus cannot depend on race condition
# for tests.
################################################################################

if [ "X$manual_mode" == "X1" ]; then
    rm ULOG*
    echo "Running client / thread die.."

    (./atmiclt74 thread_die 2>&1) > ./atmiclt-dom1.log

    ULOG=`cat ULOG* | grep "unclean shutdown"`

    if [ "X$ULOG" == "X" ]; then
        echo "There shall be unclean shutdown of ther thread!"
        go_out -3
    fi
fi

################################################################################
# Main thread die, other threads die too..
# thus here we must see in ndrxd logs that 
################################################################################
rm ULOG*

echo "Running client / main thread die..."

(./atmiclt74 main_die 2>&1) > ./atmiclt-dom1.log

ULOG=`cat ULOG* | grep "Client process"`

if [ "X$ULOG" == "X" ]; then
    echo "There shall be normal process die...!"
    go_out -4
fi

ULOG=`cat ULOG* | grep "unclean shutdown"`

if [ "X$ULOG" != "X" ]; then
    echo "There shall be no unclean shutdown as main thread died..!"
    go_out -5
fi

NDRXDLOG=`grep "Previous same process" ndrxd-dom1.log`

if [ "X$NDRXDLOG" == "X" ]; then
    echo "Ndrxd shall notify that thread queues are dead too & removed...!"
    go_out -6
fi

sleep 5

#
# check the queues, all must be unlinked
#

echo "Queues:"
xadmin pqa -a

CLTQ=`xadmin pqa -a | grep atmiclt74`

if [ "X$CLTQ" != "X" ]; then
    echo "There must be no client queues left!"
    go_out -7
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    go_out -8
fi

go_out 0

# vim: set ts=4 sw=4 et smartindent:

