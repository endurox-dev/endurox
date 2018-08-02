#!/bin/bash
## 
## @brief @(#) Test021 - XA testing
##
## @file cli-tests.sh
##
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 3 of the License, or (at your option) any later
## version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
## PARTICULAR PURPOSE. See the GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
##
## -----------------------------------------------------------------------------
## A commercial use license is available from Mavimax, Ltd
## contact@mavimax.com
## -----------------------------------------------------------------------------
##

IFS=$'\n'

# Include test functions
source ./test-func-include.sh

echo "Cli tests..."
(./atmiclt21-cli 2>&1) > ./atmiclt21-cli-dom1.log || exit 1

# Should print 100x transactions:
ensure_tran 100

###############################################################################
# Now abort all trans...
###############################################################################
REFS=`xadmin pt | grep 'TM ref' | cut -d ':' -f2 | cut -d ')' -f1`
for t in $REFS
do
        cmd="xadmin abort $t -y" 
        echo "Running [$cmd"]
        eval $cmd
done
###############################################################################
# Now do commit tests, will fail because we are not  in PREPARE tx stage
###############################################################################
(./atmiclt21-cli 2>&1) > ./atmiclt21-cli-dom1.log || exit 1
REFS=`xadmin pt | grep 'TM ref' | cut -d ':' -f2 | cut -d ')' -f1`

for t in $REFS
do
        cmd="xadmin commit $t -y" 
        echo "Running [$cmd"]
        eval $cmd
done

ensure_tran 100



