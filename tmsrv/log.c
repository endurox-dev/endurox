/* 
** tmsrv - transaction logging & accounting
**
** @file log.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
#include <uthash.h>
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
        return FAIL;\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
private atmi_xa_log_t *M_tx_hash = NULL; 
MUTEX_LOCKDECL(M_tx_hash_lock); /* Operations with hash list            */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int tms_log_write_line(atmi_xa_log_t *p_tl, char command, const char *fmt, ...);
private int tms_parse_info(char *buf, atmi_xa_log_t *p_tl);
private int tms_parse_stage(char *buf, atmi_xa_log_t *p_tl);
private int tms_parse_rmstatus(char *buf, atmi_xa_log_t *p_tl);


/**
 * Unlock transaction
 * @param p_tl
 * @return SUCCEED/FAIL
 */
public int tms_unlock_entry(atmi_xa_log_t *p_tl)
{
    CHK_THREAD_ACCESS;
    
    NDRX_LOG(log_info, "Transaction [%s] unlocked by thread %ul", p_tl->tmxid,
            p_tl->lockthreadid);
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    p_tl->lockthreadid = 0;
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    return SUCCEED;
}
/**
 * Get the log entry of the transaction
 * Now we should lock it for thread.
 * @param tmxid - serialized XID
 * @return NULL or log entry
 */
public atmi_xa_log_t * tms_log_get_entry(char *tmxid)
{
    atmi_xa_log_t *r = NULL;
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    HASH_FIND_STR( M_tx_hash, tmxid, r);
    
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
            NDRX_LOG(log_info, "Transaction [%s] locked for thread_id: %lu",
                    tmxid,
                    r->lockthreadid);
            r->lockthreadid = ndrx_gettid();
        }
    }
    
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    return r;
}

/**
 * Log transaction as started
 * @param xai - XA Info struct
 * @param txtout - transaction timeout
 * @return SUCCEED/FAIL
 */
public int tms_log_start(atmi_xa_tx_info_t *xai, int txtout, long tmflags)
{
    int ret = SUCCEED;
    atmi_xa_log_t *tmp = NULL;
    
    /* 1. Add stuff to hash list */
    if (NULL==(tmp = calloc(sizeof(atmi_xa_log_t), 1)))
    {
        NDRX_LOG(log_error, "calloc() failed: %s", strerror(errno));
        ret=FAIL;
        goto out;
    }
    tmp->txstage = XA_TX_STAGE_ACTIVE;
    /* Only for static... */
    if (!(tmflags & TMTXFLAGS_DYNAMIC_REG))
    {
        tmp->rmstatus[xai->tmrmid-1].rmstatus = XA_RM_STATUS_ACTIVE;
    }
    tmp->tmnodeid = xai->tmnodeid;
    tmp->tmsrvid = xai->tmsrvid;
    tmp->tmrmid = xai->tmrmid;
    strcpy(tmp->tmxid, xai->tmxid);
    
    tmp->t_start = nstdutil_utc_tstamp();
    tmp->t_update = nstdutil_utc_tstamp();
    tmp->txtout = txtout;
    
    n_timer_reset(&tmp->ttimer);
    
    MUTEX_LOCK_V(M_tx_hash_lock);
    HASH_ADD_STR( M_tx_hash, tmxid, tmp);
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
out:
    return ret;
}

/**
 * Add RM to transaction
 * @param xai
 * @param rmid - 1 based rmid
 * @return 
 */
public int tms_log_addrm(atmi_xa_tx_info_t *xai, short rmid, int *p_is_already_logged)
{
    int ret = SUCCEED;
    atmi_xa_log_t *p_tl= NULL;
    atmi_xa_rm_status_t stat;
    
    memset(&stat, 0, sizeof(stat));
    stat.rmstatus = XA_RM_STATUS_ACTIVE;
    
    if (NULL==(p_tl = tms_log_get_entry(xai->tmxid)))
    {
        NDRX_LOG(log_error, "No transaction under xid_str: [%s]", 
                xai->tmxid);
        FAIL_OUT(ret);
    }
    
    if (1 > rmid || rmid>NDRX_MAX_RMS)
    {
        NDRX_LOG(log_error, "RMID %hd out of bounds!!!");
        FAIL_OUT(ret);
    }
    
    if (p_tl->rmstatus[rmid-1].rmstatus && NULL!=p_is_already_logged)
    {
        *p_is_already_logged = TRUE;
    }
    
    p_tl->rmstatus[rmid-1] = stat;
    
    NDRX_LOG(log_info, "RMID %hd joined to xid_str: [%s]", 
            rmid, xai->tmxid);
    
    /* unlock transaction from thread */
    tms_unlock_entry(p_tl);
    
out:
    return ret;
}

/**
 * Get the log file name for particular transaction
 * @param p_tl
 * @return 
 */
private void tms_get_file_name(atmi_xa_log_t *p_tl)
{
    sprintf(p_tl->fname, "%s/TRN-%ld-%hd-%d-%s", G_tmsrv_cfg.tlog_dir, 
            tpgetnodeid(), G_atmi_env.xa_rmid, G_server_conf.srv_id,
            p_tl->tmxid);
}

/**
 * Get the log file name for particular transaction
 * @param p_tl
 * @return 
 */
public int tms_open_logfile(atmi_xa_log_t *p_tl, char *mode)
{
    int ret = SUCCEED;
    
    if (EOS==p_tl->fname[0])
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
    if (NULL==(p_tl->f=fopen(p_tl->fname, mode)))
    {
        userlog("Failed to open XA transaction log file: [%s]: %s", 
                p_tl->fname, strerror(errno));
        NDRX_LOG(log_error, "Failed to open XA transaction "
                "log file: [%s]: %s", 
                p_tl->fname, strerror(errno));
        ret=FAIL;
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
public int tms_load_logfile(char *logfile, char *tmxid, atmi_xa_log_t **pp_tl)
{
    int ret = SUCCEED;
    int len;
    char buf[1024]; /* Should be enough... */
    char *p;
    int line = 1;
    
    *pp_tl = NULL;
    
    if (NULL==(*pp_tl = calloc(sizeof(atmi_xa_log_t), 1)))
    {
        NDRX_LOG(log_error, "calloc() failed: %s", strerror(errno));
        FAIL_OUT(ret);
    }
    /* set file name */
    if (strlen(logfile)>PATH_MAX)
    {
        NDRX_LOG(log_error, "Invalid file name!");
        FAIL_OUT(ret);
    }
    
    strcpy((*pp_tl)->fname, logfile);
    
    /* TODO: might want to check buffer sizes? */
    len = strlen(tmxid);
    if (len>sizeof((*pp_tl)->tmxid)-1)
    {
        NDRX_LOG(log_error, "tmxid too long [%s]!", tmxid);
        FAIL_OUT(ret);
    }
    
    strcpy((*pp_tl)->tmxid, tmxid);
    (*pp_tl)->is_background = TRUE;
    /* Open the file */
    if (SUCCEED!=tms_open_logfile(*pp_tl, "r+"))
    {
        NDRX_LOG(log_error, "Failed to open transaction file!");
        FAIL_OUT(ret);
    }
    
    /* Read line by line & call parsing functions 
     * we should also parse start of the transaction (date/time)
     */
    while (fgets(buf, sizeof(buf), (*pp_tl)->f))
    {
        len = strlen(buf);
        if ('\n'==buf[len-1])
        {
            buf[len-1]=EOS;
            len--;
        }
        
        if ('\r'==buf[len-1])
        {
            buf[len-1]=EOS;
            len--;
        }
        
        NDRX_LOG(log_debug, "Parsing line: [%s]", buf);
        p = strchr(buf, ':');
        
        if (NULL==p)
        {
            NDRX_LOG(log_error, "Symbol ':' not found on line [%d]!", 
                    line);
            FAIL_OUT(ret);
        }
        p++;
        
        /* Classify & parse record */
        NDRX_LOG(log_debug, "Record: %c", *p);
        switch (*p)
        {
            case LOG_COMMAND_I: 
                if (SUCCEED!=tms_parse_info(buf, *pp_tl))
                {
                    FAIL_OUT(ret);
                }
                break;
            case LOG_COMMAND_STAGE: 
                if (SUCCEED!=tms_parse_stage(buf, *pp_tl))
                {
                    FAIL_OUT(ret);
                }
                break;
            case LOG_COMMAND_RMSTAT:
                if (SUCCEED!=tms_parse_rmstatus(buf, *pp_tl))
                {
                    FAIL_OUT(ret);
                }
                break;
            default:
                NDRX_LOG(log_warn, "Unknown record %c on line %d", *p, line);
                break;
        }
        
        line++;
    }
    
    /* Add transaction to the hash. We need locking here. */
    MUTEX_LOCK_V(M_tx_hash_lock);
    HASH_ADD_STR( M_tx_hash, tmxid, (*pp_tl));
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    
    NDRX_LOG(log_debug, "TX [%s] loaded OK", tmxid);
out:
    /* Clean up if error. */
    if (SUCCEED!=ret && NULL!=*pp_tl)
    {
        free(pp_tl);
        *pp_tl = NULL;
    }

    return ret;
}

/**
 * Test to see is log file open
 * @param p_tl
 * @return TRUE/FALSE
 */
public int tms_is_logfile_open(atmi_xa_log_t *p_tl)
{
    if (p_tl->f)
        return TRUE;
    else
        return FALSE;
}

/**
 * Close the log file
 * @param p_tl
 */
public void tms_close_logfile(atmi_xa_log_t *p_tl)
{
    if (NULL!=p_tl->f)
    {
        fclose(p_tl->f);
        p_tl->f = NULL;
    }
}

/**
 * Removes also transaction from hash. Either fully
 * committed or fully aborted...
 * @param p_tl - becomes invalid after removal!
 */
public void tms_remove_logfile(atmi_xa_log_t *p_tl)
{
    int have_file = FALSE;
    
    if (tms_is_logfile_open(p_tl))
    {
        have_file = TRUE;
        tms_close_logfile(p_tl);
    }/* Check for file existance, if was not open */
    else if (0 == access(p_tl->fname, 0))
    {
        have_file = TRUE;
    }
        
    if (have_file)
    {
        if (SUCCEED!=unlink(p_tl->fname))
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
    HASH_DEL(M_tx_hash, p_tl); 
    MUTEX_UNLOCK_V(M_tx_hash_lock);
    free(p_tl);
    
}

/**
 * We should have universal log writter
 * TODO: Does not check number of chars written...
 */
private int tms_log_write_line(atmi_xa_log_t *p_tl, char command, const char *fmt, ...)
{
    int ret = SUCCEED;
    char msg[LOG_MAX+1] = {EOS};
    char msg2[LOG_MAX+1] = {EOS};
    va_list ap;
    
    CHK_THREAD_ACCESS;
    
    /* If log not open - just skip... */
    if (NULL==p_tl->f)
        return SUCCEED;
     
    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    if (fprintf(p_tl->f, "%lld:%c:%s\n", nstdutil_utc_tstamp(), command, msg)<0)
    {
        ret=FAIL;
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
public int tms_log_info(atmi_xa_log_t *p_tl)
{
    int ret = SUCCEED;
    char rmsbuf[NDRX_MAX_RMS*3+1]={EOS};
    int i;
    int len;
    
    CHK_THREAD_ACCESS;
    
    /* log the RMs participating in transaction */
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
    if (SUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_I, "%hd:%hd:%hd:%ld:%s", 
            p_tl->tmrmid, p_tl->tmnodeid, p_tl->tmsrvid, p_tl->txtout, rmsbuf))
    {
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}
/* Tokenizer variables */
#define TOKEN_READ_VARS \
            int first = TRUE;\
            char *p;

/* Read token for parsing */
#define TOKEN_READ(X, Y)\
    if (NULL==(p = strtok(first?buf:NULL, ":")))\
    {\
        NDRX_LOG(log_error, "Missing token: %s!", X);\
        FAIL_OUT(ret);\
    }\
    if (first)\
        first = FALSE;\

/**
 * Parse & rebuild the info from file record
 * @param buf - record from transaction file
 * @param p_tl - transaction record
 * @return SUCCEED/FAIL
 */
private int tms_parse_info(char *buf, atmi_xa_log_t *p_tl)
{
    int ret = SUCCEED;
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
    
    p2 = strtok_r (p, ",", &saveptr1);
    while (p2 != NULL)
    {
        /* Put the p2 in join state */
        rmid = atoi(p2);
        if (rmid<1 || rmid>NDRX_MAX_RMS)
        {
            NDRX_LOG(log_error, "Invalid RMID: %d!", rmid);
            FAIL_OUT(ret);
        }
        
        p_tl->rmstatus[rmid-1].rmstatus =XA_RM_STATUS_ACTIVE;
        
        p2 = strtok_r (NULL, ",", &saveptr1);
    }
    
out:                
    return ret;
}

/**
 * Change tx state + write transaction stage
 * FORMAT: <STAGE_CODE>
 * @param p_tl
 * @return 
 */
public int tms_log_stage(atmi_xa_log_t *p_tl, short stage)
{
    int ret = SUCCEED;
    
    CHK_THREAD_ACCESS;
    
    if (p_tl->txstage!=stage)
    {
        p_tl->txstage = stage;

        NDRX_LOG(log_debug, "tms_log_stage: new stage - %hd", 
                p_tl->txstage);

        if (SUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_STAGE, "%hd", stage))
        {
            ret=FAIL;
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
private int tms_parse_stage(char *buf, atmi_xa_log_t *p_tl)
{
   int ret = SUCCEED;
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
 * @param p_tl
 * @param stage
 * @return SUCCEED/FAIL
 */
public int tms_log_rmstatus(atmi_xa_log_t *p_tl, short rmid, 
        char rmstatus, int  rmerrorcode, short  rmreason)
{
    int ret = SUCCEED;
    int do_log = FALSE;
    
    CHK_THREAD_ACCESS;
    
    if (p_tl->rmstatus[rmid-1].rmstatus != rmstatus)
    {
        do_log = TRUE;
    }
    
    p_tl->rmstatus[rmid-1].rmstatus = rmstatus;
    p_tl->rmstatus[rmid-1].rmerrorcode = rmerrorcode;
    p_tl->rmstatus[rmid-1].rmreason = rmreason;
    
    if (do_log)
    {
        if (SUCCEED!=tms_log_write_line(p_tl, LOG_COMMAND_RMSTAT, "%hd:%c:%d:%hd", 
                rmid, rmstatus, rmerrorcode, rmreason))
        {
            ret=FAIL;
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
private int tms_parse_rmstatus(char *buf, atmi_xa_log_t *p_tl)
{
    int ret = SUCCEED;
    char rmstatus;
    int rmerrorcode;
    short rmreason;
    int rmid;
    TOKEN_READ_VARS;
   
    TOKEN_READ("rmstat", "tstamp");
    p_tl->t_update = atol(p);
    TOKEN_READ("info", "cmdid");
    
    TOKEN_READ("rmstat", "rmid");
   
    rmid = atoi(p);
    if (rmid<1 || rmid>NDRX_MAX_RMS)
    {
        NDRX_LOG(log_error, "Invalid RMID: %d!", rmid);
        FAIL_OUT(ret);
    }
   
    TOKEN_READ("rmstat", "rmstatus");
    rmstatus = *p;

    TOKEN_READ("rmstat", "rmerrorcode");
    rmerrorcode = atoi(p);

    TOKEN_READ("rmstat", "rmreason");
    rmreason = (short)atoi(p);
   
    p_tl->rmstatus[rmid-1].rmstatus = rmstatus;
    p_tl->rmstatus[rmid-1].rmerrorcode = rmerrorcode;
    p_tl->rmstatus[rmid-1].rmreason = rmreason;
   
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
public int tm_chk_tx_status(atmi_xa_log_t *p_tl)
{
    int ret = TPEHEURISTIC; /* By default we have heuristic descision */
    int i;
    int all_aborted = TRUE;
    int all_committed = TRUE;
    
    CHK_THREAD_ACCESS;
    
    for (i=0; i<NDRX_MAX_RMS; i++)
    {
        if (!(XA_RM_STATUS_COMMITTED == p_tl->rmstatus[i].rmstatus ||
            XA_RM_STATUS_COMMITTED_RO == p_tl->rmstatus[i].rmstatus)
                )
        {
            all_committed = FALSE;
            break;
        }
        
        if (!(XA_RM_STATUS_ABORTED == p_tl->rmstatus[i].rmstatus ||
            XA_RM_STATUS_COMMITTED_RO == p_tl->rmstatus[i].rmstatus)
                )
        {
            all_aborted = FALSE;
            break;
        }
    }
    
    if (all_aborted || all_committed)
    {
        ret = SUCCEED;
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
        p_tl->is_background = TRUE;
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
public atmi_xa_log_list_t* tms_copy_hash2list(int copy_mode)
{
    atmi_xa_log_list_t *ret = NULL;
    atmi_xa_log_t * r, *rt;
    atmi_xa_log_list_t *tmp;
    
    if (copy_mode & COPY_MODE_ACQLOCK)
    {
        MUTEX_LOCK_V(M_tx_hash_lock);
    }
    
    /* No changes to hash list during the lock. */    
    
    HASH_ITER(hh, M_tx_hash, r, rt)
    {
        /* Only background items... */
        if (r->is_background && !(copy_mode & COPY_MODE_BACKGROUND))
            continue;
        
        if (!r->is_background && !(copy_mode & COPY_MODE_FOREGROUND))
            continue;
        
        if (NULL==(tmp = calloc(1, sizeof(atmi_xa_log_list_t))))
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
public void tms_tx_hash_lock(void)
{
    MUTEX_LOCK_V(M_tx_hash_lock);
}

/**
 * Unlock the transaction log hash
 */
public void tms_tx_hash_unlock(void)
{
    MUTEX_UNLOCK_V(M_tx_hash_lock);
}

/**
 * Copy TLOG info to FB (for display purposes)
 * @param p_ub - Dest UBF
 * @param p_tl - Transaction log
 * @return 
 */
public int tms_log_cpy_info_to_fb(UBFH *p_ub, atmi_xa_log_t *p_tl)
{
    int ret = SUCCEED;
    long tspent;
    short i;
    tspent = p_tl->txtout - n_timer_get_delta_sec(&p_tl->ttimer);    
    
    if (tspent<0)
        tspent = 0;
    
    if (
            SUCCEED!=Bchg(p_ub, TMXID, 0, (char *)p_tl->tmxid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMRMID, 0, (char *)&(p_tl->tmrmid), 0L) ||
            SUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&(p_tl->tmnodeid), 0L) ||
            SUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&(p_tl->tmsrvid), 0L) ||
            SUCCEED!=Bchg(p_ub, TMTXTOUT, 0, (char *)&(p_tl->txtout), 0L) ||
            SUCCEED!=Bchg(p_ub, TMTXTOUT_LEFT, 0, (char *)&tspent, 0L) ||
            SUCCEED!=Bchg(p_ub, TMTXSTAGE, 0, (char *)&(p_tl->txstage), 0L) ||
            SUCCEED!=Bchg(p_ub, TMTXTRYCNT, 0, (char *)&(p_tl->trycount), 0L) ||
            SUCCEED!=Bchg(p_ub, TMTXTRYMAXCNT, 0, (char *)&(G_tmsrv_cfg.max_tries), 0L)
        )
    {
        NDRX_LOG(log_error, "Failed to return fields: [%s]", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* return the info about RMs: */
    for (i=0; i<NDRX_MAX_RMS; i++)
    {
        if (p_tl->rmstatus[i].rmstatus)
        {
            /* cast to long... */
            long rmerrorcode =  (long)p_tl->rmstatus[i].rmerrorcode;
            short rmid = i+1;
            if (
                SUCCEED!=Badd(p_ub, TMTXRMID, (char *)&rmid, 0L) ||
                SUCCEED!=Badd(p_ub, TMTXRMSTATUS,
                    (char *)&(p_tl->rmstatus[i].rmstatus), 0L) ||
                SUCCEED!=Badd(p_ub, TMTXRMERRCODE,
                    (char *)&rmerrorcode, 0L) ||
                SUCCEED!=Badd(p_ub, TMTXRMREASON,
                    (char *)&(p_tl->rmstatus[i].rmreason), 0L)
                )
            {
                NDRX_LOG(log_error, "Failed to return fields: [%s]", 
                            Bstrerror(Berror));
                FAIL_OUT(ret);
            }
        }
    } /* for */
    
out:
    return ret;
}

