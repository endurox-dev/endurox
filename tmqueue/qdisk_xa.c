/**
 * @brief Q XA Backend
 *   Prepare the folders & subfolders
 *   So we will have following directory structure:
 *   - active
 *   - prepared
 *   - committed
 *   Initially file will be named after the XID
 *   Once it becomes committed, it is named after message_id
 *   If we have a update to Q (try counter for example)
 *   Then we issue new transaction file with different command inside, but it contains
 *   the message_id
 *   Once do the commit and it is not and message, but update file, then
 *   we update message file.
 *   If the command is delete, then we unlink the file.
 *   Once Queue record is completed (rolled back or committed) we shall send ACK
 *   of COMMAND BLOCK back to queue server via TPCALL command to QSPACE server.
 *   this will allow to synchronize internal status of the messages.
 *   Initially (active) file is named is named after XID. When doing commit, it is
 *   renamed to msg_id.
 *   If we restore the system after the restart, then committed & prepare directory is scanned.
 *   If msg is committed + there is command in prepared, then it is marked as locked.
 *   When scanning prepared directory, we shall read the msg_id from files.
 *   We shall support multiple TMQs running over single Q space, but they each should,
 *   manage it's own set of queued messages.
 *   The queued file names shall contain [QSPACE].[SERVERID].[MSG_ID|XID]
 *   To post the updates to proper TMQ, it should advertise QSPACE.[SERVER_ID]
 *   In case of Active-Active env, servers only on node shall be run.
 *   On the other node we could put in standby qspace+server_id on the same shared dir.
 * ----------
 *   TODO: needs to consider to splitting this into two:
 *   1) generic XA switch communicating with Qspace via minicall
 *   2) Qspace server which servers as a backend for XA commands.
 *
 * @file qdisk_xa.c
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
/* #define TXN_TRACE */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic char ndrx_G_qspace[XATMI_SERVICE_NAME_LENGTH+1];   /**< Name of the queue space  */
expublic char ndrx_G_qspacesvc[XATMI_SERVICE_NAME_LENGTH+1];/**< real service name      */

/* default storage engine: */
expublic ndrx_tmq_storage_t *ndrx_G_tmq_storage = &ndrx_G_tmq_store_files;
expublic ndrx_tmq_qdisk_xa_cfg_t *ndrx_G_p_qdisk_xa_cfg;
/*---------------------------Statics------------------------------------*/
exprivate int volatile M_folder_set = EXFALSE;   /**< init flag                     */
exprivate MUTEX_LOCKDECL(M_folder_lock); /**< protect against race codition during path make*/
exprivate MUTEX_LOCKDECL(M_init);   /**< init lock      */

exprivate int M_is_tmqueue = EXFALSE;   /**< is this process a tmqueue ?        */
/*---------------------------Prototypes---------------------------------*/

expublic int xa_open_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

expublic int xa_open_entry_dyn(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_dyn(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_end_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry_dyn(XID *xid, int rmid, long flags);
expublic int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags);

expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);

exprivate int read_tx_from_file(char *fname, char *block, int len, int *err,
        int *tmq_err);

exprivate int xa_rollback_entry_tmq(char *tmxid, long flags);
exprivate int xa_prepare_entry_tmq(char *tmxid, long flags);
exprivate int xa_commit_entry_tmq(char *tmxid, long flags);
exprivate void tmq_chkdisk_th(void *ptr, int *p_finish_off);

struct xa_switch_t ndrxqstatsw = 
{ 
    .name = "ndrxqstatsw",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

struct xa_switch_t ndrxqdynsw = 
{ 
    .name = "ndrxqdynsw",
    .flags = TMREGISTER,
    .version = 0,
    .xa_open_entry = xa_open_entry_dyn,
    .xa_close_entry = xa_close_entry_dyn,
    .xa_start_entry = xa_start_entry_dyn,
    .xa_end_entry = xa_end_entry_dyn,
    .xa_rollback_entry = xa_rollback_entry_dyn,
    .xa_prepare_entry = xa_prepare_entry_dyn,
    .xa_commit_entry = xa_commit_entry_dyn,
    .xa_recover_entry = xa_recover_entry_dyn,
    .xa_forget_entry = xa_forget_entry_dyn,
    .xa_complete_entry = xa_complete_entry_dyn
};

/**
 * Mark the current instance as part or not as part of tmqueue
 * @param qdisk_xa_cfg ptr to XA library configuration structure
 */
expublic void tmq_set_tmqueue(
    ndrx_tmq_qdisk_xa_cfg_t * qdisk_xa_cfg
)
{
    ndrx_G_p_qdisk_xa_cfg = qdisk_xa_cfg;
    
    if (NULL!=ndrx_G_p_qdisk_xa_cfg)
    {
        M_is_tmqueue = EXTRUE;
    }
    else
    {
        M_is_tmqueue = EXFALSE;
        NDRX_LOG(log_debug, "tmq_set_tmqueue: not a tmqueue (external XA client)");
    }

    if (NULL!=ndrx_G_p_qdisk_xa_cfg)
    {
        NDRX_LOG(log_debug, "qdisk_xa config: M_is_tmqueue=%d "
            "pf_tmq_setup_cmdheader_dum=%p pf_tmq_dum_add=%p pf_tmq_unlock_msg=%p "
            "pf_tmq_msgid_exists=%p pf_tpexit=%p",
            M_is_tmqueue, ndrx_G_p_qdisk_xa_cfg->pf_tmq_setup_cmdheader_dum,
            ndrx_G_p_qdisk_xa_cfg->pf_tmq_dum_add, ndrx_G_p_qdisk_xa_cfg->pf_tmq_unlock_msg,
            ndrx_G_p_qdisk_xa_cfg->pf_tmq_msgid_exists, ndrx_G_p_qdisk_xa_cfg->pf_tpexit);

        /* Return some functions from our scope back */
        ndrx_G_p_qdisk_xa_cfg->pf_tmq_chkdisk_th = tmq_chkdisk_th;
    }
}

/**
 * Set filename base
 * @param xid
 * @param rmid
 * @return 
 */
exprivate char *set_filename_base(XID *xid)
{
    atmi_xa_serialize_xid(xid, G_atmi_tls->qdisk_tls->filename_base);    
    return G_atmi_tls->qdisk_tls->filename_base;
}

/**
 * Set base xid
 * @param tmxid xid string
 * @return ptr to buffer
 */
exprivate char *set_filename_base_tmxid(char *tmxid)
{
    /* may overlap in dynamic reg mode */
    if (tmxid!=(char *)G_atmi_tls->qdisk_tls->filename_base)
    {
        NDRX_STRCPY_SAFE(G_atmi_tls->qdisk_tls->filename_base, tmxid);
    }

    return G_atmi_tls->qdisk_tls->filename_base;
}

/**
 * Configure storage engine
 * @param fname
 * @return EXTRUE/EXFALSE
 */
exprivate int configure_storage_engine(void)
{
    int ret = EXSUCCEED;

    if (NULL!=ndrx_G_plugins.p_ndrx_tms_store)
    {
        ndrx_G_tmq_storage = ndrx_G_plugins.p_ndrx_tms_store;
    }

    if (0!=strncmp(ndrx_G_tmq_storage->magic, NDRX_TMQ_STOREIF_MAGIC, NDRX_TMQ_STOREIF_MAGIC_LEN))
    {
        NDRX_LOG(log_error, "ERROR ! Invalid tmq data store magic: expected [%s] got [%c%c%c%c]!",
            NDRX_TMQ_STOREIF_MAGIC,
            ndrx_G_tmq_storage->magic[0],
            ndrx_G_tmq_storage->magic[1],
            ndrx_G_tmq_storage->magic[2],
            ndrx_G_tmq_storage->magic[3]);
        EXFAIL_OUT(ret);
    }

    /* validate the version */
    if (ndrx_G_tmq_storage->sw_version != NDRX_TMQ_STOREIF_VERSION)
    {
        NDRX_LOG(log_error, "ERROR ! Invalid tmq data store version: expected [%d] got [%d]!",
                NDRX_TMQ_STOREIF_VERSION, ndrx_G_tmq_storage->sw_version);
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_warn, "Transactional queue storage engine: [%s], version: %d",
                ndrx_G_tmq_storage->name, ndrx_G_tmq_storage->sw_version);

out:
    return ret;
}

/**
 * Open API.
 * Now keeps the settings of the queue space too.
 * @param sw Current switch
 * @param xa_info New format: dir="/path_to_dir",qspace='SAMPLESPACE' (escaped)
 * @param rmid according to XA spec
 * @param flags according to XA spec
 * @return XA_ return codes
 */
expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = XA_OK, err, i;
    static int first = EXTRUE;
    char *info_tmp = NULL;
    char *p, *val;
    
    /* mark that suspend no required by this resource... */
    if (first)
    {
        MUTEX_LOCK_V(M_init);
        if (first)
        {
            ndrx_xa_nosuspend(EXTRUE);
            first=EXFALSE;

            /*  only for tmqueue version  */
            if (M_is_tmqueue
                && EXSUCCEED!=configure_storage_engine())
            {
                MUTEX_UNLOCK_V(M_init);
                EXFAIL_OUT(ret);
            }
        }
        MUTEX_UNLOCK_V(M_init);
    }
    
    if (G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_warn, "xa_open_entry() - already open!");
        return XA_OK;
    }
    
    G_atmi_tls->qdisk_tls=NDRX_FPMALLOC(sizeof(ndrx_qdisk_tls_t), 0);
            
    if (NULL==G_atmi_tls->qdisk_tls)
    {
        int err=errno;
        
        NDRX_LOG(log_warn, "xa_open_entry() - failed to malloc TLS data: %s",
                strerror(err));
        return XAER_RMERR;
    }

    G_atmi_tls->qdisk_tls->is_reg=EXFALSE;
    G_atmi_tls->qdisk_tls->filename_base[0]=EXEOS;
    G_atmi_tls->qdisk_tls->filename_active[0]=EXEOS;
    G_atmi_tls->qdisk_tls->filename_prepared[0]=EXEOS;

    ndrx_growlist_init(&G_atmi_tls->qdisk_tls->recover_namelist, 256, sizeof(XID));
    
    G_atmi_tls->qdisk_tls->recover_open=EXFALSE;
    G_atmi_tls->qdisk_tls->recover_i=0;
            
    G_atmi_tls->qdisk_is_open = EXTRUE;
    G_atmi_tls->qdisk_rmid = rmid;
    
#define UNLOCK_OUT MUTEX_UNLOCK_V(M_folder_lock);\
                ret=XAER_RMERR;\
                goto out;
                
    /* Load only once? */
    if (!M_folder_set)
    {
        /* LOCK & Check */
        MUTEX_LOCK_V(M_folder_lock);
        
        if (!M_folder_set)
        {
            info_tmp = NDRX_STRDUP(xa_info);
            
            if (NULL==info_tmp)
            {
                err=errno;
                NDRX_LOG(log_error, "Failed to strdup: %s", strerror(err));
                userlog("Failed to strdup: %s", strerror(err));
                UNLOCK_OUT;
            }
            
#define ARGS_DELIM      ","
#define ARGS_QUOTE      "'\""
#define ARG_DIR         "datadir"
#define ARG_QSPACE      "qspace"
            
            /* preserve values in quotes... as atomic values */
            for (p = ndrx_strtokblk ( info_tmp, ARGS_DELIM, ARGS_QUOTE), i=0; 
                    NULL!=p;
                    p = ndrx_strtokblk (NULL, ARGS_DELIM, ARGS_QUOTE), i++)
            {
                if (NULL!=(val = strchr(p, '=')))
                {
                    *val = EXEOS;
                    val++;
                }
                
                /* set data dir (only for tmq part) */
                if (0==strcmp(ARG_DIR, p) && M_is_tmqueue)
                {
                    NDRX_STRCPY_SAFE(ndrx_G_p_qdisk_xa_cfg->data_folder, val);
                    ret = ndrx_G_tmq_storage->pf_storage_init(ndrx_G_tmq_storage, 
                        ndrx_G_p_qdisk_xa_cfg);
                        
                    if (EXSUCCEED!=ret)
                    {
                        NDRX_LOG(log_error, "Failed to prepare data directory [%s]", val);
                        UNLOCK_OUT;
                    }   
                }
                else if (0==strcmp(ARG_QSPACE, p))
                {
                    NDRX_STRCPY_SAFE(ndrx_G_qspace, val);
                }
                /* all other ignored... */
            }
            
            if (EXEOS==ndrx_G_qspace[0])
            {
                NDRX_LOG(log_error, "[%s] setting not found in open string!", ARG_QSPACE);
                UNLOCK_OUT;
            }
            
            if (NULL!=ndrx_G_p_qdisk_xa_cfg && EXEOS==ndrx_G_p_qdisk_xa_cfg->data_folder[0])
            {
                NDRX_LOG(log_error, "[%s] setting not found in open string!", ARG_DIR);
                UNLOCK_OUT;
            }
            
            snprintf(ndrx_G_qspacesvc, sizeof(ndrx_G_qspacesvc),
                    NDRX_SVC_QSPACE, ndrx_G_qspace);

            NDRX_LOG(log_debug, "Qspace set to: [%s]", ndrx_G_qspace);
            NDRX_LOG(log_debug, "Qspace svc set to: [%s]", ndrx_G_qspacesvc);
            M_folder_set=EXTRUE;
            
        }
        MUTEX_UNLOCK_V(M_folder_lock);
        
    }
out:
    
    if (NULL!=info_tmp)
    {
        NDRX_FREE(info_tmp);
    }
    return ret;
}
/**
 * Close entry
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    NDRX_LOG(log_info, "xa_close_entry() called");
    
    if (NULL!=G_atmi_tls->qdisk_tls)
    {
        NDRX_FPFREE(G_atmi_tls->qdisk_tls);
        G_atmi_tls->qdisk_tls=NULL;
    }
    
    G_atmi_tls->qdisk_is_open = EXFALSE;
    
    return XA_OK;
}

/**
 * TMQ queue internal version of transaction starting
 * @return XA error code
 */
exprivate int xa_start_entry_tmq(char *tmxid, long flags)
{
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;
    
    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);

    if ( (flags & TMJOIN) || (flags & TMRESUME) )
    {
        if (NULL==p_tl && !locke)
        {
            NDRX_LOG(log_error, "Xid [%s] TMJOIN/TMRESUME but tran not found",
                    tmxid);
            ret = XAER_NOTA;
            goto out;
        }

        NDRX_LOG(log_info, "Xid [%s] join OK", tmxid);
    }
    else
    {
        /* this is new tran */
        if (NULL!=p_tl || locke)
        {
            NDRX_LOG(log_error, "Cannot start Xid [%s] already in progress",
                    tmxid);
            ret = XAER_DUPID;
            goto out;
        }
        else
        {
            
            if (EXSUCCEED!=tmq_log_start(tmxid))
            {
                NDRX_LOG(log_error, "Failed to start transaction for tmxid [%s]",
                        tmxid);
                ret = XAER_RMERR;
                goto out;
            }
            NDRX_LOG(log_info, "Queue transaction Xid [%s] started OK", tmxid);
        }
    }
    
out:
    
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}

/**
 * Serve the transaction call
 * @param cd call descriptor
 * @param p_ub UBF buffer
 * @param cmd tmq command code
 * @return XA error code
 */
expublic int ndrx_xa_qminiservce(int cd, UBFH *p_ub, char cmd)
{
    long ret = XA_OK;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    short nodeid, nodeid_loc;
    
    BFLDLEN len = sizeof(tmxid);
    
    /* 
     * ensure that we are recving requests only from our transaction
     * manager.
     * Recover trasnsactoins does not have xid on input.
     */
    if (TMQ_CMD_RECOVERTRANS!=cmd && EXSUCCEED!=Bget(p_ub, TMXID, 0, tmxid, &len))
    {
        NDRX_LOG(log_error, "Failed to get TMXID!");
        ret = XAER_INVAL;
        goto out;
    }
    
    if (EXSUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&nodeid, 0L))
    {
        NDRX_LOG(log_error, "Failed to get TMNODEID!");
        ret = XAER_INVAL;
        goto out;
    }

    if (G_atmi_env.procgrp_no && ndrx_sg_is_singleton(G_atmi_env.procgrp_no)
        /* and start transaction from any node */
        && TMQ_CMD_STARTTRAN!=cmd)
    {
        /* if we are in singleton group mode, ensure
         * that our nodeid matches with caller nodeid. To avoid any dead
         * communication with failed node.
         */
        nodeid_loc=tpgetnodeid();
        if (nodeid!=nodeid_loc)
        {
            NDRX_LOG(log_error, "ndrx_xa_qminiservce: singleton group tmqueue+tmsrv must be on "
			"the same node but our node is %hd rcvd call from %hd - XAER_RMFAIL",
			nodeid_loc, nodeid);
            userlog("ndrx_xa_qminiservce: singleton group tmqueue+tmsrv must be on "
			"the same node but our node is %hd rcvd call from %hd - XAER_RMFAIL",
			nodeid_loc, nodeid);
            ret = XAER_RMFAIL;
            goto out;
        }
    }

    switch (cmd)
    {
        case TMQ_CMD_STARTTRAN:
            ret = xa_start_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_ABORTTRAN:
            ret = xa_rollback_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_PREPARETRAN:
            ret = xa_prepare_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_COMMITRAN:
            ret = xa_commit_entry_tmq(tmxid, 0);
            break;
        case TMQ_CMD_RECOVERTRANS:
            ret = xa_recover_entry_tmq(cd, p_ub, 0);
            break;
        default:
            NDRX_LOG(log_error, "Invalid command code [%c]", cmd);
            ret = XAER_INVAL;
            break;
    }
    
out:
            
    NDRX_LOG(log_info, "returns XA status: %d", ret);
    
    if (EXSUCCEED!=Bchg(p_ub, TMTXRMERRCODE, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_error, "Failed to setup TMTXRMERRCODE: %s", 
                Bstrerror(Berror));
        ret = XAER_RMERR;
    }

    return ret;
}


/**
 * Set the file name of transaciton file (the base)
 * If exists and join - ok. Otherwise fail.
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_start_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    /* if we are tmq -> for join, perform lookup.
     * for start check, if tran exists -> error, if not exists, start
     * if doing from other process call the tmqueue for start/join (just chk)
     */
    
    if (M_is_tmqueue)
    {
        ret = xa_start_entry_tmq(tmxid, flags);
    }
    else if (! ( (flags & TMJOIN) || (flags & TMRESUME) ))
    {
        /* in case if no join from external process
         * request the tmsrv to start the transaction
         * also we are interested in return code
         */
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_STARTTRAN);
    }
    
out:

    return ret;
}

/**
 * XA call end entry. Nothing special to do here.
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    if (G_atmi_tls->qdisk_tls->is_reg)
    {
        if (EXSUCCEED!=ax_unreg(rmid, 0))
        {
            NDRX_LOG(log_error, "ERROR! xa_end_entry() - "
                    "ax_unreg() fail!");
            return XAER_RMERR;
        }
        
        G_atmi_tls->qdisk_tls->is_reg = EXFALSE;
    }
    
    /* no special processing for tran */
    
out:

    return XA_OK;
}

/**
 * Internal TMQ version of rollback
 * @param tmxid xid serialized
 * @param flags any flags
 * @return  XA error code
 */
exprivate int xa_rollback_entry_tmq(char *tmxid, long flags)
{
    char *fn = "xa_rollback_entry_tmq";
    int ret=XA_OK;
    
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        ret=XAER_RMERR;
        goto out;
    }
    
    set_filename_base_tmxid(tmxid);

    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            ret=XAER_RMFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_info, "Q transaction [%s] does not exists", tmxid);

            /* TODO: verify the disk... (only in case) sinelgrp ... ?
             */
            ret=ndrx_G_tmq_storage->pf_storage_prep_exists(ndrx_G_tmq_storage, tmxid);

            if (EXTRUE==ret)
            {
                /*
                 * it really failure here. transaction must not exists
                 * on the disk if log does not exists.
                 * possible concurrent run.
                 * Restart now...
                 */
                NDRX_LOG(log_error, "(rollback) Integrity problem, transaction [%s] "
                    "exists on disk, but not in mem-log - restarting", tmxid);
                userlog("(rollback) Integrity problem, transaction [%s] "
                    "exists on disk, but not in mem-log - restarting", tmxid);
                ndrx_G_p_qdisk_xa_cfg->pf_tpexit();
                ret=XAER_RMFAIL;
                goto out;
            }
            else if (EXFAIL==ret)
            {
                ret=XAER_RMFAIL;
                goto out;
            }
            else
            {
                ret=XAER_NOTA;
                goto out;
            }
        }
    }
    
    p_tl->txstage = XA_TX_STAGE_ABORTING;
    p_tl->is_abort_only=EXTRUE;

    /* call rollback commands */
    if (XA_OK!=ndrx_G_tmq_storage->pf_storage_rollback_cmds(ndrx_G_tmq_storage, p_tl))
    {
        NDRX_LOG(log_error, "Failed to rollback commands for tmxid [%s]", tmxid);
        ret = XAER_RMFAIL;
        goto out;
    }
    
#ifdef TXN_TRACE
    userlog("ABORT: tmxid=[%s] seqno=%d", tmxid, p_tl->seqno);
    NDRX_LOG(log_error, "ABORT: tmxid=[%s] seqno=%d", tmxid, p_tl->seqno);
#endif
    
    if (NULL!=p_tl->cmds)
    {
        NDRX_LOG(log_error, "Failed to abort Q transaction [%s] -> commands exists",
                tmxid);
        ret=XAER_RMERR;
        goto out;
    }
    
    /* delete message from log */
    tmq_remove_logfree(p_tl, EXTRUE);
out:
    return ret;
}

/**
 * Process local or remote call
 * @return 
 */
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_rollback_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_rollback_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_ABORTTRAN);
    }
    
out:

    return ret;
}

/**
 * Prepare transactions (according to TL log)
 * @return XA status
 */
exprivate int xa_prepare_entry_tmq(char *tmxid, long flags)
{
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;
    int did_move=EXFALSE;
    qtran_log_cmd_t *el, *elt;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry_tmq() - XA not open!");
        ret=XAER_RMERR;
        goto out;
    }
    
    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            ret=XAER_RMFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Q transaction [%s] does not exists", tmxid);
            ret=XAER_NOTA;
            goto out;
        }
    }
    
    if (p_tl->is_abort_only)
    {
        NDRX_LOG(log_error, "Q transaction [%s] is abort only!", tmxid);
        ret = XAER_RMERR;
        goto out;
    }
    
    if (XA_TX_STAGE_ACTIVE!=p_tl->txstage)
    {
        NDRX_LOG(log_error, "Q transaction [%s] expected stage %hd (active) got %hd",
                tmxid, XA_TX_STAGE_ACTIVE, p_tl->txstage);
        
        ret = XAER_RMERR;
        p_tl->is_abort_only=EXTRUE;
        goto out;
    }

    /* check lock & sequence */
    tmq_log_checkpointseq(p_tl);
    
    p_tl->txstage = XA_TX_STAGE_PREPARING;
    
    /* TODO:  
     * Read active records -> as normal. If file cannot be read, create dummy
     * record for given tran. And thus if having active record or prepared+active 
     * at startup will automatically mean that transaction is abort-only. 
     * 
     * At the startup, all abort-only transaction shall be aborted
     * 
     * If doing prepare with empty RO transaction, inject dummy command prior
     * (transaction marking) so that at restart we have transaction log and
     * thus we can respond commit OK, thought might be optional as TMSRV
     * prepared resources with XAER_NOTA transaction would assume that committed OK
     */
    
    if (NULL==p_tl->cmds)
    {
        /* do not write the file, as we will use existing on disk */
        if (EXSUCCEED!=ndrx_G_p_qdisk_xa_cfg->pf_tmq_dum_add(p_tl->tmxid))
        {
            ret = XAER_RMERR;
            p_tl->is_abort_only=EXTRUE;
            goto out;
        }
        
        NDRX_LOG(log_debug, "Dummy marker added");
        
    }
    
    /* call rollback commands */
    if (XA_OK!=(ret=ndrx_G_tmq_storage->pf_storage_prepare_cmds(ndrx_G_tmq_storage, p_tl)))
    {
        NDRX_LOG(log_error, "Failed to prepare commands for tmxid [%s]", tmxid);
        goto out;
    }
    
    /* If all OK, switch transaction to prepared */
    p_tl->txstage = XA_TX_STAGE_PREPARED;
    
out:
            
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}

/**
 * XA prepare entry call
 * Basically we move all messages for active to prepared folder (still named by
 * xid_str)
 * 
 * @return 
 */
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_prepare_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_prepare_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_PREPARETRAN);
    }
    
out:

    return ret;
}

/**
 * Commit the transaction.
 * Move messages from prepared folder to files names with msgid
 * @param tmxid serialized branch xid
 * @param flags not used
 * @return XA error code
 */
exprivate int xa_commit_entry_tmq(char *tmxid, long flags)
{
    int locke = EXFALSE;
    qtran_log_t * p_tl = NULL;
    int ret = XA_OK;

    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }

    set_filename_base_tmxid(tmxid);
    
    /* Firstly try to locate the tran */
    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, &locke);
    
    if (NULL==p_tl)
    {
        if (locke)
        {
            NDRX_LOG(log_error, "Q transaction [%s] locked", tmxid);
            ret=XAER_RMFAIL;
            goto out;
        }
        else
        {
            NDRX_LOG(log_error, "Q transaction [%s] does not exists (in mem log)", tmxid);
            
            /* TODO: perform checks only, if using singleton group. */
            ret=ndrx_G_tmq_storage->pf_storage_prep_exists(ndrx_G_tmq_storage, tmxid);

            if (EXTRUE==ret)
            {
                /* it really failure here. transaction must not exists
                 * on the disk if log does not exists.
                 * possible concurrent run.
                 * Restart now...
                 */
               NDRX_LOG(log_error, "(commit) Integrity problem, transaction [%s] "
                    "exists on disk, but not in mem-log - restarting", tmxid);
                userlog("(commit) Integrity problem, transaction [%s] "
                    "exists on disk, but not in mem-log - restarting", tmxid);

                ret=XAER_RMFAIL;

                ndrx_G_p_qdisk_xa_cfg->pf_tpexit();
                goto out;
            }
            else if (EXFAIL==ret)
            {
                ret=XAER_RMFAIL;
                goto out;
            }
            else
            {
                ret=XAER_NOTA;
                goto out;
            }
        }
    }
    
    if (p_tl->is_abort_only)
    {
        NDRX_LOG(log_error, "Q transaction [%s] is abort only!", tmxid);
        ret = XAER_RMERR;
        goto out;
    }
    
    /* allow to retry... */
    if (XA_TX_STAGE_PREPARED!=p_tl->txstage &&
        XA_TX_STAGE_COMMITTING!=p_tl->txstage)
    {
        NDRX_LOG(log_error, "Q transaction [%s] expected stage %hd (prepared) or %hd (committing) got %hd",
                tmxid, XA_TX_STAGE_PREPARED, XA_TX_STAGE_COMMITTING, p_tl->txstage);
        
        ret = XAER_RMERR;
        p_tl->is_abort_only=EXTRUE;
        goto out;
    }
    
    p_tl->txstage = XA_TX_STAGE_COMMITTING;
    
    /* process command by command to stage to prepared ... */
#ifdef TXN_TRACE
    /* Enable these commands (and for ABORT: too), to trace down
     * transaction loss. I.e. defective transaction is in case,
     * if we see in the log COMMIT: and ABORT:. There
     * shall be only one of these, not both
     *
     * To post-process, say run-suspend.sh of test 104:
     * extract the xids:
     * $ grep ":COMMIT:" ULOG.20231028  | cut -d '[' -f 2 | cut -d ']' -f1 > commit.xids
     *
     * verify that there aren't any aborts:
     * $ cat commit.xids | xargs -i grep {} ULOG.20231028 | grep --color=auto ABORT
     *
     */
    userlog("COMMIT: tmxid=[%s] seqno=%d", tmxid, p_tl->seqno);
    NDRX_LOG(log_error, "COMMIT: tmxid=[%s] seqno=%d", tmxid, p_tl->seqno);
#endif

    /* commit the commands: */
    if ((ret=ndrx_G_tmq_storage->pf_storage_commit_cmds(ndrx_G_tmq_storage, p_tl)))
    {
        NDRX_LOG(log_error, "Failed to commit commands for tmxid [%s]", tmxid);
        goto out;
    }

    /* delete message from log */
    tmq_remove_logfree(p_tl, EXTRUE);
    p_tl = NULL;
    
    NDRX_LOG(log_info, "tmxid [%s] all sequences committed ok", tmxid);
    
out:
    NDRX_LOG(log_info, "Commit returns %d", ret);

    /* unlock if tran is still alive */
    if (NULL!=p_tl && !locke)
    {
        tmq_log_unlock(p_tl);
    }

    return ret;
}

/**
 * Local or remove transaction commit
 * @return XA error 
 */
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    char *tmxid;
    int ret = XA_OK;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_commit_entry() - XA not open!");
        ret = XAER_RMERR;
        goto out;
    }
    
    tmxid = set_filename_base(xid);
    
    if (M_is_tmqueue)
    {
        ret = xa_commit_entry_tmq(tmxid, flags);
    }
    else
    {
        ret=ndrx_xa_qminicall(tmxid, TMQ_CMD_COMMITRAN);
    }
    
out:

    return ret;
}

/** continue with dirent free */
#define RECOVER_CONTINUE \
            continue;\
   
/** Close cursor. */
#define RECOVER_CLOSE_CURSOR \
        /* reset any stuff left open from previous scan... */\
        ndrx_growlist_free(&G_atmi_tls->qdisk_tls->recover_namelist);\
        G_atmi_tls->qdisk_tls->recover_open=EXFALSE;\
        G_atmi_tls->qdisk_tls->recover_i=0;

/**
 * In conversational mode, list the prepared transations.
 * Doing this way, so that switch user (in case if it is not tmq)
 * does not have to deal with the internals of the storage.
 * @param cd XATMI service call descriptor
 * @param p_ub UBF buffer (with request + used for transporting listings)
 * @param flags call flags, currenty 0
 * @return 
 */
expublic int xa_recover_entry_tmq(int cd, UBFH *p_ub, long flags)
{
    int ret = XA_OK;
    int err, i, cnt;
    long revent;
    int do_restart;
    void *cursor=NULL;
    char fname[PATH_MAX+1];

    ret = ndrx_G_tmq_storage->pf_storage_list_start(ndrx_G_tmq_storage, &cursor,
            NDRX_TMQ_STORAGE_LIST_MODE_PREPARED);

    if (XA_OK!=ret)
    {
        NDRX_LOG(log_error, "Failed to scan q directory prepared direcotry");
        ret=XAER_RMERR;
        goto out;
    }

    /* NOTE: duplciates are not expected to be recieved from list_next */
    while (EXTRUE==(ret=(ndrx_G_tmq_storage->pf_storage_list_next(ndrx_G_tmq_storage, cursor, fname, sizeof(fname)))))
    {
        NDRX_LOG(log_debug, "XID [%s] recovered", fname);

        /* 
         * send back to caller..., in case if buffer, full
         * send to the caller & flush it...
         */
        do
        {
            do_restart=EXFALSE;
            if (EXSUCCEED!=Badd(p_ub, TMXID, fname, 0))
            {
                if (BNOSPACE==Berror)
                {
                    NDRX_LOG(log_debug, "Buffer full - sending to caller");
                    /* send stuff & reset buffer.. */
                    if (EXSUCCEED!=tpsend(cd, (char *)p_ub, 0, 0, &revent))
                    {
                        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                    tpstrerror(tperrno), revent);
                        ret=XAER_RMFAIL;
                        goto out;
                    }

                    if (EXSUCCEED!=Bdelall(p_ub, TMXID))
                    {
                        NDRX_LOG(log_error, "Failed to reset call buffer: %s",
                                    Bstrerror(Berror));
                        ret=XAER_RMFAIL;
                        goto out;
                    }

                    do_restart=EXTRUE;
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to add TMXID [%s] to the buffer: %s",
                                    fname, Bstrerror(Berror));
                    ret=XAER_RMFAIL;
                    goto out;
                }
            }
        } while (do_restart);
    }

    if (XA_OK!=ret)
    {
        NDRX_LOG(log_error, "pf_storage_list_next() failed: %s", ret);
        goto out;
    }

    /* NOTE: that some porition of the data would be received with the tpreturn! */
out:

    if (NULL!=cursor)
    {
        ndrx_G_tmq_storage->pf_storage_list_end(ndrx_G_tmq_storage, cursor);
    }

    NDRX_LOG(log_info, "%s returns %ld", __func__, ret);

    return ret;
}

/**
 * Lists currently prepared transactions
 * NOTE! Currently messages does not store RMID.
 * This means that one directory cannot shared between several 
 * Might have race condition with in-progress prepare. But I think it is
 * is not a big problem. As anyway we normally will run abort or commit.
 * And commit will require that all is prepared.
 * ----
 * This collects the prepard transactions, and reports in conversational mode
 * back the list to the caller.
 * @param sw XA switch
 * @param xid where to unload the xids
 * @param count positions avaialble
 * @param rmid RM id
 * @param flags
 * @return error or nr Xids recovered
 */
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    int ret = XA_OK;
    int err;
    XID xtmp, *p_xid;
    int current_unload_pos=0; /* where to unload the stuff.. */
    int cd = EXFAIL;
    UBFH *p_ub = NULL;
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_recover_entry() - XA not open!");
        ret=XAER_PROTO;
        goto out;
    }
    
    if (NULL==xid && count >0)
    {
        NDRX_LOG(log_error, "ERROR: xid is NULL and count >0");
        ret=XAER_INVAL;
        goto out;
    }
    
    if (!G_atmi_tls->qdisk_tls->recover_open && ! (flags & TMSTARTRSCAN))
    {
        NDRX_LOG(log_error, "ERROR: Scan not open and TMSTARTRSCAN not specified");
        ret=XAER_INVAL;
        goto out;
    }
    
    /* close the scan */
    if (flags & TMSTARTRSCAN)
    {
        /* if was not open, no problem.. */
        RECOVER_CLOSE_CURSOR;
    }
    
    /* start the scan if requested so ...  */
    if (flags & TMSTARTRSCAN)
    {
        long len, revent=0;

        /* Load into memory all prepared XIDs, so that
         * engine can list them (with the pages)
         */
        cd = ndrx_xa_qminiconnect(TMQ_CMD_RECOVERTRANS, &p_ub, TPRECVONLY);

        if (EXFAIL==cd)
        {
            NDRX_LOG(log_error, "Failed to tpconnect() for prepared transaction listing");
            ret=XAER_RMFAIL;
            goto out;
        }

        G_atmi_tls->qdisk_tls->recover_open=EXTRUE;
        
        /* fetch the list... 
         * cd is set to EXFAIL in last loop (when server did return)
         */
        while (EXFAIL!=cd && (EXSUCCEED==tprecv(cd, (char **)&p_ub, &len, 0, &revent) 
            || TPEV_SVCSUCC==revent))
        {
            BFLDOCC occ = Boccur(p_ub, TMXID);
            char *p;
            int i;

            /* no close... */
            if (TPEV_SVCSUCC==revent)
            {
                cd=EXFAIL;
            }

            for (i=0; i<occ; i++)
            {
                p = Bfind(p_ub, TMXID, i, NULL);

                if (NULL==p)
                {
                    NDRX_LOG(log_error, "Failed to get TMXID[%d]: %s",
                            i, Bstrerror(Berror));
                    ret=XAER_RMFAIL;
                    goto out;
                }

                NDRX_LOG(log_debug, "Fetch xid_str[ %s] to position %d", p, G_atmi_tls->qdisk_tls->recover_i);

                if (NULL==atmi_xa_deserialize_xid((unsigned char *)p, &xtmp))
                {
                    NDRX_LOG(log_error, "Failed to deserialize xid: [%s] - skip", p);
                    continue;
                }

                if (EXSUCCEED!=ndrx_growlist_append(&G_atmi_tls->qdisk_tls->recover_namelist, 
                        &xtmp))
                {
                    NDRX_LOG(log_error, "Failed to add xid_str [%s] to growlist - OOM?", p);
                    ret=XAER_RMFAIL;
                    goto out;
                }
                G_atmi_tls->qdisk_tls->recover_i++;
            } /* for occ in buffer */
        }

        if (TPEV_SVCSUCC!=revent)
        {
            NDRX_LOG(log_error, "Failed to tprecv(cd=%d) for prepared transaction listing revent=%ld: %s", 
                cd, revent, tpstrerror(tperrno));
            ret=XAER_RMFAIL;
            goto out;
        }
    } /* scan start... */

    NDRX_LOG(log_info, "Nr of prepared transactions (left in cursor): %d", 
        G_atmi_tls->qdisk_tls->recover_i);
    
    /* nothing to return */
    if (0==G_atmi_tls->qdisk_tls->recover_i)
    {
        ret=0;
        goto out;
    }
    
    p_xid=(XID *)G_atmi_tls->qdisk_tls->recover_namelist.mem;
    /** start to unload xids, we got to match the same names */
    while ((count - current_unload_pos) > 0 && 
            G_atmi_tls->qdisk_tls->recover_i--)
    {
        
        /* Okey unload the xid finally */
        memcpy(&xid[current_unload_pos]
            , &p_xid[G_atmi_tls->qdisk_tls->recover_i]
            , sizeof(XID));

        /* ensure that have a log entry 
         * probably will not work... as all stuff must go through
         * the Qspace...?
         * however we can choose not to do this. As xa_rollback will bypass these and 
         * would be cleaned up at the next boot.
         * if going for commit, then tmq will reboot, as no tlog, but file exists on disk.
         */
#if 0
        if (EXSUCCEED!=tmq_check_prepared(fname, fname_full))
        {
            /* having some issues with leaks: */
            NDRX_FREE(G_atmi_tls->qdisk_tls->recover_namelist[G_atmi_tls->qdisk_tls->recover_i]);
            ret=XAER_RMFAIL;
            goto out;
        }
#endif
        
        /* maps against strings fetched in tprecv() */
        NDRX_LOG(log_debug, "Unload XID position %d", G_atmi_tls->qdisk_tls->recover_i);
        ret++;
        current_unload_pos++;
        RECOVER_CONTINUE;
    }
    
out:

    /* terminate the scan */
    NDRX_LOG(log_info, "recover: count=%ld, ret=%d, flags=%ld", 
        count, ret, flags);

    if (    ret>=0 
            && ( (flags & TMENDRSCAN) || ret < count)
            && G_atmi_tls->qdisk_tls->recover_open
            )
    {
        NDRX_LOG(log_info, "recover: closing cursor");
        
        /* if was not open, no problem.. */
        RECOVER_CLOSE_CURSOR;
    }

    /* terminate conv if left open... */
    if (EXFAIL!=cd)
    {
        tpdiscon(cd);
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret; /* no transactions found */
}

/**
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_forget_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "ERROR! xa_forget_entry() - not using!!");
    return XAER_RMERR;
}

/**
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param handle
 * @param retval
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
{
    if (!G_atmi_tls->qdisk_is_open)
    {
        NDRX_LOG(log_error, "ERROR! xa_complete_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "ERROR! xa_complete_entry() - not using!!");
    return XAER_RMERR;
}


/* Static entries */
expublic int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxqstatsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxqstatsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxqstatsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxqstatsw, xid, rmid, flags);
}
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxqstatsw, handle, retval, rmid, flags);
}

/* Dynamic entries */
expublic int xa_open_entry_dyn( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxqdynsw, xa_info, rmid, flags);
}
expublic int xa_close_entry_dyn(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxqdynsw, xa_info, rmid, flags);
}
expublic int xa_start_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_end_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_rollback_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_prepare_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_commit_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_recover_entry_dyn(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxqdynsw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_dyn(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxqdynsw, xid, rmid, flags);
}
expublic int xa_complete_entry_dyn(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxqdynsw, handle, retval, rmid, flags);
}

/**
 * Verify that all committed messages on the disk, actually are present
 * in the memory...
 */
exprivate void tmq_chkdisk_th(void *ptr, int *p_finish_off)
{
    int *p_chkdisk_time = (int *)ptr;
    char tmp[PATH_MAX+1];
    int i;
    int ret=EXSUCCEED;
    int len;
    void *cursor=NULL;

    ret = ndrx_G_tmq_storage->pf_storage_list_start(ndrx_G_tmq_storage, 
            &cursor, NDRX_TMQ_STORAGE_LIST_MODE_COMMITTED);

    if (cursor == NULL)
    {
        NDRX_LOG(log_error, "pf_storage_list_start failed: %d", ret);
        EXFAIL_OUT(ret);
    }

    while (EXTRUE==(ret=ndrx_G_tmq_storage->pf_storage_list_next(ndrx_G_tmq_storage, 
        cursor, tmp, sizeof(tmp))))
    {
        /* for performance reasons which check
         * any file (even if several Q spaces works on the same
         * directory). with -X flag it is mandatory that
         * any Q space have it's own directory of work
         * otherwise it will just regulary restart due to
         * unknown messages found. Message IDs does not encode
         * Q space name, thus cannot filter our file or not from the
         * filename.
         */
        /* try to lookup in message hash */
        if (!ndrx_G_p_qdisk_xa_cfg->pf_tmq_msgid_exists(tmp))
        {
            /* verify back, that message is not removed form the disk */
            if (ndrx_G_tmq_storage->pf_storage_comm_exists(ndrx_G_tmq_storage, tmp))
            {
                NDRX_LOG(log_error, "ERROR: Unkown message file "
                        "exists [%s] (duplicate processes or two Q "
                        "spaces works in the same directory) - restarting...",
                    tmp);
                userlog("ERROR: Unkown message file exists"
                        " [%s] (duplicate processes or two Q "
                        "spaces works in the same directory) - restarting...",
                    tmp);
                ndrx_G_p_qdisk_xa_cfg->pf_tpexit();
                break;
            }
        }
    }

out:
    if (NULL!=cursor)
    {
        ndrx_G_tmq_storage->pf_storage_list_end(ndrx_G_tmq_storage, cursor);
    }
}

/**
 * Write the message data to TX file
 * @param msg message (the structure is projected on larger memory block to fit in the whole msg
 * @param int_diag internal diagnostics, flags
 * @return SUCCEED/FAIL
 */
expublic int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg, int *int_diag)
{
    int ret = EXSUCCEED;
    char tmp[TMMSGIDLEN_STR+1];
    size_t len;
    /*
    uint64_t lockt =  msg->lockthreadid;
    
     do not want to lock be written out to files: 
    msg->lockthreadid = 0;
    NO NO NO !!!! We still need a lock!
    */
    
     /* detect the actual len... */
    len = tmq_get_block_len((char *)msg);
    
    NDRX_DUMP(log_debug, "Writing new message to disk", 
                (char *)msg, len);
    
    if (EXSUCCEED!=ndrx_G_tmq_storage->pf_storage_write_block(ndrx_G_tmq_storage, 
            (char *)msg, len, NULL, int_diag))
    {
        NDRX_LOG(log_error, "tmq_storage_write_cmd_newmsg() failed for msg %s", 
                tmq_msgid_serialize(msg->hdr.msgid, tmp));
        EXFAIL_OUT(ret);
    }
    /*
    msg->lockthreadid = lockt;
     */
    
    NDRX_LOG(log_info, "Message [%s] written ok to active TX file", 
            tmq_msgid_serialize(msg->hdr.msgid, tmp));
    
out:

    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
