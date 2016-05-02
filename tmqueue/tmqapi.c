/* 
** Tmqueue server - transaction monitor
** After that log transaction to hash & to disk for tracking the stuff...
** TODO: We should have similar control like "TP_COMMIT_CONTROL" -
** either return after stuff logged or after really commit completed.
** Error handling:
** - System errors we will track via ATMI interface error functions
** - XA errors will be tracked via XA error interface
**
** @file tmqapi.c
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

#include <ndrxdcmn.h>

#include "tmqueue.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "nstdutil.h"
#include <qcommon.h>
#include "tmqd.h"
#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private __thread int is_xa_open = FALSE;
/*---------------------------Prototypes---------------------------------*/

/******************************************************************************/
/*                               Q API COMMANDS                               */
/******************************************************************************/

/**
 * Do the internal commit of transaction (request sent from other TM)
 * @param p_ub
 * @return 
 */
public int tmq_enqueue(UBFH *p_ub)
{
    int ret = SUCCEED;
    tmq_msg_t *p_msg = NULL;
    char *data = NULL;
    BFLDLEN len = 0;
    TPQCTL qctl_out;
    int local_tx = FALSE;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_enqueue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_enqueue called with", p_ub);
    
    if (!is_xa_open)
    {
        if (SUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            is_xa_open = TRUE;
        }
    }
            
    if (!tpgetlev())
    {
        NDRX_LOG(log_debug, "Not in global transaction, starting local...");
        if (SUCCEED!=tpbegin(G_tmqueue_cfg.dflt_timeout, 0))
        {
            NDRX_LOG(log_error, "Failed to start global tx!");
            userlog("Failed to start global tx!");
        }
        else
        {
            NDRX_LOG(log_debug, "Transaction started ok...");
            local_tx = TRUE;
        }
    }
    
    memset(&qctl_out, 0, sizeof(qctl_out));
    
    if (NULL==(data = Bgetalloc(p_ub, EX_DATA, 0, &len)))
    {
        NDRX_LOG(log_error, "Missing EX_DATA!");
        userlog("Missing EX_DATA!");
        
        strcpy(qctl_out.diagmsg, "Missing EX_DATA!");
        qctl_out.diagnostic = QMEINVAL;
        
        FAIL_OUT(ret);
    }
    
    /*
     * Get the message size in EX_DATA
     */
    p_msg = malloc(sizeof(tmq_msg_t)+len);
    
    if (NULL==p_msg)
    {
        NDRX_LOG(log_error, "Failed to malloc tmq_msg_t!");
        userlog("Failed to malloc tmq_msg_t!");
        
        strcpy(qctl_out.diagmsg, "Failed to malloc tmq_msg_t!");
        qctl_out.diagnostic = QMEOS;
        
        FAIL_OUT(ret);
    }
    
    memset(p_msg, 0, sizeof(tmq_msg_t));
    
    memcpy(p_msg->msg, data, len);
    p_msg->len = len;    
    
    NDRX_DUMP(log_debug, "Got message for Q: ", p_msg->msg, p_msg->len);
    
    if (SUCCEED!=Bget(p_ub, EX_QNAME, 0, p_msg->hdr.qname, 0))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to get EX_QNAME");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to get EX_QNAME!");
        qctl_out.diagnostic = QMEINVAL;
        
        FAIL_OUT(ret);
    }
    
    /* Restore back the C structure */
    if (SUCCEED!=tmq_tpqctl_from_ubf_enqreq(p_ub, &p_msg->qctl))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        userlog("tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        qctl_out.diagnostic = QMESYSTEM;
        
        FAIL_OUT(ret);
    }
    
    /* Build up the message. */
    tmq_setup_cmdheader_newmsg(&p_msg->hdr, p_msg->hdr.qname, 
            tpgetnodeid(), G_server_conf.srv_id, G_tmqueue_cfg.qspace);
    
    memcpy(qctl_out.msgid, p_msg->hdr.msgid, TMMSGIDLEN);
    p_msg->lockthreadid = ndrx_gettid(); /* Mark as locked by thread */
    p_msg->msgtstamp = nstdutil_utc_tstamp();
    p_msg->status = TMQ_STATUS_ACTIVE;
    
    NDRX_LOG(log_info, "Messag prepared ok, about to enqueue to [%s] Q...",
            p_msg->hdr.qname);
    
    if (SUCCEED!=tmq_msg_add(p_msg))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to enqueue!");
        userlog("tmq_enqueue: failed to enqueue!");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to enqueue!");
        
        qctl_out.diagnostic = QMESYSTEM;
        
        FAIL_OUT(ret);
    }
    
out:
    /* free up the temp memory */
    if (NULL!=data)
    {
        free(data);
    }

    if (SUCCEED!=ret && NULL!=p_msg)
    {
        NDRX_LOG(log_warn, "About to free p_msg!");
        free(p_msg);
    }

    if (local_tx)
    {
        if (SUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Aborting transaction");
            tpabort(0);
        } else
        {
            NDRX_LOG(log_info, "Committing transaction!");
            if (SUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Commit failed!");
                userlog("Commit failed!");
                strcpy(qctl_out.diagmsg, "tmq_enqueue: commit failed!");
                ret=FAIL;
            }
        }
    }

    /* Setup response fields
     * Not sure about existing ones (seems like they will stay in buffer)
     * i.e. request fields
     */
    if (SUCCEED!=tmq_tpqctl_to_ubf_enqrsp(p_ub, &qctl_out))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to generate response buffer!");
        userlog("tmq_enqueue: failed to generate response buffer!");
    }

    return ret;
}

/**
 * Dequeue message
 * TODO: Support TPQGETBYMSGID
 * TODO: Support TPQGETBYCORRID
 * @param p_ub
 * @return 
 */
public int tmq_dequeue(UBFH *p_ub)
{
    /* Search for not locked message, lock it, issue command to disk for
     * delete & return the message to buffer (also needs to resize the buffer correctly)
     */
    int ret = SUCCEED;
    tmq_msg_t *p_msg = NULL;
    TPQCTL qctl_out;
    int local_tx = FALSE;
    char qname[TMQNAMELEN+1];
    long buf_realoc_size;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_dequeue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_dequeue called with", p_ub);
    
    if (!is_xa_open)
    {
        if (SUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            is_xa_open = TRUE;
        }
    }
    
    /* TODO: Read ctl (to have msgid search for, or corelator id) */
    if (!tpgetlev())
    {
        NDRX_LOG(log_debug, "Not in global transaction, starting local...");
        if (SUCCEED!=tpbegin(G_tmqueue_cfg.dflt_timeout, 0))
        {
            NDRX_LOG(log_error, "Failed to start global tx!");
            userlog("Failed to start global tx!");
        }
        else
        {
            NDRX_LOG(log_debug, "Transaction started ok...");
            local_tx = TRUE;
        }
    }
    
    memset(&qctl_out, 0, sizeof(qctl_out));
    
    if (SUCCEED!=Bget(p_ub, EX_QNAME, 0, qname, 0))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to get EX_QNAME");
        strcpy(qctl_out.diagmsg, "tmq_dequeue: failed to get EX_QNAME!");
        qctl_out.diagnostic = QMEINVAL;
        
        FAIL_OUT(ret);
    }
    
    /* Get FB size (current) */
    if (NULL==(p_msg = tmq_msg_dequeue_fifo(qname)))
    {
        NDRX_LOG(log_error, "tmq_dequeue: not message in Q [%s]", qname);
        strcpy(qctl_out.diagmsg, "tmq_dequeue: no message int Q!");
        qctl_out.diagnostic = QMENOMSG;
        
        FAIL_OUT(ret);
    }
    
    /* Use the original metadata */
    memcpy(&qctl_out, &p_msg->qctl, sizeof(qctl_out));
    
    buf_realoc_size = Bused (p_ub) + p_msg->len + 1024;
    
    if (NULL==(p_ub = (UBFH *)tprealloc ((char *)p_ub, buf_realoc_size)))
    {
        NDRX_LOG(log_error, "Failed to allocate buffer to size: %ld", buf_realoc_size);
        userlog("Failed to allocate buffer to size: %ld", buf_realoc_size);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_DATA, 0, p_msg->msg, p_msg->len))
    {
        NDRX_LOG(log_error, "failed to set EX_DATA!");
        userlog("failed to set EX_DATA!");
        
        strcpy(qctl_out.diagmsg, "failed to set EX_DATA!");
        qctl_out.diagnostic = QMEINVAL;
        
        FAIL_OUT(ret);
    }
    
    
out:

    if (local_tx)
    {
        if (SUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Aborting transaction");
            tpabort(0);
        } 
        else
        {
            NDRX_LOG(log_info, "Committing transaction!");
            if (SUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Commit failed!");
                userlog("Commit failed!");
                strcpy(qctl_out.diagmsg, "tmq_enqueue: commit failed!");
                ret=FAIL;
            }
        }
    }

    /* Setup response fields
     * Not sure about existing ones (seems like they will stay in buffer)
     * i.e. request fields
     */
    if (SUCCEED!=tmq_tpqctl_to_ubf_deqrsp(p_ub, &qctl_out))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to generate response buffer!");
        userlog("tmq_enqueue: failed to generate response buffer!");
    }

    return ret;
}

/**
 * Unlock the message
 * @param p_ub
 * @return 
 */
public int tmq_notify(UBFH *p_ub)
{
    int ret = SUCCEED;
    union tmq_upd_block b;
    BFLDLEN len = sizeof(b);
     
    if (SUCCEED!=Bget(p_ub, EX_DATA, 0, (char *)&b, &len))
    {
        NDRX_LOG(log_error, "Failed to get EX_DATA: %s", Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tmq_unlock_msg(&b))
    {
        NDRX_LOG(log_error, "Failed to unlock message...");
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}

/******************************************************************************/
/*                         COMMAND LINE API                                   */
/******************************************************************************/
/**
 * Return list of transactions
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
public int tmq_printqueue(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
        
    /* Bfprint(p_ub, stderr); */

    if (FAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
        FAIL_OUT(ret);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }

    
out:

    return ret;
}
