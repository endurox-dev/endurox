/* 
** ATMI level cache - invalidate
**
** @file atmi_cache_inval.c
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
 * Invalidate their cache
 * @param svc service name called
 * @param cache cache descriptor (invalidator)
 * @param key key built (their key)
 * @param idata input data
 * @param ilen input data len
 * @return EXSUCCEED/EXFAIL (tperror set)
 */
expublic int ndrx_cache_inval_their(char *svc, ndrx_tpcallcache_t *cache, 
        char *key, char *idata, long ilen)
{
    int ret = EXFALSE;
    int tran_started = EXFALSE;
    EDB_txn *txn;
    
    
    /* If this is not full keygrp inval, then remove record from group */
    if (cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
    {
        if (cache->flags & NDRX_TPCACHE_TPCF_INVLKEYGRP)
        {
            /* Remove full group */
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_inval_by_key(cache->keygrpdb, 
                    key, NULL, cache->cachedbnm)))
            {
                NDRX_LOG(log_error, "failed to remove keygroup!");
                goto out;
            }
            
        }
        else
        {
            /* remove just key item... and continue */
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache, 
                    idata, ilen, key, EXTRUE)))
            {
                NDRX_LOG(log_error, "Failed to remove key [%s] from keygroup!");
                goto out;
            }
        }
    }
    
    /*
     * just delete record from theyr cache, ptr to cache we have already inside
     * the cache object 
     */
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->inval_cache->cachedb, &txn, 0)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    NDRX_LOG(log_debug, "Delete their cache [%s] idx %d",
            cache->inval_svc, cache->inval_idx);
    
    /* If their is part of the cache group, then we shall 
     * invalidate whole group.. 
     */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->inval_cache->cachedb, txn, 
            key, NULL)))
    {
        if (EDB_NOTFOUND==ret)
        {
            ret=EXSUCCEED;
        }
        else
        {
            EXFAIL_OUT(ret);
        }
    }
    
    /* broadcast if needed */
    if (cache->inval_cache->cachedb->flags & NDRX_TPCACHE_FLAGS_BCASTDEL)
    {
        if (EXSUCCEED!=ndrx_cache_broadcast(cache->inval_cache, 
                cache->inval_svc, idata, ilen, 
                NDRX_CACHE_BCAST_MODE_DEL, NDRX_TPCACHE_BCAST_DELFULL, 0, 0, 0, 0))
        {
            NDRX_LOG(log_error, "WARNING ! Failed to broadcast delete event - continue");
            
            if (0!=tperrno)
            {
                NDRX_LOG(log_error, "TP Error set -> fail");
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            ndrx_cache_edb_commit(cache->inval_cache->cachedb, txn);
        }
        else
        {
            ndrx_cache_edb_abort(cache->inval_cache->cachedb, txn);
        }
    }

    return ret;
}
