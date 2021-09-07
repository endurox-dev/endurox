/**
 * @brief Memory based structures for Q.
 *  Due to fact that qdisk_xa in tmqmode may use callbacks from qspace
 *  Qspace is linked int qdisk_xa. Thought proper way would be to add callbacks
 *  
 *
 * @file qspace.c
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

#include <ndrx_config.h>

#ifdef EX_OS_FREEBSD
#define _WITH_GETLINE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>
#include <exuuid.h>

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include <exthpool.h>
#include "nstdutil.h"
#include "tmqueue.h"
#include "cconfig.h"
#include "qtran.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_TOKEN_SIZE          64 /**< max key=value buffer size of qdef element */
#define WORKERS_DEFAULT         2   /**< by default we have 2 workers for autoqs */

#define TMQ_QC_NAME             "name"
#define TMQ_QC_SVCNM            "svcnm"
#define TMQ_QC_TRIES            "tries"
#define TMQ_QC_AUTOQ            "autoq"
#define TMQ_QC_WAITINIT         "waitinit"
#define TMQ_QC_WAITRETRY        "waitretry"
#define TMQ_QC_WAITRETRYINC     "waitretryinc"
#define TMQ_QC_WAITRETRYMAX     "waitretrymax"
#define TMQ_QC_MEMONLY          "memonly"
#define TMQ_QC_MODE             "mode"
#define TMQ_QC_TXTOUT           "txtout"    /**< transaction timeout override */
#define TMQ_QC_ERRORQ           "errorq"    /**< Name of the error queue, opt */
#define TMQ_QC_WORKERS          "workers"   /**< max number of workders       */
#define TMQ_QC_SYNC             "sync"      /**< sync forward q               */

#define EXHASH_FIND_STR_H2(head,findstr,out)                                     \
    EXHASH_FIND(h2,head,findstr,strlen(findstr),out)

#define EXHASH_ADD_STR_H2(head,strfield,add)                                     \
    EXHASH_ADD(h2,head,strfield,strlen(add->strfield),add)

#define EXHASH_DEL_H2(head,delptr)                                               \
    EXHASH_DELETE(h2,head,delptr)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* Handler for MSG Hash. */
expublic tmq_memmsg_t *G_msgid_hash = NULL;

/* Handler for Q hash */
expublic tmq_qhash_t *G_qhash = NULL;

/*
 * Any public operations must be locked
 */
exprivate MUTEX_LOCKDECL(M_q_lock);

/* Configuration section */
expublic tmq_qconfig_t *G_qconf = NULL; 

exprivate MUTEX_LOCKDECL(M_msgid_gen_lock); /* Thread locking for xid generation  */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate tmq_memmsg_t* tmq_get_msg_by_msgid_str(char *msgid_str);

/**
 * Process message blocks on disk read (after cold startup)
 * @param tmxid serialized trnasaction id
 * @param p_block
 * @param state TMQ_TXSTATE_ according to 
 * @param seqno command sequence number within the transaction
 * @return EXSUCCEED/EXFAIL
 */
exprivate int process_block(char *tmxid, union tmq_block **p_block, int state, int seqno)
{
    int ret = EXSUCCEED;
    
    
    /* in case if state is prepared or active -> add transaction with given
     * sequence
     */
    if (TMQ_TXSTATE_ACTIVE==state || TMQ_TXSTATE_PREPARED==state)
    {
        char entry_status;
        
        if (TMQ_TXSTATE_PREPARED==state)
        {
            entry_status = XA_RM_STATUS_PREP;
        }
        else
        {
            entry_status = XA_RM_STATUS_ACTIVE;
        }
        
        if (EXSUCCEED!=tmq_log_addcmd(tmxid, seqno, (char *)*p_block, entry_status))
        {
            NDRX_LOG(log_error, "Failed to add tmxid [%s] seqno %d",
                    tmxid, seqno);
            EXFAIL_OUT(ret);
        }
    }
    
    switch((*p_block)->hdr.command_code)
    {
        case TMQ_STORCMD_NEWMSG:
            
            if (EXSUCCEED!=tmq_msg_add((tmq_msg_t **)p_block, EXTRUE, NULL, NULL))
            {
                NDRX_LOG(log_error, "Failed to enqueue!");
                EXFAIL_OUT(ret);
            }
            
            break;
        case TMQ_STORCMD_DUM:
            
            break;
        default:
            
            if (EXSUCCEED!=tmq_lock_msg((*p_block)->hdr.msgid))
            {
                if (TMQ_TXSTATE_PREPARED==state 
                        && TMQ_STORCMD_DEL==(*p_block)->hdr.command_code)
                {
                    NDRX_LOG(log_info, "Delete command and message not found - assume deleted");
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to lock message!");
                    EXFAIL_OUT(ret);
                }
            }
            
            break;
    }
    
out:
    /* free the mem if needed: */
    if (NULL!=*p_block)
    {
        NDRX_FREE((char *)*p_block);
        *p_block = NULL;
    }
    return ret;
}


/**
 * Load the messages from QSPACE (after startup)...
 * This does not need a lock, because it uses other globals
 * @return EXSUCCEED/EXFAIL
 */
expublic int tmq_load_msgs(void)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_info, "Reading messages from disk...");
    /* populate all queues - from XA source */
    if (EXSUCCEED!=tmq_storage_get_blocks(process_block,  (short)tpgetnodeid(), 
            (short)tpgetsrvid()))
    {
        EXFAIL_OUT(ret);
    }
    
    /* sort all queues (by submission time) */
    tmq_sort_queues();
    
out:
    NDRX_LOG(log_info, "tmq_load_msgs return %d", ret);
    return ret;
}

/**
 * Add dummy marker for given transaction, increment sequence number
 * @param tmxid transaction id (must be in the transaction regsitry)
 * @return EXSUCCEED/EXFAIL
 */
expublic int tmq_dum_add(char *tmxid)
{
    tmq_msg_dum_t dum;
    int ret = EXSUCCEED;
        
    /* Build up the message. 
     * Note that we might not be in transaction mode, in case if
     * doing prepare and we find that there is nothing to prepare.
     */
    tmq_setup_cmdheader_dum(&dum.hdr, NULL, tpgetnodeid(), 0, ndrx_G_qspace, 0);
    dum.hdr.command_code = TMQ_STORCMD_DUM;
    
    /* this adds transaction to log: */
    if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&dum, 
                "Dummy transaction marker", tmxid, NULL))
    {
        NDRX_LOG(log_error, "Failed to write dummy command block to disk [%s]", tmxid);
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

/**
 * Setup dummy header
 * @param hdr header to setup
 * @param qname queue name
 */
expublic int tmq_setup_cmdheader_dum(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags)
{
    int ret = EXSUCCEED;
    
    NDRX_STRCPY_SAFE(hdr->qspace, qspace);
   /* strcpy(hdr->qname, qname); same object, causes core dumps on osx */
    hdr->command_code = TMQ_STORCMD_NEWMSG;
    NDRX_STRNCPY(hdr->magic, TMQ_MAGIC, TMQ_MAGIC_LEN);
    
    /* trailing magic */
    hdr->nodeid = nodeid;
    hdr->srvid = srvid;
    hdr->flags = flags;
    memset(hdr->reserved, 0, sizeof(hdr->reserved));
    memset(hdr->msgid, 0, sizeof(hdr->msgid));
    NDRX_STRNCPY(hdr->magic2, TMQ_MAGIC2, TMQ_MAGIC_LEN);
    
out:
    return ret;
}

/**
 * Setup queue header
 * @param hdr header to setup
 * @param qname queue name
 */
expublic int tmq_setup_cmdheader_newmsg(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags)
{
    int ret = EXSUCCEED;
    
    NDRX_STRCPY_SAFE(hdr->qspace, qspace);
   /* strcpy(hdr->qname, qname); same object, causes core dumps on osx */
    hdr->command_code = TMQ_STORCMD_NEWMSG;
    NDRX_STRNCPY(hdr->magic, TMQ_MAGIC, TMQ_MAGIC_LEN);
    
    /* trailing magic */
    hdr->nodeid = nodeid;
    hdr->srvid = srvid;
    hdr->flags = flags;
    memset(hdr->reserved, 0, sizeof(hdr->reserved));
    NDRX_STRNCPY(hdr->magic2, TMQ_MAGIC2, TMQ_MAGIC_LEN);
    
    tmq_msgid_gen(hdr->msgid);
    
out:
    return ret;
}


/**
 * Generate new transaction id, native form (byte array)
 * Note this initializes the msgid.
 * @param msgid value to return
 */
expublic void tmq_msgid_gen(char *msgid)
{
    exuuid_t uuid_val;
    short node_id = (short) ndrx_get_G_atmi_env()->our_nodeid;
    short srv_id = (short) G_srv_id;
   
    memset(msgid, 0, TMMSGIDLEN);
    
    /* Do the locking, so that we get unique xids... */
    MUTEX_LOCK_V(M_msgid_gen_lock);
    exuuid_generate(uuid_val);
    MUTEX_UNLOCK_V(M_msgid_gen_lock);
    
    memcpy(msgid, uuid_val, sizeof(exuuid_t));
    /* Have an additional infos for transaction id... */
    memcpy(msgid  
            +sizeof(exuuid_t)  
            ,(char *)&(node_id), sizeof(short));
    memcpy(msgid  
            +sizeof(exuuid_t) 
            +sizeof(short)
            ,(char *)&(srv_id), sizeof(short));    
    
    NDRX_LOG(log_debug, "MSGID: struct size: %d", sizeof(exuuid_t)+sizeof(short)+ sizeof(short));
}


/**
 * Load the key config parameter
 * @param qconf
 * @param key
 * @param value
 */
exprivate int load_param(tmq_qconfig_t * qconf, char *key, char *value)
{
    int ret = EXSUCCEED; 
    
    NDRX_LOG(log_info, "loading q param: [%s] = [%s]", key, value);
    
    /* - Not used.
    if (0==strcmp(key, TMQ_QC_NAME))
    {
        NDRX_STRNCPY(qconf->name, value, sizeof(qconf->name)-1);
        qconf->name[sizeof(qconf->name)-1] = EOS;
    }
    else  */
    if (0==strcmp(key, TMQ_QC_SVCNM))
    {
        NDRX_STRCPY_SAFE(qconf->svcnm, value);
    }
    else if (0==strcmp(key, TMQ_QC_ERRORQ))
    {
        NDRX_STRCPY_SAFE(qconf->errorq, value);
    }
    else if (0==strcmp(key, TMQ_QC_TRIES))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
        
        qconf->tries = ival;
    }
    else if (0==strcmp(key, TMQ_QC_AUTOQ))
    {
        char val = toupper(value[0]);

        if (NULL==strchr(TMQ_AUTOQ_ALLFLAGS, val))
        {
            NDRX_LOG(log_error, "Invalid value [%c] for key [%s] (allowed: [%s])", 
                    val, key, TMQ_AUTOQ_ALLFLAGS);
            EXFAIL_OUT(ret);
        }
        
        qconf->autoq = val;
        
    }
    else if (0==strcmp(key, TMQ_QC_WAITINIT))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
        
        qconf->waitinit = ival;
    }
    else if (0==strcmp(key, TMQ_QC_WAITRETRY))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
        
        qconf->waitretry = ival;
    }
    else if (0==strcmp(key, TMQ_QC_WAITRETRYINC))
    {
        NDRX_LOG(log_warn, "Ignoring [%s]", TMQ_QC_WAITRETRYINC);
    }
    else if (0==strcmp(key, TMQ_QC_WAITRETRYMAX))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
        
        qconf->waitretrymax = ival;
    }
    else if (0==strcmp(key, TMQ_QC_WORKERS))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 1)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=1)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
        
        qconf->workers = ival;
    }
    else if (0==strcmp(key, TMQ_QC_SYNC))
    {
        int ival = ndrx_args_confirm(value);
        
        if (EXFAIL==ival)
        {
            if (1==strlen(value) && NULL!=strstr(TMQ_ARGS_COMMIT, value))
            {
                ival = TMQ_SYNC_TPCOMMIT;
            }
            else
            {
                NDRX_LOG(log_error, "Invalid value [%s] for %s", value, TMQ_QC_SYNC);
                EXFAIL_OUT(ret);   
            }
        }
        else if (ival)
        {
            ival = TMQ_SYNC_TPACALL;
        }
        else
        {
            ival=TMQ_SYNC_NONE;
        }
        
        qconf->sync = ival;
        
    }
    else if (0==strcmp(key, TMQ_QC_MEMONLY))
    {
        /* CURRENTLY NOT SUPPORTED.
        qconf->memonly = FALSE;
        if (value[0]=='y' || value[0]=='Y')
        {
            qconf->memonly = TRUE;
        }
        */
    }
    else if (0==strcmp(key, TMQ_QC_TXTOUT))
    {   
        /* transaction timeout */
        qconf->txtout = atoi(value);
    }
    else if (0==strcmp(key, TMQ_QC_MODE))
    {
        if (0==strcmp(value, "fifo") || 0==strcmp(value, "FIFO"))
        {
            qconf->mode = TMQ_MODE_FIFO;
        }
        else if (0==strcmp(value, "lifo") || 0==strcmp(value, "LIFO") )
        {
            qconf->mode = TMQ_MODE_LIFO;
        }
        else
        {
            NDRX_LOG(log_error, "Not supported Q mode (must be fifo or lifo)", 
                    value, key);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_error, "Unknown Q config setting = [%s]", key);
        EXFAIL_OUT(ret);
    }
    
out:

    return ret;
}

/**
 * Get Q config by name
 * @param name queue name
 * @return NULL or ptr to q config.
 */
exprivate tmq_qconfig_t * tmq_qconf_get(char *qname)
{
    tmq_qconfig_t *ret = NULL;
    
    EXHASH_FIND_STR( G_qconf, qname, ret);
    
    return ret;
}

/**
 * Return Q config with default if not found
 * TODO: Think about copy off the contents of qconf for substituing service 
 * to Q name.
 * @param qname qname
 * @param p_is_defaulted returns 1 if queue uses defaults Q
 * @return  NULL or ptr to config
 */
expublic tmq_qconfig_t * tmq_qconf_get_with_default(char *qname, int *p_is_defaulted)
{
    
    tmq_qconfig_t * ret = tmq_qconf_get(qname);
    
    if  (NULL==ret)
    {
        NDRX_LOG(log_info, "Q config [%s] not found, trying to default to [%s]", 
                qname, TMQ_DEFAULT_Q);
        if (NULL==(ret = tmq_qconf_get(TMQ_DEFAULT_Q)))
        {
            NDRX_LOG(log_error, "Q [%s] is not defined and default config [%s] not found!",
                qname, TMQ_DEFAULT_Q);
            userlog("Q [%s] is not defined and default config [%s] not found!",
                qname, TMQ_DEFAULT_Q);
        }
        else if (NULL!=p_is_defaulted)
        {
            *p_is_defaulted = EXTRUE;
        }
    }
            
    return ret;
}


/**
 * Return string version of Q config
 * @param qname
 * @param p_is_defaulted
 * @param out_buf space where to print queue config
 * @param out_bufsz output buffer size
 * @return EXSUCCEED/EXFAIL
 */
expublic int tmq_build_q_def(char *qname, int *p_is_defaulted, char *out_buf, size_t out_bufsz)
{
    tmq_qconfig_t * qdef = NULL;
    int ret = EXSUCCEED;
    
    MUTEX_LOCK_V(M_q_lock);

    if (NULL==(qdef=tmq_qconf_get_with_default(qname, p_is_defaulted)))
    {
        EXFAIL_OUT(ret);
    }
    
    snprintf(out_buf, out_bufsz, "%s,svcnm=%s,autoq=%c,tries=%d,waitinit=%d,waitretry=%d,"
                        "waitretrymax=%d,mode=%s,txtout=%d,workers=%d",
            qdef->qname, 
            qdef->svcnm, 
            qdef->autoq,
            qdef->tries,
            qdef->waitinit,
            qdef->waitretry,
            qdef->waitretrymax,
            qdef->mode == TMQ_MODE_LIFO?"lifo":"fifo",
            qdef->txtout,
            qdef->workers);

    if (EXEOS!=qdef->errorq[0])
    {
        int len = strlen(out_buf);
        snprintf(out_buf+len, out_bufsz-len, ",errorq=%s", qdef->errorq);
    }

out:
    MUTEX_UNLOCK_V(M_q_lock);

    return ret;
}


/**
 * Get the static copy of Q data for extract (non-locked) use.
 * @param qname
 * @param qconf_out where to store/copy the q def data
 * @return SUCCEED/FAIL
 */
expublic int tmq_qconf_get_with_default_static(char *qname, tmq_qconfig_t *qconf_out)
{
    int ret = EXSUCCEED;
    tmq_qconfig_t * tmp = NULL;
    
    MUTEX_LOCK_V(M_q_lock);
    
    tmp = tmq_qconf_get(qname);

    if  (NULL==tmp)
    {
        NDRX_LOG(log_warn, "Q config [%s] not found, trying to default to [%s]", 
                qname, TMQ_DEFAULT_Q);
        if (NULL==(tmp = tmq_qconf_get(TMQ_DEFAULT_Q)))
        {
            NDRX_LOG(log_error, "Q [%s] is not defined and default config [%s] not found!",
                qname, TMQ_DEFAULT_Q);
            userlog("Q [%s] is not defined and default config [%s] not found!",
                qname, TMQ_DEFAULT_Q);
            EXFAIL_OUT(ret);
        }
        
    }
    
    memcpy(qconf_out, tmp, sizeof(*qconf_out));
        
out:    
    MUTEX_UNLOCK_V(M_q_lock);
    
    return ret;
}

/**
 * Remove queue probably then existing messages will fall back to default Q
 * @param name
 * @return 
 */
exprivate int tmq_qconf_delete(char *qname)
{
    int ret = EXSUCCEED;
    tmq_qconfig_t *qconf;
    
    if (NULL!=(qconf=tmq_qconf_get(qname)))
    {
        EXHASH_DEL( G_qconf, qconf);
        NDRX_FREE(qconf);
    }
    else
    {
        NDRX_LOG(log_warn, "[%s] - queue not found", qname);
    }
    
out:
    return ret;
}

/**
 * Reload the config of queues
 * @param cf
 * @return 
 */
expublic int tmq_reload_conf(char *cf)
{
    FILE *f = NULL;
#ifdef HAVE_GETLINE
    char *line = NULL;
#else
    char line[PATH_MAX];
#endif
    size_t len = 0;
    int ret = EXSUCCEED;
    ssize_t read;
    ndrx_inicfg_section_keyval_t * csection = NULL, *val = NULL, *val_tmp = NULL;
    
    if (NULL!=ndrx_get_G_cconfig() && 
            EXSUCCEED==ndrx_cconfig_get(NDRX_CONF_SECTION_QUEUE, &csection))
    {
        EXHASH_ITER(hh, csection, val, val_tmp)
        {
            if (EXSUCCEED!=tmq_qconf_addupd(val->val, val->key))
            {
                EXFAIL_OUT(ret);
            }
        }
    }
    else /* fallback to old config */
    {
        if (NULL==(f=NDRX_FOPEN(cf, "r")))
        {
            NDRX_LOG(log_error, "Failed to open [%s]:%s", cf, strerror(errno));
            EXFAIL_OUT(ret);
        }

#ifdef HAVE_GETLINE
        while (EXFAIL!=(read = getline(&line, &len, f))) 
#else
        len = sizeof(line);
        while (NULL!=fgets(line, len, f))
#endif
        {
            ndrx_str_strip(line, " \n\r\t");

            /* Ignore comments & newlines */
            if ('#'==*line || EXEOS==*line)
            {
                continue;
            }

            if (EXSUCCEED!=tmq_qconf_addupd(line, NULL))
            {
                EXFAIL_OUT(ret);
            }
        }
    }
    
    
out:

    /* 25/04/2018 moved down here, to avoid any memory leak if loop fails */
#ifdef HAVE_GETLINE
    if (NULL!=line)
    {
        NDRX_FREE(line);
    }
#endif
                
    if (NULL!=csection)
    {
        ndrx_keyval_hash_free(csection);
    }

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    return ret;
}

/**
 * Add queue definition. Support also update
 * We shall support Q update too...
 * Syntax: -q VISA,svcnm=VISAIF,autoq=y|n,waitinit=30,waitretry=10,waitretrymax=40,memonly=y|n
 * @param qdefstr queue definition
 * @param name optional name (already parsed)
 * @return  SUCCEED/FAIL
 */
expublic int tmq_qconf_addupd(char *qconfstr, char *name)
{
    tmq_qconfig_t * qconf=NULL;
    tmq_qconfig_t * dflt=NULL;
    char * p;
    char * p2;
    int got_default = EXFALSE;
    int qupdate = EXFALSE;
    char buf[MAX_TOKEN_SIZE];
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_info, "Add new Q: [%s]", qconfstr);
    
    MUTEX_LOCK_V(M_q_lock);
    
    if (NULL==name)
    {
        p = strtok (qconfstr,",");
    }
    else
    {
        p = name;
    }
    
    if (NULL!=p)
    {
        NDRX_LOG(log_info, "Got token: [%s]", p);
        NDRX_STRCPY_SAFE(buf, p);
        
        NDRX_LOG(log_debug, "Q name: [%s]", buf);
        
        if (NULL== (qconf = tmq_qconf_get(buf)))
        {
            NDRX_LOG(log_info, "Q not found, adding: [%s]", buf);
            qconf= NDRX_CALLOC(1, sizeof(tmq_qconfig_t));
                    
            /* Try to load initial config from @ (TMQ_DEFAULT_Q) Q */
            qconf->mode = TMQ_MODE_FIFO; /* default to FIFO... */
            qconf->txtout = EXFAIL;     /* use default */
            qconf->workers = WORKERS_DEFAULT;   /* set default workers to 2 */
            if (NULL!=(dflt=tmq_qconf_get(TMQ_DEFAULT_Q)))
            {
                memcpy(qconf, dflt, sizeof(*dflt));
                got_default = EXTRUE;
            }
            
            NDRX_STRCPY_SAFE(qconf->qname, buf);
        }
        else
        {
            NDRX_LOG(log_info, "Q found, updating: [%s]", buf);
            qupdate = EXTRUE;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Missing Q name");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==name)
    {
        p = strtok (NULL, ","); /* continue... */
    }
    else
    {
        p = strtok (qconfstr, ","); /* continue... */
    }
    
    while (p != NULL)
    {
        NDRX_LOG(log_info, "Got pair [%s]", p);
        
        NDRX_STRCPY_SAFE(buf, p);
        
        p2 = strchr(buf, '=');
        
        if (NULL == p2)
        {
            NDRX_LOG(log_error, "Invalid key=value token [%s] expected '='", buf);
            
            userlog("Error defining queue (%s) expected in '=' in token (%s)", 
                    qconfstr, buf);
            EXFAIL_OUT(ret);
        }
        *p2 = EXEOS;
        p2++;
        
        if (EXEOS==*p2)
        {
            NDRX_LOG(log_error, "Empty value for token [%s]", buf);
            userlog("Error defining queue (%s) invalid value for token (%s)", 
                    qconfstr, buf);
            EXFAIL_OUT(ret);
        }
        
        /*
         * Load the value into structure
         */
        if (EXSUCCEED!=load_param(qconf, buf, p2))
        {
            NDRX_LOG(log_error, "Failed to load token [%s]/[%s]", buf, p2);
            userlog("Error defining queue (%s) failed to load token [%s]/[%s]", 
                    buf, p2);
            EXFAIL_OUT(ret);
        }
        
        p = strtok (NULL, ",");
    }
    
    /* Validate the config... */
    
    if (0==strcmp(qconf->qname, TMQ_DEFAULT_Q) && got_default)
    {
        NDRX_LOG(log_error, "Missing [%s] param", TMQ_QC_NAME);
        /* TODO: Return some diagnostics... => EX_QDIAGNOSTIC invalid qname */
        EXFAIL_OUT(ret);
    }
    /* If autoq, then service must be set. */

    if (!qupdate)
    {
        EXHASH_ADD_STR( G_qconf, qname, qconf );
    }
    
out:

    /* kill the record if invalid. */
    if (EXSUCCEED!=ret && NULL!=qconf && !qupdate)
    {
        NDRX_LOG(log_warn, "qconf -> free");
        NDRX_FREE(qconf);
    }

    MUTEX_UNLOCK_V(M_q_lock);
    return ret;

}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
exprivate tmq_qhash_t * tmq_qhash_get(char *qname)
{
    tmq_qhash_t * ret = NULL;
   
    EXHASH_FIND_STR( G_qhash, qname, ret);    
    
    return ret;
}

/**
 * Get new qhash entry + add it to hash.
 * @param qname
 * @return 
 */
exprivate tmq_qhash_t * tmq_qhash_new(char *qname)
{
    tmq_qhash_t * ret = NDRX_CALLOC(1, sizeof(tmq_qhash_t));
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_qhash_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_qhash_t: %s", strerror(errno));
        goto out;
    }
    
    NDRX_STRCPY_SAFE(ret->qname, qname);
    
    EXHASH_ADD_STR( G_qhash, qname, ret );
    
out:
    return ret;
}

/**
 * Add message to queue
 * Think about TPQLOCKED so that other thread does not get message in progress..
 * In two phase commit mode, we need to unlock message only when it is enqueued on disk.
 * 
 * @param msg double ptr to message
 * @param diag qctl for diag purposes.
 * @param int_diag internal diagnostics, flags
 * @return 
 */
expublic int tmq_msg_add(tmq_msg_t **msg, int is_recovery, TPQCTL *diag, int *int_diag)
{
    int ret = EXSUCCEED;
    int is_locked = EXFALSE;
    tmq_qhash_t *qhash;
    tmq_memmsg_t *mmsg = NDRX_CALLOC(1, sizeof(tmq_memmsg_t));
    tmq_qconfig_t * qconf;
    char msgid_str[TMMSGIDLEN_STR+1];
    char corrid_str[TMCORRIDLEN_STR+1];
    int hashed=EXFALSE, hashedcor=EXFALSE, cdl=EXFALSE;
    
    MUTEX_LOCK_V(M_q_lock);
    is_locked = EXTRUE;
    
    qhash = tmq_qhash_get((*msg)->hdr.qname);
    qconf = tmq_qconf_get_with_default((*msg)->hdr.qname, NULL);
    
    if (NULL==mmsg)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==qconf)
    {
        NDRX_LOG(log_error, "queue config not found! Cannot enqueue!");
        userlog("queue config not found! Cannot enqueue!");
        
        if (NULL!=diag)
        {
            diag->diagnostic = QMEBADQUEUE;
            snprintf(diag->diagmsg, sizeof(diag->diagmsg), "Queue [%s] not defined",
                    (*msg)->hdr.qname);
        }
        EXFAIL_OUT(ret);
    }
    
    mmsg->msg = *msg;
    
    /* Add the hash of IDs / check that msg isn't duplicate */
    tmq_msgid_serialize(mmsg->msg->hdr.msgid, msgid_str); 
    NDRX_LOG(log_info, "Adding to G_msgid_hash [%s]", msgid_str);
           
    if (NULL!=tmq_get_msg_by_msgid_str(msgid_str))
    {
        NDRX_LOG(log_error, "Message with msgid [%s] already exists!", msgid_str);
        userlog("Message with msgid [%s] already exists!", msgid_str);
        EXFAIL_OUT(ret);
    }
    
    /* Get the entry for hash of queues: */
    if (NULL==qhash && NULL==(qhash=tmq_qhash_new((*msg)->hdr.qname)))
    {
        NDRX_LOG(log_error, "Failed to get/create qhash entry for Q [%s]", 
                (*msg)->hdr.qname);
        EXFAIL_OUT(ret);
    }
    
    /* Add the message to end of the queue */
    CDL_APPEND(qhash->q, mmsg);
    cdl=EXTRUE;
    
    NDRX_STRCPY_SAFE(mmsg->msgid_str, msgid_str);
    EXHASH_ADD_STR( G_msgid_hash, msgid_str, mmsg);
    hashed=EXTRUE;

    if (mmsg->msg->qctl.flags & TPQCORRID)
    {
        tmq_msgid_serialize((*msg)->qctl.corrid, corrid_str);
        NDRX_STRCPY_SAFE(mmsg->corrid_str, corrid_str);
        NDRX_LOG(log_debug, "Adding to corrid_hash [%s] of queue [%s]",
            corrid_str, (*msg)->hdr.qname);
        if (EXSUCCEED!=tmq_cor_msg_add(qconf, qhash, mmsg))
        {
            NDRX_LOG(log_error, "Failed to add msg to corhash!");
            EXFAIL_OUT(ret);
        }
        hashedcor=EXTRUE;
    }
    /* have to unlock here, because tmq_storage_write_cmd_newmsg() migth callback to
     * us and that might cause stall.
     */
    MUTEX_UNLOCK_V(M_q_lock);
    is_locked = EXFALSE;
    
    /* Decide do we need to add the msg to disk?! 
     * Needs to send a command to XA sub-system to prepare msg/command to disk,
     * if it is not memory only.
     * So next step todo is to write xa command handler & dumping commands to disk.
     */
    if (!qconf->memonly)
    {
        /* for recovery no need to put command as we read from command file */
        if (!is_recovery)
        {
            if (EXSUCCEED!=tmq_storage_write_cmd_newmsg(mmsg->msg, int_diag))
            {
                NDRX_LOG(log_error, "Failed to add message to persistent store!");
                
                /* set the OS error */
                if (NULL!=diag)
                {
                    diag->diagnostic = QMEOS;
                    snprintf(diag->diagmsg, sizeof(diag->diagmsg), "Failed to persist msg");
                }
                
                EXFAIL_OUT(ret);
            }
        }
    }
    else
    {
        NDRX_LOG(log_info, "Mem only Q, not persisting.");   
    }
    
    /* Add only if all OK 
     * Note locked here...
     */
    MUTEX_LOCK_V(M_q_lock);
    qhash->numenq++;
    MUTEX_UNLOCK_V(M_q_lock);
    
    NDRX_LOG(log_info, "Message with id [%s] successfully enqueued to [%s] "
            "queue (DEBUG: locked %ld)",
            tmq_msgid_serialize((*msg)->hdr.msgid, msgid_str), (*msg)->hdr.qname, 
            mmsg->msg->lockthreadid);
    
    /* message is in use */
    *msg = NULL;
    
out:
     
                
    if (is_locked)
    {
        MUTEX_UNLOCK_V(M_q_lock);
        is_locked=EXFALSE;   
    }

    /* NOT SURE IS THIS GOOD as this might cause segmentation fault.
     * as added to mem, but failed to write to disk.
     * Message will be kept locked, thus we can free it.
     * Also as disk is failed, we will never get unlock by tmsrv as file
     * is not available. Thus it is safe here.
     */
    if (EXSUCCEED!=ret && mmsg!=NULL)
    {
        /* remove messages hashes, due to failure */
        MUTEX_LOCK_V(M_q_lock);
        
        if (hashed)
        {
            EXHASH_DEL( G_msgid_hash, mmsg);
        }
        
        if (hashedcor)
        {
            tmq_cor_msg_del(qhash, mmsg);
        }
    
        if (cdl)
        {
           CDL_DELETE(qhash->q, mmsg);
        }
        
        MUTEX_UNLOCK_V(M_q_lock);
        
        NDRX_FREE(mmsg);
        mmsg=NULL;
    }

    /* free up message, if it is not used anymore. */
    if (NULL==mmsg)
    {
        NDRX_FREE((*msg));
        *msg = NULL;
    }


    return ret;
}

/**
 * Tests is auto message valid for dequeue (calculate the counters)
 * @param node
 * @return 
 */
expublic int tmq_is_auto_valid_for_deq(tmq_memmsg_t *node, tmq_qconfig_t *qconf)
{
    int ret = EXFALSE;
    int retry_inc;
    unsigned long long next_try;
    long utc_sec, utc_usec;
    
    if (0==node->msg->trycounter)
    {
        next_try = node->msg->trytstamp + 
                ((unsigned long long)qconf->waitinit);
        
        NDRX_LOG(log_debug, "First try, sleep %d sec", qconf->waitinit);
    }
    else 
    {
        retry_inc = qconf->waitretry*node->msg->trycounter;
    
        if (retry_inc > qconf->waitretrymax)
        {
            retry_inc = qconf->waitretrymax;
        }
        
        NDRX_LOG(log_debug, "Try no %d, sleep %d sec", 
                node->msg->trycounter, retry_inc);
        
        next_try = node->msg->trytstamp + 
                retry_inc;
    }
    
    ndrx_utc_tstamp2(&utc_sec, &utc_usec);
    NDRX_LOG(log_debug, "Next try at: %ld current clock: %ld",
            next_try, utc_sec);
            
    if (next_try<=utc_sec)
    {
        NDRX_LOG(log_debug, "Message accepted for dequeue...");
        ret=EXTRUE;
    }
    else
    {
        NDRX_LOG(log_debug, "Message NOT accepted for dequeue...");
    }
    
out:
    return ret;
}

/**
 * Get the fifo message from Q
 * @param qname queue to lookup.
 * @param diagnostic specific queue error code
 * @param corrid_str dequeue by correlator (if not NULL)
 * @param int_diag internal diagnostics, flags
 * @return NULL (no msg), or ptr to msg
 */
expublic tmq_msg_t * tmq_msg_dequeue(char *qname, long flags, int is_auto, long *diagnostic, 
        char *diagmsg, size_t diagmsgsz, char *corrid_str, int *int_diag)
{
    tmq_qhash_t *qhash;
    tmq_corhash_t *corhash;
    tmq_memmsg_t *node = NULL;
    tmq_memmsg_t *start = NULL;
    tmq_msg_t * ret = NULL;
    tmq_msg_del_t block;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_qconfig_t *qconf;
    int is_locked=EXFALSE;
    
    *diagnostic=EXSUCCEED;
    
    NDRX_LOG(log_debug, "FIFO/LIFO dequeue for [%s]", qname);
    MUTEX_LOCK_V(M_q_lock);
    is_locked=EXTRUE;
    
    /* Find the non locked message in memory */
    
    /* Lock the message for current thread. 
     * The thread will later issue xA command either:
     * - Increase counter
     * - Remove the message.
     */
    
    if (NULL==(qconf=tmq_qconf_get_with_default(qname, NULL)))
    {
        
        NDRX_LOG(log_error, "Failed to get q config [%s]", 
                qname);
        
        *diagnostic=QMEBADQUEUE;
        snprintf(diagmsg, diagmsgsz, "Queue [%s] not defined", qname);
        
        goto out;
    }
    
    /* if not such queue, the no msg */
    if (NULL==(qhash = tmq_qhash_get(qname)))
    {
        NDRX_LOG(log_warn, "Q [%s] is NULL/empty", qname);
        *diagnostic=QMENOMSG;
        snprintf(diagmsg, diagmsgsz, "Q [%s] is NULL/empty", qname);
        goto out;
    }

    NDRX_LOG(log_debug, "mode corrid_str[%s]: %s", corrid_str?corrid_str:"N/A",
               TMQ_MODE_LIFO == qconf->mode?"LIFO":"FIFO");
    
    /* if no hash available -> assume no msg*/
    if (NULL!=corrid_str && NULL==(corhash = tmq_cor_find(qhash, corrid_str)))
    {
        NDRX_LOG(log_info, "Q [%s] corrid_str [%s] not defined", 
            qname, corrid_str);
        snprintf(diagmsg, diagmsgsz, "Q [%s] corrid_str [%s] not defined", 
            qname, corrid_str);
        *diagnostic=QMENOMSG;
        goto out;
    }

    /* Start from first one & loop over the list while 
     * - we get to the first non-locked message
     * - or we get to the end with no msg, then return FAIL.
     */
    if (TMQ_MODE_LIFO == qconf->mode)
    {
        /* LIFO mode */
        if (NULL!=corrid_str)
        {
            /* we do not have empty hashes,
             * thus msg must be in there.
            */
            node = corhash->corq->prev2;
            start = corhash->corq->prev2;
        }
        else if (NULL!=qhash->q)
        {
            node = qhash->q->prev;
            start = qhash->q->prev;
        }
    }
    else
    {
        /* FIFO */
        if (NULL!=corrid_str)
        {
            node = corhash->corq;
            start = corhash->corq;
        }
        else
        {
            node = qhash->q;
            start = qhash->q;
        }
    }
            
    do
    {
        if (NULL!=node)
        {
            NDRX_LOG(log_debug, "Testing: msg_str: [%s] locked: %llu is_auto: %d",
                    tmq_msgid_serialize(node->msg->hdr.msgid, msgid_str),
                    node->msg->lockthreadid,
                    is_auto
                    );
            if (!node->msg->lockthreadid && (!is_auto || 
                    tmq_is_auto_valid_for_deq(node, qconf)))
            {
                ret = node->msg;
                break;
            }
            if (TMQ_MODE_LIFO == qconf->mode)
            {
                /* LIFO mode */
                if (NULL!=corrid_str)
                {
                    node = node->prev2;
                }
                else
                {
                    node = node->prev;
                }
            }
            else
            {
                /* default to FIFO */
                if (NULL!=corrid_str)
                {
                    node = node->next2;
                }
                else
                {
                    node = node->next;
                }
            }
        } /* if (NULL!=node) */
    }
    while (NULL!=node && node!=start);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_debug, "Q [%s] is empty or all msgs locked", qname);
        goto out;
    }
    
    /* Write some stuff to log */
    
    tmq_msgid_serialize(ret->hdr.msgid, msgid_str);
    NDRX_LOG(log_info, "Dequeued message: [%s]", msgid_str);
    NDRX_DUMP(log_debug, "Dequeued message", ret->msg, ret->len);
    
    /* Lock the message */
    ret->lockthreadid = ndrx_gettid();
    
    MUTEX_UNLOCK_V(M_q_lock);
    is_locked=EXFALSE;
    
    /* Is it must not be a peek and must not be an autoq */
    if (!(flags & TPQPEEK) && !is_auto)
    {   
        /* Issue command for msg remove */   
        memcpy(&block.hdr, &ret->hdr, sizeof(ret->hdr));
        block.hdr.command_code = TMQ_STORCMD_DEL;

        if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&block, 
                "Removing dequeued message", NULL, int_diag))
        {
            NDRX_LOG(log_error, "Failed to remove msg...");
            /* unlock msg... */
            MUTEX_LOCK_V(M_q_lock);
            ret->lockthreadid = 0;
            MUTEX_UNLOCK_V(M_q_lock);
            
            ret = NULL;
            *diagnostic=QMEOS;
            NDRX_STRCPY_SAFE_DST(diagmsg, "tmq_dequeue: disk write error!", diagmsgsz);
            goto out;
        }
    }
    
out:
        
    if (is_locked)
    {
        MUTEX_UNLOCK_V(M_q_lock);
    }

    /* set default error code */
    if (NULL==ret && EXSUCCEED==*diagnostic)
    {
        NDRX_STRCPY_SAFE_DST(diagmsg, "tmq_dequeue: no message in Q!", diagmsgsz);
        *diagnostic=QMENOMSG;
    }

    return ret;
}

/**
 * Dequeue message by msgid
 * @param msgid
 * @param diagnostic queue error code, if any
 * @param int_diag internal diagnostics, flags
 * @return 
 */
expublic tmq_msg_t * tmq_msg_dequeue_by_msgid(char *msgid, long flags, long *diagnostic, 
        char *diagmsg, size_t diagmsgsz, int *int_diag)
{
    tmq_msg_t * ret = NULL;
    tmq_msg_del_t del;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t *mmsg;
    int is_locked=EXFALSE;
    
    *diagnostic=EXSUCCEED;
    
    MUTEX_LOCK_V(M_q_lock);
    is_locked=EXTRUE;
    
    /* Write some stuff to log */
    
    tmq_msgid_serialize(msgid, msgid_str);
    NDRX_LOG(log_info, "MSGID: Dequeuing message by [%s]", msgid_str);
    
    if (NULL==(mmsg = tmq_get_msg_by_msgid_str(msgid_str)))
    {
        NDRX_LOG(log_error, "Message not found by msgid_str [%s]", msgid_str);
        goto out;
    }

    ret = mmsg->msg;

    NDRX_DUMP(log_debug, "Dequeued message", ret->msg, ret->len);

    /* Lock the message */
    ret->lockthreadid = ndrx_gettid();
    
    /* release the lock.. */
    MUTEX_UNLOCK_V(M_q_lock);
    is_locked=EXFALSE;
    
    /* Issue command for msg remove */
    memcpy(&del.hdr, &ret->hdr, sizeof(ret->hdr));
    
    del.hdr.command_code = TMQ_STORCMD_DEL;
    
    if (!(flags & TPQPEEK))
    {
        if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&del, 
                "Removing dequeued message", NULL, int_diag))
        {
            NDRX_LOG(log_error, "Failed to remove msg...");
            /* unlock msg... */
            MUTEX_LOCK_V(M_q_lock);
            ret->lockthreadid = 0;
            MUTEX_UNLOCK_V(M_q_lock);
            ret = NULL;
            *diagnostic=QMEOS;
            NDRX_STRCPY_SAFE_DST(diagmsg, "tmq_dequeue: disk write error!", diagmsgsz);
            goto out;
        }
    }
    
out:
    if (is_locked)
    {
        MUTEX_UNLOCK_V(M_q_lock);
    }

    /* set default error code */
    if (NULL==ret && EXSUCCEED==*diagnostic)
    {
        NDRX_STRCPY_SAFE_DST(diagmsg, "tmq_dequeue: no message in Q!", diagmsgsz);
        *diagnostic=QMENOMSG;
    }

    return ret;
}

/**
 * Get message by msgid
 * @param msgid
 * @return 
 */
exprivate tmq_memmsg_t* tmq_get_msg_by_msgid_str(char *msgid_str)
{
    tmq_memmsg_t *ret;
    
    EXHASH_FIND_STR( G_msgid_hash, msgid_str, ret);
    
    return ret;
}

/**
 * Remove mem message
 * 
 * @param msg
 */
exprivate void tmq_remove_msg(tmq_memmsg_t *mmsg)
{
    char msgid_str[TMMSGIDLEN_STR+1];   
    tmq_msgid_serialize(mmsg->msg->hdr.msgid, msgid_str);
    
    tmq_qhash_t *qhash = tmq_qhash_get(mmsg->msg->hdr.qname);
    
    NDRX_LOG(log_info, "Removing msgid [%s] from [%s] q", msgid_str, mmsg->msg->hdr.qname);
    
    if (NULL!=qhash)
    {
        qhash->numdeq++;
        
        /* Add the message to end of the queue */
        CDL_DELETE(qhash->q, mmsg);    
    }
    
    /* Add the hash of IDs */
    EXHASH_DEL( G_msgid_hash, mmsg);
    
    if (TPQCORRID & mmsg->msg->qctl.flags)
    {
        tmq_cor_msg_del(qhash, mmsg);
    }
    
    NDRX_FREE(mmsg->msg);
    NDRX_FREE(mmsg);
}

/**
 * Unlock message by updated block
 * We can:
 * - update content + unlock
 * - or remove the message
 * @param p_b
 * @return 
 */
expublic int tmq_unlock_msg(union tmq_upd_block *b)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(b->hdr.msgid, msgid_str);
    
    NDRX_LOG(log_info, "Unlocking/updating: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    /* no msg to process for dummy */
    if (TMQ_STORCMD_DUM!=b->hdr.command_code)
    {
        mmsg = tmq_get_msg_by_msgid_str(msgid_str);

        if (NULL==mmsg)
        {   
            NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);

            /* might be a case when message file was deleted, but command block
             * not. Thus for command block might be false re-attempt.
             */
            goto out;
        }
    }
    
    switch (b->hdr.command_code)
    {
        case TMQ_STORCMD_DEL:
            NDRX_LOG(log_info, "Removing message...");
            tmq_remove_msg(mmsg);
            mmsg = NULL;
            break;
        case TMQ_STORCMD_UPD:
            UPD_MSG((mmsg->msg), (&b->upd));
        /* And still we want unblock: */
        case TMQ_STORCMD_NEWMSG:
        case TMQ_STORCMD_UNLOCK:
            NDRX_LOG(log_info, "Unlocking message...");
            mmsg->msg->lockthreadid = 0;
            
            /* wakeup the Q... runner */
            ndrx_forward_chkrun(mmsg);
            
            break;
        case TMQ_STORCMD_DUM:
            /* nothing todo; */
            break;
        default:
            NDRX_LOG(log_info, "Unknown command [%c]", b->hdr.command_code);
            EXFAIL_OUT(ret);
            break; 
    }
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Unlock memory message by msgid (used for PEEK)
 * @param msgid
 * @return 
 */
expublic int tmq_unlock_msg_by_msgid(char *msgid)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(msgid, msgid_str);
    
    NDRX_LOG(log_info, "Unlocking/updating: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    mmsg = tmq_get_msg_by_msgid_str(msgid_str);
    
    if (NULL==mmsg)
    {   
        NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);
        EXFAIL_OUT(ret);
    }
    
    mmsg->msg->lockthreadid = 0;
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}


/**
 * Load the messages
 * @param msgid
 * @return 
 */
expublic int tmq_lock_msg(char *msgid)
{
    int ret = EXSUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(msgid, msgid_str);
    
    NDRX_LOG(log_info, "Locking: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    mmsg = tmq_get_msg_by_msgid_str(msgid_str);
    
    if (NULL==mmsg)
    {   
        NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);
        EXFAIL_OUT(ret);
    }
    
    /* Lock the message */
    mmsg->msg->lockthreadid = ndrx_gettid();
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}


/**
 * compare two Q entries, by time + counter
 * @param q1
 * @param q2
 * @return 
 */
expublic int q_msg_sort(tmq_memmsg_t *q1, tmq_memmsg_t *q2)
{
    
    return ndrx_compare3(q1->msg->msgtstamp, q1->msg->msgtstamp_usec, q1->msg->msgtstamp_cntr, 
            q2->msg->msgtstamp, q2->msg->msgtstamp_usec, q2->msg->msgtstamp_cntr);
    
}

/**
 * Return list of auto queues
 * @return NULL or list
 */
expublic fwd_qlist_t *tmq_get_qlist(int auto_only, int incl_def)
{
    fwd_qlist_t * ret = NULL;
    fwd_qlist_t * tmp = NULL;
    
    tmq_qhash_t *q, *qtmp;
    tmq_qconfig_t *qconf;
    
    tmq_qconfig_t *qc, *qctmp;
    
    MUTEX_LOCK_V(M_q_lock);
    
    EXHASH_ITER(hh, G_qhash, q, qtmp)
    {
        if (NULL!=(qconf=tmq_qconf_get_with_default(q->qname, NULL)) && 
                ((auto_only && TMQ_AUTOQ_ISAUTO(qconf->autoq)) || !auto_only))
        {
            if (NULL==(tmp = NDRX_CALLOC(1, sizeof(fwd_qlist_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                ret = NULL;
                goto out;
            }
            /* have some stats */
            NDRX_STRCPY_SAFE(tmp->qname, q->qname);
            tmp->succ = q->succ;
            tmp->fail = q->fail;
            
            tmp->numenq = q->numenq;
            tmp->numdeq = q->numdeq;
            tmp->workers = qconf->workers;
            
            DL_APPEND(ret, tmp);
        }
        
    }
    
    /* If we need to include definitions
     * Then iterate over qdefs and change which are not in the G_qhash
     * Those add to return DL
     */
    if (incl_def)
    {
        EXHASH_ITER(hh, G_qconf, qc, qctmp)
        {
            if (NULL==tmq_qhash_get(qc->qname))
            {
                if (NULL==(tmp = NDRX_CALLOC(1, sizeof(fwd_qlist_t))))
                {
                    int err = errno;
                    NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                    userlog("Failed to alloc: %s", strerror(err));
                    ret = NULL;
                    goto out;
                }
                NDRX_LOG(log_debug, "tmq_get_qlist: %s", qc->qname);
                NDRX_STRCPY_SAFE(tmp->qname, qc->qname);
                DL_APPEND(ret, tmp);
            }
        }
    }
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Return list of messages in the q
 * @return NULL or list
 */
expublic tmq_memmsg_t *tmq_get_msglist(char *qname)
{
    tmq_qhash_t *qhash;
    tmq_memmsg_t *node;
    tmq_memmsg_t * ret = NULL;
    tmq_memmsg_t * tmp = NULL;
    tmq_msg_t * msg = NULL;
    
    NDRX_LOG(log_debug, "tmq_get_msglist listing for [%s]", qname);
    MUTEX_LOCK_V(M_q_lock);
    
    if (NULL==(qhash = tmq_qhash_get(qname)))
    {
        NDRX_LOG(log_warn, "Q [%s] is NULL/empty", qname);
        goto out;
    }
    
    /* Start from first one & loop over the list while 
     * - we get to the first non-locked message
     * - or we get to the end with no msg, then return FAIL.
     */
    node = qhash->q;
    
    do
    {
        if (NULL!=node)
        {
            if (NULL==(tmp = NDRX_CALLOC(1, sizeof(tmq_memmsg_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                ret = NULL;
                goto out;
            }
            
            if (NULL==(msg = NDRX_MALLOC(sizeof(tmq_msg_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                ret = NULL;
                goto out;
            }
            
            memcpy(msg, node->msg, sizeof(tmq_msg_t));
            tmp->msg = msg;
            
            DL_APPEND(ret, tmp);
            
            /* default to FIFO */
            node = node->next;
        }
    }
    while (NULL!=node && node!=qhash->q);
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Sort Qs
 * @return SUCCEED/FAIL 
 */
expublic int tmq_sort_queues(void)
{
    int ret = EXSUCCEED;
    tmq_qhash_t *q, *qtmp;
    
    MUTEX_LOCK_V(M_q_lock);
    
    /* iterate over Q hash & and sort them by Q time */
    EXHASH_ITER(hh, G_qhash, q, qtmp)
    {
        /* standard q sorting ... */
        CDL_SORT(q->q, q_msg_sort);

        /* correlator q sorting... */
        tmq_cor_sort_queues(q);
    }   
    
out:
            
    MUTEX_UNLOCK_V(M_q_lock);

    return ret;
}

/**
 * Update queue statistics
 * @param qname
 * @param msgs_diff
 * @param succ_diff
 * @param fail_diff
 * @return 
 */
expublic int tmq_update_q_stats(char *qname, long succ_diff, long fail_diff)
{
    tmq_qhash_t  *q;
    MUTEX_LOCK_V(M_q_lock);
    
    if (NULL!=(q = tmq_qhash_get(qname)))
    {
        q->succ += succ_diff;
        q->fail += fail_diff;
    }
    
out:
            
    MUTEX_UNLOCK_V(M_q_lock);

    return EXSUCCEED;
}

/**
 * Return infos about enqueued messages.
 * @param qname
 * @param p_msgs
 * @param p_locked
 * @return 
 */
expublic void tmq_get_q_stats(char *qname, long *p_msgs, long *p_locked)
{
    tmq_qhash_t  *q;        
    tmq_memmsg_t *node;
    MUTEX_LOCK_V(M_q_lock);
    
    if (NULL!=(q = tmq_qhash_get(qname)))
    {
        node = q->q;
        
        do
        {
            if (NULL!=node)
            {
                *p_msgs = *p_msgs +1 ;
                if (node->msg->lockthreadid)
                {
                    *p_locked = *p_locked +1 ;
                }
                /* default to FIFO */
                node = node->next;
            }

        }
        while (NULL!=node && node!=q->q);
    }
    
    MUTEX_UNLOCK_V(M_q_lock);
}

/******************************************************************************/
/*                         COMMAND LINE SUPPORT                               */
/******************************************************************************/

/* vim: set ts=4 sw=4 et smartindent: */
