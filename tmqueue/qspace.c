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

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include "thpool.h"
#include "nstdutil.h"
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

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* Handler for MSG Hash. */
public tmq_memmsg_t *G_msgid_hash;

/* Handler for Q hash */
public tmq_qhash_t *G_qhash;
/* - think later about use. MUTEX_LOCKDECL(M_qhash_lock);  Operations with hash list            */


/* Configuration section */
public tmq_qconfig_t *G_qconf; 
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

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
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->tries = ival;
    }
    else if (0==strcmp(key, TMQ_QC_AUTOQ))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitinit = ival;
        
    }
    else if (0==strcmp(key, TMQ_QC_WAITINIT))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
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
        if (!nstdutil_isint(value) || ival < 0)
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
        if (!nstdutil_isint(value) || ival < 0)
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
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretrymax = ival;
    }
    else if (0==strcmp(key, TMQ_QC_MEMONLY))
    {
        if (value[0]=='y' || value[0]=='Y')
        {
            qconf->memonly = TRUE;
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
public tmq_qconfig_t * tmq_qconf_get(char *qname)
{
    tmq_qconfig_t *ret = NULL;
    
    HASH_FIND_STR( G_qconf, qname, ret);    
    
    return ret;
}

/**
 * TODO: Return Q config with default
 * @param qname
 * @return 
 */
public tmq_qconfig_t * tmq_qconf_get_with_default(char *qname)
{
    return NULL; /* TODO:  */
}

/**
 * Remove queue probably then existing messages will fall back to default Q
 * @param name
 * @return 
 */
public int tmq_qconf_delete(char *qname)
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
 * Add queue definition. Support also update
 * We shall support Q update too...
 * Syntax: -q VISA,svc=VISAIF,auto=y|n,waitinit=30,waitretry=10,waitretryinc=5,waitretrymax=40,memonly=y|n
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
    
    p = strtok (qconfstr,",");
    
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
    
    if (qupdate)
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

    return ret;

}

/**
 * Get QHASH record for particular q
 * @param qname
 * @return 
 */
public tmq_qhash_t * tmq_qhash_get(char *qname)
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
public tmq_qhash_t * tmq_qhash_new(char *qname)
{
    tmq_qhash_t * ret = calloc(1, sizeof(tmq_qhash_t));
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_qhash_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_qhash_t: %s", strerror(errno));
    }
    
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
public int tmq_msg_add(tmq_msg_t *msg)
{
    int ret = SUCCEED;
    tmq_qhash_t *qhash = tmq_qhash_get(msg->hdr.qname);
    tmq_memmsg_t *mmsg = calloc(1, sizeof(tmq_memmsg_t));
    
    if (NULL==mmsg)
    {
        NDRX_LOG(log_error, "Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        userlog("Failed to alloc tmq_memmsg_t: %s", strerror(errno));
        FAIL_OUT(ret);
    }
    
    memcpy(&mmsg->msg, msg, sizeof(*msg));
    
    /* Get the entry for hash of queues: */
    if (NULL==qhash && NULL==tmq_qhash_new(msg->hdr.qname))
    {
        NDRX_LOG(log_error, "Failed to get/create qhash entry for Q [%s]", 
                msg->hdr.qname);
        FAIL_OUT(ret);
    }
    
    /* Add the message to end of the queue */
    DL_APPEND(qhash->q, mmsg);
    
    /* Add the hash of IDs */
    HASH_ADD_STR( G_msgid_hash, msgid_str, mmsg);
    
    /* Decide do we need to add the msg to disk?! 
     * Needs to send a command to XA sub-system to prepare msg/command to disk,
     * if it is not memory only.
     * So next step todo is to write xa command handler & dumping commands to disk.
     */
    /* TODO: check the config tmq_qconf_get_with_default() is it memory q only? */
    if (SUCCEED!=tmq_storage_write_cmd_newmsg(&mmsg->msg))
    {
        NDRX_LOG(log_error, "Failed to add message to persistent store!");
        FAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Message with id [%s] successfully enqueued to [%s] queue",
            msg->hdr.msgid, msg->hdr.qname);
    
out:

    if (SUCCEED!=ret && mmsg!=NULL)
    {
        free(mmsg);
    }

    return ret;
}

/**
 * Get the fifo message from Q
 * @param msg
 * @return 
 */
public int tmq_msg_dequeue_fifo(char *qname, tmq_msg_t *msg)
{
    int ret = SUCCEED;
    
    /* Find the non locked message in memory */
    
    /* Lock the message for current thread. 
     * The thread will later issue xA command either:
     * - Increase counter
     * - Remove the message.
     */
    
out:
    return ret;
}
