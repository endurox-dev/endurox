#!/bin/bash
##
## @brief Reload RQADDR setting - resources shall clean up for service - test launcher
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

export TESTNAME="test095_rqaddrel"

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
    xadmin killall atmiclt95

    popd 2>/dev/null
    exit $1
}

rm *.log 2>/dev/null
rm ULOG* 2>/dev/null

set_dom1;
xadmin down -y
RET=0

###############################################################################
echo ">>> Checking non rq-addr sv mode"
###############################################################################
> ndrxconfig.xml.rqaddr

xadmin start -y || go_out 1

if [ "X$RET" != "X0" ]; then
    go_out -1
fi

xadmin psvc -r 2>/dev/null

#
# each binary (10 servers) have a resource + title
#
num_lines=`xadmin psvc -r 2>/dev/null | wc -l | awk '{print $1}'`
if [ "X$num_lines" != "X11" ]; then
    echo "Expected 11 lines in output, got $num_lines"
    go_out -1
fi

###############################################################################
echo ">>> Checking rq-addr sv mode"
###############################################################################
echo "<rqaddr>TEST</rqaddr>" > ndrxconfig.xml.rqaddr
xadmin reload
xadmin sreload -y

xadmin psvc -r 2>/dev/null

#
# We have a common request queue + title
#
num_lines=`xadmin psvc -r 2>/dev/null | wc -l | awk '{print $1}'`
if [ "X$num_lines" != "X2" ]; then
    echo "Expected 2 lines in output, got $num_lines"
    go_out -1
fi

# Catch is there is test error!!!
if [ "X`grep TESTERROR *.log`" != "X" ]; then
    echo "Test error detected!"
    RET=-2
fi

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

