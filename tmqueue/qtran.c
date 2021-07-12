/**
 * @brief keep the track of tmqueue transactions
 *  Needs transaction id, status (active, (preparing?), aborting, prepared)
 *  Needs counter for max file name, someting like request next...?
 *  Needs time setting + timeout, for automatic rollback.
 *
 * @file qtran.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <stdarg.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <nstdutil.h>

#include "userlog.h"
#include <xa_cmn.h>
#include <exhash.h>
#include <unistd.h>
#include <Exfields.h>
#include "qtran.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CHK_THREAD_ACCESS if (ndrx_gettid()!=p_tl->lockthreadid)\
    {\
        NDRX_LOG(log_error, "Transaction [%s] not locked for thread %" PRIu64 ", but for %" PRIu64,\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        userlog("Transaction [%s] not locked for thread %" PRIu64 ", but for %" PRIu64,\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        return EXFAIL;\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate qtran_log_t *M_tx_hash = NULL; 
exprivate MUTEX_LOCKDECL(M_tx_hash_lock); /* Operations with hash list            */

/* TODO: have some timeout setting...? */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Unlock transaction
 * @param p_tl
 * @return SUCCEED/FAIL
 */
expublic int tmq_unlock_entry(qtran_log_t *p_tl)
{
    CHK_THREAD_ACCESS;
    
    NDRX_LOG(log_info, "Transaction [%s] unlocked by thread %" PRIu64, p_tl->tmxid,
            p_tl->lockthreadid);
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    p_tl->lockthreadid = 0;
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    return EXSUCCEED;
}
/**
 * Get the log entry of the transaction
 * Now we should lock it for thread.
 * TODO: Add option for wait on lock.
 * This would be needed for cases for example when Postgres may report
 * the status in background, and for some reason TMSRV already processes the
 * transaction (ether abort, or new register branch, etc...) and some stalled
 * PG process wants to finish the work off. Thus we need to waited lock for
 * foreground operations.
 * @param tmxid - serialized XID
 * @param[in] dowait milliseconds to wait for lock, before give up
 * @param[out] locke lock error
 * @return NULL or log entry
 */
expublic qtran_log_t * tmq_log_get_entry(char *tmxid, int dowait, int *locke)
{
    qtran_log_t *r = NULL;
    ndrx_stopwatch_t w;
    
    if (dowait)
    {
        ndrx_stopwatch_reset(&w);
    }
    
    if (NULL!=locke)
    {
        *locke=EXFALSE;
    }
    
restart:
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_FIND_STR( M_tx_hash, tmxid, r);
    
    if (NULL!=r)
    {
        if (r->lockthreadid)
        {
            if (dowait && ndrx_stopwatch_get_delta(&w) < dowait)
            {
                MUTEX_UNLOCK_V(M_tx_hash_lock);
                /* sleep 100 msec */
                usleep(100000);
                goto restart;
                
            }
            
            NDRX_LOG(log_error, "Q Transaction [%s] already locked for thread_id: %"
                    PRIu64 " lock time: %d msec",
                    tmxid, r->lockthreadid, dowait);
            
            userlog("tmqueue: Transaction [%s] already locked for thread_id: %" PRIu64
                    "lock time: %d msec",
                    tmxid, r->lockthreadid, dowait);
            r = NULL;
            
            /* cannot get lock */
            if (NULL!=locke)
            {
                *locke=EXTRUE;
            }
            
        }
        else
        {
            r->lockthreadid = ndrx_gettid();
            NDRX_LOG(log_info, "Transaction [%s] locked for thread_id: %" PRIu64,
                    tmxid, r->lockthreadid);
        }
    }
    
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    return r;
}

/**
 * Start the log entry.
 * Log shall be started from xa_start() only when not perform join.
 * If performing join, and if we are tmqueue, then check the transaction log
 * for entry.
 * @return SUCCEED/FAIL
 */
expublic int tmq_log_start(char *tmxid)
{
    int ret = EXSUCCEED;
    int hash_added = EXFALSE;
    qtran_log_t *tmp = NULL;
    
    /* 1. Add stuff to hash list */
    if (NULL==(tmp = NDRX_CALLOC(sizeof(qtran_log_t), 1)))
    {
        NDRX_LOG(log_error, "NDRX_CALLOC() failed: %s", strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    tmp->txstage = XA_TX_STAGE_ACTIVE;
    tmp->t_start = ndrx_utc_tstamp();
    tmp->t_update = ndrx_utc_tstamp();
    ndrx_stopwatch_reset(&tmp->ttimer);
    
    /* lock for us, yet it is not shared*/
    tmp->lockthreadid = ndrx_gettid();
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_ADD_STR( M_tx_hash, tmxid, tmp);
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    hash_added = EXTRUE;
    
out:
    
    /* unlock */
    if (EXSUCCEED==ret && NULL!=tmp)
    {
        tmq_unlock_entry(tmp);
    }

    return ret;
}

/**
 * Add command to the log 
 * @param tmxid transaction id (serialized)
 * @param seqno command sequence number
 * @param command_code command code
 * @param status using XA_RM_STATUS* constants
 * @return EXSUCCEED/EXFAIL
 */
expublic int tmq_log_addcmd(char *tmxid, int seqno, char command_code, char status,
        tmq_msg_t * p_msg)
{
    int ret = EXSUCCEED;
    qtran_log_t *p_tl= NULL;
    qtran_log_cmd_t *cmd=NULL;
    
    NDRX_LOG(log_info, "Adding Q tran cmd: [%s] seqno: %d, "
            "command_code: %c, status: %c, p_msg: %p",
            tmxid, seqno, command_code, status, p_msg);
    
    if (NULL==(p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, NULL)))
    {
        NDRX_LOG(log_error, "No Q transaction/lock timeout under xid_str: [%s]", 
                tmxid);
        ret=EXFAIL;
        goto out_nolock;
    }
    
    /* Alloc new command block */
    if (NULL==(cmd = NDRX_FPMALLOC(sizeof(qtran_log_cmd_t), 0)))
    {
        NDRX_LOG(log_error, "Failed to fpmalloc %d bytes: %s", 
                sizeof(qtran_log_cmd_t), strerror(errno));
        userlog("Failed to fpmalloc %d bytes: %s", 
                sizeof(qtran_log_cmd_t), strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    cmd->seqno=seqno;
    cmd->command_code=command_code;
    cmd->status=status;
    cmd->p_msg=p_msg;
    
    DL_APPEND(p_tl->cmds, cmd);

   
out:
    /* unlock transaction from thread */
    tmq_unlock_entry(p_tl);

out_nolock:
    
    return ret;
}


/**
 * Free up log file (just memory)
 * @param p_tl log handle
 * @param hash_rm remove log entry from tran hash
 */
expublic void tmq_remove_logfree(qtran_log_t *p_tl, int hash_rm)
{
    
    if (hash_rm)
    {
        MUTEX_LOCK_V(M_tx_hash_lock);
        EXHASH_DEL(M_tx_hash, p_tl); 
        MUTEX_UNLOCK_V(M_tx_hash_lock);
    }
    
    NDRX_FPFREE(p_tl);
}

/**
 * Copy the background items to the linked list.
 * The idea is that this is processed by background. During that time, it does not
 * remove any items from hash. Thus pointers should be still valid. 
 * TODO: We should copy here transaction info too....
 * 
 * @param p_tl
 * @return 
 */
expublic qtran_log_list_t* tmq_copy_hash2list(int copy_mode)
{
    qtran_log_list_t *ret = NULL;
    qtran_log_t * r, *rt;
    qtran_log_list_t *tmp;
    
    if (copy_mode & COPY_MODE_ACQLOCK)
    {
        MUTEX_LOCK_V(M_tx_hash_lock);
    }
    
    /* No changes to hash list during the lock. */    
    
    EXHASH_ITER(hh, M_tx_hash, r, rt)
    {
        /* Only background items... */
        if (r->is_background && !(copy_mode & COPY_MODE_BACKGROUND))
            continue;
        
        if (!r->is_background && !(copy_mode & COPY_MODE_FOREGROUND))
            continue;
                
        if (NULL==(tmp = NDRX_FPMALLOC(sizeof(qtran_log_list_t), 0)))
        {
            NDRX_LOG(log_error, "Failed to fpmalloc %d: %s", 
                    sizeof(qtran_log_list_t), strerror(errno));
            goto out;
        }
        
        /* we should copy full TL structure, because some other thread might
         * will use it.
         * Having some invalid pointers inside does not worry us, because
         * we just need a list for a printing or xids for background txn lookup
         */
        memcpy(&tmp->p_tl, r, sizeof(*r));
        
        LL_APPEND(ret, tmp);
    }
    
out:
    if (copy_mode & COPY_MODE_ACQLOCK)
    {
        MUTEX_UNLOCK_V(M_tx_hash_lock);
    }

    return ret;
}

/**
 * Lock the transaction log hash
 */
expublic void tmq_tx_hash_lock(void)
{
    MUTEX_LOCK_V(M_tx_hash_lock);
}

/**
 * Unlock the transaction log hash
 */
expublic void tmq_tx_hash_unlock(void)
{
    MUTEX_UNLOCK_V(M_tx_hash_lock);
}

/* vim: set ts=4 sw=4 et smartindent: */
