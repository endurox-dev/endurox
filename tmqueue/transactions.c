/**
 * @brief keep the track of tmqueue transactions
 *  Needs transaction id, status (active, (preparing?), aborting, prepared)
 *  Needs counter for max file name, someting like request next...?
 *  Needs time setting + timeout, for automatic rollback.
 *
 * @file transactions.c
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

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include "userlog.h"
#include <xa_cmn.h>
#include <exhash.h>
#include <unistd.h>
#include <Exfields.h>
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
exprivate atmi_xa_log_t *M_tx_hash = NULL; 
exprivate MUTEX_LOCKDECL(M_tx_hash_lock); /* Operations with hash list            */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Unlock transaction
 * @param p_tl
 * @return SUCCEED/FAIL
 */
expublic int tms_unlock_entry(atmi_xa_log_t *p_tl)
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
expublic atmi_xa_log_t * tms_log_get_entry(char *tmxid, int dowait, int *locke)
{
    atmi_xa_log_t *r = NULL;
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
            
            NDRX_LOG(log_error, "Transaction [%s] already locked for thread_id: %"
                    PRIu64 " lock time: %d msec",
                    tmxid, r->lockthreadid, dowait);
            
            userlog("tmsrv: Transaction [%s] already locked for thread_id: %" PRIu64
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
 * Log transaction as started
 * @param xai - XA Info struct
 * @param txtout - transaction timeout
 * @param[out] btid branch transaction id (if not dynamic reg)
 * @return SUCCEED/FAIL
 */
expublic int tms_log_start(atmi_xa_tx_info_t *xai, int txtout, long tmflags,
        long *btid)
{
    int ret = EXSUCCEED;
    int hash_added = EXFALSE;
    atmi_xa_log_t *tmp = NULL;
    atmi_xa_rm_status_btid_t *bt;
    /* 1. Add stuff to hash list */
    if (NULL==(tmp = NDRX_CALLOC(sizeof(atmi_xa_log_t), 1)))
    {
        NDRX_LOG(log_error, "NDRX_CALLOC() failed: %s", strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    tmp->txstage = XA_TX_STAGE_ACTIVE;
    
    tmp->tmnodeid = xai->tmnodeid;
    tmp->tmsrvid = xai->tmsrvid;
    tmp->tmrmid = xai->tmrmid;
    NDRX_STRCPY_SAFE(tmp->tmxid, xai->tmxid);
    
    tmp->t_start = ndrx_utc_tstamp();
    tmp->t_update = ndrx_utc_tstamp();
    tmp->txtout = txtout;
    tmp->log_version = LOG_VERSION_2;   /**< Now with CRC32 groups */
    ndrx_stopwatch_reset(&tmp->ttimer);
    
    /* lock for us, yet it is not shared*/
    tmp->lockthreadid = ndrx_gettid();
    
    /* TODO: Open the log file & write tms_close_logfile
     * Only question, how long 
     */
    
    /* Open log file */
    if (EXSUCCEED!=tms_open_logfile(tmp, "w"))
    {
        NDRX_LOG(log_error, "Failed to create transaction log file");
        userlog("Failed to create transaction log file");
        EXFAIL_OUT(ret);
    }
    
    /* log the opening infos */
    if (EXSUCCEED!=tms_log_info(tmp))
    {
        NDRX_LOG(log_error, "Failed to log tran info");
        userlog("Failed to log tran info");
        EXFAIL_OUT(ret);
    }
    
    /* Only for static... */
    if (!(tmflags & TMFLAGS_DYNAMIC_REG))
    {
        /* assign btid? 
        tmp->rmstatus[xai->tmrmid-1].rmstatus = XA_RM_STATUS_ACTIVE;
        */
        char start_stat = XA_RM_STATUS_ACTIVE;
        
        *btid = tms_btid_gettid(tmp, xai->tmrmid);

        /* Just get the TID, status will be changed with file update  */
        if (EXSUCCEED!=tms_btid_add(tmp, xai->tmrmid, *btid, XA_RM_STATUS_NULL, 
                0, 0, &bt))
        {
            NDRX_LOG(log_error, "Failed to add BTID: %ld", *btid);
            EXFAIL_OUT(ret);
        }
        
        if (tmflags & TMFLAGS_TPNOSTARTXID)
        {
            NDRX_LOG(log_info, "TPNOSTARTXID => starting as %c - prepared", 
                    XA_RM_STATUS_PREP);
            start_stat = XA_RM_STATUS_UNKOWN;
        }
        
        /* log to transaction file -> Write off RM status */
        if (EXSUCCEED!=tms_log_rmstatus(tmp, bt, start_stat, 0, 0))
        {
            NDRX_LOG(log_error, "Failed to write RM status to file: %ld", *btid);
            EXFAIL_OUT(ret);
        }

    }
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_ADD_STR( M_tx_hash, tmxid, tmp);
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    hash_added = EXTRUE;
    
out:
    
    /* unlock */
    if (EXSUCCEED!=ret)
    {
        tms_remove_logfile(tmp, hash_added);
    }
    else
    {
        if (NULL!=tmp)
        {
            tms_unlock_entry(tmp);
        }
    }
    return ret;
}

/**
 * Add RM to transaction.
 * If tid is known, then check and/or add.
 * If add, then ensure that we step up the BTID counter, if the number is
 * higher than counter have.
 * @param xai
 * @param rmid - 1 based rmid
 * @param[in,out] btid branch tid (if it is known, i.e. 0), if not known, then -1
 *  for unknown BTID requests, new BTID is generated
 * @return EXSUCCEED/EXFAIL
 */
expublic int tms_log_addrm(atmi_xa_tx_info_t *xai, short rmid, int *p_is_already_logged,
        long *btid, long flags)
{
    int ret = EXSUCCEED;
    atmi_xa_log_t *p_tl= NULL;
    atmi_xa_rm_status_btid_t *bt = NULL;
    
    /* atmi_xa_rm_status_t stat; */
    /*
    memset(&stat, 0, sizeof(stat));
    stat.rmstatus = XA_RM_STATUS_ACTIVE;
     * 
    */
    
    if (NULL==(p_tl = tms_log_get_entry(xai->tmxid, NDRX_LOCK_WAIT_TIME, NULL)))
    {
        NDRX_LOG(log_error, "No transaction/lock timeout under xid_str: [%s]", 
                xai->tmxid);
        ret=EXFAIL;
        goto out_nolock;
    }
    
    if (1 > rmid || rmid>NDRX_MAX_RMS)
    {
        NDRX_LOG(log_error, "RMID %hd out of bounds!!!");
        EXFAIL_OUT(ret);
    }
    
    /* if processing in background (say time-out rollback, the commit shall not be
     * accepted)
     */
    if (p_tl->is_background || XA_TX_STAGE_ACTIVE!=p_tl->txstage)
    {
        NDRX_LOG(log_error, "cannot register xid [%s] is_background: (%d) stage: (%hd)", 
                xai->tmxid, p_tl->is_background, p_tl->txstage);
        EXFAIL_OUT(ret);
    }
    
    /*
    if (p_tl->rmstatus[rmid-1].rmstatus && NULL!=p_is_already_logged)
    {
        *p_is_already_logged = EXTRUE;
    }
     * */
    /*
    p_tl->rmstatus[rmid-1] = stat;
    */
    
    ret = tms_btid_addupd(p_tl, rmid, btid, XA_RM_STATUS_NULL, 
            0, 0, p_is_already_logged, &bt);
    
    /* log to transaction file */
    if (!(*p_is_already_logged))
    {
        char start_stat = XA_RM_STATUS_ACTIVE;
        
        NDRX_LOG(log_info, "RMID %hd/%ld added to xid_str: [%s]", 
            rmid, *btid, xai->tmxid);
        
        if (flags & TMFLAGS_TPNOSTARTXID)
        {
            NDRX_LOG(log_info, "TPNOSTARTXID => adding as %c - unknown", 
                    XA_RM_STATUS_UNKOWN);
            start_stat = XA_RM_STATUS_UNKOWN;
        }
        
        if (EXSUCCEED!=tms_log_rmstatus(p_tl, bt, start_stat, 0, 0))
        {
            NDRX_LOG(log_error, "Failed to write RM status to file: %ld", *btid);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_info, "RMID %hd/%ld already joined to xid_str: [%s]", 
            rmid, *btid, xai->tmxid);
    }
   
out:
    /* unlock transaction from thread */
    tms_unlock_entry(p_tl);

out_nolock:
    
    return ret;
}


/**
 * Free up log file (just memory)
 * @param p_tl log handle
 * @param hash_rm remove log entry from tran hash
 */
expublic void tms_remove_logfree(atmi_xa_log_t *p_tl, int hash_rm)
{
    int i;
    atmi_xa_rm_status_btid_t *el, *elt;
    
    if (hash_rm)
    {
        MUTEX_LOCK_V(M_tx_hash_lock);
        EXHASH_DEL(M_tx_hash, p_tl); 
        MUTEX_UNLOCK_V(M_tx_hash_lock);
    }

    /* Remove branch TIDs */
    
    for (i=0; i<NDRX_MAX_RMS; i++)
    {
        EXHASH_ITER(hh, p_tl->rmstatus[i].btid_hash, el, elt)
        {
            EXHASH_DEL(p_tl->rmstatus[i].btid_hash, el);
            NDRX_FREE(el);
        }
    }
    
    NDRX_FREE(p_tl);
}

/**
 * Change tx state + write transaction stage
 * FORMAT: <STAGE_CODE>
 * @param p_tl
 * @param forced is decision forced? I.e. no restore on error.
 * @return EXSUCCEED/EXFAIL
 */
expublic int tms_log_stage(atmi_xa_log_t *p_tl, short stage, int forced)
{
    int ret = EXSUCCEED;
    short stage_org=EXFAIL;
    /* <Crash testing> */
    int make_crash = EXFALSE; /**< Crash simulation */
    int crash_stage, crash_class;
    /* </Crash testing> */
    
    CHK_THREAD_ACCESS;
    
    if (p_tl->txstage!=stage)
    {
        stage_org = p_tl->txstage;
        p_tl->txstage = stage;

        NDRX_LOG(log_debug, "tms_log_stage: new stage - %hd (cc %d)", 
                p_tl->txstage, G_atmi_env.test_tmsrv_commit_crash);
        
        
        /* <Crash testing> */
        
        /* QA: commit crash test point... 
         * Once crash flag is disabled, commit shall be finished by background
         * process.
         */
        crash_stage = G_atmi_env.test_tmsrv_commit_crash % 100;
        crash_class = G_atmi_env.test_tmsrv_commit_crash / 100;
        
        /* let write && exit */
        if (stage > 0 && crash_class==CRASH_CLASS_EXIT && stage == crash_stage)
        {
            NDRX_LOG(log_debug, "QA commit crash...");
            G_atmi_env.test_tmsrv_write_fail=EXTRUE;
            make_crash=EXTRUE;
        }
        
        /* no write & report error */
        if (stage > 0 && crash_class==CRASH_CLASS_NO_WRITE && stage == crash_stage)
        {
            NDRX_LOG(log_debug, "QA no write crash");
            ret=EXFAIL;
        }
        /* </Crash testing> */
        else if (EXSUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_STAGE, "%hd", stage))
        {
            ret=EXFAIL;
            goto out;
        }

        /* in case if switching to committing, we must sync the log & directory */
        if (XA_TX_STAGE_COMMITTING==stage &&
            (EXSUCCEED!=ndrx_fsync_fsync(p_tl->f, G_atmi_env.xa_fsync_flags) || 
                EXSUCCEED!=ndrx_fsync_dsync(G_tmsrv_cfg.tlog_dir, G_atmi_env.xa_fsync_flags)))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:
                
    /* <Crash testing> */
    if (make_crash)
    {
        exit(1);
    }
    /* </Crash testing> */

    /* If failed to log the stage switch, restore original transaction
     * stage
     */

    if (forced)
    {
        return EXSUCCEED;
    }
    else if (EXSUCCEED!=ret && EXFAIL!=stage_org)
    {
        p_tl->txstage = stage_org;
    }

    /* if not forced, get the real result */
    return ret;
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
expublic atmi_xa_log_list_t* tms_copy_hash2list(int copy_mode)
{
    atmi_xa_log_list_t *ret = NULL;
    atmi_xa_log_t * r, *rt;
    atmi_xa_log_list_t *tmp;
    
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
                
        if (NULL==(tmp = NDRX_CALLOC(1, sizeof(atmi_xa_log_list_t))))
        {
            NDRX_LOG(log_error, "Failed to malloc %d: %s", 
                    sizeof(atmi_xa_log_list_t), strerror(errno));
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
expublic void tms_tx_hash_lock(void)
{
    MUTEX_LOCK_V(M_tx_hash_lock);
}

/**
 * Unlock the transaction log hash
 */
expublic void tms_tx_hash_unlock(void)
{
    MUTEX_UNLOCK_V(M_tx_hash_lock);
}

/* vim: set ts=4 sw=4 et smartindent: */
