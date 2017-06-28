/* 
** Tmqueue server - transaction monitor
**
** @file tmqapi.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
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
exprivate __thread int M_is_xa_open = EXFALSE;
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
expublic int tmq_enqueue(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    tmq_msg_t *p_msg = NULL;
    char *data = NULL;
    BFLDLEN len = 0;
    TPQCTL qctl_out;
    int local_tx = EXFALSE;
    
    /* To guarentee unique order in same Q space...: */
    static long t_sec = 0;
    static long t_usec = 0;
    static int t_cntr = 0;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_enqueue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_enqueue called with", p_ub);
    
    if (!M_is_xa_open)
    {
        if (EXSUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            M_is_xa_open = EXTRUE;
        }
    }
            
    if (!tpgetlev())
    {
        NDRX_LOG(log_debug, "Not in global transaction, starting local...");
        if (EXSUCCEED!=tpbegin(G_tmqueue_cfg.dflt_timeout, 0))
        {
            NDRX_LOG(log_error, "Failed to start global tx!");
            userlog("Failed to start global tx!");
        }
        else
        {
            NDRX_LOG(log_debug, "Transaction started ok...");
            local_tx = EXTRUE;
        }
    }
    
    memset(&qctl_out, 0, sizeof(qctl_out));
    
    if (NULL==(data = Bgetalloc(p_ub, EX_DATA, 0, &len)))
    {
        NDRX_LOG(log_error, "Missing EX_DATA!");
        userlog("Missing EX_DATA!");
        
        strcpy(qctl_out.diagmsg, "Missing EX_DATA!");
        qctl_out.diagnostic = QMEINVAL;
        
        EXFAIL_OUT(ret);
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
        
        EXFAIL_OUT(ret);
    }
    
    memset(p_msg, 0, sizeof(tmq_msg_t));
    
    memcpy(p_msg->msg, data, len);
    p_msg->len = len;    
    
    NDRX_DUMP(log_debug, "Got message for Q: ", p_msg->msg, p_msg->len);
    
    if (EXSUCCEED!=Bget(p_ub, EX_QNAME, 0, p_msg->hdr.qname, 0))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to get EX_QNAME");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to get EX_QNAME!");
        qctl_out.diagnostic = QMEINVAL;
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bget(p_ub, EX_DATA_BUFTYP, 0, (char *)&p_msg->buftyp, 0))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to get EX_DATA_BUFTYP");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to get EX_DATA_BUFTYP!");
        qctl_out.diagnostic = QMEINVAL;
        
        EXFAIL_OUT(ret);
    }
    
    /* Restore back the C structure */
    if (EXSUCCEED!=tmq_tpqctl_from_ubf_enqreq(p_ub, &p_msg->qctl))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        userlog("tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        qctl_out.diagnostic = QMESYSTEM;
        
        EXFAIL_OUT(ret);
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
    
    if (EXSUCCEED!=tmq_msg_add(p_msg, EXFALSE))
    {
        NDRX_LOG(log_error, "tmq_enqueue: failed to enqueue!");
        userlog("tmq_enqueue: failed to enqueue!");
        
        strcpy(qctl_out.diagmsg, "tmq_enqueue: failed to enqueue!");
        
        qctl_out.diagnostic = QMESYSTEM;
        
        EXFAIL_OUT(ret);
    }
    
out:
    /* free up the temp memory */
    if (NULL!=data)
    {
        NDRX_FREE(data);
    }

    if (EXSUCCEED!=ret && NULL!=p_msg)
    {
        NDRX_LOG(log_warn, "About to free p_msg!");
        NDRX_FREE(p_msg);
    }

    if (local_tx)
    {
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Aborting transaction");
            tpabort(0);
        } else
        {
            NDRX_LOG(log_info, "Committing transaction!");
            if (EXSUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Commit failed!");
                userlog("Commit failed!");
                strcpy(qctl_out.diagmsg, "tmq_enqueue: commit failed!");
                ret=EXFAIL;
            }
        }
    }

    /* Setup response fields
     * Not sure about existing ones (seems like they will stay in buffer)
     * i.e. request fields
     */
    if (EXSUCCEED!=tmq_tpqctl_to_ubf_enqrsp(p_ub, &qctl_out))
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
expublic int tmq_dequeue(UBFH **pp_ub)
{
    /* Search for not locked message, lock it, issue command to disk for
     * delete & return the message to buffer (also needs to resize the buffer correctly)
     */
    int ret = EXSUCCEED;
    tmq_msg_t *p_msg = NULL;
    TPQCTL qctl_out, qctl_in;
    int local_tx = EXFALSE;
    char qname[TMQNAMELEN+1];
    long buf_realoc_size;
    
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_dequeue()");
    
    ndrx_debug_dump_UBF(log_info, "tmq_dequeue called with", *pp_ub);
    
    if (!M_is_xa_open)
    {
        if (EXSUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
        }
        else
        {
            M_is_xa_open = EXTRUE;
        }
    }
    
    memset(&qctl_in, 0, sizeof(qctl_in));
    
    if (EXSUCCEED!=tmq_tpqctl_from_ubf_deqreq(*pp_ub, &qctl_in))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to read request qctl!");
        userlog("tmq_dequeue: failed to read request qctl!");
        EXFAIL_OUT(ret);
    }
    
    if (!tpgetlev())
    {
        NDRX_LOG(log_debug, "Not in global transaction, starting local...");
        if (EXSUCCEED!=tpbegin(G_tmqueue_cfg.dflt_timeout, 0))
        {
            NDRX_LOG(log_error, "Failed to start global tx!");
            userlog("Failed to start global tx!");
        }
        else
        {
            NDRX_LOG(log_debug, "Transaction started ok...");
            local_tx = EXTRUE;
        }
    }
    
    memset(&qctl_out, 0, sizeof(qctl_out));
    
    if (EXSUCCEED!=Bget(*pp_ub, EX_QNAME, 0, qname, 0))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to get EX_QNAME");
        strcpy(qctl_out.diagmsg, "tmq_dequeue: failed to get EX_QNAME!");
        qctl_out.diagnostic = QMEINVAL;
        
        EXFAIL_OUT(ret);
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
            EXFAIL_OUT(ret);
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
            EXFAIL_OUT(ret);
        }
    }
    else if (NULL==(p_msg = tmq_msg_dequeue(qname, qctl_in.flags, EXFALSE)))
    {
        NDRX_LOG(log_error, "tmq_dequeue: no message in Q [%s]", qname);
        strcpy(qctl_out.diagmsg, "tmq_dequeue: no message in Q!");
        qctl_out.diagnostic = QMENOMSG;
        
        EXFAIL_OUT(ret);
    }
    
    /* Use the original metadata */
    memcpy(&qctl_out, &p_msg->qctl, sizeof(qctl_out));
    
    buf_realoc_size = Bused (*pp_ub) + p_msg->len + 1024;
    
    if (NULL==(*pp_ub = (UBFH *)tprealloc ((char *)*pp_ub, buf_realoc_size)))
    {
        NDRX_LOG(log_error, "Failed to allocate buffer to size: %ld", buf_realoc_size);
        userlog("Failed to allocate buffer to size: %ld", buf_realoc_size);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(*pp_ub, EX_DATA, 0, p_msg->msg, p_msg->len))
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
        
        EXFAIL_OUT(ret);
    }
    
    /* Unlock msg if it was peek */
    if (TPQPEEK & qctl_in.flags && 
            EXSUCCEED!=tmq_unlock_msg_by_msgid(p_msg->qctl.msgid))
    {
        NDRX_LOG(log_error, "Failed to unlock msg!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(*pp_ub, EX_DATA_BUFTYP, 0, (char *)&p_msg->buftyp, 0))
    {
        NDRX_LOG(log_error, "tmq_dequeue: failed to set EX_DATA_BUFTYP");
        
        strcpy(qctl_out.diagmsg, "tmq_dequeue: failed to set EX_DATA_BUFTYP!");
        qctl_out.diagnostic = QMEINVAL;
        
        EXFAIL_OUT(ret);
    }
    
    
out:

    if (local_tx)
    {
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Aborting transaction");
            tpabort(0);
        } 
        else
        {
            NDRX_LOG(log_info, "Committing transaction!");
            if (EXSUCCEED!=tpcommit(0))
            {
                NDRX_LOG(log_error, "Commit failed!");
                userlog("Commit failed!");
                strcpy(qctl_out.diagmsg, "tmq_enqueue: commit failed!");
                ret=EXFAIL;
            }
        }
    }

    /* Setup response fields
     * Not sure about existing ones (seems like they will stay in buffer)
     * i.e. request fields
     */
    if (EXSUCCEED!=tmq_tpqctl_to_ubf_deqrsp(*pp_ub, &qctl_out))
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
expublic int tex_mq_notify(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    union tmq_upd_block b;
    BFLDLEN len = sizeof(b);
     
    if (EXSUCCEED!=Bget(p_ub, EX_DATA, 0, (char *)&b, &len))
    {
        NDRX_LOG(log_error, "Failed to get EX_DATA: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tmq_unlock_msg(&b))
    {
        NDRX_LOG(log_error, "Failed to unlock message...");
        EXFAIL_OUT(ret);
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
expublic int tmq_mqlq(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    fwd_qlist_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_printqueue";
    /* Get list of queues */
    
    if (NULL==(list = tmq_get_qlist(EXFALSE, EXTRUE)))
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
        
        if (EXSUCCEED!=Bchg(p_ub, EX_QSPACE, 0, G_tmqueue_cfg.qspace, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNAME, 0, el->qname, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMMSG, 0, (char *)&msgs, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMLOCKED, 0, (char *)&locked, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMSUCCEED, 0, (char *)&el->succ, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMFAIL, 0, (char *)&el->fail, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMENQ, 0, (char *)&el->numenq, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QNUMDEQ, 0, (char *)&el->numdeq, 0L) 
                )
        {
            NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        if (EXFAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            EXFAIL_OUT(ret);
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
expublic int tmq_mqlc(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    fwd_qlist_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_mqlc";
    char qdef[TMQ_QDEF_MAX];
    char flags[128];
    int is_default = EXFALSE;
    /* Get list of queues */
    
    if (NULL==(list = tmq_get_qlist(EXFALSE, EXTRUE)))
    {
        NDRX_LOG(log_info, "%s: No queues found", fn);
    }
    else
    {
        NDRX_LOG(log_info, "%s: Queues found", fn);
    }
    
    DL_FOREACH_SAFE(list,el,tmp)
    {
        is_default = EXFALSE;
        if (EXSUCCEED==tmq_build_q_def(el->qname, &is_default, qdef))
        {
            NDRX_LOG(log_debug, "returning %s/%s", G_tmqueue_cfg.qspace, el->qname);
            
            flags[0] = EXEOS;
            
            if (is_default)
            {
                strcat(flags, "D");
            }
            

            if (EXSUCCEED!=Bchg(p_ub, EX_QSPACE, 0, G_tmqueue_cfg.qspace, 0L) ||
                EXSUCCEED!=Bchg(p_ub, EX_QNAME, 0, el->qname, 0L) ||
                EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
                EXSUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
                EXSUCCEED!=CBchg(p_ub, EX_DATA, 0, qdef, 0L, BFLD_STRING) ||
                EXSUCCEED!=Bchg(p_ub, EX_QSTRFLAGS, 0, flags, 0L)
                )
            {
                NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            if (EXFAIL == tpsend(cd,
                                (char *)p_ub,
                                0L,
                                0,
                                &revent))
            {
                NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                    tpstrerror(tperrno), revent);
                EXFAIL_OUT(ret);
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
expublic int tmq_mqlm(UBFH *p_ub, int cd)
{
    int ret = EXSUCCEED;
    long revent;
    tmq_memmsg_t *el, *tmp, *list;
    short nodeid = tpgetnodeid();
    short srvid = tpgetsrvid();
    char *fn = "tmq_mqlm";
    char qname[TMQNAMELEN+1];
    short is_locked;
    char msgid_str[TMMSGIDLEN_STR+1];

    /* Get list of queues */
    
    if (EXSUCCEED!=Bget(p_ub, EX_QNAME, 0, qname, 0L))
    {
        NDRX_LOG(log_error, "Failed to get qname");
        EXFAIL_OUT(ret);
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
            is_locked = EXTRUE;
        }
        else
        {
            is_locked = EXFALSE;
        }
        
        tmq_msgid_serialize(el->msg->hdr.msgid, msgid_str);
        
        if (EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QMSGIDSTR, 0, msgid_str, 0L)  ||
            EXSUCCEED!=Bchg(p_ub, EX_TSTAMP1_STR, 0, 
                ndrx_get_strtstamp2(0, el->msg->msgtstamp, el->msg->msgtstamp_usec), 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_TSTAMP2_STR, 0, 
                ndrx_get_strtstamp2(1, el->msg->trytstamp, el->msg->trytstamp_usec), 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QMSGTRIES, 0, (char *)&el->msg->trycounter, 0L) ||
            EXSUCCEED!=Bchg(p_ub, EX_QMSGLOCKED, 0, (char *)&is_locked, 0L)
                )
        {
            NDRX_LOG(log_error, "failed to setup FB: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            EXFAIL_OUT(ret);
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
expublic int tmq_mqrc(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    
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
expublic int tmq_mqch(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    char conf[512];
    BFLDLEN len = sizeof(conf);
    
    if (EXSUCCEED!=CBget(p_ub, EX_DATA, 0, conf, &len, BFLD_STRING))
    {
        NDRX_LOG(log_error, "Failed to get EX_DATA!");
        EXFAIL_OUT(ret);
    }
    
    ret = tmq_qconf_addupd(conf, NULL);
    
out:
    return ret;
}
