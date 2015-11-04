#!/bin/bash
## 
## This script is part of `ndrx' admin utilty
## this script checks is it safe to assume that ndrx is not running (i.e. this list all processes)
## Returns : 0 - no ndrxd processes running, 1 - ndrxd process is running
##
## @file ndrxd_chkdown.sh
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

FILTER="${USER}.*ndrxd -k ${NDRX_RNDK}"
res=`ps -ef | grep -e "$FILTER" | grep -v grep`
ret=1

if [ "X$res" == "X" ]; then
	ret=0	
fi

exit $ret

