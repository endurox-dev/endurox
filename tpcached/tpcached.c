/* 
** Cache sanity daemon - this will remove expired records from db
** We shall move all scanning to RO mode. Build the list for removal
** and process the writes in separate run (if requied). Also duplicate processing
** shall be left only for scandup flag - this will simplify the code.
**
** @file tpcached.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#include <atmi.h>
#include <atmi_int.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <atmi_cache.h>
#include "tpcached.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


exprivate int M_sleep = 5;          /* perform actions at every X seconds */

/* for this mask, sigint, sigterm, check sigs periodically */
exprivate int M_shutdown = EXFALSE;  /* do we have shutdown requested?    */

exprivate sigset_t  M_mask;

/*---------------------------Prototypes---------------------------------*/

/**
 * Perform init (read -i command line argument - interval)
 * @return 
 */
expublic int init(int argc, char** argv)
{
    int ret = EXSUCCEED;
    signed char c;
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'i': 
                M_sleep = atoi(optarg);
                break;
            default:
                /*return FAIL;*/
                break;
        }
    }

    NDRX_LOG(log_debug, "Periodic sleep: %d secs", M_sleep);
    
    
    /* mask signal */
    sigemptyset(&M_mask);
    sigaddset(&M_mask, SIGINT);
    sigaddset(&M_mask, SIGTERM);

    if (EXSUCCEED!=sigprocmask(SIG_SETMASK, &M_mask, NULL))
    {
        NDRX_LOG(log_error, "Failed to set SIG_SETMASK: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Process database by record expiry
 * loop over the db and remove expired records.
 * This also removes records for which service does not exists (if marked so
 * by flags)
 * So we need to schedule records here for removal....
 * @param db cache database 
 * @return EXSUCCED/EXFAIL
 */
exprivate int proc_db_expiry_nosvc(ndrx_tpcache_db_t *db)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    int cursor_open = EXFALSE;
    EDB_cursor *cursor;
    EDB_cursor_op op;
    EDB_val keydb, val;
    long t;
    long tusec;
    long deleted = 0;
    int tmp_is_bridge;
    char send_q[NDRX_MAX_Q_SIZE+1];
    char prev_key[NDRX_CACHE_KEY_MAX+1] = {EXEOS};
    char cur_key[NDRX_CACHE_KEY_MAX+1] = {EXEOS};
    
    ndrx_tpcached_msglist_t * exp_list = NULL;
            
            
    ndrx_tpcache_data_t *pdata;
    
    NDRX_LOG(log_debug, "%s enter dbname=[%s]", __func__, db->cachedb);
    
    /* start transaction */
    if (EXSUCCEED!=(ret=ndrx_cache_edb_begin(db, &txn, 0)))
    {
        NDRX_LOG(log_error, "%s: failed to start tran: %s", __func__, 
                tpstrerror(tperrno));
        goto out;
    }
    
    tran_started = EXTRUE;
    
    
    /* loop over the database */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor");
        EXFAIL_OUT(ret);
    }
    cursor_open = EXTRUE;
    ndrx_utc_tstamp2(&t, &tusec);
    
    /* loop over the db and match records  */
    
    /* Do not process duplicate record expiries, only new rec only... 
     * as killing new rec, old record will be killed too
     */
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
                NDRX_LOG(log_error, "Failed to loop over the [%s] db", db->cachedb);
                break;
            }
        }
        
        /* test is last symbols EOS of data, if not this might cause core dump! */
        
        NDRX_CACHE_CHECK_DBKEY((&keydb), TPMINVAL);
        
        pdata = (ndrx_tpcache_data_t *)val.mv_data;
        
        NDRX_CACHE_CHECK_DBDATA((&val), pdata, keydb.mv_data, TPMINVAL);
        
        /* we have timestamp for putting record in DB, but we need to calculate
         * the difference between 
         */
        
        NDRX_LOG(log_info, "TESTING: Record with key [%s]: current UTC: %ld, "
                    "record expiry %ld sec. Record expiry UTC: %ld (delta: %ld)", 
                    keydb.mv_data, t,  db->expiry, pdata->t + db->expiry,
                    (long)(t - pdata->t));
        
        if (EXEOS!=prev_key[0] && 0==strcmp(keydb.mv_data, prev_key))
        {
            NDRX_LOG(log_info, "Duplicate key - skip...");
            goto next;
        }
        else 
        {
            NDRX_STRCPY_SAFE(prev_key, keydb.mv_data);
        }
        
        /* so either record is expired or service does not exists */
        if (   ((db->flags & NDRX_TPCACHE_FLAGS_EXPIRY) && (pdata->t + db->expiry < t))
                ||
                ((db->flags & NDRX_TPCACHE_FLAGS_CLRNOSVC) && 
                    EXSUCCEED!=ndrx_shm_get_svc(pdata->svcnm, send_q, &tmp_is_bridge, NULL))
            )
        {
            NDRX_LOG(log_info, "Record with key [%s] expired: current UTC: %ld, "
                    "record expiry %ld sec. Record expiry UTC: %ld", 
                    keydb.mv_data, t,  db->expiry, pdata->t + db->expiry);
            
            /* copy is needed because key data might change during group delete */
            NDRX_STRCPY_SAFE(cur_key, keydb.mv_data);

            if (EXSUCCEED!=ndrx_tpcached_add_msg(&exp_list, &keydb, NULL))
            {
                NDRX_LOG(log_debug, "Failed to add record to removal list!");
                EXFAIL_OUT(ret);
            }
            
        }
next:
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }
        
    } while (EXSUCCEED==ret);
    
    /* RO tran abort, no need to commit! */
    cursor_open = EXFALSE;
    edb_cursor_close(cursor);
    ndrx_cache_edb_abort(db, txn);
    tran_started=EXFALSE;
    
    if (NULL!=exp_list)
    {
        if (0 > (deleted = ndrx_tpcached_kill_list(db, &exp_list)))
        {
            NDRX_LOG(log_debug, "Failed to remove expired records!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_info, "Deleted %ld records", deleted);
    }
    else
    {
        NDRX_LOG(log_debug, "No records expired");
    }

out:
     
    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    if (tran_started)
    {
        ndrx_cache_edb_abort(db, txn);
    }

    /* free the list in case of failure */
    if (NULL!=exp_list)
    {
        ndrx_tpcached_free_list(&exp_list);
    }

    return ret;
}

/**
 * Compare in reverse 
 * @param a
 * @param b
 * @return 
 */
exprivate int cmpfunc_lru (const void * a, const void * b)
{
    
    ndrx_tpcache_datasort_t **ad = (ndrx_tpcache_datasort_t **)a;
    ndrx_tpcache_datasort_t **bd = (ndrx_tpcache_datasort_t **)b;
    
    /* if records are empty (not filled, malloc'd then numbers will be higher anyway */
    
    /* to get newer rec first, we change the compare order */
    
    return ndrx_utc_cmp(&(*bd)->data.hit_t, &(*bd)->data.hit_tusec, 
            &(*ad)->data.hit_t, &(*ad)->data.hit_tusec);
    
}
/**
 * Sort by hits
 * @param a
 * @param b
 * @return 
 */
exprivate int cmpfunc_hits (const void * a, const void * b) 
{
    ndrx_tpcache_datasort_t **ad = (ndrx_tpcache_datasort_t **)a;
    ndrx_tpcache_datasort_t **bd = (ndrx_tpcache_datasort_t **)b;

    long res = (*bd)->data.hits - (*ad)->data.hits;
 
    /* do some custom work because of int */
    if (res < 0)
    {
        return -1;
    }
    else if (res > 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * Fifo order by date
 * @param a
 * @param b
 * @return 
 */
exprivate int cmpfunc_fifo (const void * a, const void * b) 
{
    
    ndrx_tpcache_datasort_t **ad = (ndrx_tpcache_datasort_t **)a;
    ndrx_tpcache_datasort_t **bd = (ndrx_tpcache_datasort_t **)b;
    
    /* if records are empty (not filled, malloc'd then numbers will be higher anyway */
    
    /* to get newer rec first, we change the compare order */
    
    return ndrx_utc_cmp(&(*bd)->data.t, &(*bd)->data.tusec, 
            &(*ad)->data.t, &(*ad)->data.tusec);
}

/**
 * Process single db - by limit rule    
 * @param db
 * @return EXSUCCEED/EXFAIL
 */
exprivate int proc_db_limit(ndrx_tpcache_db_t *db)
{
    int ret = EXSUCCEED;
    EDB_stat stat;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    int cursor_open = EXFALSE;
    ndrx_tpcache_datasort_t **dsort = NULL;
    EDB_cursor_op op;
    EDB_val keydb, val;
    EDB_cursor *cursor;
    ndrx_tpcache_data_t *pdata;
    long i;
    long nodeid = tpgetnodeid();
    long deleted=0, dupsdel=0;
    ndrx_tpcached_msglist_t * dup_list = NULL;
    
    NDRX_LOG(log_debug, "%s enter dbname=[%s]", __func__, db->cachedb);
    /* Get size of db */
    
    /* start transaction */
    if (EXSUCCEED!=ndrx_cache_edb_begin(db, &txn, EDB_RDONLY))
    {
        NDRX_LOG(log_error, "Failed start transaction: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    tran_started = EXTRUE;
    
    
    if (EXSUCCEED!=ndrx_cache_edb_stat (db, txn, &stat))
    {
        NDRX_LOG(log_error, "Failed to get db statistics: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "number of keys in db: %ld, limit: %d", 
            stat.ms_entries, db->limit);
    
    if (stat.ms_entries <= db->limit)
    {
        NDRX_LOG(6, "Under the limit -> no need to delete recs..");
        goto out;
    }
    
    /* allocate ptr array of number elements in db */
    
    /* open cursor firstly... */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor!");
        EXFAIL_OUT(ret);
    }
    cursor_open = EXTRUE;
    /* I guess after cursor open entries shall not grow? */
    NDRX_CALLOC_OUT(dsort, stat.ms_entries, sizeof(ndrx_tpcache_datasort_t*), 
            void);
    
    for (i=0; i<stat.ms_entries; i++)
    {
        NDRX_CALLOC_OUT(dsort[i], 1, sizeof(ndrx_tpcache_datasort_t), ndrx_tpcache_datasort_t);
        
        NDRX_LOG(log_error, "Alloc dsort %ld = %p", i, dsort[i]);
    }
    
    
    /* transfer all keys to array (allocate each cell), also got to copy key data/strdup.. */
    op = EDB_FIRST;
    i = 0;
    do
    {
        NDRX_LOG(log_debug, "%s: [%s] db loop %ld, entries: %ld",  
                        __func__, db->cachedb, i, (long) stat.ms_entries);
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
                NDRX_LOG(log_error, "Failed to loop over the [%s] db", db->cachedb);
                break;
            }
        }
        
        /* test is last symbols EOS of data, if not this might cause core dump! */
        
        NDRX_CACHE_CHECK_DBKEY((&keydb), TPMINVAL);
        
        pdata = (ndrx_tpcache_data_t *)val.mv_data;
        
        NDRX_CACHE_CHECK_DBDATA((&val), pdata, keydb.mv_data, TPMINVAL);

        
        /* check isn't duplicate records in DB? 
         * Well we need a test case here... to see how lmdb will act in this
         * case will it return only first sorted? Or return all keys in random
         * order?
         * LMDB sorts ok. no problem where. As first comes the highest order
         * entry (higher timestamp), thus second we remove...
         */
        if (i>0 && 0!=strcmp(dsort[i-1]->key.mv_data, keydb.mv_data) || 0==i)
        {
            /* populate array */
            
            
            NDRX_LOG(log_error, "Adding to qsort... [%ld]", i);
            
            /* just copy header, not full data...  */
            memcpy(&(dsort[i]->data), pdata, sizeof(ndrx_tpcache_data_t));
            /*
                    
            NDRX_DUMP(log_error, "Data", &(dsort[i]->data), sizeof(ndrx_tpcache_data_t));
            
*/
            /* duplicate key data (it is string!) */

            if (NULL==(dsort[i]->key.mv_data = NDRX_STRDUP(keydb.mv_data)))
            {
                NDRX_LOG(log_error, "Failed to strdup: %s", keydb.mv_data);
                EXFAIL_OUT(ret);
            }

            dsort[i]->key.mv_size = strlen(dsort[i]->key.mv_data)+1;
        } 
        else 
        {
            /* this is duplicate record, we will help the system and clean it up */
            NDRX_LOG(log_debug, "Removing duplicate: [%s] (mark for del)", 
                    keydb.mv_data);
            if (EXSUCCEED!=ndrx_tpcached_add_msg(&dup_list, &keydb, &val))
            {
                NDRX_LOG(log_debug, "Failed to add record to removal list!");
                EXFAIL_OUT(ret);
            }
        }
        
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }
        
        i++;
        
        if (i>=stat.ms_entries)
        {
            /* TODO: test case when new records appears while one is performing cursor op... */
            NDRX_LOG(log_debug, "soft EOF");
            break;
        }
        
    } while (EXSUCCEED==ret);
    
    /* RO tran abort... */
    edb_cursor_close(cursor);
    cursor_open = EXFALSE;
    ndrx_cache_edb_abort(db, txn);
    tran_started=EXFALSE;
    
    
    /* sort array to according techniques:
     * lru, hits, fifo (tstamp based) */
    
    if (db->flags & NDRX_TPCACHE_FLAGS_LRU)
    {
        qsort(dsort, stat.ms_entries, sizeof(ndrx_tpcache_datasort_t*),
                  cmpfunc_lru);
    }
    else if (db->flags & NDRX_TPCACHE_FLAGS_HITS)
    {
        qsort(dsort, stat.ms_entries, sizeof(ndrx_tpcache_datasort_t*),
                  cmpfunc_hits);
    }
    else if (db->flags & NDRX_TPCACHE_FLAGS_FIFO)
    {
        qsort(dsort, stat.ms_entries, sizeof(ndrx_tpcache_datasort_t*),
                  cmpfunc_fifo);
    }
    else
    {
        NDRX_LOG(log_error, "Invalid db flags: %ld", db->flags);
        EXFAIL_OUT(ret);
    }    
    
    if (db->limit >= stat.ms_entries)
    {
        NDRX_LOG(log_debug, "Nothing to delete");
        goto out;
    }
    
    /* empty lists are always at the end of the array */
    /* then go over the linear array, and remove records which goes over the cache */
    /* just print the sorted arrays... */
    NDRX_LOG(log_debug, "%s: starting RW tran", __func__);
    
    if (EXSUCCEED!=ndrx_cache_edb_begin(db, &txn, 0))
    {
        NDRX_LOG(log_error, "Failed start transaction: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    tran_started = EXTRUE;
    
    for (i=db->limit; i<stat.ms_entries; i++)
    {
        char *p = dsort[i]->key.mv_data;
        
        
        NDRX_LOG(log_debug, "Cache infos: key [%s], last used: %ld.%ld", 
                dsort[i]->key.mv_data, dsort[i]->data.hit_t, dsort[i]->data.hit_tusec);
        
        
        if (EXEOS!=p[0])
        {
            /* this is ok entry, lets remove it! */        
            
            NDRX_LOG(log_info, "About to delete: key=[%s]", dsort[i]->key.mv_data);
            
            if (EXSUCCEED!=ndrx_cache_inval_by_key(db->cachedb, db, 
                    dsort[i]->key.mv_data, (short)nodeid, txn, EXTRUE))
            {
                NDRX_LOG(log_debug, "Failed to delete record by key [%s]", 
                        dsort[i]->key.mv_data);
                EXFAIL_OUT(ret);
            }
            /* use existing delete func... */
            deleted++;
        }
    }
    
    NDRX_LOG(log_info, "Deleted %ld records, %ld duplicates del", deleted, dupsdel);

out:
    
    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

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

    if (NULL!=dup_list)
    {
        if (EXSUCCEED==ret)
        {
            if (0 > (deleted = ndrx_tpcached_kill_list(db, &dup_list)))
            {
                NDRX_LOG(log_debug, "Failed to remove duplicate records!");
                ret=EXFAIL;
            }
            NDRX_LOG(log_info, "Deleted %ld records", deleted);
        }
    }

    if (NULL!=dup_list)
    {
        ndrx_tpcached_free_list(&dup_list);
    }

    /* kill the list -> free some memory! */
    if (NULL!=dsort)
    {
        for (i=0; i<stat.ms_entries; i++)
        {
            NDRX_FREE(dsort[i]->key.mv_data);
            NDRX_FREE(dsort[i]);
        }
        
        NDRX_FREE(dsort);
        dsort = NULL;
    }

    return ret;
}

/**
 * Scan for duplicates and remove them. This could be useful for services
 * for which there are lot of caching, but less later accessing. Thus have
 * some housekeeping
 * @param db
 * @return EXSUCCEED/EXFAIL
 */
exprivate int proc_db_dups(ndrx_tpcache_db_t *db)
{
    int ret = EXSUCCEED;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    int cursor_open = EXFALSE;
    EDB_cursor_op op;
    EDB_val keydb, val;
    EDB_cursor *cursor = NULL;
    long deleted=0;
    long i;
    ndrx_tpcache_data_t *pdata;
    char *prvkey = NULL;
    ndrx_tpcached_msglist_t * dup_list = NULL;

    NDRX_LOG(log_debug, "%s enter dbname=[%s]", __func__, db->cachedb);
    /* Get size of db */
    
    /* start transaction */
    if (EXSUCCEED!=ndrx_cache_edb_begin(db, &txn, EDB_RDONLY))
    {
        NDRX_LOG(log_error, "Failed start transaction: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    tran_started = EXTRUE;
    
    /* open cursor firstly... */
    if (EXSUCCEED!=ndrx_cache_edb_cursor_open(db, txn, &cursor))
    {
        NDRX_LOG(log_error, "Failed to open cursor!");
        EXFAIL_OUT(ret);
    }
    cursor_open = EXTRUE;
    
    /* transfer all keys to array (allocate each cell), also got to copy key data/strdup.. */
    op = EDB_FIRST;
    i = 0;
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
                NDRX_LOG(log_error, "Failed to loop over the [%s] db", db->cachedb);
                break;
            }
        }
        
        /* test is last symbols EOS of data, if not this might cause core dump! */
        
        NDRX_CACHE_CHECK_DBKEY((&keydb), TPMINVAL);
        
        pdata = (ndrx_tpcache_data_t *)val.mv_data;
        
        NDRX_CACHE_CHECK_DBDATA((&val), pdata, keydb.mv_data, TPMINVAL);
        
        /* store prev key */
        if (i!=0)
        {
            /* this is duplicate record, we will help the system and clean it up */
            
            if (0==strcmp(prvkey, (char *)keydb.mv_data))
            {
                NDRX_LOG(log_debug, "Removing duplicate: [%s] (mark for removal)", 
                        keydb.mv_data);
                if (EXSUCCEED!=ndrx_tpcached_add_msg(&dup_list, &keydb, &val))
                {
                    NDRX_LOG(log_debug, "Failed to add record to removal list!");
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_FREE(prvkey);
            }
        }
        
        if (NULL==(prvkey = NDRX_STRDUP(keydb.mv_data)))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to strdup: %s", strerror(err));
            userlog("Failed to strdup: %s", strerror(err));
            EXFAIL_OUT(ret);
        }
        
        if (EDB_FIRST == op)
        {
            op = EDB_NEXT;
        }
        
        i++;
        
    } while (EXSUCCEED==ret);
    
    /* RD only abort */
    edb_cursor_close(cursor);
    cursor_open = EXFALSE;
    ndrx_cache_edb_abort(db, txn);
    tran_started=EXFALSE;
    
    
    if (NULL!=dup_list)
    {
        if (0 > (deleted = ndrx_tpcached_kill_list(db, &dup_list)))
        {
            NDRX_LOG(log_debug, "Failed to remove duplicate records!");
            EXFAIL_OUT(ret);
        }
        NDRX_LOG(log_info, "Deleted %ld records", deleted);
    }
    else
    {
        NDRX_LOG(log_debug, "No duplicate expired");
    }

out:
                
    if (NULL!=prvkey)
    {
        NDRX_FREE(prvkey);
    }

    if (NULL!=dup_list)
    {
        ndrx_tpcached_free_list(&dup_list);
    }

    if (cursor_open)
    {
        edb_cursor_close(cursor);
    }

    /* rd only */
    if (tran_started)
    {
        ndrx_cache_edb_abort(db, txn);
    }

    return ret;
}

/**
 * Check that we are pending some signals (specially for Apple...)
 * @return EXTRUE/EXFALSE
 */
exprivate int is_shutdown_pending(void)
{
    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, SIGINT) ||
        sigismember(&pending, SIGTERM))
    {
        return EXTRUE;
    }
    
    return EXFALSE;
}

/**
 * Main entry point for `tpcached' utility
 */
expublic int main(int argc, char** argv)
{

    int ret=EXSUCCEED;

    struct timespec timeout;
    siginfo_t info;
    int result = 0;
    int i;
    ndrx_tpcache_db_t *dbh, *el, *elt;

    /* local init */
    
    if (EXSUCCEED!=init(argc, argv))
    {
        NDRX_LOG(log_error, "Failed to init!");
        EXFAIL_OUT(ret);
    }
    
    /* ATMI init */
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "Failed to init: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* 
     * loop over all databases
     * if database is limited (i.e. limit > 0), then do following:
     * - Read keys or (header with out data) into memory (linear mem)
     * and perform corresponding qsort
     * then remove records which we have at tail of the array.
     * - sleep configured time
     */
    
    timeout.tv_sec = M_sleep;
    timeout.tv_nsec = 0;


    while (!M_shutdown)
    {
        /* wait for signal or timeout... */
#if EX_OS_DARWIN
        for (i=0; i<M_sleep; i++)
        {
            if (is_shutdown_pending())
            {
                NDRX_LOG(log_debug, "Shutdown requested by signal...");
                M_shutdown = EXTRUE;
                break;
            }
            else
            {
                sleep(1);
            }
        }
        if (M_shutdown)
        {
            break;
        }
#else
        result = sigtimedwait( &M_mask, &info, &timeout );

        if (result > 0)
        {
            if (SIGINT == result || SIGTERM == result)
            {
                NDRX_LOG(log_warn, "Signal received: %d - shutting down", result);
                M_shutdown = EXTRUE;
                break;
            }
            else
            {
                NDRX_LOG(log_warn, "Signal received: %d - ignore", result);
            }
        }
        else if (EXFAIL==result)
        {
            int err = errno;
            
            if (EAGAIN!=err)
            {
                NDRX_LOG(log_error, "sigtimedwait failed: %s", strerror(err));
                EXFAIL_OUT(ret);
            }
        }
#endif
        NDRX_LOG(log_debug, "Scanning...");
        /* Get the DBs */
        /* interval process */
        dbh = ndrx_cache_dbgethash();

        EXHASH_ITER(hh, dbh, el, elt)
        {
            /* process db */
            if ( (el->flags & NDRX_TPCACHE_FLAGS_EXPIRY)
                    ||
                    (el->flags & NDRX_TPCACHE_FLAGS_CLRNOSVC))
            {
                if (EXSUCCEED!=proc_db_expiry_nosvc(el))
                {
                   NDRX_LOG(log_error, "Failed to process expiry cache: [%s]", 
                           el->cachedb);
                   EXFAIL_OUT(ret);
                }
            }
            
            /* Allow expiry messages to be space limited too */
            if (
                    el->flags & NDRX_TPCACHE_FLAGS_LRU ||
                    el->flags & NDRX_TPCACHE_FLAGS_HITS ||
                    el->flags & NDRX_TPCACHE_FLAGS_FIFO
                 ) 
            {
                if (EXSUCCEED!=proc_db_limit(el))
                {
                   NDRX_LOG(log_error, "Failed to process limit cache: [%s]", 
                           el->cachedb);
                   EXFAIL_OUT(ret);
                }
            }
            
            /* And we might search for duplicates in cluster configuration */
            if (el->flags & NDRX_TPCACHE_FLAGS_SCANDUP)
            {
               NDRX_LOG(log_error, "scanning for duplicates");
               
                if (EXSUCCEED!=proc_db_dups(el))
                {
                   NDRX_LOG(log_error, "Failed to process limit cache: [%s]", 
                           el->cachedb);
                   EXFAIL_OUT(ret);
                }
            }
        }
    }
    
out:
    /* un-initialize */
    tpterm();
    return ret;
}

