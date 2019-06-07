/**
 * @brief tmsrv - transaction logging & accounting
 *      for systems which are not support TMJOIN, we shall create additional
 *      XIDs for each of the involved process session. These XIDs shall be used
 *      as "sub-xids". There will be Master XID involved in process and
 *      sub-xids will be logged in tmsrv and will be known by processes locally
 *      Thus p_tl->rmstatus needs to be extended with hash list of the local
 *      transaction ids. We shall keep the structure universal, and use
 *      sub-xids even TMJOIN is supported.
 *
 * @file log.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#define LOG_COMMAND_STAGE           'S' /* Identify stage of txn */
#define LOG_COMMAND_I               'I' /* Info about txn */
#define LOG_COMMAND_RMSTAT          'R' /* Log the RM status */

#define CHK_THREAD_ACCESS if (ndrx_gettid()!=p_tl->lockthreadid)\
    {\
        NDRX_LOG(log_error, "Transaction [%s] not locked for thread %ul, but for %ul!",\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        userlog("Transaction [%s] not locked for thread %ul, but for %ul!",\
                p_tl->tmxid, ndrx_gettid(), p_tl->lockthreadid);\
        return EXFAIL;\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate atmi_xa_log_t *M_tx_hash = NULL; 
MUTEX_LOCKDECL(M_tx_hash_lock); /* Operations with hash list            */

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
    
    NDRX_LOG(log_info, "Transaction [%s] unlocked by thread %ul", p_tl->tmxid,
            p_tl->lockthreadid);
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    p_tl->lockthreadid = 0;
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    return EXSUCCEED;
}
/**
 * Get the log entry of the transaction
 * Now we should lock it for thread.
 * @param tmxid - serialized XID
 * @return NULL or log entry
 */
expublic atmi_xa_log_t * tms_log_get_entry(char *tmxid)
{
    atmi_xa_log_t *r = NULL;
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_FIND_STR( M_tx_hash, tmxid, r);
    
    if (NULL!=r)
    {
        if (r->lockthreadid)
        {
            NDRX_LOG(log_error, "Transaction [%s] already locked for thread_id: %lu",
                    tmxid,
                    r->lockthreadid);
            
            userlog("tmsrv: Transaction [%s] already locked for thread_id: %lu",
                    tmxid,
                    r->lockthreadid);
            r = NULL;
        }
        else
        {
            r->lockthreadid = ndrx_gettid();
            NDRX_LOG(log_info, "Transaction [%s] locked for thread_id: %lu",
                    tmxid,
                    r->lockthreadid);
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
    if (!(tmflags & TMTXFLAGS_DYNAMIC_REG))
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
        
        if (tmflags & TMTXFLAGS_TPNOSTARTXID)
        {
            NDRX_LOG(log_info, "TPNOSTARTXID => starting as %c - prepared", 
                    XA_RM_STATUS_PREP);
            start_stat = XA_RM_STATUS_PREP;
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
    
out:
    
    /* unlock */
    if (NULL!=tmp)
    {
        tms_unlock_entry(tmp);
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
        long *btid)
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
    
    if (NULL==(p_tl = tms_log_get_entry(xai->tmxid)))
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
        NDRX_LOG(log_info, "RMID %hd/%ld added to xid_str: [%s]", 
            rmid, *btid, xai->tmxid);
        
        if (EXSUCCEED!=tms_log_rmstatus(p_tl, bt, XA_RM_STATUS_ACTIVE, 0, 0))
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
    
    /* TODO: might want to check buffer sizes? */
    len = strlen(tmxid);
    if (len>sizeof((*pp_tl)->tmxid)-1)
    {
        NDRX_LOG(log_error, "tmxid too long [%s]!", tmxid);
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE((*pp_tl)->tmxid, tmxid);
    (*pp_tl)->is_background = EXTRUE;
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
        len = strlen(buf);
        if ('\n'==buf[len-1])
        {
            buf[len-1]=EXEOS;
            len--;
        }
        
        if ('\r'==buf[len-1])
        {
            buf[len-1]=EXEOS;
            len--;
        }
        
        NDRX_LOG(log_debug, "Parsing line: [%s]", buf);
        p = strchr(buf, ':');
        
        if (NULL==p)
        {
            NDRX_LOG(log_error, "Symbol ':' not found on line [%d]!", 
                    line);
            EXFAIL_OUT(ret);
        }
        p++;
        
        /* Classify & parse record */
        NDRX_LOG(log_debug, "Record: %c", *p);
        switch (*p)
        {
            case LOG_COMMAND_I: 
                if (EXSUCCEED!=tms_parse_info(buf, *pp_tl))
                {
                    EXFAIL_OUT(ret);
                }
                break;
            case LOG_COMMAND_STAGE: 
                if (EXSUCCEED!=tms_parse_stage(buf, *pp_tl))
                {
                    EXFAIL_OUT(ret);
                }
                break;
            case LOG_COMMAND_RMSTAT:
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
    
    /* Add transaction to the hash. We need locking here. 
     * 
     * If transaction was in prepare stage XA_TX_STAGE_PREPARING (40)
     * We need to abort it because, there is no chance that caller will get
     * response back.
     */
    if (XA_TX_STAGE_PREPARING == (*pp_tl)->txstage)
    {
        NDRX_LOG(log_error, "XA Transaction [%s] was in preparing stage and "
                "tmsrv is restarted - ABORTING", (*pp_tl)->tmxid);
        
        userlog("XA Transaction [%s] was in preparing stage and "
                "tmsrv is restarted - ABORTING", (*pp_tl)->tmxid);
        
        /* change the status (+ log) */
        (*pp_tl)->lockthreadid = ndrx_gettid();
        tms_log_stage(*pp_tl, XA_TX_STAGE_ABORTING);
        (*pp_tl)->lockthreadid = 0;
    }
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_ADD_STR( M_tx_hash, tmxid, (*pp_tl));
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    NDRX_LOG(log_debug, "TX [%s] loaded OK", tmxid);
out:
    /* Clean up if error. */
    if (EXSUCCEED!=ret && NULL!=*pp_tl)
    {
        NDRX_FREE((*pp_tl));
        *pp_tl = NULL;
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
 * Removes also transaction from hash. Either fully
 * committed or fully aborted...
 * @param p_tl - becomes invalid after removal!
 */
expublic void tms_remove_logfile(atmi_xa_log_t *p_tl)
{
    int have_file = EXFALSE;
    int i;
    atmi_xa_rm_status_btid_t *el, *elt;
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
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    EXHASH_DEL(M_tx_hash, p_tl); 
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
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
 * We should have universal log writter
 * TODO: Does not check number of chars written...
 */
exprivate int tms_log_write_line(atmi_xa_log_t *p_tl, char command, const char *fmt, ...)
{
    int ret = EXSUCCEED;
    char msg[LOG_MAX+1] = {EXEOS};
    char msg2[LOG_MAX+1] = {EXEOS};
    va_list ap;
    
    CHK_THREAD_ACCESS;
    
    /* If log not open - just skip... */
    if (NULL==p_tl->f)
        return EXSUCCEED;
     
    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    if (fprintf(p_tl->f, "%lld:%c:%s\n", ndrx_utc_tstamp(), command, msg)<0)
    {
        ret=EXFAIL;
        goto out;
    }
    fflush(p_tl->f);
    
out:
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
    
    if (EXSUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_I, "%hd:%hd:%hd:%ld:%s", 
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
        NDRX_LOG(log_error, "Missing token: %s!", X);\
        EXFAIL_OUT(ret);\
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
    
    /* Read first time stamp */
    TOKEN_READ("info", "tstamp");
    p_tl->t_start = atol(p);
    
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
    TOKEN_READ("info", "rmsbuf");
    
#if 0
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

/**
 * Change tx state + write transaction stage
 * FORMAT: <STAGE_CODE>
 * @param p_tl
 * @return 
 */
expublic int tms_log_stage(atmi_xa_log_t *p_tl, short stage)
{
    int ret = EXSUCCEED;
    
    CHK_THREAD_ACCESS;
    
    if (p_tl->txstage!=stage)
    {
        p_tl->txstage = stage;

        NDRX_LOG(log_debug, "tms_log_stage: new stage - %hd", 
                p_tl->txstage);

        if (EXSUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_STAGE, "%hd", stage))
        {
            ret=EXFAIL;
            goto out;
        }
    }
    
out:
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
   
    TOKEN_READ("rmstat", "btid");
    btid = atol(p);
    
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
 * Return the current transactions status
 * TODO: We also need a worker thread which will complete the stucked transactions.
 * TODO: We should close log file here too!!!!!
 * @param p_tl
 * @return 
 */
expublic int tm_chk_tx_status(atmi_xa_log_t *p_tl)
{
    int ret = TPEHEURISTIC; /* By default we have heuristic descision */
    int i;
    int all_aborted = EXTRUE;
    int all_committed = EXTRUE;
    atmi_xa_rm_status_btid_t *el, *elt;
    CHK_THREAD_ACCESS;
    
    for (i=0; i<NDRX_MAX_RMS; i++)
    {
        /* HASH Iterate over the branches */
        EXHASH_ITER(hh, p_tl->rmstatus[i].btid_hash, el, elt)
        {
            if (!(XA_RM_STATUS_COMMITTED == el->rmstatus ||
                XA_RM_STATUS_COMMITTED_RO == el->rmstatus)
                    )
            {
                all_committed = EXFALSE;
                break;
            }

            if (!(XA_RM_STATUS_ABORTED == el->rmstatus ||
                XA_RM_STATUS_COMMITTED_RO == el->rmstatus)
                    )
            {
                all_aborted = EXFALSE;
                break;
            }
        } /* Hash iter */
    }
    
    if (all_aborted || all_committed)
    {
        ret = EXSUCCEED;
        /* TODO: We should unlink the transaction log file... 
         * (and remove from hash) */
        
        if (all_committed)
        {
            /* Mark transaction as committed */
            tms_log_stage(p_tl, XA_TX_STAGE_COMMITTED);
        }
        
        if (all_aborted)
        {
            /* Mark transaction as committed */
            tms_log_stage(p_tl, XA_TX_STAGE_ABORTED);
        }
        
        /* p_tl becomes invalid! */
        tms_remove_logfile(p_tl);
         
    }
    else
    {
        /* Move it to background: */
        NDRX_LOG(log_warn, "Transaction with xid: [%s] moved to "
                "background for completion...", p_tl->tmxid);
        p_tl->is_background = EXTRUE;
    }

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
