/**
 * @brief Process messages from linked list (deletes to avoid RW cursors)
 *
 * @file msglist.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <atmi_int.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>
#include <atmi_cache.h>
#include "tpcached.h"
#include "utlist.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Free up record
 * @param entry linked list record
 */
exprivate void free_rec(ndrx_tpcached_msglist_t * entry)
{
    if (NULL!=entry->keydb.mv_data)
    {
        NDRX_FREE(entry->keydb.mv_data);
    }
    
    if (NULL!=entry->val.mv_data)
    {
        NDRX_FREE(entry->val.mv_data);
    }
    
    NDRX_FREE(entry);
}

/**
 * Add message to linked list
 * @param list list to be created or msg added
 * @param keydb key db (mandatory)
 * @param val value optional (may be NULL)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tpcached_add_msg(ndrx_tpcached_msglist_t ** list,
        EDB_val *keydb, EDB_val *val)
{
    int ret = EXSUCCEED;
    ndrx_tpcached_msglist_t * entry = NULL;
    
    
    /* Allocate MSG list entry */
    
    NDRX_CALLOC_OUT(entry, 1, sizeof(ndrx_tpcached_msglist_t), ndrx_tpcached_msglist_t);
    
    /* fill up the key */
    entry->keydb.mv_size = keydb->mv_size;
    NDRX_MALLOC_OUT(entry->keydb.mv_data, entry->keydb.mv_size, void);
    memcpy(entry->keydb.mv_data, keydb->mv_data, entry->keydb.mv_size);
    
    /* fill up the data, if provided */
    
    if (NULL!=val)
    {
        entry->val.mv_size = val->mv_size;
        NDRX_MALLOC_OUT(entry->val.mv_data, entry->val.mv_size, void);
        memcpy(entry->val.mv_data, val->mv_data, entry->val.mv_size);
        
        entry->is_full = EXTRUE;
    }
    
    LL_APPEND((*list), entry);
    entry = NULL;
    
out:
    if (NULL!=entry)
    {
        free_rec(entry);
    }
    return ret;
}

/**
 * Remove all messages from linked list, we run RW transaction!!! (ALL LOCKS)!
 * @param list list of msg to delete
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tpcached_kill_list(ndrx_tpcache_db_t *db, 
        ndrx_tpcached_msglist_t ** list)
{
    int ret = EXSUCCEED;
    ndrx_tpcached_msglist_t * el, *elt;
    EDB_txn *txn = NULL;
    int tran_started = EXFALSE;
    int deleted = 0;
    short nodeid = (short)tpgetnodeid();
    /* start new rw transaction  */
    
    if (EXSUCCEED!=ndrx_cache_edb_begin(db, &txn, 0))
    {
        NDRX_LOG(log_error, "Failed start transaction: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    tran_started = EXTRUE;
    
    LL_FOREACH_SAFE((*list), el, elt)
    {
        if (el->is_full)
        {
            NDRX_LOG(log_debug, "Delete by key + data (key: [%s])", el->keydb.mv_data);
            
            if (EXSUCCEED!=(ret=ndrx_cache_edb_del (db, txn, el->keydb.mv_data, 
                    &el->val)))
            {
                if (ret!=EDB_NOTFOUND)
                {
                    EXFAIL_OUT(ret);
                }
                else
                {
                    ret=EXSUCCEED;
                }
            }
            
        }
        else
        {
            NDRX_LOG(log_debug, "Delete by key only with bcast(key: [%s])", 
                    el->keydb.mv_data);
            
            if (EXSUCCEED!=ndrx_cache_inval_by_key(db->cachedb, db, 
                    el->keydb.mv_data, nodeid, txn, EXTRUE))
            {
                NDRX_LOG(log_debug, "Failed to delete record by key [%s]", 
                        el->keydb.mv_data);
                EXFAIL_OUT(ret);
            }
        }
        
        LL_DELETE(*list, el);
        free_rec(el);
        deleted++;
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

    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_info, "%s: delete %d rec(s)", __func__, deleted);
        return deleted;
    }
    else
    {
        NDRX_LOG(log_info, "%s: returns %d", __func__, ret);
        return ret;
    }
}

/**
 * Just clean up the list
 * @param list
 */
expublic void ndrx_tpcached_free_list(ndrx_tpcached_msglist_t ** list)
{
    ndrx_tpcached_msglist_t * el, *elt;
    LL_FOREACH_SAFE((*list), el, elt)
    {
        LL_DELETE(*list, el);
        free_rec(el);;
    }
}
