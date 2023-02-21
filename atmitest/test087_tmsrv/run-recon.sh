#!/bin/bash
##
## @brief TMSRV / XA drive re-connect checks
##
## @file run-recon.sh
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

echo ""
echo "************************************************************************"
echo "Start RECON"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:-7:2:0
xa_end_entry:-7:2:0
xa_rollback_entry:-7:2:8
xa_prepare_entry:-7:2:-7
xa_commit_entry:0:1:0
xa_recover_entry:0:1:0
xa_forget_entry:-7:2:0
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

# start only here, as we want fresh tables...
xadmin start -y || go_out 1

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


#
# Get the final readings...
# 
xadmin stop -y

# clt: 1 - at boot
# clt: 2 - restart of xa_start
# clt: 2 - restart of xa_end
# srv: 1 - at boot
# srv: 2 - restart of xa_start
# srv: 2 - restart of end
# tms: 3 - at boot (3x threads, incl background?)
# tms: 0 - at prep ( connection closed)
# tms: 3 - rollback (reconn + 2x attempts)
# tms: 2 - forget
# tms: 3 - prep attempts
verify_ulog "RM1" "xa_open" "21";
verify_ulog "RM1" "xa_close" "21";

# clt 3x start + join after call
# srv 3x start
#
verify_ulog "RM1" "xa_start" "7";

#
# End must match the start count
#
verify_ulog "RM1" "xa_end" "7";
verify_ulog "RM1" "xa_prepare" "4";
#
# (1x reconnect (for counter))
# 1 org attempt
# 2 re-conn, second attempt OK, gives 8
# 
verify_ulog "RM1" "xa_rollback" "3";
verify_ulog "RM1" "xa_forget" "3";
verify_ulog "RM1" "xa_commit" "0";
verify_logfiles "log1" "0";

#
# clt - 1x
# srv - 1x
# tms - 2x (main + thread)
# the same for close.
#
verify_ulog "RM2" "xa_open" "4";
verify_ulog "RM2" "xa_close" "4";
verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "1";
verify_ulog "RM2" "xa_forget" "0";
verify_logfiles "log2" "0";


echo ""
echo "************************************************************************"
echo "COMMIT Retry"
echo "************************************************************************"

cat << EOF > lib1.rets
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

clean_ulog;
xadmin start -y || go_out 1

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#
# Get the final readings...
# 
xadmin stop -y

#verify results ops...

# clt: 1
# sv: 1
# tms: 3 (normal)
# tms: 2 (retry)
verify_ulog "RM1" "xa_open" "7";
verify_ulog "RM1" "xa_close" "7";
verify_ulog "RM1" "xa_prepare" "1";
verify_ulog "RM1" "xa_commit" "3";
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
echo "RECOVER Retry"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:0:1:0
xa_commit_entry:0:1:0
xa_recover_entry:-7:2:0
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
xa_recover_entry:-7:2:0
xa_forget_entry:0:1:0
xa_complete_entry:0:1:0
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
EOF

clean_ulog;
xadmin start -y || go_out 1

NDRX_CCTAG="RM1" tmrecovercl
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi

#
# Get the final readings...
# 
xadmin stop -y

#
# tms: 3x std opens (main, bgthr, worker)
# tms: 2x recons on recover
# tms: 1x atmisv con
#
verify_ulog "RM1" "xa_open" "6";
verify_ulog "RM1" "xa_close" "6";
verify_ulog "RM1" "xa_prepare" "0";
verify_ulog "RM1" "xa_commit" "0";
verify_ulog "RM1" "xa_rollback" "0";
verify_ulog "RM1" "xa_forget" "0";
# 
# 1x Org attempt
# 2x retries
#
verify_ulog "RM1" "xa_recover" "3";
verify_logfiles "log1" "0"
verify_ulog "RM1" "xa_end" "0";

verify_ulog "RM2" "xa_open" "6";
verify_ulog "RM2" "xa_close" "6";
verify_ulog "RM2" "xa_prepare" "0";
verify_ulog "RM2" "xa_commit" "0";
verify_ulog "RM2" "xa_rollback" "0";
verify_ulog "RM2" "xa_forget" "0";
verify_ulog "RM2" "xa_recover" "3";
verify_logfiles "log2" "0"

echo ""
echo "************************************************************************"
echo "OPEN/CLOSE Retry (system does not start...)"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:-5:2:0
xa_close_entry:-7:2:0
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
xa_open_entry:-5:1:0
xa_close_entry:-7:1:0
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

clean_ulog;
xadmin start -y || go_out 1

ERR=`NDRX_CCTAG="RM1" ./atmiclt87 2>&1`
RET=$?
# print the stuff
echo "[$ERR]"

if [ "X$RET" == "X0" ]; then
    echo "atmiclt87 must fail"
    go_out 1
fi

if [[ $ERR != *"TPERMERR"* ]]; then
    echo "Expected TPERMERR"
    go_out 1
fi

xadmin stop -y

echo ""
echo "************************************************************************"
echo "Prepare Retry, other err -> TPEABORT"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-3:1:0
xa_commit_entry:0:2:0
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

clean_ulog;
xadmin start -y || go_out 1

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

#
# Get the final readings...
# 
xadmin stop -y

echo ""
echo "************************************************************************"
echo "Prepare Retry, other err configred recon"
echo "************************************************************************"

cat << EOF > lib1.rets
xa_open_entry:0:1:0
xa_close_entry:0:1:0
xa_start_entry:0:1:0
xa_end_entry:0:1:0
xa_rollback_entry:0:1:0
xa_prepare_entry:-3:1:0
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

export NDRX_XA_FLAGS="RECON:*:3:100:-3,-7"
clean_ulog;
xadmin start -y || go_out 1

NDRX_CCTAG="RM1" ./atmiclt87
RET=$?

if [ "X$RET" != "X0" ]; then
    echo "Build atmiclt87 failed"
    go_out 1
fi


go_out 0

# vim: set ts=4 sw=4 et smartindent:
