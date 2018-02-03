/* 
** ATMI level cache - event processing (distributed cache)
**
** @file atmi_cache_events.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <atmi_tls.h>
#include <typed_buf.h>

#include "thlock.h"
#include "userlog.h"
#include "utlist.h"
#include "exregex.h"
#include <exparson.h>
#include <atmi_cache.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Broadcast buffer as event
 * @param svc service name of cache event occurring
 * @param idata input data
 * @param ilen input data len
 * @param event_type event type, see
 * @param flags flags to put in event NDRX_CACHE_BCAST_MODE_*
 * @return EXSUCCEED/EXFAIL, tperror installed if any
 */
expublic int ndrx_cache_broadcast(char *svc, char *idata, long ilen, 
           int event_type, char *flags)
{
    int ret = EXSUCCEED;
    char *fmt;
    char event[XATMI_EVENT_MAX+1];
    
    if (NDRX_CACHE_BCAST_MODE_PUT==event_type)
    {
        fmt = NDRX_CACHE_EV_PUT;
    }
    else if (NDRX_CACHE_BCAST_MODE_DEL==event_type)
    {
        fmt = NDRX_CACHE_EV_DEL;
    }
    else
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "Invalid broadcast event type: %d", 
                event_type);
        EXFAIL_OUT(ret);
    }
    
    /* Build the event to broadcast */
    
    snprintf(event, sizeof(event), fmt, (int)tpgetnodeid(), flags, svc);
    
    NDRX_LOG(log_debug, "Broadcasting event: [%s]", event);
    
    if (EXSUCCEED!=tppost(event, idata, ilen, TPNOTRAN|TPNOREPLY))
    {
        NDRX_CACHE_ERROR("Failed to send event [%s]: %s", 
                event, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:

    return ret;
}
