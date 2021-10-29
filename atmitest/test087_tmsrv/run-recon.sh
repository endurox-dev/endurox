#!/bin/bash
##
## @brief TMSRV / XA drive re-connect checks
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

export TESTNAME="test087_tmsrv"

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

#
# Load shared run functions
#
source ./funcs.sh

#
# Configure for retry
#
export NDRX_XA_FLAGS="RECON:*:3:100"

#
# Firstly test with join
#
buildprograms "";

xadmin start -y || go_out 1


echo ""
echo "************************************************************************"
echo "Start RECON"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:-7:2:0
xa_end_entry:-7:2:0
xa_rollback_entry:-7:3:8
xa_prepare_entry:-7:4:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:-7:3:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF


ERR=`NDRX_CCTAG="RM1" ./atmiclt87 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPEABORT"* ]]; then
    echo "Expected TPEABORT"
    go_out 1
fi

#verify restults ops...
# TODO count/trace down number of open + closes
verify_ulog "RM1" "xa_open" "23";
verify_ulog "RM1" "xa_close" "19";
verify_ulog "RM1" "xa_start" "7";
verify_ulog "RM1" "xa_end" "7";
verify_ulog "RM1" "xa_prepare" "4";
verify_ulog "RM1" "xa_rollback" "4";
verify_ulog "RM1" "xa_forget" "4";
verify_ulog "RM1" "xa_commit" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""

go_out 0

# vim: set ts=4 sw=4 et smartindent:

