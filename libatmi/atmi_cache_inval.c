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
    char flags[]={NDRX_TPCACHE_BCAST_DELFULLC, EXEOS};
    EDB_txn *txn;
    
    
    /* If this is not full keygrp inval, then remove record from group */
    if (cache->inval_cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
    {
        if (cache->flags & NDRX_TPCACHE_TPCF_INVLKEYGRP)
        {
            NDRX_LOG(log_debug, "Invalidate whole group!");
            /* Remove full group */
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_inval_by_key(
                    cache->inval_cache->keygrpdb, 
                    key, NULL, cache->inval_cache->cachedbnm)))
            {
                NDRX_LOG(log_error, "failed to remove keygroup!");
                goto out;
            }
            
            /* Broadcast the  */
            flags[0] = NDRX_TPCACHE_BCAST_GROUPC;
            goto ok_broadcast;
        }
        else
        {
            /* remove just key item... and continue */
            NDRX_LOG(log_debug, "Removing single key item from group (1)");
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
    
    /* remove from group... */
    if (cache->inval_cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS &&
            !(cache->flags & NDRX_TPCACHE_TPCF_INVLKEYGRP))
    {
        NDRX_LOG(log_debug, "Removing single key item from group (2)");
        if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache->inval_cache, 
                    idata, ilen, key, NULL, EXTRUE)))
        {
            NDRX_LOG(log_error, "Failed to remove key [%s] from keygroup!");
            goto out;
        }
    }
    
ok_broadcast:
    /* broadcast if needed */
    if (cache->inval_cache->cachedb->flags & NDRX_TPCACHE_FLAGS_BCASTDEL)
    {
        NDRX_LOG(log_debug, "Broadcast flags [%s]", flags);
        if (EXSUCCEED!=ndrx_cache_broadcast(cache->inval_cache, 
                cache->inval_svc, idata, ilen, 
                NDRX_CACHE_BCAST_MODE_DEL, flags, 0, 0, 0, 0))
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

/**
 * Delete cache record by data
 * @param svc Service name to search cache for
 * @param idata input data (XATMI buffer) received from their node
 * @param ilen input data len (XATMI buffer)
 * @param flags how to delete - if "F" then do not check they key, just full db drop
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cache_inval_by_data(char *svc, char *idata, long ilen, char *flags)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_svc_t *svcc = NULL;
    buffer_obj_t *buffer_info;
    typed_buffer_descr_t *buf_type;
    ndrx_tpcallcache_t *cache;
    char key[NDRX_CACHE_KEY_MAX+1];
    char errdet[MAX_TP_ERROR_LEN+1];
    int tran_started = EXFALSE;
    EDB_txn *txn = NULL;
    int delete_from_keygroup = EXFALSE;
    
    /* find svc, find cache, build key, delete record 
     * The logic will be more or less like a cache lookup...
     */
    
    /* Find service in cache */
    EXHASH_FIND_STR(ndrx_G_tpcache_svc, svc, svcc);
    
    if (NULL==svcc)
    {
        NDRX_LOG(log_debug, "No cache defined for service [%s]", svc);
        ret = NDRX_TPCACHE_ENOCACHE;
        ndrx_TPset_error_fmt(TPENOENT, "No cache defined for service [%s]", svc);
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
    
    
    /* find exact cache */
    if (NULL!=(cache = ndrx_cache_findtpcall(svcc, buf_type, idata, ilen, EXFAIL)))
    {
        NDRX_LOG(log_debug, "No caches matched by expressions for [%s]", svc);
        ndrx_TPset_error_fmt(TPENOENT, "No caches matched by expressions for [%s]", 
                svc);
        ret = NDRX_TPCACHE_ENOCACHE;
        goto out;
    }
    
    /* now delete the record, generate key */
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
    
    /* now delete the record */
    
    NDRX_LOG(log_debug, "Delete record by key [%s] from cache svc [%s] index: %d", 
            key, cache->svcnm, cache->idx);
    
    
    if (cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
    {
        /* we are part of keyitems group... perform group operation */
        
        /* Delete by keygroup */
        if (strchr(flags, NDRX_TPCACHE_BCAST_GROUPC))
        {
            NDRX_LOG(log_debug, "Full group delete");
            
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_inval_by_data(cache, idata, 
                    ilen, NULL)))
            {
                NDRX_LOG(log_error, "Failed to delete group!");
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Delete only keyitem from group");
            delete_from_keygroup=EXTRUE;
        }
        
    }
    
    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(cache->cachedb, &txn, 0)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran", __func__);
        goto out;
    }
    
    /* delete record (fully) */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_del (cache->cachedb, txn, key, NULL)))
    {
        /* ignore not deleted error... */
        if (EDB_NOTFOUND==ret)
        {
            ret=EXSUCCEED;
        }    
    }
    
    /* update keygroup */
    
    if (delete_from_keygroup)
    {
        if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache, idata, ilen, key, 
                NULL, EXTRUE)))
        {
            NDRX_LOG(log_error, "Failed to delete key from keygroup [%s]/[%s]!",
                    key, cache->keygrpdb->cachedb);
            goto out;
        }
    }
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(cache->cachedb, txn))
            {
                NDRX_LOG(log_error, "Failed to commit: %s", tpstrerror(tperrno));
                ret=EXFAIL;
            }
        }
        else
        {
            ndrx_cache_edb_abort(cache->cachedb, txn);
        }
    }
    

    return ret;
}

/**
 * Drop cache by name
 * NOTE ! The drop will not perform dropping of keygroup.
 * This does not perform any kind of broadcast
 * @param cachedbnm cache dabase name (in config, subsect)
 * @return EXSUCCEED/EXFAIL (tperror set)
 */
expublic int ndrx_cache_drop(char *cachedbnm, short nodeid)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    ndrx_tpcache_db_t* db;
    int tran_started = EXFALSE;
    
    NDRX_LOG(log_info, "Resetting cache db [%s] source node: [%hd]", 
            db->cachedb, nodeid);
    
    /* find cachedb */
    
    if (NULL==(db=ndrx_cache_dbresolve(cachedbnm, NDRX_TPCACH_INIT_NORMAL)))
    {
        NDRX_CACHE_TPERRORNOU(TPENOENT, "Failed to get cache record for [%s]: %s", 
                cachedbnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    if (EXSUCCEED!=(ret=edb_drop(txn, db->dbi, 0)))
    {
        NDRX_CACHE_TPERROR(ndrx_cache_maperr(ret), 
                "CACHE: Failed to clear db: [%s]: %s", 
                db->cachedb, edb_strerror(ret));

        EXFAIL_OUT(ret);
    }
    
    /* check if we should broadcast the drop */
    
    NDRX_LOG(log_warn, "Cache [%s] dropped", cachedbnm);
    
    if ( (db->flags & NDRX_TPCACHE_FLAGS_BCASTPUT) &&
            tpgetnodeid()==nodeid )
    {
        NDRX_LOG(log_debug, "Same node -> broadcast event of drop");
        
        /* Broadcast NULL buffer event (ignore result) */
        if (EXSUCCEED!=ndrx_cache_broadcast(NULL, cachedbnm, NULL, 0, 
                NDRX_CACHE_BCAST_MODE_KIL,  NDRX_TPCACHE_BCAST_DFLT, 0, 0, 0, 0))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to broadcast: %s", 
                    __func__, tpstrerror(tperrno));
        }
    }
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(db, txn))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to commit: %s", 
                    __func__, tpstrerror(tperrno));
                ndrx_cache_edb_abort(db, txn);
            }
        }
        else
        {
            ndrx_cache_edb_abort(db, txn);
        }
    }
    
    return ret;
}

/**
 * Invalidate cache by expression
 * Key group is not affected by this as we do not have reference to keygroup or
 * recover group key from keyitem and perform delete accordingly?
 * @param cachedbnm
 * @param keyexpr
 * @cmds binary commands, here we are interested either regexp kill or 
 * plain single key delete
 * @nodeid nodeid posting the record, if it is ours then broadcast event, 
 * if ours then broadcast (if required)
 * @return 0>= - number of records deleted /EXFAIL (tperror set)
 */
expublic long ndrx_cache_inval_by_expr(char *cachedbnm, char *keyexpr, short nodeid)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    ndrx_tpcache_db_t* db;
    int tran_started = EXFALSE;
    regex_t re;
    int re_compiled = EXFALSE;
    EDB_cursor *cursor;
    EDB_cursor_op op;
    EDB_val keydb, val;
    int deleted = 0;
    UBFH *p_ub = NULL;
    ndrx_tpcallcache_t* cache;
    ndrx_tpcache_data_t *exdata;
    
    NDRX_LOG(log_info, "delete cachedb [%s] by expression [%s] from node %d", 
            cachedbnm, keyexpr, nodeid);
    
    /* try compile regexp */
    if (EXSUCCEED!=ndrx_regcomp(&re, keyexpr))
    {
        NDRX_CACHE_TPERRORNOU(TPEINVAL, "Failed to compile regexp [%s]", keyexpr);
        EXFAIL_OUT(ret);
    }
    re_compiled = EXTRUE;
    /* find cachedb */
    
    if (NULL==(db=ndrx_cache_dbresolve(cachedbnm, NDRX_TPCACH_INIT_NORMAL)))
    {
        NDRX_CACHE_TPERRORNOU(TPENOENT, "Failed to get cache record for [%s]: %s", 
                cachedbnm, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: failed to start tran", __func__);
        goto out;
    }
    
    tran_started = EXTRUE;
    
    
    /* loop over the database */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor");
        EXFAIL_OUT(ret);
    }
    
    /* loop over the db and match records  */
    
    op = EDB_FIRST;
    do
    {
        if (EXSUCCEED!=(ret = ndrx_cache_edb_cursor_getfullkey(db, cursor, 
                &keydb, &val, op)))
        {
            if (EDB_NOTFOUND==ret)
            {
                /* this is ok */
                ret = EXSUCCEED;
                break;
            }
            else
            {
                NDRX_LOG(log_error, "Failed to loop over the [%s] db", cachedbnm);
                break;
            }
        }
        
        /* test is last symbols EOS of data, if not this might cause core dump! */
        NDRX_CACHE_CHECK_DBKEY((&keydb), TPESYSTEM);
        exdata = (ndrx_tpcache_data_t *)val.mv_data;
        NDRX_CACHE_CHECK_DBDATA((&val), exdata, keydb.mv_data, TPESYSTEM);

        /* match regex on key */
        
        if (EXSUCCEED==ndrx_regexec(&re, keydb.mv_data))
        {
            char keygrp[NDRX_CACHE_KEY_MAX+1] = {EXEOS};
            
            NDRX_LOG(log_debug, "Key [%s] matched - deleting", keydb.mv_data);
            
            
            /* lookup cache and if in keygroup, then we shall recover the
             * data (prepare incoming) and send the record for 
             * ndrx_cache_keygrp_addupd(). This will ensure that we can process
             * expiry records correctly and clean up the group accordingly.
             */
            
            if (exdata->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
            {
                if (NULL==(cache = ndrx_cache_findtpcall_byidx(exdata->svcnm, 
                    exdata->cache_idx)))
                {
                    NDRX_LOG(log_warn, "Failed to find tpcall cache - no group update!");
                }
                else if (!(cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS))
                {
                    NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Cache record with key indicated "
                            "as part of keygroup, but pointed cache not! "
                            "Have you changed cache config with saved data?");
                    EXFAIL_OUT(ret);
                }
                else
                {
                    NDRX_LOG(log_debug, "Key is part of key-items -> recovering "
                            "buffer and building the key");
                    
                    if (EXSUCCEED!=(ret = ndrx_cache_keygrp_getkey_from_data(cache, 
                        exdata, keygrp, sizeof(keygrp))))
                    {
                        NDRX_LOG(log_error, "Failed to get keygroup key!");
                        goto out;
                    }
                }
            }
           
            if (db->flags & NDRX_TPCACHE_FLAGS_KEYGRP)
            {
               if (EXSUCCEED!=(ret=ndrx_cache_keygrp_inval_by_key(db, keydb.mv_data, 
                       txn, NULL)))
               {
                   NDRX_LOG(log_error, "Failed to remove group record!");
                   EXFAIL_OUT(ret);
               }
            }
            if (EXSUCCEED!=ndrx_cache_edb_delfullkey (db, txn, &keydb, NULL))
            {
                NDRX_LOG(log_debug, "Failed to delete record by key [%s]", 
                        keydb.mv_data);
                EXFAIL_OUT(ret);
            }
            
            /* OK now update group  */
            if (EXEOS!=keygrp[0])
            {
                NDRX_LOG(log_debug, "Removing keyitem [%s] from keygroup [%s]",
                        keyexpr, keygrp);
                
                /* remove just key item... and continue */
                if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache, 
                        NULL, 0, keydb.mv_data, keygrp, EXTRUE)))
                {
                    NDRX_LOG(log_error, "Failed to remove key [%s] from keygroup!");
                    goto out;
                }
            } 
            
            deleted++;
        }
        else
        {
            NDRX_LOG(log_debug, "Key [%s] matched not matched [%s]", 
                    keydb.mv_data, keyexpr);
        }
        
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }
        
    } while (EXSUCCEED==ret);
    

    if ( (db->flags & NDRX_TPCACHE_FLAGS_BCASTPUT) &&
            tpgetnodeid()==nodeid )
    {
        char cmd;
        NDRX_LOG(log_debug, "Same node -> broadcast event of drop");
        
        if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
        {
            NDRX_LOG(log_error, "Failed to allocate UBF buffer!");
            EXFAIL_OUT(ret);
        }
        
        /* Set command code (optional, actual command is encoded in event) */
        cmd = NDRX_CACHE_SVCMD_DELBYEXPR;
        if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_CMD, 0, &cmd, 0L))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set command code of "
                    "[%c] to UBF: %s", __func__, cmd, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* Set expression string, mandatory */
        if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_OPEXPR, 0, keyexpr, 0L))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set operation expression "
                    "[%s] to UBF: %s", __func__, keyexpr, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        
        if (EXSUCCEED!=ndrx_cache_broadcast(NULL, cachedbnm, (char *)p_ub, 0, 
                NDRX_CACHE_BCAST_MODE_MSK, NDRX_TPCACHE_BCAST_DFLT,
                0, 0, 0, 0))
        {
            NDRX_LOG(log_error, "Failed to post event of [%s] expression delete",
                    keyexpr);
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(db, txn))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to commit: %s", 
                    __func__, tpstrerror(tperrno));
                ndrx_cache_edb_abort(db, txn);
            }
        }
        else
        {
            ndrx_cache_edb_abort(db, txn);
        }
    }

    if (re_compiled)
    {
        ndrx_regfree(&re);
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    NDRX_LOG(log_debug, "%s returns %d (deleted: %d)", __func__, ret, deleted);
    
    if (EXSUCCEED==ret)
    {
        return deleted;
    }
    else
    {
        return ret;
    }
}

/**
 * Invalidate by key (delete record too)
 * Also this one is not accessible for keygroup as no data reference for
 * building the key. Or we need to recover saved cache data and build key
 * from there?
 * @param cachedbnm
 * @param db_resolved do not lookup the hash, if already have db handler.
 * @param key note key must be copy to normal memory (not from db) as might cause
 * issues with ptr shifting of mdb record.
 * @param nodeid nodeid posting the record, if it is ours then broadcast event, 
 * if ours then broadcast (if required)
 * @param txn any transaction open (if not open process will open one and commit)
 * @return 0..1 - nr of recs deleted/EXFAIL (tperror set)
 */
expublic int ndrx_cache_inval_by_key(char *cachedbnm, ndrx_tpcache_db_t* db_resolved, 
        char *key, short nodeid, EDB_txn *txn)
{
    int ret = EXSUCCEED;
    ndrx_tpcache_db_t* db;
    int tran_started = EXFALSE;
    UBFH *p_ub = NULL;
    int deleted = 0;
    char cmd;
    char keygrp[NDRX_CACHE_KEY_MAX+1] = {EXEOS};
    
    EDB_val keydb, val;
    ndrx_tpcallcache_t* cache;
    ndrx_tpcache_data_t *exdata;
    
    NDRX_LOG(log_info, "%s: Delete cache db [%s] record by key [%s] source node: [%hd]", 
            __func__, cachedbnm, key, nodeid);
    
    /* find cachedb */
    if (NULL!=db_resolved)
    {
        db = db_resolved;
    }
    else
    {
        if (NULL==(db=ndrx_cache_dbresolve(cachedbnm, NDRX_TPCACH_INIT_NORMAL)))
        {
            NDRX_CACHE_TPERRORNOU(TPENOENT, "Failed to get cache record for [%s]: %s", 
                    cachedbnm, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    /* start transaction */
    if (NULL==txn)
    {
        if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: failed to start tran", __func__);
            goto out;
        }
        tran_started = EXTRUE;
    }
    
    /* we do not know anything about they record here, is it part of cache or
     * not?
     * So we need to perform lookup... check flags and update keygroup if
     * needed
     */
    
    if (EXSUCCEED==(ret=ndrx_cache_edb_get(db, txn, key, &val, EXFALSE)))
    {
        /* validate db rec... */
        exdata = (ndrx_tpcache_data_t *)val.mv_data;
        NDRX_CACHE_CHECK_DBDATA((&val), exdata, keydb.mv_data, TPESYSTEM);
        
        
        /* get key group key */
        if (exdata->flags & NDRX_TPCACHE_TPCF_KEYITEMS)
        {
            NDRX_LOG(log_debug, "record is key item of group");
            if (NULL==(cache = ndrx_cache_findtpcall_byidx(exdata->svcnm, 
                exdata->cache_idx)))
            {
                NDRX_LOG(log_warn, "Failed to find tpcall cache - no group update!");
            }
            else if (!(cache->flags & NDRX_TPCACHE_TPCF_KEYITEMS))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Cache record with key indicated "
                        "as part of keygroup, but pointed cache not! "
                        "Have you changed cache config with saved data?");
                EXFAIL_OUT(ret);
            }
            else
            {
                NDRX_LOG(log_debug, "Key is part of key-items -> recovering "
                        "buffer and building the key");

                if (EXSUCCEED!=(ret = ndrx_cache_keygrp_getkey_from_data(cache, 
                    exdata, keygrp, sizeof(keygrp))))
                {
                    NDRX_LOG(log_error, "Failed to get keygroup key!");
                    goto out;
                }
            }
        }
        
        /* ok we might be a group record, this we need to remove child items... */
        
        if (db->flags & NDRX_TPCACHE_FLAGS_KEYGRP)
        {
            NDRX_LOG(log_debug, "Removing key group");
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_inval_by_key(db, key, txn, NULL)))
            {
                NDRX_LOG(log_error, "Failed to remove group record!");
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Removing rec by key [%s]", key);
            if (EXSUCCEED!=(ret=ndrx_cache_edb_del (db, txn, key, NULL)))
            {
                if (ret!=EDB_NOTFOUND)
                {
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                 deleted = 1;
            }
        }
        
        /* OK now update group  */
        if (EXEOS!=keygrp[0])
        {
            NDRX_LOG(log_debug, "Removing keyitem [%s] from keygroup [%s]",
                    key, keygrp);

            /* remove just key item... and continue */
            if (EXSUCCEED!=(ret=ndrx_cache_keygrp_addupd(cache, 
                    NULL, 0, key, keygrp, EXTRUE)))
            {
                NDRX_LOG(log_error, "Failed to remove key [%s] from keygroup!");
                goto out;
            }
        }

        
    }
    else if (ret!=EDB_NOTFOUND)
    {
        NDRX_LOG(log_error, "Failed to get DB record!");
        EXFAIL_OUT(ret);
    }
    
    /* continue anyway, we need a broadcast */
    
    if ( (db->flags & NDRX_TPCACHE_FLAGS_BCASTPUT) &&
            tpgetnodeid()==nodeid )
    {
        NDRX_LOG(log_debug, "Same node -> broadcast event of delete key");
        
        if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
        {
            NDRX_LOG(log_error, "Failed to allocate UBF buffer!");
            EXFAIL_OUT(ret);
        }
        
        /* Set command code (optional, actual command is encoded in event) */
        cmd = NDRX_CACHE_SVCMD_DELBYKEY;
        if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_CMD, 0, &cmd, 0L))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set command code of "
                    "[%c] to UBF: %s", __func__, cmd, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* Set expression string, mandatory */
        if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_OPEXPR, 0, key, 0L))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set operation expression "
                    "[%s] to UBF: %s", __func__, key, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* Broadcast NULL buffer event (ignore result) */
        if (EXSUCCEED!=ndrx_cache_broadcast(NULL, cachedbnm, (char *)p_ub, 0, 
                NDRX_CACHE_BCAST_MODE_DKY,  NDRX_TPCACHE_BCAST_DFLT, 0, 0, 0, 0))
        {
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to broadcast: %s", 
                    __func__, tpstrerror(tperrno));
        }
    }
    
out:

    if (tran_started)
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=ndrx_cache_edb_commit(db, txn))
            {
                NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to commit: %s", 
                    __func__, tpstrerror(tperrno));
                ndrx_cache_edb_abort(db, txn);
            }
        }
        else
        {
            ndrx_cache_edb_abort(db, txn);
        }
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    
    return ret;
}

/**
 * Broadcast delete by key. No local deletes.
 * @param cachedbnm cache database name
 * @param key key to delete
 * @param nodeid cluster node id
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cache_inval_by_key_bcastonly(char *cachedbnm, char *key, short nodeid)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    char cmd;

    NDRX_LOG(log_debug, "Same node -> broadcast event of delete key");

    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBF buffer!");
        EXFAIL_OUT(ret);
    }

    /* Set command code (optional, actual command is encoded in event) */
    cmd = NDRX_CACHE_SVCMD_DELBYKEY;
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_CMD, 0, &cmd, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set command code of "
                "[%c] to UBF: %s", __func__, cmd, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    /* Set expression string, mandatory */
    if (EXSUCCEED!=Bchg(p_ub, EX_CACHE_OPEXPR, 0, key, 0L))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to set operation expression "
                "[%s] to UBF: %s", __func__, key, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    /* Broadcast NULL buffer event (ignore result) */
    if (EXSUCCEED!=ndrx_cache_broadcast(NULL, cachedbnm, (char *)p_ub, 0, 
            NDRX_CACHE_BCAST_MODE_DKY,  NDRX_TPCACHE_BCAST_DFLT, 0, 0, 0, 0))
    {
        NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Failed to broadcast: %s", 
                __func__, tpstrerror(tperrno));
    }
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}


