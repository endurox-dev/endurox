/**
 * @brief ATMI level cache - event processing (distributed cache)
 *
 * @file atmi_cache_events.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
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
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Broadcast buffer as event
 * @param cache cache on which we perform broadcast
 * @param svc service name of cache event occurring or cachedb name (depending on event)
 * @param idata input data
 * @param ilen input data len
 * @param event_type event type, see
 * @param flags flags to put in event NDRX_CACHE_BCAST_MODE_*
 * @param user1 user data field 1 (microseconds)
 * @param user2 user data field 2 (epoch seconds)
 * @param user3 user data field 1 (tperrno)
 * @param user4 user data field 2 (tpurcode)
 * @return EXSUCCEED/EXFAIL, tperror installed if any
 */
expublic int ndrx_cache_broadcast(ndrx_tpcallcache_t *cache, 
        char *svc, char *idata, long ilen, int event_type, char *flags,
        int user1, long user2, int user3, long user4)
{
    int ret = EXSUCCEED;
    char *fmt;
    char event[XATMI_EVENT_MAX+1];
    char *odata = NULL;
    long olen;
    
    if (NDRX_CACHE_BCAST_MODE_PUT==event_type)
    {
        fmt = NDRX_CACHE_EV_PUT;
        
        odata = idata;
        olen = ilen;
    }
    else if (NDRX_CACHE_BCAST_MODE_DEL==event_type)
    {
        fmt = NDRX_CACHE_EV_DEL;
        
        /* prepare projection on what we want to broadcast */
        
        if (ndrx_G_tpcache_types[cache->buf_type->type_id].pf_cache_del)
        {
            if (EXSUCCEED!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_cache_del(
                    cache, idata, ilen, &odata, &olen))
            {
                NDRX_LOG(log_error, "Failed to prepare broadcast data for delete");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            odata = idata;
            olen = ilen;
        }
        
    }
    else if (NDRX_CACHE_BCAST_MODE_KIL==event_type)
    {
        fmt = NDRX_CACHE_EV_KILL;
        
        odata = idata;
        olen = ilen;
    }
    else if (NDRX_CACHE_BCAST_MODE_MSK==event_type)
    {
        fmt = NDRX_CACHE_EV_MSKDEL;
        odata = idata;
        olen = ilen;
    }
    else if (NDRX_CACHE_BCAST_MODE_DKY==event_type)
    {
        fmt = NDRX_CACHE_EV_KEYDEL;
        odata = idata;
        olen = ilen;
    }       
    else
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "Invalid broadcast event type: %d", 
                event_type);
        EXFAIL_OUT(ret);
    }
    
    /* Build the event to broadcast */
    
    snprintf(event, sizeof(event), fmt, (int)tpgetnodeid(), flags, svc);
    
    NDRX_LOG(log_debug, "Broadcasting event: [%s] user1=%d user2=%ld "
            "user3=%d user4=%ld", event, user1, user2, user3, user4);
    
    if (EXFAIL==ndrx_tppost(event, odata, olen, TPNOTRAN|TPNOREPLY, user1, user2,
            user3, user4))
    {
        NDRX_CACHE_ERROR("Failed to send event [%s]: %s", 
                event, tpstrerror(tperrno));
        
        /* ignore status, unset error */
        
        ndrx_TPunset_error();
        
        EXFAIL_OUT(ret);
    }
    
out:

    if (odata!=NULL && odata!=idata)
    {
        tpfree(odata);
    }

    return ret;
}

/**
 * Return list of event to which subscribe in current CCTAG. Note that we have
 * only service instance of cache events. thus we must see all cctags of caches
 * Multiple executables may run
 * @param list string list of events to subscribe to
 * @return EXFAIL/EXSUCCEED (in case of succeed string list free required if not
 * NULL)
 */
expublic int ndrx_cache_events_get(string_list_t **list)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_db_t *el, *elt;
    
    /* loop over all databases and get subscribe events */
    
    EXHASH_ITER(hh, ndrx_G_tpcache_db, el, elt)
    {
        if (EXEOS!=el->subscr[0])
        {
            if (EXSUCCEED!=ndrx_string_list_add(list, el->subscr))
            {
                NDRX_LOG(log_error, "failed to add string to list [%s]", 
                        el->subscr);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:

    if (EXSUCCEED!=ret && NULL!=*list)
    {
        ndrx_string_list_free(*list);
        *list = NULL;
    }

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
