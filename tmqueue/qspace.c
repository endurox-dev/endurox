/* 
** Memory based structures for Q.
**
** @file tmqueue.c
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
#include "thpool.h"
#include "nstdutil.h"
#include "tmqueue.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_TOKEN_SIZE          64 /* max key=value buffer size of qdef element */

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

#define HASH_FIND_STR_H2(head,findstr,out)                                     \
    HASH_FIND(h2,head,findstr,strlen(findstr),out)

#define HASH_ADD_STR_H2(head,strfield,add)                                     \
    HASH_ADD(h2,head,strfield,strlen(add->strfield),add)

#define HASH_DEL_H2(head,delptr)                                               \
    HASH_DELETE(h2,head,delptr)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* Handler for MSG Hash. */
public tmq_memmsg_t *G_msgid_hash = NULL;

/* Handler for Correlator ID hash */
public tmq_memmsg_t *G_corid_hash = NULL;

/* Handler for Q hash */
public tmq_qhash_t *G_qhash = NULL;

/*
 * Any public operations must be locked
 */
MUTEX_LOCKDECL(M_q_lock);

/* Configuration section */
public tmq_qconfig_t *G_qconf = NULL; 

MUTEX_LOCKDECL(M_msgid_gen_lock); /* Thread locking for xid generation  */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private tmq_memmsg_t* tmq_get_msg_by_msgid_str(char *msgid_str);
private tmq_memmsg_t* tmq_get_msg_by_corid_str(char *corid_str);

/**
 * Setup queue header
 * @param hdr header to setup
 * @param qname queue name
 */
public int tmq_setup_cmdheader_newmsg(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags)
{
    int ret = SUCCEED;
    
    strcpy(hdr->qspace, qspace);
    strcpy(hdr->qname, qname);
    hdr->command_code = TMQ_STORCMD_NEWMSG;
    strncpy(hdr->magic, TMQ_MAGIC, TMQ_MAGIC_LEN);
    hdr->nodeid = nodeid;
    hdr->srvid = srvid;
    hdr->flags = flags;
    
    tmq_msgid_gen(hdr->msgid);
    
out:
    return ret;
}


/**
 * Generate new transaction id, native form (byte array)
 * Note this initializes the msgid.
 * @param msgid value to return
 */
public void tmq_msgid_gen(char *msgid)
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
    
    NDRX_LOG(log_error, "MSGID: struct size: %d", sizeof(exuuid_t)+sizeof(short)+ sizeof(short));
}


/**
 * Load the key config parameter
 * @param qconf
 * @param key
 * @param value
 */
private int load_param(tmq_qconfig_t * qconf, char *key, char *value)
{
    int ret = SUCCEED; 
    
    NDRX_LOG(log_info, "loading q param: [%s] = [%s]", key, value);
    
    /* - Not used.
    if (0==strcmp(key, TMQ_QC_NAME))
    {
        strncpy(qconf->name, value, sizeof(qconf->name)-1);
        qconf->name[sizeof(qconf->name)-1] = EOS;
    }
    else  */
    if (0==strcmp(key, TMQ_QC_SVCNM))
    {
        strncpy(qconf->svcnm, value, sizeof(qconf->svcnm)-1);
        qconf->svcnm[sizeof(qconf->svcnm)-1] = EOS;
    }
    else if (0==strcmp(key, TMQ_QC_TRIES))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->tries = ival;
    }
    else if (0==strcmp(key, TMQ_QC_AUTOQ))
    {
        qconf->autoq = FALSE;
        
        if (value[0]=='y' || value[0]=='Y')
        {
            qconf->autoq = TRUE;
        }
    }
    else if (0==strcmp(key, TMQ_QC_WAITINIT))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
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
            FAIL_OUT(ret);
        }
        
        qconf->waitretry = ival;
    }
    else if (0==strcmp(key, TMQ_QC_WAITRETRYINC))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretryinc = ival;
    }
    else if (0==strcmp(key, TMQ_QC_WAITRETRYMAX))
    {
        int ival = atoi(value);
        if (!ndrx_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretrymax = ival;
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
            FAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_error, "Unknown Q config setting = [%s]", key);
        FAIL_OUT(ret);
    }
    
out:

    return ret;
}

/**
 * Get Q config by name
 * @param name queue name
 * @return NULL or ptr to q config.
 */
private tmq_qconfig_t * tmq_qconf_get(char *qname)
{
    tmq_qconfig_t *ret = NULL;
    
    HASH_FIND_STR( G_qconf, qname, ret);
    
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
private tmq_qconfig_t * tmq_qconf_get_with_default(char *qname, int *p_is_defaulted)
{
    
    tmq_qconfig_t * ret = tmq_qconf_get(qname);
    
    if  (NULL==ret)
    {
        NDRX_LOG(log_warn, "Q config [%s] not found, trying to default to [%s]", 
                qname, TMQ_DEFAULT_Q);
        if (NULL==(ret = tmq_qconf_get(TMQ_DEFAULT_Q)))
        {
            NDRX_LOG(log_error, "Default Q config [%s] not found!", TMQ_DEFAULT_Q);
            userlog("Default Q config [%s] not found! Please add !", TMQ_DEFAULT_Q);
        }
        else if (NULL!=p_is_defaulted)
        {
            *p_is_defaulted = TRUE;
        }
    }
            
    return ret;
}


/**
 * Return string version of Q config
 * @param qname
 * @param p_is_defaulted
 * @return 
 */
public int tmq_build_q_def(char *qname, int *p_is_defaulted, char *out_buf)
{
    tmq_qconfig_t * qdef = NULL;
    int ret = SUCCEED;
    
    MUTEX_LOCK_V(M_q_lock);
    if (NULL==(qdef=tmq_qconf_get_with_default(qname, p_is_defaulted)))
    {
        FAIL_OUT(ret);
    }
    
    sprintf(out_buf, "%s,svcnm=%s,autoq=%s,tries=%d,waitinit=%d,waitretry=%d,"
                        "waitretryinc=%d,waitretrymax=%d,mode=%s",
            qdef->qname, 
            qdef->svcnm, 
            (qdef->autoq?"y":"n"),
            qdef->tries,
            qdef->waitinit,
            qdef->waitretry,
            qdef->waitretryinc,
            qdef->waitretrymax,
            qdef->mode == TMQ_MODE_LIFO?"lifo":"fifo");

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
public int tmq_qconf_get_with_default_static(char *qname, tmq_qconfig_t *qconf_out)
{
    int ret = SUCCEED;
    tmq_qconfig_t * tmp = NULL;
    
    MUTEX_LOCK_V(M_q_lock);
    
    tmp = tmq_qconf_get(qname);

    if  (NULL==tmp)
    {
        NDRX_LOG(log_warn, "Q config [%s] not found, trying to default to [%s]", 
                qname, TMQ_DEFAULT_Q);
        if (NULL==(tmp = tmq_qconf_get(TMQ_DEFAULT_Q)))
        {
            NDRX_LOG(log_error, "Default Q config [%s] not found!", TMQ_DEFAULT_Q);
            userlog("Default Q config [%s] not found! Please add !", TMQ_DEFAULT_Q);
            FAIL_OUT(ret);
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
private int tmq_qconf_delete(char *qname)
{
    int ret = SUCCEED;
    tmq_qconfig_t *qconf;
    
    if (NULL!=(qconf=tmq_qconf_get(qname)))
    {
        HASH_DEL( G_qconf, qconf);
        free(qconf);
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
public int tmq_reload_conf(char *cf)
{
    FILE *f = NULL;
    char *line = NULL;
    size_t len = 0;
    int ret = SUCCEED;
    ssize_t read;
    
    if (NULL==(f=fopen(cf, "r")))
    {
        NDRX_LOG(log_error, "Failed to open [%s]:%s", cf, strerror(errno));
        FAIL_OUT(ret);
    }
    
    while (FAIL!=(read = getline(&line, &len, f))) 
    {
        ndrx_str_strip(line, " \n\r\t");
        
        /* Ignore comments & newlines */
        if ('#'==*line || EOS==*line)
        {
            continue;
        }
        
        if (SUCCEED!=tmq_qconf_addupd(line))
        {
            FAIL_OUT(ret);
        }
    }
    free(line);
    
    
out:

    if (NULL!=f)
    {
        fclose(f);
    }

    return ret;
}

/**
 * Add queue definition. Support also update
 * We shall support Q update too...
 * Syntax: -q VISA,svcnm=VISAIF,autoq=y|n,waitinit=30,waitretry=10,waitretryinc=5,waitretrymax=40,memonly=y|n
 * @param qdef
 * @return  SUCCEED/FAIL
 */
public int tmq_qconf_addupd(char *qconfstr)
{
    tmq_qconfig_t * qconf;
    tmq_qconfig_t * dflt;
    char * p;
    char * p2;
    int got_default = FALSE;
    int qupdate = FALSE;
    char buf[MAX_TOKEN_SIZE];
    int ret = SUCCEED;
    
    NDRX_LOG(log_info, "Add new Q: [%s]", qconfstr);
    
    if (NULL==qconf)
    {
        NDRX_LOG(log_error, "Malloc failed for tmq_qconfig_t!");
        FAIL_OUT(ret);
    }
    
    MUTEX_LOCK_V(M_q_lock);
    
    p = strtok (qconfstr,",");
    
    if (NULL!=p)
    {
        NDRX_LOG(log_info, "Got token: [%s]", p);
        strncpy(buf, p, sizeof(qconf->qname)-1);
        buf[sizeof(qconf->qname)-1] = EOS;
        
        NDRX_LOG(log_debug, "Q name: [%s]", buf);
        
        if (NULL== (qconf = tmq_qconf_get(buf)))
        {
            NDRX_LOG(log_info, "Q not found, adding: [%s]", buf);
            qconf= calloc(1, sizeof(tmq_qconfig_t));
                    
            /* Try to load initial config from @ (TMQ_DEFAULT_Q) Q */
            qconf->mode = TMQ_MODE_FIFO; /* default to FIFO... */
            if (NULL!=(dflt=tmq_qconf_get(TMQ_DEFAULT_Q)))
            {
                memcpy(qconf, dflt, sizeof(*dflt));
                got_default = TRUE;
            }
            
            strcpy(qconf->qname, buf);
        }
        else
        {
            NDRX_LOG(log_info, "Q found, updating: [%s]", buf);
            qupdate = TRUE;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Missing Q name");
        FAIL_OUT(ret);
    }
    
    p = strtok (NULL, ","); /* continue... */
    
    while (p != NULL)
    {
        NDRX_LOG(log_info, "Got pair [%s]", p);
        
        strncpy(buf, p, sizeof(buf)-1);
        buf[sizeof(buf)-1] = EOS;
        
        p2 = strchr(buf, '=');
        
        if (NULL == p2)
        {
            NDRX_LOG(log_error, "Invalid key=value token [%s] expected '='", buf);
            
            userlog("Error defining queue (%s) expected in '=' in token (%s)", 
                    qconfstr, buf);
            FAIL_OUT(ret);
        }
        *p2 = EOS;
        p2++;
        
        if (EOS==*p2)
        {
            NDRX_LOG(log_error, "Empty value for token [%s]", buf);
            userlog("Error defining queue (%s) invalid value for token (%s)", 
                    qconfstr, buf);
            FAIL_OUT(ret);
        }
        
        /*
         * Load the value into structure
         */
        if (SUCCEED!=load_param(qconf, buf, p2))
        {
            NDRX_LOG(log_error, "Failed to load token [%s]/[%s]", buf, p2);
            userlog("Error defining queue (%s) failed to load token [%s]/[%s]", 
                    buf, p2);
            FAIL_OUT(ret);
        }
        
        p = strtok (NULL, ",");
    }
    
    /* Validate the config... */
    
    if (0==strcmp(qconf->qname, TMQ_DEFAULT_Q) && got_default)
    {
        NDRX_LOG(log_error, "Missing [%s] param", TMQ_QC_NAME);
        /* TODO: Return some diagnostics... => EX_QDIAGNOSTIC invalid qname */
        FAIL_OUT(ret);
    }
    /* If autoq, then service must be set. */

    if (!qupdate)
    {
        HASH_ADD_STR( G_qconf, qname, qconf );
    }
    
out:

    /* kill the record if invalid. */
    if (SUCCEED!=ret && NULL!=qconf && !qupdate)
    {
        NDRX_LOG(log_warn, "qconf -> free");
        free(qconf);
    }

    MUTEX_UNLOCK_V(M_q_lock);
    return ret;

}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
private tmq_qhash_t * tmq_qhash_get(char *qname)
{
    tmq_qhash_t * ret = NULL;
   
    HASH_FIND_STR( G_qhash, qname, ret);    
    
    return ret;
}

/**
 * Get new qhash entry + add it to hash.
 * @param qname
 * @return 
 */
private tmq_qhash_t * tmq_qhash_new(char *qname)
{
    tmq_qhash_t * ret = calloc(1, sizeof(tmq_qhash_t));
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_qhash_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_qhash_t: %s", strerror(errno));
    }
    
    strcpy(ret->qname, qname);
    
    HASH_ADD_STR( G_qhash, qname, ret );
    
    return ret;
}

/**
 * Add message to queue
 * Think about TPQLOCKED so that other thread does not get message in progress..
 * In two phase commit mode, we need to unlock message only when it is enqueued on disk.
 * 
 * @param msg
 * @return 
 */
public int tmq_msg_add(tmq_msg_t *msg, int is_recovery)
{
    int ret = SUCCEED;
    int is_locked = FALSE;
    tmq_qhash_t *qhash;
    tmq_memmsg_t *mmsg = calloc(1, sizeof(tmq_memmsg_t));
    tmq_qconfig_t * qconf;
    char msgid_str[TMMSGIDLEN_STR+1];
    char corid_str[TMCORRIDLEN_STR+1];
    
    MUTEX_LOCK_V(M_q_lock);
    is_locked = TRUE;
    
    qhash = tmq_qhash_get(msg->hdr.qname);
    qconf = tmq_qconf_get_with_default(msg->hdr.qname, NULL);
    
    if (NULL==mmsg)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        FAIL_OUT(ret);
    }
    
    if (NULL==qconf)
    {
        NDRX_LOG(log_error, "queue config not found! Cannot enqueue!");
        userlog("queue config not found! Cannot enqueue!");
        FAIL_OUT(ret);
    }
    
    if (msg->qctl.flags & TPQCORRID)
    {
        tmq_msgid_serialize(msg->qctl.corrid, corid_str);
        NDRX_LOG(log_debug, "Correlator id: [%s]", corid_str);
        
        if (NULL!=tmq_get_msg_by_corid_str(corid_str))
        {
            NDRX_LOG(log_error, "Message with corid [%s] already exists!", corid_str);
            userlog("Message with corid [%s] already exists!", corid_str);
            FAIL_OUT(ret);
        }
    }
    
    mmsg->msg = msg;
    
    /* Get the entry for hash of queues: */
    if (NULL==qhash && NULL==(qhash=tmq_qhash_new(msg->hdr.qname)))
    {
        NDRX_LOG(log_error, "Failed to get/create qhash entry for Q [%s]", 
                msg->hdr.qname);
        FAIL_OUT(ret);
    }
    
    /* Add the message to end of the queue */
    CDL_PREPEND(qhash->q, mmsg);    
    
    /* Add the hash of IDs */
    tmq_msgid_serialize(mmsg->msg->hdr.msgid, msgid_str); 
    NDRX_LOG(log_debug, "Adding to G_msgid_hash [%s]", msgid_str);
    
    strcpy(mmsg->msgid_str, msgid_str);
    HASH_ADD_STR( G_msgid_hash, msgid_str, mmsg);
    
    if (mmsg->msg->qctl.flags & TPQCORRID)
    {
        NDRX_LOG(log_debug, "Adding to G_corid_hash [%s]", corid_str);
        
        strcpy(mmsg->corid_str, corid_str);
        HASH_ADD_STR_H2( G_corid_hash, corid_str, mmsg);
    }
    /* have to unlock here, because tmq_storage_write_cmd_newmsg() migth callback to
     * us and that might cause stall.
     */
    MUTEX_UNLOCK_V(M_q_lock);
    is_locked = FALSE;
    
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
            if (SUCCEED!=tmq_storage_write_cmd_newmsg(mmsg->msg))
            {
                NDRX_LOG(log_error, "Failed to add message to persistent store!");
                FAIL_OUT(ret);
            }
        }
    }
    else
    {
        NDRX_LOG(log_info, "Mem only Q, not persisting.");   
    }
    
    qhash->numenq++;
    
    NDRX_LOG(log_debug, "Message with id [%s] successfully enqueued to [%s] queue (DEBUG: locked %ld)",
            tmq_msgid_serialize(msg->hdr.msgid, msgid_str), msg->hdr.qname, mmsg->msg->lockthreadid);
    
out:
                
    /* NOT SURE IS THIS GOOD as this might cause segmentation fault.
     * as added to mem, but failed to write to disk.
     */
    if (SUCCEED!=ret && mmsg!=NULL)
    {
        free(mmsg);
    }

    if (is_locked)
    {
        MUTEX_UNLOCK_V(M_q_lock);
    }


    return ret;
}

/**
 * Tests is auto message valid for dequeue (calculate the counters)
 * @param node
 * @return 
 */
private int tmq_is_auto_valid_for_deq(tmq_memmsg_t *node, tmq_qconfig_t *qconf)
{
    int ret = FALSE;
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
        ret=TRUE;
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
 * @return NULL (no msg), or ptr to msg
 */
public tmq_msg_t * tmq_msg_dequeue(char *qname, long flags, int is_auto)
{
    tmq_qhash_t *qhash;
    tmq_memmsg_t *node = NULL;
    tmq_memmsg_t *start = NULL;
    tmq_msg_t * ret = NULL;
    union tmq_block block;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_qconfig_t *qconf;
    
    
    NDRX_LOG(log_debug, "FIFO/LIFO dequeue for [%s]", qname);
    MUTEX_LOCK_V(M_q_lock);
    
    /* Find the non locked message in memory */
    
    /* Lock the message for current thread. 
     * The thread will later issue xA command either:
     * - Increase counter
     * - Remove the message.
     */
    if (NULL==(qhash = tmq_qhash_get(qname)))
    {
        NDRX_LOG(log_warn, "Q [%s] is NULL/empty", qname);
        goto out;
    }
    
    if (NULL==(qconf=tmq_qconf_get_with_default(qname, NULL)))
    {
        
        NDRX_LOG(log_error, "Failed to get q config [%s]", 
                node->msg->hdr.qname);
        goto out;
    }
    NDRX_LOG(log_debug, "mode: %s", TMQ_MODE_LIFO == qconf->mode?"LIFO":"FIFO");
    
    /* Start from first one & loop over the list while 
     * - we get to the first non-locked message
     * - or we get to the end with no msg, then return FAIL.
     */
    if (TMQ_MODE_LIFO == qconf->mode)
    {
        /* LIFO mode */
        if (NULL!=qhash->q)
        {
            node = qhash->q->prev;
            start = qhash->q->prev;
        }
    }
    else
    {
        /* FIFO */
        node = qhash->q;
        start = qhash->q;
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
                node = node->prev;
            }
            else
            {
                /* default to FIFO */
                node = node->next;
            }
        }
    }
    while (NULL!=node && node!=start);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_warn, "Q [%s] is empty or all msgs locked", qname);
        goto out;
    }
    
    /* Write some stuff to log */
    
    tmq_msgid_serialize(ret->hdr.msgid, msgid_str);
    NDRX_LOG(log_info, "Dequeued message: [%s]", msgid_str);
    NDRX_DUMP(log_debug, "Dequeued message", ret->msg, ret->len);
    
    /* Lock the message */
    ret->lockthreadid = ndrx_gettid();
    
    /* Is it must not be a peek and must not be an autoq */
    if (!(flags & TPQPEEK) && !is_auto)
    {   
        /* Issue command for msg remove */
        memset(&block, 0, sizeof(block));    
        memcpy(&block.hdr, &ret->hdr, sizeof(ret->hdr));
        block.hdr.command_code = TMQ_STORCMD_DEL;

        if (SUCCEED!=tmq_storage_write_cmd_block(&block, 
                "Removing dequeued message"))
        {
            NDRX_LOG(log_error, "Failed to remove msg...");
            /* unlock msg... */
            ret->lockthreadid = 0;
            ret = NULL;
            goto out;
        }
    }
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Dequeue message by msgid
 * @param msgid
 * @return 
 */
public tmq_msg_t * tmq_msg_dequeue_by_msgid(char *msgid, long flags)
{
    tmq_msg_t * ret = NULL;
    union tmq_block block;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t *mmsg;
    
    MUTEX_LOCK_V(M_q_lock);
       
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
    
    /* Issue command for msg remove */
    memset(&block, 0, sizeof(block));    
    memcpy(&block.hdr, &ret->hdr, sizeof(ret->hdr));
    
    block.hdr.command_code = TMQ_STORCMD_DEL;
    
    if (!(flags & TPQPEEK))
    {
        if (SUCCEED!=tmq_storage_write_cmd_block(&block, 
                "Removing dequeued message"))
        {
            NDRX_LOG(log_error, "Failed to remove msg...");
            /* unlock msg... */
            ret->lockthreadid = 0;
            ret = NULL;
            goto out;
        }
    }
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Dequeue message by corid
 * @param msgid
 * @return 
 */
public tmq_msg_t * tmq_msg_dequeue_by_corid(char *corid, long flags)
{
    tmq_msg_t * ret = NULL;
    union tmq_block block;
    char corid_str[TMCORRIDLEN_STR+1];
    tmq_memmsg_t *mmsg;
    
    MUTEX_LOCK_V(M_q_lock);
       
    /* Write some stuff to log */
    
    tmq_msgid_serialize(corid, corid_str);
    NDRX_LOG(log_info, "CORID: Dequeuing message by [%s]", corid_str);
    
    if (NULL==(mmsg = tmq_get_msg_by_corid_str(corid_str)))
    {
        NDRX_LOG(log_error, "Message not found by corid_str [%s]", corid_str);
        goto out;
    }
    
    /* Lock the message */
    ret = mmsg->msg;
    
    NDRX_DUMP(log_debug, "Dequeued message", ret->msg, ret->len);
    
    ret->lockthreadid = ndrx_gettid();
    
    /* Issue command for msg remove */
    memset(&block, 0, sizeof(block));    
    memcpy(&block.hdr, &ret->hdr, sizeof(ret->hdr));
    
    if (!(flags & TPQPEEK))   
    {    
        block.hdr.command_code = TMQ_STORCMD_DEL;

        if (SUCCEED!=tmq_storage_write_cmd_block(&block, 
                "Removing dequeued message"))
        {
            NDRX_LOG(log_error, "Failed to remove msg...");
            /* unlock msg... */
            ret->lockthreadid = 0;
            ret = NULL;
            goto out;
        }
    }
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Get message by msgid
 * @param msgid
 * @return 
 */
private tmq_memmsg_t* tmq_get_msg_by_msgid_str(char *msgid_str)
{
    tmq_memmsg_t *ret;
    
    HASH_FIND_STR( G_msgid_hash, msgid_str, ret);
    
    return ret;
}


/**
 * Get message by corid string version
 * @param corid_str
 * @return 
 */
private tmq_memmsg_t* tmq_get_msg_by_corid_str(char *corid_str)
{
    tmq_memmsg_t *ret;
    
    HASH_FIND_STR_H2( G_corid_hash, corid_str, ret);
    
    return ret;
}

/**
 * get message by correlator
 * @param corid correlator (binary)
 * @return ptr to memmsg if found or NULL
 */
private tmq_memmsg_t* tmq_get_msg_by_corid(char *corid)
{
    tmq_memmsg_t *ret;
    char corid_str[TMCORRIDLEN_STR+1];
    
    tmq_corid_serialize(corid, corid_str);
    
    return tmq_get_msg_by_corid_str(corid_str);
    
    return ret;
}



/**
 * Remove mem message
 * 
 * @param msg
 */
private void tmq_remove_msg(tmq_memmsg_t *mmsg)
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
    HASH_DEL( G_msgid_hash, mmsg);
    
    if (TPQCORRID & mmsg->msg->qctl.flags)
    {
        HASH_DEL_H2( G_corid_hash, mmsg);
    }
    
    free(mmsg->msg);
    free(mmsg);
}

/**
 * Unlock message by updated block
 * We can:
 * - update content + unlock
 * - or remove the message
 * @param p_b
 * @return 
 */
public int tmq_unlock_msg(union tmq_upd_block *b)
{
    int ret = SUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(b->hdr.msgid, msgid_str);
    
    NDRX_LOG(log_info, "Unlocking/updating: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    mmsg = tmq_get_msg_by_msgid_str(msgid_str);
    
    if (NULL==mmsg)
    {   
        NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);
        FAIL_OUT(ret);
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
            mmsg->msg->lockthreadid = 0;
        /* And still we want unblock: */
        case TMQ_STORCMD_NEWMSG:
        case TMQ_STORCMD_UNLOCK:
            NDRX_LOG(log_info, "Unlocking message...");
            mmsg->msg->lockthreadid = 0;
            break;
        default:
            NDRX_LOG(log_info, "Unknown command [%c]", b->hdr.command_code);
            FAIL_OUT(ret);
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
public int tmq_unlock_msg_by_msgid(char *msgid)
{
    int ret = SUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(msgid, msgid_str);
    
    NDRX_LOG(log_info, "Unlocking/updating: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    mmsg = tmq_get_msg_by_msgid_str(msgid_str);
    
    if (NULL==mmsg)
    {   
        NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);
        FAIL_OUT(ret);
    }
    
    mmsg->msg->lockthreadid = 0;
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}


/**
 * 
 * @param msgid
 * @return 
 */
public int tmq_lock_msg(char *msgid)
{
    int ret = SUCCEED;
    char msgid_str[TMMSGIDLEN_STR+1];
    tmq_memmsg_t* mmsg;
    
    tmq_msgid_serialize(msgid, msgid_str);
    
    NDRX_LOG(log_info, "Locking: %s", msgid_str);
    
    MUTEX_LOCK_V(M_q_lock);
    
    mmsg = tmq_get_msg_by_msgid_str(msgid_str);
    
    if (NULL==mmsg)
    {   
        NDRX_LOG(log_error, "Message not found: [%s] - no update", msgid_str);
        FAIL_OUT(ret);
    }
    
    /* Lock the message */
    mmsg->msg->lockthreadid = ndrx_gettid();
    
out:
    MUTEX_UNLOCK_V(M_q_lock);
    return ret;
}

/**
 * Process message blocks on disk read (after cold startup)
 * @param p_block
 * @return 
 */
private int process_block(union tmq_block **p_block)
{
    int ret = SUCCEED;
    
    switch((*p_block)->hdr.command_code)
    {
        case TMQ_STORCMD_NEWMSG:
            
            if (SUCCEED!=tmq_msg_add((tmq_msg_t *)(*p_block), TRUE))
            {
                NDRX_LOG(log_error, "Failed to enqueue!");
                FAIL_OUT(ret);
            }
            *p_block = NULL;
            break;
        default:
            if (SUCCEED!=tmq_lock_msg((*p_block)->hdr.msgid))
            {
                NDRX_LOG(log_error, "Failed to lock message!");
                FAIL_OUT(ret);
            }
            break;
    }
    
out:
    /* free the mem if needed: */
    if (NULL!=*p_block)
    {
        free((char *)*p_block);
        *p_block = NULL;
    }
    return ret;
}

/**
 * compare two Q entries, by time + counter
 * @param q1
 * @param q2
 * @return 
 */
private int q_msg_sort(tmq_memmsg_t *q1, tmq_memmsg_t *q2)
{
    
    return ndrx_compare3(q1->msg->msgtstamp, q1->msg->msgtstamp_usec, q1->msg->msgtstamp_cntr, 
            q2->msg->msgtstamp, q2->msg->msgtstamp_usec, q2->msg->msgtstamp_cntr);
    
}

/**
 * Return list of auto queues
 * @return NULL or list
 */
public fwd_qlist_t *tmq_get_qlist(int auto_only, int incl_def)
{
    fwd_qlist_t * ret = NULL;
    fwd_qlist_t * tmp = NULL;
    
    tmq_qhash_t *q, *qtmp;
    tmq_qconfig_t *qconf;
    
    tmq_qconfig_t *qc, *qctmp;
    
    MUTEX_LOCK_V(M_q_lock);
    
    HASH_ITER(hh, G_qhash, q, qtmp)
    {
        if (NULL!=(qconf=tmq_qconf_get_with_default(q->qname, NULL)) && 
                ((auto_only && qconf->autoq) || !auto_only))
        {
            if (NULL==(tmp = calloc(1, sizeof(fwd_qlist_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                ret = NULL;
                goto out;
            }
            NDRX_LOG(log_debug, "tmq_get_qlist: %s", q->qname);
            strcpy(tmp->qname, q->qname);
            tmp->succ = q->succ;
            tmp->fail = q->fail;
            
            tmp->numenq = q->numenq;
            tmp->numdeq = q->numdeq;
            
            DL_APPEND(ret, tmp);
        }
        
    }
    
    /* If we need to include definitions
     * Then iterate over qdefs and change which are not in the G_qhash
     * Those add to return DL
     */
    if (incl_def)
    {
        HASH_ITER(hh, G_qconf, qc, qctmp)
        {
            if (NULL==tmq_qhash_get(qc->qname))
            {
                if (NULL==(tmp = calloc(1, sizeof(fwd_qlist_t))))
                {
                    int err = errno;
                    NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                    userlog("Failed to alloc: %s", strerror(err));
                    ret = NULL;
                    goto out;
                }
                NDRX_LOG(log_debug, "tmq_get_qlist: %s", qc->qname);
                strcpy(tmp->qname, qc->qname);
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
public tmq_memmsg_t *tmq_get_msglist(char *qname)
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
            if (NULL==(tmp = calloc(1, sizeof(tmq_memmsg_t))))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
                userlog("Failed to alloc: %s", strerror(err));
                ret = NULL;
                goto out;
            }
            
            if (NULL==(msg = malloc(sizeof(tmq_msg_t))))
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
public int tmq_sort_queues(void)
{
    int ret = SUCCEED;
    tmq_qhash_t *q, *qtmp;
    
    MUTEX_LOCK_V(M_q_lock);
    
    /* iterate over Q hash & and sort them by Q time */
    HASH_ITER(hh, G_qhash, q, qtmp)
    {
        CDL_SORT(q->q, q_msg_sort);
    }   
    
out:
            
    MUTEX_UNLOCK_V(M_q_lock);

    return ret;
}

/**
 * Load the messages from QSPACE (after startup)...
 * This does not need a lock, because it uses other globals
 * @param b
 * @return 
 */
public int tmq_load_msgs(void)
{
    int ret = SUCCEED;
    
    NDRX_LOG(log_info, "Reading messages from disk...");
    /* populate all queues - from XA source */
    if (SUCCEED!=tmq_storage_get_blocks(process_block,  (short)tpgetnodeid(), 
            (short)tpgetsrvid()))
    {
        FAIL_OUT(ret);
    }
    
    /* sort all queues (by submission time) */
    tmq_sort_queues();
    
out:
    NDRX_LOG(log_info, "tmq_load_msgs return %d", ret);
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
public int tmq_update_q_stats(char *qname, long succ_diff, long fail_diff)
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

    return SUCCEED;
}

/**
 * Return infos about enqueued messages.
 * @param qname
 * @param p_msgs
 * @param p_locked
 * @return 
 */
public void tmq_get_q_stats(char *qname, long *p_msgs, long *p_locked)
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

