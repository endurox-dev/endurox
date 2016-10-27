/* 
** Tmqueue server - transaction monitor
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
#include "cconfig.h"
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private __thread int M_is_xa_open = FALSE;
MUTEX_LOCKDECL(M_tstamp_lock);
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
    
    /* To guarentee unique order in same Q space...: */
    static long t_sec = 0;
    static long t_usec = 0;
    static int t_cntr = 0;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_enqueue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_enqueue called with", p_ub);
    
    if (!M_is_xa_open)
    {
        if (SUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            M_is_xa_open = TRUE;
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
    p_msg = NDRX_MALLOC(sizeof(tmq_msg_t)+len);
    
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
    
    if (SUCCEED!=Bget(p_ub, EX_DATA_BUFTYP, 0, (char *)&p_msg->buftyp, 0))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to get EX_DATA_BUFTYP");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to get EX_DATA_BUFTYP!");
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
            tpgetnodeid(), G_server_conf.srv_id, G_tmqueue_cfg.qspace, p_msg->qctl.flags);
    
    /* Return the message id. */
    memcpy(qctl_out.msgid, p_msg->hdr.msgid, TMMSGIDLEN);
    memcpy(p_msg->qctl.msgid, p_msg->hdr.msgid, TMMSGIDLEN);
    
    p_msg->lockthreadid = ndrx_gettid(); /* Mark as locked by thread */

    
    ndrx_utc_tstamp2(&p_msg->msgtstamp, &p_msg->msgtstamp_usec);
    
    MUTEX_LOCK_V(M_tstamp_lock);
    
    if (p_msg->msgtstamp == t_sec && p_msg->msgtstamp_usec == t_usec)
    {
        t_cntr++;
    }
    else
    {
        t_sec = p_msg->msgtstamp;
        t_usec = p_msg->msgtstamp_usec;
        t_cntr = 0;
    }
    MUTEX_UNLOCK_V(M_tstamp_lock);
    
    p_msg->msgtstamp_cntr = t_cntr;
    p_msg->status = TMQ_STATUS_ACTIVE;
    
    NDRX_LOG(log_info, "Messag prepared ok, about to enqueue to [%s] Q...",
            p_msg->hdr.qname);
    
    if (SUCCEED!=tmq_msg_add(p_msg, FALSE))
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
        NDRX_FREE(data);
    }

    if (SUCCEED!=ret && NULL!=p_msg)
    {
        NDRX_LOG(log_warn, "About to free p_msg!");
        NDRX_FREE(p_msg);
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
 * @param p_ub
 * @return 
 */
public int tmq_dequeue(UBFH **pp_ub)
{
    /* Search for not locked message, lock it, issue command to disk for
     * delete & return the message to buffer (also needs to resize the buffer correctly)
     */
    int ret = SUCCEED;
    tmq_msg_t *p_msg = NULL;
    TPQCTL qctl_out, qctl_in;
    int local_tx = FALSE;
    char qname[TMQNAMELEN+1];
    long buf_realoc_size;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_dequeue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_dequeue called with", *pp_ub);
    
    if (!M_is_xa_open)
    {
        if (SUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            M_is_xa_open = TRUE;
        }
    }
    
    memset(&qctl_in, 0, sizeof(qctl_in));
    
    if (SUCCEED!=tmq_tpqctl_from_ubf_deqreq(*pp_ub, &qctl_in))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to read request qctl!");
        userlog("tmq_dequeue: failed to read request qctl!");
        FAIL_OUT(ret);
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
    
    if (SUCCEED!=Bget(*pp_ub, EX_QNAME, 0, qname, 0))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to get EX_QNAME");
        strcpy(qctl_out.diagmsg, "tmq_dequeue: failed to get EX_QNAME!");
        qctl_out.diagnostic = QMEINVAL;
        
        FAIL_OUT(ret);
    }
    
    /* Get FB size (current) */
    NDRX_LOG(log_warn, "qctl_req flags: %ld", qctl_in.flags);
    
    if (qctl_in.flags & TPQGETBYMSGID)
    {
        if (NULL==(p_msg = tmq_msg_dequeue_by_msgid(qctl_in.msgid, qctl_in.flags)))
        {
            char msgid_str[TMMSGIDLEN_STR+1];
            
            tmq_msgid_serialize(qctl_in.msgid, msgid_str);
            
            NDRX_LOG(log_error, "tmq_dequeue: no message found for given msgid [%s]", 
                    msgid_str);
            strcpy(qctl_out.diagmsg, "tmq_dequeue: no message found for given msgid");
            qctl_out.diagnostic = QMENOMSG;
            FAIL_OUT(ret);
        }
    }
    else if (qctl_in.flags & TPQGETBYCORRID)
    {
        if (NULL==(p_msg = tmq_msg_dequeue_by_corid(qctl_in.corrid, qctl_in.flags)))
        {
            char corid_str[TMCORRIDLEN_STR+1];
            
            tmq_corid_serialize(qctl_in.corrid, corid_str);
            
            NDRX_LOG(log_error, "tmq_dequeue: no message found for given msgid [%s]", 
                    corid_str);
            strcpy(qctl_out.diagmsg, "tmq_dequeue: no message found for given msgid");
            qctl_out.diagnostic = QMENOMSG;
            FAIL_OUT(ret);
        }
    }
    else if (NULL==(p_msg = tmq_msg_dequeue(qname, qctl_in.flags, FALSE)))
    {
        NDRX_LOG(log_error, "tmq_dequeue: no message in Q [%s]", qname);
        strcpy(qctl_out.diagmsg, "tmq_dequeue: no message in Q!");
        qctl_out.diagnostic = QMENOMSG;
        
        FAIL_OUT(ret);
    }
    
    /* Use the original metadata */
    memcpy(&qctl_out, &p_msg->qctl, sizeof(qctl_out));
    
    buf_realoc_size = Bused (*pp_ub) + p_msg->len + 1024;
    
    if (NULL==(*pp_ub = (UBFH *)tprealloc ((char *)*pp_ub, buf_realoc_size)))
    {
        NDRX_LOG(log_error, "Failed to allocate buffer to size: %ld", buf_realoc_size);
        userlog("Failed to allocate buffer to size: %ld", buf_realoc_size);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(*pp_ub, EX_DATA, 0, p_msg->msg, p_msg->len))
    {
        NDRX_LOG(log_error, "failed to set EX_DATA!");
        userlog("failed to set EX_DATA!");
        
        strcpy(qctl_out.diagmsg, "failed to set EX_DATA!");
        qctl_out.diagnostic = QMEINVAL;
        
        /* Unlock msg if it was peek */
        if (TPQPEEK & qctl_in.flags)
        {
            tmq_unlock_msg_by_msgid(p_msg->qctl.msgid);
        }
        
        FAIL_OUT(ret);
    }
    
    /* Unlock msg if it was peek */
    if (TPQPEEK & qctl_in.flags && 
            SUCCEED!=tmq_unlock_msg_by_msgid(p_msg->qctl.msgid))
    {
        NDRX_LOG(log_error, "Failed to unlock msg!");
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(*pp_ub, EX_DATA_BUFTYP, 0, (char *)&p_msg->buftyp, 0))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to set EX_DATA_BUFTYP");
        
        strcpy(qctl_out.diagmsg, "tmq_dequeue: failed to set EX_DATA_BUFTYP!");
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
    if (SUCCEED!=tmq_tpqctl_to_ubf_deqrsp(*pp_ub, &qctl_out))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to generate response buffer!");
        userlog("tmq_dequeue: failed to generate response buffer!");
    }

    return ret;
}

/**
 * Unlock the message
 * @param p_ub
 * @return 
 */
public int tex_mq_notify(UBFH *p_ub)
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
 * Return list of queues
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
public int tmq_mqlq(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
    fwd_qlist_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_printqueue";
    /* Get list of queues */
    
    if (NULL==(list = tmq_get_qlist(FALSE, TRUE)))
    {
        NDRX_LOG(log_info, "%s: No queues found", fn);
    }
    else
    {
        NDRX_LOG(log_info, "%s: Queues found", fn);
    }
    
    DL_FOREACH_SAFE(list,el,tmp)
    {
        long msgs = 0;
        long locked = 0;
    
        tmq_get_q_stats(el->qname, &msgs, &locked);
                
        NDRX_LOG(log_debug, "returning %s/%s", G_tmqueue_cfg.qspace, el->qname);
        
        if (SUCCEED!=Bchg(p_ub, EX_QSPACE, 0, G_tmqueue_cfg.qspace, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNAME, 0, el->qname, 0L) ||
            SUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMMSG, 0, (char *)&msgs, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMLOCKED, 0, (char *)&locked, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMSUCCEED, 0, (char *)&el->succ, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMFAIL, 0, (char *)&el->fail, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMENQ, 0, (char *)&el->numenq, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QNUMDEQ, 0, (char *)&el->numdeq, 0L) 
                )
        {
            NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
            FAIL_OUT(ret);
        }
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
        
        DL_DELETE(list, el);
        NDRX_FREE((char *)el);
    }
    
out:

    return ret;
}

/**
 * List queue definitions/config (return the defaulted flag if so)
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
public int tmq_mqlc(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
    fwd_qlist_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_mqlc";
    char qdef[TMQ_QDEF_MAX];
    char flags[128];
    int is_default = FALSE;
    /* Get list of queues */
    
    if (NULL==(list = tmq_get_qlist(FALSE, TRUE)))
    {
        NDRX_LOG(log_info, "%s: No queues found", fn);
    }
    else
    {
        NDRX_LOG(log_info, "%s: Queues found", fn);
    }
    
    DL_FOREACH_SAFE(list,el,tmp)
    {
        is_default = FALSE;
        if (SUCCEED==tmq_build_q_def(el->qname, &is_default, qdef))
        {
            NDRX_LOG(log_debug, "returning %s/%s", G_tmqueue_cfg.qspace, el->qname);
            
            flags[0] = EOS;
            
            if (is_default)
            {
                strcat(flags, "D");
            }
            

            if (SUCCEED!=Bchg(p_ub, EX_QSPACE, 0, G_tmqueue_cfg.qspace, 0L) ||
                SUCCEED!=Bchg(p_ub, EX_QNAME, 0, el->qname, 0L) ||
                SUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
                SUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
                SUCCEED!=CBchg(p_ub, EX_DATA, 0, qdef, 0L, BFLD_STRING) ||
                SUCCEED!=Bchg(p_ub, EX_QSTRFLAGS, 0, flags, 0L)
                )
            {
                NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
                FAIL_OUT(ret);
            }
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
        }
        
        DL_DELETE(list, el);
        NDRX_FREE((char *)el);
    }
    
out:

    return ret;
}

/**
 * List messages in queue
 * @param p_ub
 * @param cd
 * @return 
 */
public int tmq_mqlm(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
    tmq_memmsg_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_mqlm";
    char qname[TMQNAMELEN+1];
    short is_locked;
    char msgid_str[TMMSGIDLEN_STR+1];

    /* Get list of queues */
    
    if (SUCCEED!=Bget(p_ub, EX_QNAME, 0, qname, 0L))
    {
        NDRX_LOG(log_error, "Failed to get qname");
        FAIL_OUT(ret);
    }
    
    if (NULL==(list = tmq_get_msglist(qname)))
    {
        NDRX_LOG(log_info, "%s: no messages in q", fn);
    }
    else
    {
        NDRX_LOG(log_info, "%s: messages found", fn);
    }
    
    DL_FOREACH_SAFE(list,el,tmp)
    {
        if (el->msg->lockthreadid)
        {
            is_locked = TRUE;
        }
        else
        {
            is_locked = FALSE;
        }
        
        tmq_msgid_serialize(el->msg->hdr.msgid, msgid_str);
        
        if (SUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QMSGIDSTR, 0, msgid_str, 0L)  ||
            SUCCEED!=Bchg(p_ub, EX_TSTAMP1_STR, 0, 
                ndrx_get_strtstamp2(0, el->msg->msgtstamp, el->msg->msgtstamp_usec), 0L) ||
            SUCCEED!=Bchg(p_ub, EX_TSTAMP2_STR, 0, 
                ndrx_get_strtstamp2(1, el->msg->trytstamp, el->msg->trytstamp_usec), 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QMSGTRIES, 0, (char *)&el->msg->trycounter, 0L) ||
            SUCCEED!=Bchg(p_ub, EX_QMSGLOCKED, 0, (char *)&is_locked, 0L)
                )
        {
            NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
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
        
        DL_DELETE(list, el);
        NDRX_FREE((char *)el->msg);
        NDRX_FREE((char *)el);
    }
    
out:

    return ret;
}

/**
 * Reload config
 * @param p_ub
 * @return 
 */
public int tmq_mqrc(UBFH *p_ub)
{
    int ret = SUCCEED;
    
    /* if have CC */
    if (ndrx_get_G_cconfig())
    {
        ndrx_cconfig_reload();
    }
    
    ret = tmq_reload_conf(G_tmqueue_cfg.qconfig);
    
out:
    return ret;
}

/**
 * Change config
 * @param p_ub
 * @return 
 */
public int tmq_mqch(UBFH *p_ub)
{
    int ret = SUCCEED;
    char conf[512];
    BFLDLEN len = sizeof(conf);
    
    if (SUCCEED!=CBget(p_ub, EX_DATA, 0, conf, &len, BFLD_STRING))
    {
        NDRX_LOG(log_error, "Failed to get EX_DATA!");
        FAIL_OUT(ret);
    }
    
    ret = tmq_qconf_addupd(conf, NULL);
    
out:
    return ret;
}
