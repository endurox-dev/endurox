#!/bin/bash
##
##
## @file test_mkfldhdr_syntax_err.sh
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
echo "Testing mkfldhdr, Syntax Error." >&2
#####
DIR=`mktemp -d`

#  Generate files
ret=0

# Load env variables
export FLDTBLDIR="."
# We re-use script by it self - it should be completely wrong!
export FIELDTBLS="test_mkfldhdr_syntax_err.sh"

../mkfldhdr/mkfldhdr -d $DIR
# save return code
ret=$?

# Remove temp dir
rm -rf $DIR
exit $ret
# vim: set ts=4 sw=4 et smartindent:
