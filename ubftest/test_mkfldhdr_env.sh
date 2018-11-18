#!/bin/bash
##
##
## @file test_mkfldhdr_env.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
## Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## AGPL or Mavimax's license for commercial use.
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
echo "Testing mkfldhdr, reading field table from env" >&2
#####
DIR=`mktemp -d`

#  Generate files
ret=0

# Load env variables
. setenv

../mkfldhdr/mkfldhdr -d $DIR
# save return code
ret=$?

# Compare each generated file, should be the same...

if [ $ret -eq 0 ]; then
	# Compare each file by file
	LIST=`ls -1 $DIR`
	for f in $LIST; do
		echo "Comparing $DIR/$f with mkfldhdr_ref/$f" >&2
		diff $DIR/$f mkfldhdr_ref/$f
		ret=$?
		if [ $ret -ne 0 ]; then
			break;		
		fi
	done
fi

# Remove temp dir
#rm -rf $DIR
exit $ret
# vim: set ts=4 sw=4 et smartindent:
