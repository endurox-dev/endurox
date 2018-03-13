/* 
** ATMI level cache - operations.
** Inval - their change only at point when we are going to save the results
** in DB. And hopefully we get hopefully we get the newest result?
** In case of keygroup if keyitem is deleted, then all items shall be deleted,
** and keygroup shall be removed.
**
** @file atmi_cache_ops.c
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
 * Check response rule, should we cache this or not
 * @param cache cache object
 * @param save_tperrno tperror number
 * @param save_tpurcode user return code
 * @return EXFAIL/EXFALSE/EXTRUE
 */
exprivate int ndrx_cache_chkrsprule(ndrx_tpcallcache_t *cache, 
            long save_tperrno, long save_tpurcode)
{
    int ret = EXFALSE;
    char buf[512];
    UBFH *p_ub = (UBFH *)buf;
    
    if (EXSUCCEED!=Binit(p_ub, sizeof(buf)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: failed to init response check buffer: %s",
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Load the data into buffer */
    
    if (EXSUCCEED!=Bchg(p_ub, EX_TPERRNO, 0, (char *)&save_tperrno, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to set EX_TPERRNO[0] to %ld: %s",
                save_tperrno, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_TPURCODE, 0, (char *)&save_tpurcode, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to set EX_TPURCODE[0] to %ld: %s",
                save_tpurcode, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Finally evaluate the expression */
    
    if (EXFAIL==(ret=Bboolev(p_ub, cache->rsprule_tree)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "cache: Failed to evalute [%s] "
                "tree: %p expression: %s",
                cache->rsprule, cache->rsprule_tree, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Response expression [%s]: %s", cache->rsprule,
            (EXTRUE==ret?"TRUE":"FALSE"));
    
out:
            
    return ret;

}

/**
 * Save data to cache. If doing save and this is key item. then we shall update
 * the key group (add new key to keygroup...).
 * @param idata
 * @param ilen
 * @param save_tperrno
 * @param save_tpurcode
 * @param nodeid cluster node id who put the message in cache
 * @param t time stamp sec from EPOCH
 * @param tusec micro seconds of ECPOCH time
 * @param is_event did we receive this from event server?
 * @return EXSUCCEED/EXFAIL/NDRX_TPCACHE_ENOCACHE
 */
expublic int ndrx_cache_save (char *svc, char *idata, 
        long ilen, int save_tperrno, long save_tpurcode, int nodeid, long flags,
        int tusec, long t, int is_event)
{
    int ret = EXSUCCEED;
    /* have a buffer in size of ATMI message */
    char buf[NDRX_MSGSIZEMAX];
    ndrx_tpcache_svc_t *svcc = NULL;
    typed_buffer_descr_t *buf_type;
    buffer_obj_t *buffer_info;
    ndrx_tpcallcache_t *cache;
    ndrx_tpcache_data_t *exdata = (ndrx_tpcache_data_t *)buf;
    int tran_started = EXFALSE;
    EDB_txn *txn;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_val cachedata;
    int is_matched=EXFALSE;
    
    memset(exdata, 0, sizeof(ndrx_tpcache_data_t));
    
    exdata->magic = NDRX_CACHE_MAGIC;
    NDRX_STRCPY_SAFE(exdata->svcnm, svc);
    exdata->nodeid = nodeid;
    exdata->saved_tperrno = save_tperrno;
    exdata->saved_tpurcode = save_tpurcode;
    
    /* get current timestamp */
    if (EXFAIL==t)
    {
        ndrx_utc_tstamp2(&exdata->t, &exdata->tusec);
    }
    else
    {
        exdata->t = t;
        exdata->tusec = (long)tusec;
    }
    
#ifdef NDRX_TPCACHE_DEBUG
    NDRX_LOG(log_debug, "Cache time: t=%ld tusec=%ld",
            exdata->t, exdata->tusec);
#endif
        
    /* OK now translate the thing to db format (i.e. make outgoing message) */
    
    /* Find service in cache */
    EXHASH_FIND_STR(ndrx_G_tpcache_svc, svc, svcc);
    
    if (NULL==svcc)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s]", svc);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    if (NULL!=idata)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(idata)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "%s: Buffer %p not known to system!", 
                    __func__, idata);
            EXFAIL_OUT(ret);
        }
    }
    
    buf_type = &G_buf_descr[buffer_info->type_id];
    
    DL_FOREACH(svcc->caches, cache)
    {
        is_matched = EXFALSE;
        
        if (cache->buf_type->type_id == buf_type->type_id)
        {
            if (ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_eval)
            {
                ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_eval(
                        cache, idata, ilen, errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "%s: Failed to evaluate buffer [%s]: %s", 
                            __func__, cache->rule, errdet);
                    
                    EXFAIL_OUT(ret);
                }
                else if (EXFALSE==ret)
                {
#ifdef NDRX_TPCACHE_DEBUG
                    NDRX_LOG(log_debug, "Buffer RULE FALSE [%s] - continue", cache->rule);
#endif
                    continue;
                }
                
                is_matched = EXTRUE;
                ret = EXSUCCEED;
            }
            else
            {
                /* We should not get here! */
                NDRX_CACHE_TPERROR(TPEINVAL,"%s: Unsupported buffer type [%s] for cache", 
                                __func__, cache->buf_type->type);
                EXFAIL_OUT(ret);
            }
        }
        
        /* Test the rule, if and not found then stage to NDRX_TPCACHE_ENOTFOUNADD 
         * OK, we need to build a key
         */

        NDRX_STRCPY_SAFE(key, cache->keyfmt);

        /* Build the key... */
        if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[buffer_info->type_id].pf_get_key(
                cache, idata, ilen, key, sizeof(key), errdet, sizeof(errdet))))
        {
            if (NDRX_TPCACHE_ENOKEYDATA==ret)
            {
                NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
                goto out;
            }
            else
            {
                NDRX_CACHE_TPERRORNOU(TPESYSTEM, "%s: Failed to build cache key: %s", 
                        __func__, errdet);
                goto out;
            }
        }
        
        if (cache->flags & NDRX_TPCACHE_TPCF_INVAL)
        {
            /* Invalidate their cache */
            if (EXSUCCEED!=ndrx_cache_inval_their(svc, cache, key, idata, ilen))
            {
                NDRX_LOG(log_error, "Failed to invalidate their cache!");
            }
            
            is_matched=EXFALSE;
        }
        
        if (cache->flags & NDRX_TPCACHE_TPCF_NEXT)
        {
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_debug, "Next flag present, go to next cache (if have one)");
#endif
            is_matched=EXFALSE;
            continue;
        }
        else
        {
            break;
        }
    }
    
    /* cache not found */
    if (!is_matched)
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s], buffer type [%s] "
                "or all was invalidate", svc, buf_type->type);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    exdata->flags = cache->flags;
    exdata->cache_idx = cache->idx;
    exdata->atmi_type_id = buffer_info->type_id;
    exdata->atmi_buf_len = NDRX_MSGSIZEMAX - sizeof(ndrx_tpcache_data_t);
            
    if (NULL==ndrx_G_tpcache_types[cache->buf_type->type_id].pf_cache_put)
    {
        ret = NDRX_TPCACHE_ENOTYPESUPP;
        goto out;
        
    }
    
    /* Check the response rule if defined */
    
    if (NULL!=cache->rsprule_tree)
    {
        if (EXFAIL==(ret=ndrx_cache_chkrsprule(cache, (long)save_tperrno, 
                save_tpurcode)))
        {
            NDRX_LOG(log_error, "Failed to test response code");
            EXFAIL_OUT(ret);
        }
        
        if (EXFALSE==ret)
        {
            NDRX_LOG(log_info, "Response shall not be saved according to rsp rule");
            ret = EXSUCCEED;
            goto out;
        }
        
        ret = EXSUCCEED;
    }
    
    if (EXSUCCEED!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_cache_put(
            cache, exdata, buf_type, idata, ilen, flags))
    {
        /* Error shall be set by func */
        NDRX_LOG(log_error, "Failed to convert to cache format!!!");
        EXFAIL_OUT(ret);
        
    }
    
    NDRX_LOG(log_info, "About to cache data for service: [%s]", svc);
    
    
    NDRX_STRCPY_SAFE(key, cache->keyfmt);
       
    /* Build the key... */
    if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[buffer_info->type_id].pf_get_key(cache, idata, 
            ilen, key, sizeof(key), errdet, sizeof(errdet))))
    {
        if (NDRX_TPCACHE_ENOKEYDATA==ret)
        {
            NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Failed to build key: ", errdet);
            
            /* generate TP error here! */
            ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to build cache key: %s", 
                    __func__, errdet);
            goto out;
        }
            
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->cachedb, &txn, 0)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    tran_started = EXTRUE;
    
    /* Firstly add record to the key group if needed */
    
    if (cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
    {
        if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache, idata, ilen, 
                key, NULL, EXFALSE, txn)))
        {
            NDRX_LOG(log_error, "Failed to add keygroup record!");
            goto out;
        }
    }
    
    
    /* Add the record to the DB, to specific key! */
#if 0
    if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        dbflags = EDB_APPENDDUP;
    }
    else
    {
        dbflags = 0;
    }
#endif
    
    cachedata.mv_data = (void *)exdata;
    cachedata.mv_size = exdata->atmi_buf_len + sizeof(ndrx_tpcache_data_t);
    
    
    NDRX_LOG(log_info, "About to put to cache: svc: [%s] key: [%s]: size: %ld",
            svcc->svcnm, key, (long)cachedata.mv_size);
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_put (cache->cachedb, txn, 
            key, &cachedata, 0)))
    {
        NDRX_LOG(log_debug, "Failed to put DB record!");
        goto out;
    }
    
    NDRX_LOG(log_debug, "Data cached, key [%s]", key);
    
    if ((cache->cachedb->flags & NDRX_TPCACHE_FLAGS_BCASTPUT)
        && !is_event)
    {
        if (EXSUCCEED!=ndrx_cache_broadcast(cache, svc, idata, ilen, 
                NDRX_CACHE_BCAST_MODE_PUT, NDRX_TPCACHE_BCAST_DFLT, 
                (int)exdata->tusec, (long)exdata->t, 
                save_tperrno, save_tpurcode))
        {
            NDRX_LOG(log_error, "WARNING ! Failed to broadcast put event - continue");
        }
    }
    
out:

    if (tran_started)
    {
        /* in case if no cache, we might have done invalidate... */
        if (EXSUCCEED==ret || NDRX_TPCACHE_ENOCACHE==ret)
        {
            ndrx_cache_edb_commit(cache->cachedb, txn);
        }
        else
        {
            ndrx_cache_edb_abort(cache->cachedb, txn);
        }
    }

    return ret;
}

/**
 * Lookup service in cache.
 * We do not do writes here, thus we can use read only cursors...
 * @param svc service to call
 * @param idata input data buffer
 * @param ilen input len
 * @param odata output data buffer
 * @param olen output len
 * @param flags flags
 * @param should_cache should record be cached?
 * @param seterror_not_found should we generate error if record is not found?
 * @return EXSUCCEED/EXFAIL (syserr)/NDRX_TPCACHE_ENOKEYDATA (cannot build key)
 */
expublic int ndrx_cache_lookup(char *svc, char *idata, long ilen, 
        char **odata, long *olen, long flags, int *should_cache, 
        int *saved_tperrno, long *saved_tpurcode, int seterror_not_found)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_svc_t *svcc = NULL;
    typed_buffer_descr_t *buf_type;
    buffer_obj_t *buffer_info;
    ndrx_tpcallcache_t *cache;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    EDB_txn *txn;
    int cursor_open = EXFALSE;
    EDB_cursor *cursor;
    int tran_started = EXFALSE;
    EDB_val cachedata;
    EDB_val cachedata_update;
    EDB_val cachedata_delete;
    ndrx_tpcache_data_t *exdata;
    ndrx_tpcache_data_t *exdata_update;
    int is_matched;
    unsigned int flagsdb;
    /* Key size - assume 16K should be fine */
    /* get buffer type & sub-type */
    cachedata_update.mv_size = 0;
    cachedata_update.mv_data = NULL;
        
    /* Find service in cache */
    EXHASH_FIND_STR(ndrx_G_tpcache_svc, svc, svcc);
    
    if (NULL==svcc)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s]", svc);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    if (NULL!=idata)
    {
        if (NULL==(buffer_info = ndrx_find_buffer(idata)))
        {
            ndrx_TPset_error_fmt(TPEINVAL, "%s: Buffer %p not known to system!", 
                    __func__, idata);
            EXFAIL_OUT(ret);
        }
    }
    
    
    /* Loop over the tpcallcaches, if `next' flag present, then perform next
     * if we get invalidate their, then delete target records by the key */
    buf_type = &G_buf_descr[buffer_info->type_id];

    DL_FOREACH(svcc->caches, cache)
    {
        is_matched = EXFALSE;
        
        if (cache->buf_type->type_id == buf_type->type_id)
        {   
            if (ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_eval)
            {
                ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_rule_eval(
                        cache, idata, ilen, errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    NDRX_CACHE_TPERROR(TPEINVAL, "%s: Failed to evaluate buffer [%s]: %s", 
                            __func__, cache->rule, errdet);
                    
                    EXFAIL_OUT(ret);
                }
                else if (EXFALSE==ret)
                {
#ifdef NDRX_TPCACHE_DEBUG
                    NDRX_LOG(log_debug, "Buffer RULE FALSE [%s] - try next", 
                            cache->rule);
#endif
                    continue;
                }
                
                NDRX_LOG(log_debug, "rule [%s] matched", cache->rule);
                is_matched = EXTRUE;
                
                
                /* any way if matched any then we want next cache run.. */
                
                *should_cache=EXTRUE;
                
                ret = EXSUCCEED;
            }
            else
            {
                /* We should not get here! */
                NDRX_CACHE_TPERROR(TPEINVAL,"%s: Unsupported buffer type [%s] for cache", 
                                __func__, cache->buf_type->type);
                EXFAIL_OUT(ret);
            }
        }
        
        /* if we are here, the cache is matched */
        
        if (!(cache->flags & NDRX_TPCACHE_TPCF_INVAL))
        {
            /* ONLY IN CASE IF NOT INVAL - Check should we refresh? */
            if (NULL!=ndrx_G_tpcache_types[cache->buf_type->type_id].pf_refreshrule_eval &&
                    EXEOS!=cache->refreshrule[0])
            {
                ret = ndrx_G_tpcache_types[cache->buf_type->type_id].pf_refreshrule_eval(cache, 
                        idata, ilen, errdet, sizeof(errdet));
                if (EXFAIL==ret)
                {
                    /* Failed to eval refresh rule */
                    NDRX_LOG(log_error, "Failed to eval refresh rule: %s", errdet);

                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to eval refresh rule: %s", 
                            errdet);
                    EXFAIL_OUT(ret);
                }
                else if (EXTRUE==ret)
                {
                    NDRX_LOG(log_info, "Cache will be refreshed - rule matched "
                            "(do not continue cache lookup)");
                    *should_cache=EXTRUE;
                    ret = NDRX_TPCACHE_ENOCACHEDATA;
                    goto out;
                }
            }
        }

        /* Test the rule, if and not found then stage to NDRX_TPCACHE_ENOTFOUNADD 
         * OK, we need to build a key
         */

        NDRX_STRCPY_SAFE(key, cache->keyfmt);

        /* Build the key... */
        if (EXSUCCEED!=(ret = ndrx_G_tpcache_types[buffer_info->type_id].pf_get_key(
                cache, idata, ilen, key, sizeof(key), errdet, sizeof(errdet))))
        {
            if (NDRX_TPCACHE_ENOKEYDATA==ret)
            {
                NDRX_LOG(log_debug, "Failed to build key (no data for key): %s", errdet);
                goto out;
            }
            else
            {
                NDRX_LOG(log_error, "Failed to build key: ", errdet);

                /* generate TP error here! */
                ndrx_TPset_error_fmt(TPESYSTEM, "%s: Failed to build cache key: %s", 
                        __func__, errdet);
                goto out;
            }
        }
        
        if (cache->flags & NDRX_TPCACHE_TPCF_NEXT)
        {
#ifdef NDRX_TPCACHE_DEBUG
            NDRX_LOG(log_debug, "Next flag present, go to next cache (if have one)");
#endif
            is_matched=EXFALSE;
            continue;
        }
        else
        {
            break;
        }
        
    }
    
    /* cache not found */
    if (!is_matched)
    {
        
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "No cache defined for [%s], buffer type [%s] ", 
                svc, buf_type->type);
#endif
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    /* LOOP END */
    
    *should_cache=EXTRUE;
    
    if (cache->flags & NDRX_TPCACHE_TPCF_INVAL)
    {
        NDRX_LOG(log_info, "Last was invalidate cache -> let save process to inval!");
        ret = NDRX_TPCACHE_ENOCACHEDATA;
        goto out;
    }
        
    if (flags & TPNOCACHEDDATA)
    {
        /* in this case we must */
        
        NDRX_LOG(log_info, "No cache data -> request lookup for key [%s]", key);
        
        ret = NDRX_TPCACHE_ENOCACHEDATA;
        goto out;
    }
    
    /* Check the cache group now (if defined for this cache) */
    if (cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
    {
        if (EXSUCCEED!=(ret = ndrx_cache_keygrp_lookup(cache, idata, ilen, 
                odata, olen, key, flags)))
        {
            /* lets see what error says us */
            
            switch (ret)
            {
                case NDRX_TPCACHE_ENOCACHEDATA:
                    /* OK data not in cache return that not found */
                    goto out;
                    break;
                case NDRX_TPCACHE_ENOTFOUNDLIM:
                    *saved_tperrno = cache->keygroupmtperrno;
                    *saved_tpurcode = cache->keygroupmtpurcode;
                    
                    
                    NDRX_LOG(log_debug, "MAX keygroup reject tperrno: %d "
                            "tpurcode: %ld", *saved_tperrno, *saved_tpurcode);
                    
                    ret=EXSUCCEED;
                    goto out;
                    
                    break;
                default:
                    goto out;
                    break;
            }
        }
    }
    
    /* Lookup DB - check the flags if with update, requires update, then no read
     * only */
    
    /* TODO: For TIMESYNC we might move to RO and leave dupscan for tpcached! */
    if ( (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC) ||
            (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_LRU) ||
            (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_HITS))
    {
        flagsdb = 0;
    }
    else
    {
        flagsdb = EDB_RDONLY;
    }
    
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->cachedb, &txn, flagsdb)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    tran_started = EXTRUE;
    
    if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Performing timesync based complex lookup");
#endif

        if (EXSUCCEED!=ndrx_cache_edb_cursor_open(cache->cachedb, txn, &cursor))
        {
            NDRX_LOG(log_error, "Failed to open cursor!");
            EXFAIL_OUT(ret);
        }
        
        /* OK fetch the first rec of cursor, next records we shall kill (if any) */
        /* first: EDB_FIRST_DUP - this we accept and process */
        
        if (EXSUCCEED!=(ret=ndrx_cache_edb_cursor_get(cache->cachedb, cursor,
                    key, &cachedata, EDB_SET_KEY)))
        {
            if (EDB_NOTFOUND!=ret)
            {
                NDRX_LOG(log_error, "Failed to scan for data!");
                EXFAIL_OUT(ret);
            }
            /* no data found */
            ret = NDRX_TPCACHE_ENOCACHEDATA;
            goto out;
        }
        
    }
    else
    {
#ifdef NDRX_TPCACHE_DEBUG
        NDRX_LOG(log_debug, "Performing simple lookup");
#endif
        if (EXSUCCEED!=(ret=ndrx_cache_edb_get(cache->cachedb, txn, key, &cachedata,
                seterror_not_found)))
        {
            /* error already provided by wrapper */
            NDRX_LOG(log_debug, "%s: failed to get cache by [%s]", __func__, key);
            goto out;
        }
    }
    
    exdata = (ndrx_tpcache_data_t *)cachedata.mv_data;
    
    /* validate record */
        
    NDRX_CACHE_CHECK_DBDATA((&cachedata), exdata, key, TPMINVAL);

    /* OK we have a raw data... lets dump something... */
#ifdef NDRX_TPCACHE_DEBUG
    NDRX_LOG(log_debug, "Got cache record for key [%s] of service [%s]", key, svc);
    /* Dump more correctly with admin info */
    NDRX_DUMP(6, "Got cache data", (char *)cachedata.mv_data, 
            (int)cachedata.mv_size);
    NDRX_TPCACHETPCALL_DBDATA(log_debug, exdata);
#endif
    
    /* Error shall be set by func */
    
    if (EXSUCCEED!=ndrx_G_tpcache_types[buffer_info->type_id].pf_cache_get(
            cache, exdata, buf_type, idata, ilen, odata, olen, flags))
    {
        NDRX_LOG(log_error, "%s: Failed to receive data: ", __func__);
        EXFAIL_OUT(ret);
    }
    
    *saved_tperrno = exdata->saved_tperrno;
    *saved_tpurcode = exdata->saved_tpurcode;
    
    NDRX_LOG(log_debug, "cache tperrno: %d tpurcode: %ld",
            *saved_tperrno, *saved_tpurcode);
    
    /* Update cache (if needed) */
    
    
    /* perform copy if needed for cache update */
    if ((cache->cachedb->flags & NDRX_TPCACHE_FLAGS_LRU) ||
            (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_HITS))
    {
        cachedata_update.mv_size = cachedata.mv_size;
        cachedata_update.mv_data = NDRX_MALLOC(cachedata.mv_size);
        
        if (NULL==cachedata_update.mv_data)
        {
            int err = errno;
            
            NDRX_CACHE_TPERROR(TPEOS, "Failed to allocate %ld bytes: %s",
                    (long)cachedata_update.mv_size, strerror(err));
        }
        
        memcpy(cachedata_update.mv_data, cachedata.mv_data, cachedata.mv_size);
        
        exdata_update = (ndrx_tpcache_data_t *)cachedata_update.mv_data;
        /* ok this might overflow, then it will be time for cache to reset...
         * but that will be long number of requests away...
         */

	/* if we reach max then keep max */
	
	
	if (exdata_update->hits < LONG_MAX - 1)
	{
            exdata_update->hits++;
	}
	else
	{
            /* set to max value (if any runs over) */
            exdata_update->hits  = LONG_MAX;
	}
	
        ndrx_utc_tstamp2(&exdata_update->hit_t, &exdata_update->hit_tusec);

#ifdef NDRX_TPCACHE_DEBUG        
        NDRX_LOG(log_debug, "hits=%ld t=%ld t=%ld", exdata_update->hits,
                exdata_update->hit_t, exdata_update->hit_tusec);
#endif
        if (cursor_open)
        {
            edb_cursor_close(cursor);
        }
        cursor_open=EXFALSE;
        
        /* delete all records 
         * TODO: Maybe we can skip the delete step?
         */
        if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->cachedb, txn, 
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
        
        /* Add record */
        
        /* dup was mandatory... flag if using sorting, afaik, or not? */
#if 0
        if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
        {
            dbflags = EDB_APPENDDUP;
        }
        else
        {
            dbflags = 0;
        }
#endif
        if (EXSUCCEED!=(ret=ndrx_cache_edb_put (cache->cachedb, txn, 
            key, &cachedata_update, 0)))
        {
            NDRX_LOG(log_debug, "Failed to put/update DB record!");
            goto out;
        }
        
    }
    else if (cache->cachedb->flags & NDRX_TPCACHE_FLAGS_TIMESYNC)
    {
        /* fetch next for dups and remove them.. if any.. */
        /* next: MDB_NEXT_DUP  - we kill this! */
        while (EXSUCCEED==(ret=ndrx_cache_edb_cursor_get(cache->cachedb, cursor,
                    key, &cachedata_delete, EDB_NEXT_DUP)))
        {
            /* delete the record, not needed, some old cache rec */
            NDRX_DUMP(log_debug, "Deleting duplicate record...", 
                    cachedata_delete.mv_data, cachedata_delete.mv_size);
            
            if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->cachedb, txn, 
                    key, &cachedata_delete)))
            {
                if (ret!=EDB_NOTFOUND)
                {
                    /* if not found maybe next key will be found */
                    break;
                }
            }
        }
        
        if (ret!=EDB_NOTFOUND)
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            ret = EXSUCCEED;
        }
    }
    
out:

    if (NULL!=cachedata_update.mv_data)
    {
        NDRX_FREE(cachedata_update.mv_data);
    }

    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            ndrx_cache_edb_commit(cache->cachedb, txn);
        }
        else
        {
            ndrx_cache_edb_abort(cache->cachedb, txn);
        }
    }

    return ret;
}

