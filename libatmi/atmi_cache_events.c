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
    
    if (EXSUCCEED!=ndrx_tppost(event, idata, ilen, TPNOTRAN|TPNOREPLY, user1, user2,
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
                NDRX_LOG(log_error, "%s: failed to add string to list [%s]", 
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
 * TODO: add not check for broadcast
 * Drop cache by name
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
        
        if (EXEOS!=((char *)keydb.mv_data)[keydb.mv_size-1])
        {
            NDRX_DUMP(log_error, "Invalid cache key", 
                    keydb.mv_data, keydb.mv_size);
            
            NDRX_CACHE_TPERROR(TPESYSTEM, "%s: Invalid cache key, len: %ld not "
                    "terminated with EOS!", __func__, keydb.mv_size);
            EXFAIL_OUT(ret);
        }
        
        /* match regex on key */
        
        if (EXSUCCEED==ndrx_regexec(&re, keydb.mv_data))
        {
            NDRX_LOG(log_debug, "Key [%s] matched - deleting", keydb.mv_data);

            if (EXSUCCEED!=ndrx_cache_edb_delfullkey (db, txn, &keydb, NULL))
            {
                NDRX_LOG(log_debug, "Failed to delete record by key [%s]", 
                        keydb.mv_data);
                EXFAIL_OUT(ret);
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
 * @param cachedbnm
 * @param keyexpr
 * @param cmds binary commands, here we are interested either regexp kill or 
 * plain single key delete
 * @param nodeid nodeid posting the record, if it is ours then broadcast event, 
 * if ours then broadcast (if required)
 * @return 0..1 - nr of recs deleted/EXFAIL (tperror set)
 */
expublic int ndrx_cache_inval_by_key(char *cachedbnm, char *key, short nodeid)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    ndrx_tpcache_db_t* db;
    int tran_started = EXFALSE;
    UBFH *p_ub = NULL;
    int deleted = 0;
    char cmd;
        
    NDRX_LOG(log_info, "%s: Delete cache db [%s] record by key [%s] source node: [%hd]", 
            __func__, cachedbnm, key, nodeid);
    
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

