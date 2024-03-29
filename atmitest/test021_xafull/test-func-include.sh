#!/bin/bash
##
## @brief @(#) Test021 - XA testing, common functions, include
##
## @file test-func-include.sh
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

ensure_tran () {

        count=$1
        xadmin pt
        TRAN_OUT=`xadmin pt | grep 'Transaction stage' | wc | awk '{ print $1 }'`
        echo "Tran count: [$TRAN_OUT] must be: [$count]"
        if [[ "$TRAN_OUT" != "$count" ]]; then
                echo "TESTERROR: Invalid transaction count!"
                exit 1
        fi
}

# vim: set ts=4 sw=4 et smartindent:
