#!/bin/bash
##
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

# kill childs when master exist...
shopt -s huponexit

OS=`uname`

if [[ "$OS" == "Linux" ]]; then
	ls -l /dev/mqueue
	if [ $? -ne 0 ]; then
		echo "Have you mounted /dev/mqueue? Run:"
		echo "# mkdir /dev/mqueue"
        	echo "# mount -t mqueue none /dev/mqueue"
		exit 1
	fi
fi

# Load env for xmemck
. ../sampleconfig/setndrx

# start memcheck
xmemck -v20 -d35 -s60 -t95 -n 'atmiunit1|tpbridge|tmsrv|convsv21|tmqueue|cpmsrv' -m atmi -d40 -m cpmsrv -d100 -m tpbridge -m tmsrv 2>./memck.log 1>./memck.out & 

MEMCK_PID=$!
echo "Memck pid = $MEMCK_PID"

pushd .
(./atmiunit1 2>&1) > test.out
RET=$?
popd

# stop memcheck
# bash will stop?
#xadmin killall xmemck
#xadmin killall xmemck
kill -9 $MEMCK_PID

# grep the stats, >>> LEAK found, return error
echo "---- Leak info ----" >> test.out
cat memck.out >> test.out
echo "-------------------" >> test.out

# Catch memory leaks...
if [ "X`grep '>>> LEAK' memck.out`" != "X" ]; then
        echo "Memory leak detected!"
        RET=-2
fi

#
# print some stats...
#
grep with test.out

echo "atmitest/run.sh Terminates with: $RET"
exit $RET

# vim: set ts=4 sw=4 et smartindent:
