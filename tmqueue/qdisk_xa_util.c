/**
 * @brief Q XA Backend, utility functions
 *
 * @file qdisk_xa_util.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <nstopwatch.h>

#include <xa.h>
#include <atmi_int.h>
#include <unistd.h>

#include "userlog.h"
#include "tmqueue.h"
#include "nstdutil.h"
#include "Exfields.h"
#include "atmi_tls.h"
#include "tmqd.h"
#include <qcommon.h>
#include <xa_cmn.h>
#include <ubfutil.h>
#include "qtran.h"
#include <singlegrp.h>
#include <sys_test.h>
#include "qdisk_xa.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * job for importing message into our memory based queue space
 * processed by thread pool
 */
typedef struct
{
    short nodeid;
    short srvid;
    char ref[PATH_MAX+1];
    int seqno;
    int mode;
    int state;
    int (*process_block)(char *tmxid, 
        union tmq_block **p_block, int state, int seqno);
} ndrx_tmq_storage_read_block_job_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int volatile M_loader_error=EXFALSE;  /**< Is startup loader failed */
/*---------------------------Prototypes---------------------------------*/

/**
 * Minimal XA call to tmqueue, used by external processes.
 * @param tmxid serialized xid
 * @param cmd TMQ_CMD* command code
 * @return XA error code
 */
expublic int ndrx_xa_qminicall(char *tmxid, char cmd)
{
    long rsplen;
    UBFH *p_ub = NULL;
    long ret = XA_OK;
    short nodeid = (short)tpgetnodeid();
   
    p_ub = (UBFH *)tpalloc("UBF", "", 1024 );
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmxid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMXID!");
        ret = XAER_RMERR;
        goto out;
    }

    if (EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMNODEID!");
        ret = XAER_RMERR;
        goto out;
    }
    
    NDRX_LOG(log_info, "Calling QSPACE [%s] for tmxid [%s], command %c",
                ndrx_G_qspacesvc, tmxid, cmd);
    
    ndrx_debug_dump_UBF(log_info, "calling Q space with", p_ub);

    if (EXFAIL == tpcall(ndrx_G_qspacesvc, (char *)p_ub, 0L, (char **)&p_ub, 
        &rsplen, TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s failed: %s", ndrx_G_qspacesvc, tpstrerror(tperrno));
        
        /* best guess -> not available */
        ret = XAER_RMFAIL;
        
        /* anyway if have detailed response, use bellow. */
    }
    
    ndrx_debug_dump_UBF(log_info, "Reply from RM", p_ub);
    
    /* try to get the result of the OP */
    if (Bpres(p_ub, TMTXRMERRCODE, 0) &&
            EXSUCCEED!=Bget(p_ub, TMTXRMERRCODE, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_debug, "Failed to get TMTXRMERRCODE: %s", Bstrerror(Berror));
        ret = XAER_RMERR;
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    NDRX_LOG(log_info, "returns %d", ret);
    
    return ret;
}


/**
 * Minimal XA call to tmqueue, used by external processes.
 * @param tmxid serialized xid
 * @param cmd TMQ_CMD* command code
 * @param pp_ub request buffer (output on success)
 * @param flags ATMI flags used for call
 * @return connection descriptor, or -1 on error
 */
expublic int ndrx_xa_qminiconnect(char cmd, UBFH **pp_ub, long flags)
{
    long rsplen;
    long ret = XA_OK;
    short nodeid = (short)tpgetnodeid();
   
    *pp_ub = (UBFH *)tpalloc("UBF", "", 1024 );
    
    if (NULL==*pp_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate notif buffer");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(*pp_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup EX_QMSGID!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bchg(*pp_ub, TMNODEID, 0, (char *)&nodeid, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMNODEID!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Connecting to QSPACE [%s], command %c",
                ndrx_G_qspacesvc, cmd);
    
    ndrx_debug_dump_UBF(log_info, "Connecting to Q space with", *pp_ub);

    if (EXFAIL == (ret=tpconnect(ndrx_G_qspacesvc, (char *)*pp_ub, 0L, flags|TPNOTRAN)))
    {
        NDRX_LOG(log_error, "%s tpconnect() failed: %s", ndrx_G_qspacesvc, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

out:

    if ( 0 > ret && NULL!=*pp_ub)
    {
        tpfree((char *)*pp_ub);
        *pp_ub = NULL;
    }

    NDRX_LOG(log_info, "returns cd=%d, ubf ptr %p", ret, *pp_ub);
    
    return ret;
}

/**
 * Get the data size from any block
 * @param data generic data block 
 * @return data size or 0 on error (should not happen anyway)
 */
expublic size_t tmq_get_block_len(char *data)
{
    size_t ret = 0;
    
    tmq_cmdheader_t *p_hdr = (tmq_cmdheader_t *)data;
    tmq_msg_t *p_msg;
    tmq_msg_del_t *p_del;
    tmq_msg_upd_t *p_upd;
    tmq_msg_unl_t *p_unl;
    tmq_msg_dum_t *p_dum;
    
    switch (p_hdr->command_code)
    {
        case TMQ_STORCMD_NEWMSG:
            p_msg = (tmq_msg_t *)data;
            ret = sizeof(*p_msg) + p_msg->len;
            break;
        case TMQ_STORCMD_UPD:
            ret = sizeof(*p_upd);
            break;
        case TMQ_STORCMD_DEL:
            ret = sizeof(*p_del);
            break;
        case TMQ_STORCMD_UNLOCK:
            ret = sizeof(*p_unl);
            break;
        case TMQ_STORCMD_DUM:
            ret = sizeof(*p_dum);
            break;
        default:
            NDRX_LOG(log_error, "Unknown command code: %c", p_hdr->command_code);
            break;
    }
    
    return ret;
}

/**
 * Delete/Update message block write
 * @param p_block ptr to union of commands
 * @param cust_tmxid custom transaction id (not part of global tran)
 * @param int_diag internal diagnostics, flags
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_block(char *data, char *descr, char *cust_tmxid, int *int_diag)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    size_t len;
    tmq_cmdheader_t *p_hdr = (tmq_cmdheader_t *)data;
    
    /* detect the actual len... */
    len = tmq_get_block_len((char *)data);
    
    NDRX_LOG(log_info, "Writing command block: %s msg [%s]", descr, 
            tmq_msgid_serialize(p_hdr->msgid, msgid_str));
    
    NDRX_DUMP(log_debug, "Writing command block to disk", 
                (char *)data, len);
    
    if (ndrx_G_tmq_storage->pf_storage_write_block(ndrx_G_tmq_storage, 
        (char *)data, len, cust_tmxid, int_diag))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_block() failed for msg %s", 
                tmq_msgid_serialize(p_hdr->msgid, msgid_str));

        /* mark txn for rullback */
        if (NULL!=cust_tmxid)
        {
            tmq_log_set_abort_only(cust_tmxid);
        }

        EXFAIL_OUT(ret);
    }
    
out:
    return ret;

}

/**
 * Return msgid from filename
 * @param filename_in
 * @param msgid_out
 * @return SUCCEED/FAIL
 */
exprivate int tmq_get_msgid_from_filename(char *filename_in, char *msgid_out)
{
    int ret = EXSUCCEED;
    
    tmq_msgid_deserialize(filename_in, msgid_out);
    
out:
    return ret;    
}

/**
 * Thread block loader...
 * @param ptr job
 * @param p_finish_off finish offset
 * @return <N/A>
 */
exprivate void block_loader_th(void *ptr, int *p_finish_off)
{
    ndrx_tmq_storage_read_block_job_t *job = (ndrx_tmq_storage_read_block_job_t *)ptr;
    union tmq_block *p_block=NULL;
    int ret;

    if (EXSUCCEED!=ndrx_G_tmq_storage->pf_storage_read_block(ndrx_G_tmq_storage, 
        job->nodeid, job->srvid, job->ref, job->seqno, &p_block, job->mode))
    {
        NDRX_LOG(log_error, "Failed to read [%s] sequence=%d (mode=%d) -> RAISED LOADER ERROR", 
            job->ref, job->seqno, job->mode);
        M_loader_error=EXTRUE;
        EXFAIL_OUT(ret);
    }
    /* 
        * Process message block 
        * It is up to caller to free the mem & make null
        * Note that p_block might be NULL, if message was not for our qspace,
        * thus just skip it.
        */
    if (NULL!=p_block && EXSUCCEED!=job->process_block(job->ref, &p_block, job->state, job->seqno))
    {
        NDRX_LOG(log_error, "Failed to process block! -> RAISED LOADER ERROR");
        M_loader_error=EXTRUE;
        EXFAIL_OUT(ret);
    }

out:

    if (NULL!=p_block)
    {
        NDRX_FREE((char *)p_block);
    }

    /* free up the job too */
    NDRX_FPFREE(job);
}

/**
 * Restore messages from storage device.
 * TODO: File naming include 03d so that multiple tasks per file sorts alphabetically.
 * Any active transactions get's aborted automatically.
 * @param process_block callback function to process the data block
 * @param nodeid nodeid to filter
 * @param srvid srvid to filter
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_get_blocks(int (*process_block)(char *tmxid, 
        union tmq_block **p_block, int state, int seqno), short nodeid, short srvid)
{
    int ret = EXSUCCEED;
    void *cursor=NULL;
    int n, seqno;
    int i, j;
    char *p;
    char filename[PATH_MAX+1];
    char ref[PATH_MAX+1];
    int err, tmq_err;
    int read;
    int state;
    qtran_log_t *p_tl;
    
    /* prepared & active messages are all locked
     * Also if delete command is found in active, this means that committed
     * message is locked. 
     * In case if there was concurrent tmsrv operation, then it might move file
     * from active to prepare and we have finished with prepared, and start to
     * scan active. Thus we might not see this file. Thus to solve this issue,
     * the prepared folder is scanned twice. In the second time only markings
     * are put that message is busy.
     */
    int mode[] = {
            NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED
            , NDRX_TMQ_STORAGE_LIST_MODE_PREPARED | NDRX_TMQ_STORAGE_LIST_MODE_INCL_DUPS
            , NDRX_TMQ_STORAGE_LIST_MODE_ACTIVE | NDRX_TMQ_STORAGE_LIST_MODE_INCL_DUPS};
    short msg_nodeid, msg_srvid;
    char msgid[TMMSGIDLEN];

    /* prepare thread pool for msg loading... */
    threadpool loaderpool; 
    int loaderpool_ok = EXFALSE;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! tmq_storage_get_blocks() - XA not open!");
        return XAER_RMERR;
    }

    /* Startup load thread pool... */
    if (NULL==(loaderpool = ndrx_thpool_init(ndrx_G_p_qdisk_xa_cfg->loaderpoolsize,
            NULL, NULL, NULL, 0, NULL)))
    {
        NDRX_LOG(log_error, "Failed to initialize loader thread pool (cnt: %d)!", 
                ndrx_G_p_qdisk_xa_cfg->loaderpoolsize);
        EXFAIL_OUT(ret);
    }
    loaderpool_ok=EXTRUE;
    
    for (j = 0; j < N_DIM(mode); j++)
    {
        switch (j)
        {
            case 0:
                /* committed */
                state=TMQ_TXSTATE_COMMITTED;
                break;
            case 1:
                /* prepared */
                state=TMQ_TXSTATE_PREPARED;
                break;
            case 2:
                /* active */
                state=TMQ_TXSTATE_ACTIVE;
                break;
        }

        /* open the cursor... */
        if (EXSUCCEED!=ndrx_G_tmq_storage->pf_storage_list_start(ndrx_G_tmq_storage, 
                (void **)&cursor, mode[j]))
        {
            NDRX_LOG(log_error, "Failed to start list!");
            EXFAIL_OUT(ret);
        }

        while (ndrx_G_tmq_storage->pf_storage_list_next(ndrx_G_tmq_storage
                , cursor, filename, sizeof(filename)))
        {
            /* filter nodeid & serverid from the filename... */
            if (0==j) /*  For committed folder we can detect stuff from filename */
            {
                /* early filter... */
                if (EXSUCCEED!=tmq_get_msgid_from_filename(filename, msgid))
                {
                    EXFAIL_OUT(ret);
                }
                
                tmq_msgid_get_info(msgid, &msg_nodeid, &msg_srvid);
                
                NDRX_LOG(log_info, "our nodeid/srvid %hd/%hd msg: %hd/%hd",
                        nodeid, srvid, msg_nodeid, msg_srvid);

                if (nodeid!=msg_nodeid || srvid!=msg_srvid)
                {
                    NDRX_LOG(log_warn, "our nodeid/srvid %hd/%hd msg: %hd/%hd - IGNORE",
                        nodeid, srvid, msg_nodeid, msg_srvid);
                    continue;
                }
            }

            /* get the tran */
            NDRX_LOG(log_warn, "Loading [%s] mode [%d]", filename, mode[j]);

            /* for msgid, tmxid would match the message id */
            NDRX_STRCPY_SAFE(ref, filename);
            seqno=0;

            if (j>0)
            {
                p = strrchr(ref, '-');
                
                if (NULL==p)
                {
                    NDRX_LOG(log_error, "Invalid file name [%s] missing dash!", 
                            filename);
                    userlog("Invalid file name [%s] missing dash!", 
                            filename);
                    continue;
                }
                
                *p=EXEOS;
                p++;
                
                seqno = atoi(p);
                
                /* get or create a transaction! */
                p_tl = tmq_log_start_or_get(ref);
                
                if (NULL==p_tl)
                {
                    NDRX_LOG(log_error, "Failed to get transaction object for [%s] seqno %d",
                            ref, seqno);
                    EXFAIL_OUT(ret);
                }
                
                if (2==j)
                {
                    /* mark transaction as abort only! */
                    p_tl->is_abort_only=EXTRUE;
                    p_tl->txstage=XA_TX_STAGE_ACTIVE;
                }
                else
                {
                    p_tl->txstage=XA_TX_STAGE_PREPARED;
                }
                
                /* unlock tran */
                tmq_log_unlock(p_tl);
            }

            /*
             * Submit the job...
             */
            if (!M_loader_error)
            {
                /* create job */
                ndrx_tmq_storage_read_block_job_t *job = NDRX_FPMALLOC(
                        sizeof(ndrx_tmq_storage_read_block_job_t), 0);
                if (NULL==job)
                {
                    NDRX_LOG(log_error, "Failed to fpallocate job, %d bytes!",
                            sizeof(ndrx_tmq_storage_read_block_job_t));
                    EXFAIL_OUT(ret);
                }

                job->nodeid = nodeid;
                job->srvid = srvid;
                NDRX_STRCPY_SAFE(job->ref, ref);
                job->seqno = seqno;
                job->mode = mode[j];
                job->state = state;
                job->process_block = process_block;

                /* submit job */
                if (EXSUCCEED!=ndrx_thpool_add_work(loaderpool, block_loader_th, 
                        (void *)job))
                {
                    NDRX_LOG(log_error, "Failed to submit job to block_loader_th!");
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_LOG(log_error, "Loader error detected, aborting!");
                EXFAIL_OUT(ret);
            }
            /* if all ok, process_block() has freed the p_block.. */
        }

        /* close cursor */
        if (EXSUCCEED!=ndrx_G_tmq_storage->pf_storage_list_end(ndrx_G_tmq_storage, 
                cursor))
        {
            NDRX_LOG(log_error, "Failed to end list!");
            EXFAIL_OUT(ret);
        }

        cursor=NULL;

        /* Wait all jobs are finished */
        ndrx_thpool_wait(loaderpool);

        /* check error flag */
        if (M_loader_error)
        {
            NDRX_LOG(log_error, "Loader error was set, terminating store loading!");
            EXFAIL_OUT(ret);
        }
    }
    
out:

    /* this will check for struct init or not init */
    if (NULL!=cursor)
    {
        ndrx_G_tmq_storage->pf_storage_list_end(ndrx_G_tmq_storage, 
                cursor);
    }

    /* destroy the pool... */
    if (loaderpool_ok)
    {
        /* Load shutdown jobs... */
        for (i=0; i<ndrx_G_p_qdisk_xa_cfg->loaderpoolsize; i++)
        {
            ndrx_thpool_add_work(loaderpool, tmq_thread_shutdown, NULL);
        }
        ndrx_thpool_wait(loaderpool);
        ndrx_thpool_destroy(loaderpool);
    }

    return ret;
}

/**
 * Close the thread session
 */
expublic void tmq_thread_uninit(void)
{
    NDRX_LOG(log_debug, "Into tmq_thread_uninit");
    tpclose();
    tpterm();
}

/**
 * Shutdown the thread
 * @param arg
 * @param p_finish_off
 */
expublic void tmq_thread_shutdown(void *ptr, int *p_finish_off)
{
    tmq_thread_uninit();
    
    *p_finish_off = EXTRUE;
}

/* vim: set ts=4 sw=4 et smartindent: */
