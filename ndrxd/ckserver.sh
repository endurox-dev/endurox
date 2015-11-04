#!/bin/bash
## 
## @(#) This is part of ndrxd. This is called by ndrxd in cases when
## BY reply queue it founds out that server is dead... but then stands a question
## is the admin queue valid or not? Or shall be removed.
## Returns 1 - if process running
## Returns 0 - if process not running.
##
## @file ckserver.sh
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

function log()
{
	date_fmt=`date`
    if [[ "X" == "X$NDRX_DMNLOG" ]]; then
        echo "ckserver.sh:$date_fmt:$1"
    else
        echo "ckserver.sh:$date_fmt:$1" >> $NDRX_DMNLOG
    fi
}

PROCESS=$1
SRVID=$2
FILTER="${USER}.*${PROCESS}.*-k ${NDRX_RNDK}.*-i $SRVID"
log "PROCESS=[$PROCESS] SRVCID=[$SRVID] FILTER=[$FILTER]"
RES=`ps -ef | grep -e "$FILTER" | grep -v grep`
RET=1

if [ "X$RES" == "X" ]; then
	RET=0
fi
log "ret=$RET"

exit $RET

