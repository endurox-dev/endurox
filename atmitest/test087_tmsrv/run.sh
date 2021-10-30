#!/bin/bash
##
## @brief TMSRV State Driving verification - test launcher
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
# Firstly test with join
#
buildprograms "";

#
# Run OK case...
#
echo ""
echo "************************************************************************"
echo "Commit OK case ..."
echo "************************************************************************"

cat << EOF > lib1.rets
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

# start here, we want fresh tables loaded...
xadmin start -y || go_out 1

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"
# check number of suspends (1 - call suspend, 1 - sever end, 1 - client end)
verify_ulog "RM1" "xa_end" "3";

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit OK read only ..."
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:3:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


# support #729
echo "Checking XA_RDONLY log levels..."
have_output 'grep "NDRX:5:" tmsrv_lib2.log | grep "Failed to prepare local transaction"'
have_output 'grep "NDRX:5:" tmsrv_lib2.log | grep "xa_prepare_entry - fail: 3" | grep "xa.c"'
have_output 'grep "NDRX:5:" tmsrv_lib2.log | grep "ndrx_TPset_error_fmt_rsn: 16 (TPERMERR) reason: 3"'


echo ""
echo "************************************************************************"
echo "Commit OK read only ... (first RO)"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:3:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#verify results ops...

verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"


verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit OK read only ... (both)"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:3:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_prepare_entry:3:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit fail read only + aborted"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:3:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_prepare_entry:-4:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit fail aborted + aborting"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

# this is aborted
cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-4:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

# lower the rank to aborting...
cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-6:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

# no prepare please as we already know that we will do rollback due to first
# prepare fail
verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


# support #729
echo "Checking non XA_RDONLY levels at prepare error..."
have_output 'grep "NDRX:2:" tmsrv_lib1.log | grep "Failed to prepare local transaction"'
have_output 'grep "NDRX:2:" tmsrv_lib1.log | grep "xa_prepare_entry - fail: -4" | grep "xa.c"'
have_output 'grep "NDRX:3:" tmsrv_lib1.log | grep "ndrx_TPset_error_fmt_rsn: 16 (TPERMERR) reason: -4"'


echo ""
echo "************************************************************************"
echo "Commit fail aborting + aborted"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

# Lowest rank
cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-6:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

# Does not upgrade
cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-4:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit PREP XAER_NOTA  ..."
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-4:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit PREP XA_RBBASE  ..."
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:100:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit PREP XAER_PROTO (any error)  ..."
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-6:1:0
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

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit failures: XAER_RMERR - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:-3:2:0
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

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XAER_RMFAIL - retry OK"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:-7:2:0
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

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "3";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XAER_RETRY - retry OK"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:4:2:0
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

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "3";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURHAZ - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:8:1:0
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

if [[ $ERR != *"TPEHAZARD"* ]]; then
    echo "Expected TPEHAZARD"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURHAZ - heuristic  / tries exceeded"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:8:1:0
xa_recover_entry:0:1:0
xa_forget_entry:-7:4:0
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

if [[ $ERR != *"TPEHAZARD"* ]]; then
    echo "Expected TPEHAZARD"
    go_out 1
fi

# Log is left... for later processing.
verify_logfiles "log1" "1"

sleep 10

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "5";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURCOM - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:7:1:0
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

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURRB - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:6:1:0
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

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURMIX - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:5:1:0
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

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_HEURCOM / XA_HEURHAZ - hazard"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:7:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_commit_entry:8:1:0
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

if [[ $ERR != *"TPEHAZARD"* ]]; then
    echo "Expected TPEHAZARD"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "1";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit failures: XA_RBBASE - heuristic"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:100:1:0
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

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Commit failures: XAER_PROTO - (any err) -> committed"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:-6:1:0
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

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort OK"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#if [[ $ERR != *"TPEHEURISTIC"* ]]; then
#    echo "Expected TPEHEURISTIC"
#    go_out 1
#fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort OK XA_RDONLY"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:3:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort XA_HEURHAZ"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:8:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPEHAZARD"* ]]; then
    echo "Expected TPEHAZARD"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort XA_HEURRB"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:6:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_rollback_entry:6:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "1";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort XA_HEURCOM"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:7:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_rollback_entry:7:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "1";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort XA_HEURMIX"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:5:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_rollback_entry:5:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPEHEURISTIC"* ]]; then
    echo "Expected TPEHEURISTIC"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "1";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort XAER_RMFAIL"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:-7:2:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_rollback_entry:-7:2:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#if [[ $ERR != *"TPEHEURISTIC"* ]]; then
#    echo "Expected TPEHEURISTIC"
#    go_out 1
#fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "3";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "3";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Abort XAER_NOTA (OK)"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:-4:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
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
xa_rollback_entry:-4:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#if [[ $ERR != *"TPEHEURISTIC"* ]]; then
#    echo "Expected TPEHEURISTIC"
#    go_out 1
#fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Abort XAER_PROTO (any sort of error, not expected) OK"
echo "************************************************************************"

xadmin sreload -y
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:-6:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" != "X0" ]; then
    echo "atmiclt87 must not fail"
    go_out 1
fi

#if [[ $ERR != *"TPEHEURISTIC"* ]]; then
#    echo "Expected TPEHEURISTIC"
#    go_out 1
#fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Abort Crash recovery (no duplicate ops in case of heuristic ops)"
echo "************************************************************************"

xadmin sreload -y
xadmin lcf tcrash -A 36 -a -n
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:8:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 A 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPETIME"* ]]; then
    echo "Expected TPETIME"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "1"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

# restore back... now shall complete
sleep 10

#
# Print what ever we have in logs...
#
cat log1/*
xadmin lcf tcrash -A 0 -a -n

# let process to finalize
sleep 15

verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"


echo ""
echo "************************************************************************"
echo "Commit Crash recovery (no duplicate ops in case of heuristic ops)"
echo "************************************************************************"

xadmin sreload -y
xadmin lcf tcrash -A 80 -a -n
clean_ulog;

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:8:1:0
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

if [[ $ERR != *"TPETIME"* ]]; then
    echo "Expected TPETIME"
    go_out 1
fi

#verify results ops...
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "1"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"

# restore back... now shall complete
sleep 10

#
# Print what ever we have in logs...
#
cat log1/*
xadmin lcf tcrash -A 0 -a -n

# let process to finalize
sleep 20

verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "1";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "1";
verify_ulog "RM2" "xa_commit" "1";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "1";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "Automatic rollback of active records"
echo "************************************************************************"

xadmin sreload -y
xadmin lcf tcrash -A 40 -a -n
clean_ulog;

cat << EOF > lib1.rets
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

ERR=`NDRX_TOUT=5 NDRX_CCTAG="RM1" ./atmiclt87 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPETIME"* ]]; then
    echo "Expected TPETIME"
    go_out 1
fi

#verify results ops...
#verify_ulog "RM1" "xa_prepare" "0";
#verify_ulog "RM1" "xa_commit" "0";
#verify_ulog "RM1" "xa_rollback" "0";
#verify_ulog "RM1" "xa_forget" "0";
#verify_logfiles "log1" "1"

#verify_ulog "RM2" "xa_prepare" "0";
#verify_ulog "RM2" "xa_commit" "0";
#verify_ulog "RM2" "xa_rollback" "0";
#verify_ulog "RM2" "xa_forget" "0";
#verify_logfiles "log2" "0"

#
# Print what ever we have in logs...
#
cat log1/*

# Transaction shall roll back due to expiry
#sleep 20

# we get automatic rollback:
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "1";
verify_ulog "RM1" "xa_forget" "0";
verify_logfiles "log1" "0"

verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0"


# disable the command
xadmin lcf disable -s 5

echo ""
echo "************************************************************************"
echo "Retry on prepare... (unexpected error - suspend flag test)"
echo "************************************************************************"
clean_ulog;
# must have abort error...
xadmin sreload -y

cat << EOF > lib1.rets
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

cat << EOF > lib2.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:4:3:0
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

# must have abort error...
xadmin sreload -y

# Check in logs that we did suspend (as switch is marked with migrate flags)
if [ "X`grep suspend atmiclt87.log`" == "X" ]; then
    echo "Expected API suspend - but not found"
    go_out 1
fi

echo ""
echo "************************************************************************"
echo "Commit OK case ... NOSUSPEND"
echo "************************************************************************"
xadmin stop -y
clean_ulog;

cat << EOF > lib1.rets
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

export NDRX_XA_FLAGS=NOSUSPEND
> atmiclt87.log
xadmin start -y

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

# No join expected, as set by libraries...
if [ "X`grep suspend atmiclt87.log`" == "X" ]; then
    echo "No join expected (NOSUSPEND flag), but got..."
    go_out 1
fi

# check number of suspends (1 - sever end, 1 - client end)
verify_ulog "RM1" "xa_end" "2";

#
# Clean up & rebuild
#
unset NDRX_XA_FLAGS
xadmin stop -y
> atmiclt87.log

#
# Build with out join support (thus shall not be suspended)
#
buildprograms "nj";
xadmin start -y

echo ""
echo "************************************************************************"
echo "Commit OK case ... NJ"
echo "************************************************************************"
clean_ulog;
NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

# No join expected, as set by libraries...
if [ "X`grep suspend atmiclt87.log`" != "X" ]; then
    echo "No join expected, but got..."
    go_out 1
fi

# check number of suspends (1 - sever end, 1 - client end)
verify_ulog "RM1" "xa_end" "2";

go_out $RET

# vim: set ts=4 sw=4 et smartindent:

