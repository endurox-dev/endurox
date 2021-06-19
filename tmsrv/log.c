/**
 * @brief tmsrv - transaction logging & accounting
 *   for systems which are not support TMJOIN, we shall create additional
 *   XIDs for each of the involved process session. These XIDs shall be used
 *   as "sub-xids". There will be Master XID involved in process and
 *   sub-xids will be logged in tmsrv and will be known by processes locally
 *   Thus p_tl->rmstatus needs to be extended with hash list of the local
 *   transaction ids. We shall keep the structure universal, and use
 *   sub-xids even TMJOIN is supported.
 *
 * @file log.c
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
#define LOG_MAX         1024

#define LOG_COMMAND_STAGE           'S' /**< Identify stage of txn           */
#define LOG_COMMAND_I               'I' /**< Info about txn                  */
#define LOG_COMMAND_J               'J' /**< Version 2 format with checksums */
#define LOG_COMMAND_RMSTAT          'R' /**< Log the RM status               */

#define LOG_VERSION_1                1   /**< Initial version                  */
#define LOG_VERSION_2                2   /**< Version 1, contains crc32 checks */

#define CHK_THREAD_ACCESS if (ndrx_gettid()!=p_tl->lockthreadid)\
    {\
        NDRX_LOG(log_error, "Transaction [%s] not locked for thread %" PRIu64 ", but for %" PRIu64,\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        userlog("Transaction [%s] not locked for thread %" PRIu64 ", but for %" PRIu64,\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        return EXFAIL;\
    }
#define LOG_RS_SEP  ';'            /**< record seperator               */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate atmi_xa_log_t *M_tx_hash = NULL; 
exprivate MUTEX_LOCKDECL(M_tx_hash_lock); /* Operations with hash list            */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int tms_log_write_line(atmi_xa_log_t *p_tl, char command, const char *fmt, ...);
exprivate int tms_parse_info(char *buf, atmi_xa_log_t *p_tl);
exprivate int tms_parse_stage(char *buf, atmi_xa_log_t *p_tl);
exprivate int tms_parse_rmstatus(char *buf, atmi_xa_log_t *p_tl);

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
 * @return NULL or log entry
 */
expublic atmi_xa_log_t * tms_log_get_entry(char *tmxid, int dowait)
{
    atmi_xa_log_t *r = NULL;
    ndrx_stopwatch_t w;
    
    if (dowait)
    {
        ndrx_stopwatch_reset(&w);
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
    
    if (NULL==(p_tl = tms_log_get_entry(xai->tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        NDRX_LOG(log_error, "No transaction under xid_str: [%s]", 
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
 * Change RM status
 * @param xai
 * @param rmid - 1 based rmid
 * @param[in] btid branch tid 
 * @param[in] rmstatus RM status to set
 * @return EXSUCCEED/EXFAIL
 */
expublic int tms_log_chrmstat(atmi_xa_tx_info_t *xai, short rmid, 
        long btid, char rmstatus, UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_log_t *p_tl= NULL;
    atmi_xa_rm_status_btid_t *bt = NULL;
    
    NDRX_LOG(log_debug, "xid: [%s] BTID %ld change status to [%c]",
            xai->tmxid, btid, rmstatus);
    
    if (NULL==(p_tl = tms_log_get_entry(xai->tmxid, NDRX_LOCK_WAIT_TIME)))
    {
        NDRX_LOG(log_error, "No transaction under xid_str: [%s] - match ", 
                xai->tmxid);
        
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, NDRX_XA_ERSN_NOTX, 
                    "Failed to get transaction or locked for processing!");
        
        ret=EXFAIL;
        goto out_nolock;
    }
    
    bt = tms_btid_find(p_tl, rmid, btid);
    
    if (rmstatus==bt->rmstatus)
    {
        NDRX_LOG(log_warn, "xid: [%s] BTID %ld already in status [%c]",
            xai->tmxid, btid, rmstatus);
    }
    
    if (XA_RM_STATUS_UNKOWN!=bt->rmstatus)
    {
        NDRX_LOG(log_error, "No transaction under xid_str: [%s] - match ", 
                xai->tmxid);
        
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, NDRX_XA_ERSN_INVPARAM, 
                    "BTID %ld in status %c but want to set to: %c!",
                btid, bt->rmstatus, rmstatus);
        
        EXFAIL_OUT(ret);
    }
    
    /* Change status to expected one.. */
    
    if (EXSUCCEED!=tms_log_rmstatus(p_tl, bt, rmstatus, 0, 0))
    {
        NDRX_LOG(log_error, "Failed to write RM status to file: %ld, "
                "new stat: %c old stat: [%c]", 
                btid, rmstatus, bt->rmstatus);
        
        atmi_xa_set_error_fmt(p_ub, TPEMATCH, NDRX_XA_ERSN_RMLOGFAIL, 
                    "BTID %ld in status %c but want to set to: %c!",
                btid, bt->rmstatus, rmstatus);
        
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "xid: [%s] BTID %ld change status to [%c] OK",
            xai->tmxid, btid, rmstatus);
   
out:
    /* unlock transaction from thread */
    tms_unlock_entry(p_tl);

out_nolock:
    
    return ret;
}


/**
 * Get the log file name for particular transaction
 * @param p_tl
 * @return 
 */
exprivate void tms_get_file_name(atmi_xa_log_t *p_tl)
{
    snprintf(p_tl->fname, sizeof(p_tl->fname), "%s/TRN-%ld-%hd-%d-%s", 
            G_tmsrv_cfg.tlog_dir, tpgetnodeid(), G_atmi_env.xa_rmid, 
            G_server_conf.srv_id, p_tl->tmxid);
}

/**
 * Get the log file name for particular transaction
 * @param p_tl
 * @return 
 */
expublic int tms_open_logfile(atmi_xa_log_t *p_tl, char *mode)
{
    int ret = EXSUCCEED;
    
    if (EXEOS==p_tl->fname[0])
    {
        tms_get_file_name(p_tl);
    }
    
    if (NULL!=p_tl->f)
    {
        /*
        NDRX_LOG(log_warn, "Log file [%s] already open", p_tl->fname);
         */
        goto out;
    }
    
    /* Try to open the file */
    if (NULL==(p_tl->f=NDRX_FOPEN(p_tl->fname, mode)))
    {
        userlog("Failed to open XA transaction log file: [%s]: %s", 
                p_tl->fname, strerror(errno));
        NDRX_LOG(log_error, "Failed to open XA transaction "
                "log file: [%s]: %s", 
                p_tl->fname, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "XA tx log file [%s] open for [%s]", 
            p_tl->fname, mode);

out:    
    return ret;
}

/**
 * Read the log file from disk
 * @param logfile - path to log file
 * @param tmxid - transaction ID string
 * @param pp_tl - transaction log (double ptr). Returned if all ok.
 * @return SUCCEED/FAIL
 */
expublic int tms_load_logfile(char *logfile, char *tmxid, atmi_xa_log_t **pp_tl)
{
    int ret = EXSUCCEED;
    int len;
    char buf[1024]; /* Should be enough... */
    char *p;
    int line = 1;
    int got_term_last=EXFALSE, infos_ok=EXFALSE, missing_term_in_file=EXFALSE;
    int do_housekeep = EXFALSE;
    int wrote, err;
    long diff;
    *pp_tl = NULL;
    
    if (NULL==(*pp_tl = NDRX_CALLOC(sizeof(atmi_xa_log_t), 1)))
    {
        NDRX_LOG(log_error, "NDRX_CALLOC() failed: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    /* set file name */
    if (strlen(logfile)>PATH_MAX)
    {
        NDRX_LOG(log_error, "Invalid file name!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE((*pp_tl)->fname, logfile);
    
    len = strlen(tmxid);
    if (len>sizeof((*pp_tl)->tmxid)-1)
    {
        NDRX_LOG(log_error, "tmxid too long [%s]!", tmxid);
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE((*pp_tl)->tmxid, tmxid);
    /* we start with active... */
    (*pp_tl)->txstage = XA_TX_STAGE_ACTIVE;

    /* Open the file */
    if (EXSUCCEED!=tms_open_logfile(*pp_tl, "r+"))
    {
        NDRX_LOG(log_error, "Failed to open transaction file!");
        EXFAIL_OUT(ret);
    }

    /* Read line by line & call parsing functions 
     * we should also parse start of the transaction (date/time)
     */
    while (fgets(buf, sizeof(buf), (*pp_tl)->f))
    {
        got_term_last = EXFALSE;
        len = strlen(buf);
        
        if (1 < len && '\n'==buf[len-1])
        {
            buf[len-1]=EXEOS;
            len--;
            got_term_last = EXTRUE;
        }

        /* if somebody operated with Windows... */
        if (1 < len && '\r'==buf[len-1])
        {
            buf[len-1]=EXEOS;
            len--;
        }
        
        NDRX_LOG(log_debug, "Parsing line: [%s]", buf);
        
        if (!got_term_last)
        {
            /* Corrupted file, terminator expected 
             * - mark record as corrupted.
             * - Also we will try to repair the situation, if last line was corrupted
             * then we mark with with NAK.
             */
            missing_term_in_file = EXTRUE;
            continue;
        }
        
        /* If there was power off, then we might get corrupted lines...
         */
        p = strchr(buf, ':');
        
        if (NULL==p)
        {
            NDRX_LOG(log_error, "Symbol ':' not found on line [%d]!", 
                    line);
            EXFAIL_OUT(ret);
        }
        p++;
        
        /* Classify & parse record */
        NDRX_LOG(log_error, "Record: %c log_version: %d", *p, (*pp_tl)->log_version);
        
        /* If this is initial line && record J
         * or log_version > 0, perform checksuming.
         */
        if ((!infos_ok && LOG_COMMAND_J==*p) || ((*pp_tl)->log_version > LOG_VERSION_1))
        {
            char *rs = strchr(buf, LOG_RS_SEP);
            unsigned long crc32_calc=0, crc32_got=0;
            
            if (NULL==rs)
            {
                NDRX_LOG(log_error, "TMSRV log file [%s] missing %c sep on line [%s] - ignore line", 
                            logfile, LOG_RS_SEP, buf);
                userlog("TMSRV log file [%s] missing %c sep on line [%s] - ignore line", 
                            logfile, LOG_RS_SEP, buf);
                continue;
            }
            
            *rs = EXEOS;
            rs++;
            
            /* Now we get crc32 */
            crc32_calc = ndrx_Crc32_ComputeBuf(0, buf, strlen(buf));
            sscanf(rs, "%lx", &crc32_got);
            
            /* verify the log... */
            if (crc32_calc!=crc32_got)
            {
                NDRX_LOG(log_error, "TMSRV log file [%s] invalid log [%s] line "
                        "crc32 calc: [%lx] vs rec: [%lx] - ignore line", 
                        logfile, buf, crc32_calc, crc32_got);
                userlog("TMSRV log file [%s] invalid log [%s] line "
                        "crc32 calc: [%lx] vs rec: [%lx] - ignore line", 
                         logfile, buf, crc32_calc, crc32_got);
                continue;
            }
            
        }
        
        switch (*p)
        {
            /* Version 0: */
            case LOG_COMMAND_I:
            /* Version 1: */
            case LOG_COMMAND_J:
                
                if (infos_ok)
                {
                    NDRX_LOG(log_error, "TMSRV log file [%s] duplicate info block", 
                            logfile);
                    userlog("TMSRV log file [%s] duplicate info block", 
                            logfile);
                    do_housekeep=EXTRUE;
                    EXFAIL_OUT(ret);
                }
                
                /* set the version number */
                (*pp_tl)->log_version = *p - LOG_COMMAND_I + 1;
                
                if (EXSUCCEED!=tms_parse_info(buf, *pp_tl))
                {
                    EXFAIL_OUT(ret);
                }
                
                infos_ok = EXTRUE;
                break;
            case LOG_COMMAND_STAGE: 
                
                if (!infos_ok)
                {
                    NDRX_LOG(log_error, "TMSRV log file [%s] Info record required first", 
                            logfile);
                    userlog("TMSRV log file [%s] Info record required first", 
                            logfile);
                    do_housekeep=EXTRUE;
                    EXFAIL_OUT(ret);
                }
                
                if (EXSUCCEED!=tms_parse_stage(buf, *pp_tl))
                {
                    EXFAIL_OUT(ret);
                }
                
                break;
            case LOG_COMMAND_RMSTAT:
                
                if (!infos_ok)
                {
                    NDRX_LOG(log_error, "TMSRV log file [%s] Info record required first", 
                            logfile);
                    userlog("TMSRV log file [%s] Info record required first", 
                            logfile);
                    do_housekeep=EXTRUE;
                    EXFAIL_OUT(ret);
                }
                
                if (EXSUCCEED!=tms_parse_rmstatus(buf, *pp_tl))
                {
                    EXFAIL_OUT(ret);
                }
                
                break;
            default:
                NDRX_LOG(log_warn, "Unknown record %c on line %d", *p, line);
                break;
        }
        
        line++;
    }
    
    /* check was last read OK */
    if (!feof((*pp_tl)->f))
    {
        err = ferror((*pp_tl)->f);
        
        NDRX_LOG(log_error, "TMSRV log file [%s] failed to read: %s", 
                    logfile, strerror(err));
        userlog("TMSRV log file [%s] failed to read: %s", 
                    logfile, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (!infos_ok)
    {
        NDRX_LOG(log_error, "TMSRV log file [%s] no [%c] info record - not loading", 
                    logfile, LOG_COMMAND_J);
        userlog("TMSRV log file [%s] no [%c] info record - not loading", 
                    logfile, LOG_COMMAND_J);
        do_housekeep=EXTRUE;
        EXFAIL_OUT(ret);
    }
    
    /* so that next time we can parse OK */
    if (missing_term_in_file)
    {
        /* so last line bad, lets fix... */
        if (got_term_last)
        {
            NDRX_LOG(log_error, "TMSRV log file [%s] - invalid read, "
                    "previous lines seen without \\n", 
                    logfile);
            userlog("TMSRV log file [%s] - invalid read, "
                    "previous lines without \\n", 
                    logfile);
            EXFAIL_OUT(ret);
        }
        else
        {
            /* append with \n */
            NDRX_LOG(log_error, "Terminating last line (with out checksum)");

            wrote=fprintf((*pp_tl)->f, "\n");

            if (wrote!=1)
            {
                err = ferror((*pp_tl)->f);
                NDRX_LOG(log_error, "TMSRV log file [%s] failed to terminate line: %s", 
                        logfile, strerror(err));
                userlog("TMSRV log file [%s] failed to terminate line: %s", 
                        logfile, strerror(err));
                EXFAIL_OUT(ret);
            }
        }
    }
    
    /* thus it will start to drive it by background thread
     * Any transaction will be completed in background.
     */
    (*pp_tl)->is_background = EXTRUE;
    
    
    /* 
     * If we keep in active state, then timeout will kill the transaction (or it will be finished by in progress 
     *  binary, as maybe transaction is not yet completed). Thought we might loss the infos
     *  of some registered process, but then it would be aborted transaction anyway.
     *  but then record might be idling in the resource.
     * 
     * If we had stage logged, then transaction would be completed accordingly,
     * and would complete with those resources for which states is not yet
     * finalized (according to logs)
     */
    
    /* Add transaction to the hash. We need locking here. 
     * 
     * If transaction was in prepare stage XA_TX_STAGE_PREPARING (40)
     * We need to abort it because, there is no chance that caller will get
     * response back.
     */
    if (XA_TX_STAGE_PREPARING == (*pp_tl)->txstage 
            || XA_TX_STAGE_ACTIVE == (*pp_tl)->txstage)
    {
        NDRX_LOG(log_error, "XA Transaction [%s] was in active or preparing stage and "
                "tmsrv is restarted - ABORTING", (*pp_tl)->tmxid);
        
        userlog("XA Transaction [%s] was in  active or preparing stage and "
                "tmsrv is restarted - ABORTING", (*pp_tl)->tmxid);
        
        /* change the status (+ log) */
        (*pp_tl)->lockthreadid = ndrx_gettid();
        tms_log_stage(*pp_tl, XA_TX_STAGE_ABORTING, EXTRUE);
        (*pp_tl)->lockthreadid = 0;
    }
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_ADD_STR( M_tx_hash, tmxid, (*pp_tl));
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    NDRX_LOG(log_debug, "TX [%s] loaded OK", tmxid);
out:

    /* Clean up if error. */
    if (EXSUCCEED!=ret)
    {
        userlog("Failed to load/recover transaction [%s]", logfile);
        if (NULL!=*pp_tl)
        {
            /* remove any stuff added... */
            
            if (tms_is_logfile_open(*pp_tl))
            {
                tms_close_logfile(*pp_tl);
            }
            
            tms_remove_logfree(*pp_tl, EXFALSE);
            *pp_tl = NULL;
        }
    }

    /* clean up corrupted files */
    if (do_housekeep)
    {
        /* remove old corrupted logs... */
        if ((diff=ndrx_file_age(logfile)) > G_tmsrv_cfg.housekeeptime)
        {
            NDRX_LOG(log_error, "Corrupted log file [%s] age %ld sec (housekeep %d) - removing",
                    logfile, diff, G_tmsrv_cfg.housekeeptime);
            userlog("Corrupted log file [%s] age %ld sec (housekeep %d) - removing",
                    logfile, diff, G_tmsrv_cfg.housekeeptime);

            if (EXSUCCEED!=unlink(logfile))
            {
                err = errno;
                NDRX_LOG(log_error, "Failed to unlink [%s]: %s", strerror(err));
                userlog("Failed to unlink [%s]: %s", strerror(err));
            }
        }
    }

    return ret;
}

/**
 * Test to see is log file open
 * @param p_tl
 * @return TRUE/FALSE
 */
expublic int tms_is_logfile_open(atmi_xa_log_t *p_tl)
{
    if (p_tl->f)
        return EXTRUE;
    else
        return EXFALSE;
}

/**
 * Close the log file
 * @param p_tl
 */
expublic void tms_close_logfile(atmi_xa_log_t *p_tl)
{
    if (NULL!=p_tl->f)
    {
        NDRX_FCLOSE(p_tl->f);
        p_tl->f = NULL;
    }
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
 * Removes also transaction from hash. Either fully
 * committed or fully aborted...
 * @param p_tl - becomes invalid after removal!
 * @param hash_rm remove from hash lists
 */
expublic void tms_remove_logfile(atmi_xa_log_t *p_tl, int hash_rm)
{
    int have_file = EXFALSE;
    
    if (tms_is_logfile_open(p_tl))
    {
        have_file = EXTRUE;
        tms_close_logfile(p_tl);
    }/* Check for file existance, if was not open */
    else if (0 == access(p_tl->fname, 0))
    {
        have_file = EXTRUE;
    }
        
    if (have_file)
    {
        if (EXSUCCEED!=unlink(p_tl->fname))
        {
            int err = errno;
            NDRX_LOG(log_debug, "Failed to remove tx log file: %d (%s)", 
                    err, strerror(err));
            userlog("Failed to remove tx log file: %d (%s)", 
                    err, strerror(err));
        }
    }
    
    /* Remove the log for hash! */
    tms_remove_logfree(p_tl, hash_rm);

}

/**
 * We should have universal log writter
 */
exprivate int tms_log_write_line(atmi_xa_log_t *p_tl, char command, const char *fmt, ...)
{
    int ret = EXSUCCEED;
    char msg[LOG_MAX+1] = {EXEOS};
    char msg2[LOG_MAX+1] = {EXEOS};
    int len, wrote, exp;
    int make_error = EXFALSE;
    unsigned long crc32;
    va_list ap;
    
    CHK_THREAD_ACCESS;
    
    /* If log not open - just skip... */
    if (NULL==p_tl->f)
    {
        return EXSUCCEED;
    }
     
    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    snprintf(msg2, sizeof(msg2), "%lld:%c:%s", ndrx_utc_tstamp(), command, msg);
    len = strlen(msg2);
    
    /* check exactly how much bytes was written */
    
    if (1==G_atmi_env.test_tmsrv_write_fail)
    {
        make_error = EXTRUE;
    }
    else if (2==G_atmi_env.test_tmsrv_write_fail)
    {
        static int first = EXTRUE;
        
        if (first)
        {
            srand ( time(NULL) );
            first=EXFALSE;
        }
        
        /* generate 25% random errors */
        if (rand() % 4 == 0)
        {
            make_error = EXTRUE;
        }
        
    }
    
    crc32 = ndrx_Crc32_ComputeBuf(0, msg2, len);
    
    /* Simulate that only partial message is written in case
     * if disk is full or crash happens
     */
    wrote=0;
    
    /* if write file test case - write partially, also omit EOL terminator 
     * Also... needs flag for commit crash simulation... i.e. 
     * So that if commit crash is set, we partially write the command (stage change)
     * and afterwards tmsrv exits.
     * And then when flag is removed, transaction shall commit OK
     */
    if (p_tl->log_version == LOG_VERSION_1)
    {
        NDRX_LOG(log_debug, "Log format: v%d", p_tl->log_version);
        
        exp = len+1;
        if (make_error)
        {
            exp++;
        }
        wrote=fprintf(p_tl->f, "%s\n", msg2);
    }
    else
    {
        /* version 2+ */
        exp = len+8+1+1;
        
        if (make_error)
        {
            crc32+=1;
            exp++;
        }
        
        wrote=fprintf(p_tl->f, "%s%c%08lx\n", msg2, LOG_RS_SEP, crc32);
    }
    
    if (wrote != exp)
    {
        int err = errno;
        
        /* For Q/A purposes - simulate no space error, if requested */
        if (make_error)
        {
            NDRX_LOG(log_error, "QA point: make_error TRUE");
            err = ENOSPC;
        }
        
        NDRX_LOG(log_error, "ERROR! Failed to write transaction log file: req_len=%d, written=%d: %s",
                exp, wrote, strerror(err));
        userlog("ERROR! Failed to write transaction log file: req_len=%d, written=%d: %s",
                exp, wrote, strerror(err));
        
        ret=EXFAIL;
        goto out;
    }
    
    
out:
    /* flush what ever we have */
    if (EXSUCCEED!=fflush(p_tl->f))
    {
        int err=errno;
        userlog("ERROR! Failed to fflush(): %s", strerror(err));
        NDRX_LOG(log_error, "ERROR! Failed to fflush(): %s", strerror(err));
    }
    /*fsync(fileno(p_tl->f));*/
    return ret;
}

/**
 * Write general info about transaction
 * FORMAT: <RM_ID>:<NODE_ID>:<SRV_ID>
 * @param p_tl
 * @param stage
 * @return 
 */
expublic int tms_log_info(atmi_xa_log_t *p_tl)
{
    int ret = EXSUCCEED;
    char rmsbuf[NDRX_MAX_RMS*3+1]={EXEOS};
    int i;
    int len;
    
    CHK_THREAD_ACCESS;
    
    /* log the RMs participating in transaction 
    for (i=0; i<NDRX_MAX_RMS; i++)
    {
        if (p_tl->rmstatus[i].rmstatus)
        {
            len = strlen(rmsbuf);
            if (len>0)
            {
                rmsbuf[len]=',';
                rmsbuf[len+1]=',';
                len++;
            }
            sprintf(rmsbuf+len, "%d", i+1);
        }
    }
     *
     */
    
    if (EXSUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_J, "%hd:%hd:%hd:%ld:%s", 
            p_tl->tmrmid, p_tl->tmnodeid, p_tl->tmsrvid, p_tl->txtout, rmsbuf))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}
/* Tokenizer variables */
#define TOKEN_READ_VARS \
            int first = EXTRUE;\
            char *p;

/* Read token for parsing */
#define TOKEN_READ(X, Y)\
    if (NULL==(p = strtok(first?buf:NULL, ":")))\
    {\
        NDRX_LOG(log_error, "Missing token: %s.%s!", X, Y);\
        EXFAIL_OUT(ret);\
    }\
    if (first)\
        first = EXFALSE;\
        
#define TOKEN_READ_OPT(X, Y)\
    if (NULL==(p = strtok(first?buf:NULL, ":")))\
    {\
        NDRX_LOG(log_warn, "Missing token: %s.%s - optional, ignore!", X, Y);\
    }\
    if (first)\
        first = EXFALSE;\

/**
 * Parse & rebuild the info from file record
 * @param buf - record from transaction file
 * @param p_tl - transaction record
 * @return SUCCEED/FAIL
 */
exprivate int tms_parse_info(char *buf, atmi_xa_log_t *p_tl)
{
    int ret = EXSUCCEED;
    TOKEN_READ_VARS;
    char *p2;
    char *saveptr1 = NULL;
    int rmid;
    long long tdiff;
    
    /* Read first time stamp */
    TOKEN_READ("info", "tstamp");
    p_tl->t_start = strtoull (p, NULL, 10);
    
    tdiff = ndrx_utc_tstamp() - p_tl->t_start;
    /* adjust the timer...  */
    ndrx_stopwatch_reset(&p_tl->ttimer);
    ndrx_stopwatch_minus(&p_tl->ttimer, tdiff);
    NDRX_LOG(log_debug, "Transaction age: %ld ms (t_start: %llu tdiff: %lld)",
        ndrx_stopwatch_get_delta(&p_tl->ttimer), p_tl->t_start, tdiff);
             
    /* read tmrmid, tmnodeid, tmsrvid = ignore, but they must be in place! */
    TOKEN_READ("info", "cmdid");
    /* Basically we ignore these values, and read from current process!!!: */
    TOKEN_READ("info", "tmrmid");
    TOKEN_READ("info", "tmnodeid");
    TOKEN_READ("info", "tmsrvid");
    p_tl->tmrmid = G_atmi_env.xa_rmid;
    p_tl->tmnodeid = tpgetnodeid();
    p_tl->tmsrvid = G_server_conf.srv_id;
    
    TOKEN_READ("info", "txtout");
    p_tl->txtout = atol(p);
    
    /* Read involved resrouce managers - put them in join state */
#if 0
    TOKEN_READ("info", "rmsbuf");
    
    - not used anymore. All active branches are logged as RM status
    p2 = strtok_r (p, ",", &saveptr1);
    while (p2 != NULL)
    {
        /* Put the p2 in join state */
        rmid = atoi(p2);
        if (rmid<1 || rmid>NDRX_MAX_RMS)
        {
            NDRX_LOG(log_error, "Invalid RMID: %d!", rmid);
            EXFAIL_OUT(ret);
        }
        
        p_tl->rmstatus[rmid-1].rmstatus =XA_RM_STATUS_ACTIVE;
        
        p2 = strtok_r (NULL, ",", &saveptr1);
    }
#endif
    
out:                
    return ret;
}

#define CRASH_CLASS_EXIT            0 /**< exit at crash                  */
#define CRASH_CLASS_NO_WRITE        1 /**< Do not write and report error  */

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
 * Parse stage info from buffer read
 * @param buf - buffer read
 * @param p_tl - transaction record
 * @return SUCCEED/FAIL
 */
exprivate int tms_parse_stage(char *buf, atmi_xa_log_t *p_tl)
{
   int ret = EXSUCCEED;
   TOKEN_READ_VARS;
   
   TOKEN_READ("stage", "tstamp");
   p_tl->t_update = atol(p);
   TOKEN_READ("info", "cmdid");
   
   TOKEN_READ("stage", "txstage");
   p_tl->txstage = (short)atoi(p);
   
out:
   return ret;
}

/**
 * Change + write RM status to file
 * FORMAT: <rmid>:<rmstatus>:<rmerrorcode>:<rmreason>
 * @param p_tl transaction
 * @param[in] bt transaction branch
 * @param[in] rmstatus Resource manager status
 * @param[in] rmerrorcode Resource manager error code
 * @param[in] rmreason reason code
 * @return SUCCEED/FAIL
 */
expublic int tms_log_rmstatus(atmi_xa_log_t *p_tl, atmi_xa_rm_status_btid_t *bt, 
        char rmstatus, int rmerrorcode, short rmreason)
{
    int ret = EXSUCCEED;
    int do_log = EXFALSE;
    
    CHK_THREAD_ACCESS;

    if (bt->rmstatus != rmstatus)
    {
        do_log = EXTRUE;
    }
    
    bt->rmstatus = rmstatus;
    bt->rmerrorcode = rmerrorcode;
    bt->rmreason = rmreason;
    
    if (do_log)
    {
        if (EXSUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_RMSTAT, "%hd:%c:%d:%hd:%ld",
                bt->rmid, rmstatus, rmerrorcode, rmreason, bt->btid))
        {
            ret=EXFAIL;
            goto out;
        }
    }
    
out:
    return ret;
}

/**
 * Parse RM's status record from transaction file
 * @param buf - transaction record's line
 * @param p_tl - internal transaction record
 * @return SUCCEED/FAIL;
 */
exprivate int tms_parse_rmstatus(char *buf, atmi_xa_log_t *p_tl)
{
    int ret = EXSUCCEED;
    char rmstatus;
    int rmerrorcode;
    short rmreason;
    int rmid;
    long btid;
    atmi_xa_rm_status_btid_t *bt = NULL;
    
    TOKEN_READ_VARS;
   
    TOKEN_READ("rmstat", "tstamp");
    p_tl->t_update = atol(p);
    TOKEN_READ("info", "cmdid");
    
    TOKEN_READ("rmstat", "rmid");
   
    rmid = atoi(p);
    if (rmid<1 || rmid>NDRX_MAX_RMS)
    {
        NDRX_LOG(log_error, "Invalid RMID: %d!", rmid);
        EXFAIL_OUT(ret);
    }
   
    TOKEN_READ("rmstat", "rmstatus");
    rmstatus = *p;

    TOKEN_READ("rmstat", "rmerrorcode");
    rmerrorcode = atoi(p);

    TOKEN_READ("rmstat", "rmreason");
    rmreason = (short)atoi(p);

    TOKEN_READ_OPT("rmstat", "btid");
    
    if (NULL!=p)
    {
        btid = atol(p);
    }
    else
    {
        btid=0;
    }
    
    /*
    p_tl->rmstatus[rmid-1].rmstatus = rmstatus;
    p_tl->rmstatus[rmid-1].rmerrorcode = rmerrorcode;
    p_tl->rmstatus[rmid-1].rmreason = rmreason;
     * */
    
    ret = tms_btid_addupd(p_tl, rmid, 
            &btid, rmstatus, rmerrorcode, rmreason, NULL, &bt);
   
out:
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

/**
 * Copy TLOG info to FB (for display purposes)
 * The function has mandatory task to fill in the general fields about
 * transaction.
 * The actual RM/Branch tids are fill up till we get UBF buffer error (full)
 * @param p_ub - Dest UBF
 * @param p_tl - Transaction log
 * @param incl_rm_stat include RM status in response buffer
 * @return EXSUCCEED/EXFAIL
 */
expublic int tms_log_cpy_info_to_fb(UBFH *p_ub, atmi_xa_log_t *p_tl, int incl_rm_stat)
{
    int ret = EXSUCCEED;
    long tspent;
    short i;
    tspent = p_tl->txtout - ndrx_stopwatch_get_delta_sec(&p_tl->ttimer);    
    atmi_xa_rm_status_btid_t *el, *elt;
    
    if (tspent<0)
    {
        tspent = 0;
    }
    
    if (
            EXSUCCEED!=Bchg(p_ub, TMXID, 0, (char *)p_tl->tmxid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMRMID, 0, (char *)&(p_tl->tmrmid), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&(p_tl->tmnodeid), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&(p_tl->tmsrvid), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMTXTOUT, 0, (char *)&(p_tl->txtout), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMTXTOUT_LEFT, 0, (char *)&tspent, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMTXSTAGE, 0, (char *)&(p_tl->txstage), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMTXTRYCNT, 0, (char *)&(p_tl->trycount), 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMTXTRYMAXCNT, 0, (char *)&(G_tmsrv_cfg.max_tries), 0L)
        )
    {
        NDRX_LOG(log_error, "Failed to return fields: [%s]", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* return the info about RMs: */
    for (i=0; incl_rm_stat && i<NDRX_MAX_RMS; i++)
    {
        /* loop over the Branch TIDs  */
        EXHASH_ITER(hh, p_tl->rmstatus[i].btid_hash, el, elt)
        {
            /* cast to long... */
            long rmerrorcode =  (long)el->rmerrorcode;
            
            if (
                EXSUCCEED!=Badd(p_ub, TMTXRMID, (char *)&el->rmid, 0L) ||
                EXSUCCEED!=Badd(p_ub, TMTXBTID, (char *)&el->btid, 0L) ||
                EXSUCCEED!=Badd(p_ub, TMTXRMSTATUS,
                    (char *)&(el->rmstatus), 0L) ||
                EXSUCCEED!=Badd(p_ub, TMTXRMERRCODE,
                    (char *)&rmerrorcode, 0L) ||
                EXSUCCEED!=Badd(p_ub, TMTXRMREASON,
                    (char *)&(el->rmreason), 0L)
                )
            {
                /* there could be tons of these branches for 
                 * mysql/mariadb or posgresql 
                 */
                NDRX_LOG(log_error, "Failed to return fields: [%s] - ignore", 
                            Bstrerror(Berror));
                
                /* finish off. */
                goto out;
            }
        }
    } /* for */
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
