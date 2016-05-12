#!/bin/bash
## 
## @(#) Test028 - Trnsactional Message Queue tests (with static & dynamic XA drivers)
##
## @file run.sh
## 
## -----------------------------------------------------------------------------
## Enduro/X Middleware Platform for Distributed Transaction Processing
## Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
## This software is released under one of the following licenses:
## GPL or ATR Baltic's license for commercial use.
## -----------------------------------------------------------------------------
## GPL license:
## 
## This program is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
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
## A commercial use license is available from ATR Baltic, SIA
## contact@atrbaltic.com
## -----------------------------------------------------------------------------
##


#
# Dynamic tests
#
echo "Dynamic XA driver tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdiskd.so
./run-dom.sh
RET=$?

if [[ "X$RET" != "X0" ]]; then
    exit 1
fi


#
# Static tests
#
echo "Static XA driver tests..."
export NDRX_XA_DRIVERLIB_FILENAME=libndrxxaqdisks.so
./run-dom.sh
RET=$?

if [[ "X$RET" != "X0" ]]; then
    exit 1
fi

