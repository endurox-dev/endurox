/**
 * @brief Queue forward processing
 *   We will have a separate thread pool for processing automatic queue .
 *   the main forward thread will periodically scan the Q and submit the jobs to
 *   the threads (if any will be free).
 *   During the shutdown we will issue for every pool thread
 *
 * @file forward.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <tperror.h>
#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include "nstdutil.h"
#include "userlog.h"
#include <xa_cmn.h>
#include <atmi_int.h>
#include <ndrxdiag.h>
#include "qtran.h"
#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic pthread_t G_forward_thread;
expublic int volatile G_forward_req_shutdown = EXFALSE;          /**< Is shutdown request? */
expublic int volatile ndrx_G_forward_req_shutdown_ack = EXFALSE; /**< Is shutdown acked?   */


exprivate MUTEX_LOCKDECL(M_wait_mutex);
exprivate pthread_cond_t M_wait_cond = PTHREAD_COND_INITIALIZER;

exprivate __thread int M_is_xa_open = EXFALSE; /* Init flag for thread. */


exprivate fwd_qlist_t *M_next_fwd_q_list = NULL;    /**< list of queues to check msgs to fwd */
exprivate fwd_qlist_t *M_next_fwd_q_cur = NULL;     /**< current position in linked list...  */
exprivate int          M_had_msg = EXFALSE;         /**< Did we got the msg previously?      */
exprivate int          M_any_busy = EXFALSE;         /**< Is all queues busy?                 */
exprivate int          M_num_busy = 0;         /**< Number of busy jobs                 */
    
exprivate MUTEX_LOCKDECL(M_forward_lock); /* Q Forward operations sync        */

exprivate int M_force_sleep = EXFALSE;              /**< Shall the sleep be forced in case of error? */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Lock background operations
 */
expublic void forward_lock(void)
{
    MUTEX_LOCK_V(M_forward_lock);
}

/**
 * Un-lock background operations
 */
expublic void forward_unlock(void)
{
    MUTEX_UNLOCK_V(M_forward_lock);
}

/**
 * Sleep the thread, with option to wake up (by shutdown).
 * @param sleep_sec
 */
exprivate void thread_sleep(int sleep_sec)
{
    struct timespec wait_time;
    struct timeval now;
    int rt;

    gettimeofday(&now,NULL);

    wait_time.tv_sec = now.tv_sec+sleep_sec;
    wait_time.tv_nsec = now.tv_usec*1000;

    MUTEX_LOCK_V(M_wait_mutex);
    
    /* No wait if request was made here... */
    if (!G_forward_req_shutdown)
    {
        rt = pthread_cond_timedwait(&M_wait_cond, &M_wait_mutex, &wait_time);
    }
    
    MUTEX_UNLOCK_V(M_wait_mutex);
}

/**
 * Wake up the sleeping thread.
 */
expublic void forward_shutdown_wake(void)
{
    MUTEX_LOCK_V(M_wait_mutex);
    pthread_cond_signal(&M_wait_cond);
    MUTEX_UNLOCK_V(M_wait_mutex);
}

/**
 * Remove forward queue list before next lookup or at un-init
 */
exprivate void fwd_q_list_rm(void)
{
    fwd_qlist_t *elt, *tmp;
    /* Deallocate the previous DL */
    if (NULL!=M_next_fwd_q_list)
    {
        DL_FOREACH_SAFE(M_next_fwd_q_list,elt,tmp)
        {
            DL_DELETE(M_next_fwd_q_list,elt);
            NDRX_FREE(elt);
        }
    }
}

/**
 * Get next message to forward
 * So basically we iterate over the all Qs, then regenerate the Q list and
 * and iterate over again.
 * 
 * @return 
 */
exprivate tmq_msg_t * get_next_msg(void)
{
    tmq_msg_t * ret = NULL;
    long qerr = EXSUCCEED;
    char msgbuf[128];
    int again;
    long tot_cnt;

    do
    {
        again=EXFALSE;

        if (NULL==M_next_fwd_q_list || NULL == M_next_fwd_q_cur)
        {
            /* reset marking, no messages processed yet. */
            M_had_msg=EXFALSE;
            M_any_busy=EXFALSE;
            M_num_busy = 0;
            fwd_q_list_rm();

            /* Generate new list */
            M_next_fwd_q_list = tmq_get_qlist(EXTRUE, EXFALSE);
            
            if (NULL!=M_next_fwd_q_list)
            {
                M_next_fwd_q_cur = M_next_fwd_q_list;
            }
        }
        
        /*
         * get the message
         */
        while (NULL!=M_next_fwd_q_cur)
        {
            int busy = tmq_fwd_busy_cnt(M_next_fwd_q_cur->qname);
            
            NDRX_LOG(log_info, "mon: %s %ld/%ld/%d/%d", 
                    M_next_fwd_q_cur->qname
                    ,M_next_fwd_q_cur->numenq
                    ,M_next_fwd_q_cur->numdeq
                    ,busy
                    , M_next_fwd_q_cur->workers);
            
            /* no msg... */
            ret=NULL;
            
            /* not all busy... */
            M_num_busy+=busy;
            
            if (busy >= M_next_fwd_q_cur->workers)
            {
                /* Queue is busy... nothing todo... */
                M_any_busy=EXTRUE;
            }
            /* OK, so we peek for a message */
            else if (NULL==(ret=tmq_msg_dequeue(M_next_fwd_q_cur->qname, 0, EXTRUE, 
                    &qerr, msgbuf, sizeof(msgbuf), NULL)))
            {
                NDRX_LOG(log_debug, "Not messages for dequeue qerr=%ld: %s", 
                    qerr, msgbuf);
            }
            else
            {
                NDRX_LOG(log_debug, "Dequeued message");
                M_had_msg=EXTRUE;
            }

            /* schedule next queue ... */
            M_next_fwd_q_cur = M_next_fwd_q_cur->next;
        
            /* done with this loop if having msg.. */
            if (NULL!=ret)
            {
                break;
            }

        }
    
        /* if any queue had msgs, then re-scan queue list and retry */
        if (NULL==ret)
        {
            /* read again if had message... */
            if (M_any_busy)
            {
                int wait_ret;
                NDRX_LOG(log_debug, "All Qs/threads busy to the limit wait for slot...");
                
                /* wait on pool */
                do
                {
                    /* wait 1 sec... */
                    wait_ret = ndrx_thpool_timedwait_less(G_tmqueue_cfg.fwdthpool, 
                            M_num_busy, 1000);
                } 
                while (!G_forward_req_shutdown && EXTRUE!=wait_ret);
                
                again = EXTRUE;
            }
            else if (M_had_msg)
            {
                NDRX_LOG(log_debug, "Had messages in prevous run, scan Qs again");
                again = EXTRUE;
            }
        }

    } while (again && !G_forward_req_shutdown);
    
out:
    return ret;
}

/** get lock again to be sure that we are not rolled back */
#define RELOCK do {\
                    p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, NULL);\
                    /* Try to lock-up */\
                    if (NULL==p_tl)\
                    {\
                        NDRX_LOG(log_error, "Fatal error ! Expected to have transaction for [%s]",\
                                tmxid);\
                        userlog("Fatal error ! Expected to have transaction for [%s]",\
                                tmxid);\
                        EXFAIL_OUT(ret);\
                    }\
                } while (0)

/** Unlock the queue */
#define UNLOCK do {\
        tmq_log_unlock(p_tl);\
        p_tl=NULL;\
    } while (0)

/* well...! shouldn't we lock the transaction here?
 * Or... periodically lock / unlock as timeout may kill the transaction
 * in the middle thus msg ptr would become invalid
 */
#define GET_TL do {\
        /* & lock the entry */\
        tmxid = G_atmi_tls->qdisk_tls->filename_base;\
        p_tl = tmq_log_get_entry(tmxid, NDRX_LOCK_WAIT_TIME, NULL);\
        /* Try to lock-up */\
        if (NULL==p_tl)\
        {\
            NDRX_LOG(log_error, "Fatal error ! Expected to have transaction for [%s]",\
                    tmxid);\
            userlog("Fatal error ! Expected to have transaction for [%s]",\
                    tmxid);\
            EXFAIL_OUT(ret);\
        }\
    } while (0)
                
/**
 * Process of the message
 * @param ptr
 * @param p_finish_off
 */
expublic void thread_process_forward (void *ptr, int *p_finish_off)
{
    int ret = EXSUCCEED;
    tmq_msg_t * msg = (tmq_msg_t *)ptr;
    tmq_qconfig_t qconf;
    char *call_buf = NULL;
    long call_len = 0;
    
    char *rply_buf = NULL;
    long rply_len = 0;
    
    typed_buffer_descr_t *descr;
    char msgid_str[TMMSGIDLEN_STR+1];
    char *fn = "thread_process_forward";
    int tperr;
    union tmq_block cmd_block;
    int tout;
    int sent_ok=EXFALSE;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char qname[TMQNAMELEN+1];
    char *tmxid=NULL;
    qtran_log_t *p_tl=NULL;
    
    if (!M_is_xa_open)
    {
        if (EXSUCCEED!=tpopen()) /* init the lib anyway... */
        {
            NDRX_LOG(log_error, "Failed to tpopen() by worker thread: %s", 
                    tpstrerror(tperrno));
            userlog("Failed to tpopen() by worker thread: %s", tpstrerror(tperrno));
            
            /* nothing todo! */
            exit(1);
        }
        else
        {
            M_is_xa_open = EXTRUE;
        }
    }
    
    /* for later statistics release */
    NDRX_STRCPY_SAFE(qname, msg->hdr.qname);
    tmq_msgid_serialize(msg->hdr.msgid, msgid_str); 

    NDRX_LOG(log_info, "%s enter for msgid_str: [%s]", fn, msgid_str);
    
    /* Call the Service & and issue XA commands for update or delete
     *  + If message failed, forward to dead queue (if defined).
     */
    if (EXSUCCEED!=tmq_qconf_get_with_default_static(msg->hdr.qname, &qconf))
    {
        /* might happen if we reconfigure on the fly. */
        NDRX_LOG(log_error, "Failed to get qconf for [%s]", msg->hdr.qname);
        tmq_unlock_msg_by_msgid(msg->hdr.msgid);
        EXFAIL_OUT(ret);
    }
    
    if (qconf.txtout > EXFAIL)
    {
        tout = qconf.txtout;
        NDRX_LOG(log_info, "txtout set to %d sec", tout);
    }
    else
    {
        tout = G_tmqueue_cfg.dflt_timeout;
        NDRX_LOG(log_info, "txtout defaulted to %d sec", tout);
    }
    
    /* Alloc the buffer of the message type according to size (use prepare incoming?)
     */
    
    descr = &G_buf_descr[msg->buftyp];

    if (EXSUCCEED!=descr->pf_prepare_incoming(descr,
                    msg->msg,
                    msg->len,
                    &call_buf,
                    &call_len,
                    0))
    {
        NDRX_LOG(log_always, "Failed to allocate buffer type %hd!", msg->buftyp);
        tmq_unlock_msg_by_msgid(msg->hdr.msgid);
        EXFAIL_OUT(ret);
    }
    
    if (TMQ_AUTOQ_AUTOTX==qconf.autoq)
    {
        NDRX_LOG(log_debug, "Service invocation shall be performed in "
                "transactional mode...");
        
        if (EXSUCCEED!=tpbegin(tout, 0))
        {
            userlog("Failed to start tran: %s", tpstrerror(tperrno));
            NDRX_LOG(log_error, "Failed to start tran!");
            
            /* nothing todo with the msg, unlock... */
            tmq_unlock_msg_by_msgid(msg->hdr.msgid);
            EXFAIL_OUT(ret);
        }
    }
    
    /* call the service */
    
    /* substitute the special service name  */
    if (0==strcmp(qconf.svcnm, TMQ_QUEUE_SERVICE))
    {
        NDRX_STRCPY_SAFE(svcnm, qconf.qname);
    }
    else
    {
        NDRX_STRCPY_SAFE(svcnm, qconf.svcnm);
    }
    
    NDRX_LOG(log_info, "Sending request to service: [%s]", svcnm);
    
    if (EXFAIL == tpcall(svcnm, call_buf, call_len, (char **)&rply_buf, &rply_len,0))
    {
        tperr = tperrno;
        NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperr));
        
        /* Bug #421 if called in transaction, then abort current one
         * because need to increment the counters in new transaction
         */
        if (tpgetlev())
        {
            NDRX_LOG(log_error, "Abort current transaction for counter increment");
            tpabort(0L);
        }
    }
    else
    {
        sent_ok=EXTRUE;
    }
    
    NDRX_LOG(log_info, "Service answer %s for %s", (sent_ok?"ok":"fail"), msgid_str);
    
    memset(&cmd_block, 0, sizeof(cmd_block));
    memcpy(&cmd_block.hdr, &msg->hdr, sizeof(cmd_block.hdr));
    
   /* cmd_block.hdr.flags|=TPQASYNC; async complete to avoid deadlocks... 
    * No need for this: we have seperate thread pool for notify events.
    */

    /* start the transaction 
     * Note! message is not yet added to transaction with
     */
    if (!tpgetlev())
    {
        if (EXSUCCEED!=tpbegin(tout, 0))
        {
            userlog("Failed to start tran: %s", tpstrerror(tperrno));
            NDRX_LOG(log_error, "Failed to start tran!");
            tmq_unlock_msg_by_msgid(msg->hdr.msgid);
            EXFAIL_OUT(ret);
        }
    }
        
    /* 
     * just unlock the message. Increase the counter
     */
    if (sent_ok)
    {
        /* start our internal transaction */
        tmq_update_q_stats(msg->hdr.qname, 1, 0);
        cmd_block.hdr.command_code = TMQ_STORCMD_DEL;

        /* well this will generate this will add msg to transaction
         * will be handled by timeout setting...
         * No more unlock manual.
         * This will sub-lock
         */
        if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&cmd_block, 
                "Removing completed message...", NULL))
        {
            NDRX_LOG(log_error, "Failed to issue complete/remove command to xa for msgid_str [%s]", 
                    msgid_str);
            userlog("Failed to issue complete/remove command to xa for msgid_str [%s]", 
                    msgid_str);

            /* unlock the msg, as adding to log is last step, 
             * thus not in log and we are in control
             */
            tmq_unlock_msg_by_msgid(msg->hdr.msgid);
            EXFAIL_OUT(ret);
        }
        
        /* dynamic mode will promote the tmxid */
        GET_TL;
        
       /* Remove the message */
        if (msg->qctl.flags & TPQREPLYQ)
        {
            TPQCTL ctl;
        
            NDRX_LOG(log_warn, "TPQREPLYQ defined, sending answer buffer to "
                    "[%s] q in [%s] namespace", 
                    msg->qctl.replyqueue, msg->hdr.qspace);
            
            /* Send response to reply Q (load the data in FB with call details) */
            memset(&ctl, 0, sizeof(ctl));
                    
            /* this will add msg to our transaction, if all ok
             * now futher we do not control
             * the transaction here, just let let timeout handler to do the
             * job
             */
            
            /* we must release the lock here... */
            UNLOCK;
            ret = tpenqueue (msg->hdr.qspace, msg->qctl.replyqueue, &ctl, rply_buf, rply_len, 0);
            RELOCK;
            
            if (EXSUCCEED!=ret)
            {    
                if (TPEDIAGNOSTIC==tperrno)
                {
                    NDRX_LOG(log_error, "Failed to enqueue to replyqueue [%s]: %s diag: %d:%s", 
                            msg->qctl.replyqueue, tpstrerror(tperrno),
                            msg->qctl.diagnostic, msg->qctl.diagmsg);
                    userlog("Failed to enqueue to replyqueue [%s]: %s diag: %d:%s", 
                            msg->qctl.replyqueue, tpstrerror(tperrno),
                            msg->qctl.diagnostic, msg->qctl.diagmsg);
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to enqueue to replyqueue [%s]: %s", 
                            msg->qctl.replyqueue, tpstrerror(tperrno));
                    userlog("Failed to enqueue to replyqueue [%s]: %s", 
                            msg->qctl.replyqueue, tpstrerror(tperrno));
                }
                /* no unlock & sleep as we do not know where the transaction
                 * did stuck
                 */
                EXFAIL_OUT(ret);
            }
        }
        
    }
    else
    {
        /* Increase the counter */
        msg->trycounter++;
        NDRX_LOG(log_warn, "Message [%s] tries %ld, max: %ld", 
                msgid_str, msg->trycounter, qconf.tries);
        ndrx_utc_tstamp2(&msg->trytstamp, &msg->trytstamp_usec);
        
        if (msg->trycounter>=qconf.tries)
        {
            NDRX_LOG(log_error, "Message [%s] expired", msgid_str);
            
            /* test before eqn... */
            tmq_update_q_stats(msg->hdr.qname, 0, 1);
            cmd_block.hdr.command_code = TMQ_STORCMD_DEL;
            if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&cmd_block, 
                    "Removing expired message...", NULL))
            {
                NDRX_LOG(log_error, "Failed to issue complete/remove command to xa for msgid_str [%s]", 
                        msgid_str);
                userlog("Failed to issue complete/remove command to xa for msgid_str [%s]", 
                        msgid_str);
                
                /* unlock the msg, as adding to log is last step, 
                 * thus not in log and we are in control
                 */
                tmq_unlock_msg_by_msgid(msg->hdr.msgid);
                EXFAIL_OUT(ret);
            }
            
            /* dynamic mode will promote the tmxid */
            GET_TL;
            
            if (msg->qctl.flags & TPQFAILUREQ && NULL!=rply_buf)
            {
                TPQCTL ctl;
                NDRX_LOG(log_warn, "TPQFAILUREQ defined and non NULL reply, enqueue answer buffer to "
                    "[%s] q in [%s] namespace", 
                    msg->qctl.failurequeue, msg->hdr.qspace);
                

                /* Send response to reply Q (load the data in FB with call details)
                 * Keep the original flags... */
                memcpy(&ctl, &msg->qctl, sizeof(ctl));

                /* if local tran expires, the process will be unable to join
                 * transaction 
                 */
                UNLOCK;
                ret = tpenqueue (msg->hdr.qspace, msg->qctl.failurequeue, &ctl, rply_buf, rply_len, 0);
                RELOCK;
                
                if (EXSUCCEED!=ret)
                {
                    if (TPEDIAGNOSTIC==tperrno)
                    {
                        NDRX_LOG(log_error, "Failed to enqueue to failurequeue [%s]: %s diag: %d:%s", 
                                msg->qctl.replyqueue, tpstrerror(tperrno),
                                msg->qctl.diagnostic, msg->qctl.diagmsg);
                        userlog("Failed to enqueue to failurequeue [%s]: %s diag: %d:%s", 
                                msg->qctl.replyqueue, tpstrerror(tperrno),
                                msg->qctl.diagnostic, msg->qctl.diagmsg);
                    }
                    else
                    {
                        NDRX_LOG(log_error, "Failed to enqueue to failurequeue [%s]: %s", 
                                msg->qctl.replyqueue, tpstrerror(tperrno));
                        userlog("Failed to enqueue to failurequeue [%s]: %s", 
                                msg->qctl.replyqueue, tpstrerror(tperrno));
                    }
                    
                    /* let timeout/tmsrv to housekeep...
                     * only here if global transaction timed-out
                     * then will we force the sleep. Thought this would be
                     * very rare case.
                     */
                    EXFAIL_OUT(ret);
                }
            }
            
            if (EXEOS!=qconf.errorq[0])
            {
                TPQCTL ctl;
                NDRX_LOG(log_warn, "ERRORQ defined, enqueue request buffer to "
                    "[%s] q in [%s] namespace", qconf.errorq, msg->hdr.qspace);
                

                /* Send org msg to error queue.
                 * Keep the original flags... */
                memcpy(&ctl, &msg->qctl, sizeof(ctl));

                UNLOCK;
                ret = tpenqueue (msg->hdr.qspace, qconf.errorq, &ctl, call_buf, call_len, 0);
                RELOCK;
                
                if (EXSUCCEED!=ret)
                {
                    NDRX_LOG(log_error, "Failed to enqueue to errorq [%s]: %s", 
                            qconf.errorq, tpstrerror(tperrno));
                    userlog("Failed to enqueue to errorq [%s]: %s", 
                            qconf.errorq, tpstrerror(tperrno));
                    
                    /* let timeout/tmsrv to housekeep... */
                    EXFAIL_OUT(ret);
                }
            }
        }
        else
        {
            /* We need to update the message */
            UPD_MSG((&cmd_block.upd), msg);
        
            cmd_block.hdr.command_code = TMQ_STORCMD_UPD;
            
            if (EXSUCCEED!=tmq_storage_write_cmd_block((char *)&cmd_block, 
                    "Update message command", NULL))
            {
                NDRX_LOG(log_error, "Failed to issue update command to xa for msgid_str [%s]", 
                        msgid_str);
                userlog("Failed to issue update command to xa for msgid_str [%s]", 
                        msgid_str);
                
                /* unlock the msg, as adding to log is last step, 
                 * thus not in log and we are in control
                 */
                tmq_unlock_msg_by_msgid(msg->hdr.msgid);
                EXFAIL_OUT(ret);
            }
            
            /* dynamic mode will promote the tmxid */
            GET_TL;
            
        }
    }

out:    
        
    /* release the lock so that commit can complete... */        
    if (NULL!=p_tl)
    {
        tmq_log_unlock(p_tl);
    }

    /* NOTE! Cannot touch msg here anymore !?
     * what happens to msg object, if timeout rolls-back?
     */
    if (tpgetlev())
    {
        if (EXSUCCEED==ret)
        {
            if (EXSUCCEED!=tpcommit(0L))
            {
                NDRX_LOG(log_error, "Failed to commit => aborting + force sleep");
                userlog("Failed to commit => aborting + force sleep");
                M_force_sleep=EXTRUE;
                tpabort(0L);
            }
        }
        else
        {
            NDRX_LOG(log_error, "System failure during msg processing => aborting + force sleep");
            userlog("System failure during msg processing => aborting + force sleep");
            tpabort(0L);
            M_force_sleep=EXTRUE;
        }
    }
    else if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "System failure => force sleep");
        M_force_sleep=EXTRUE;
    }
    
    if (NULL!=call_buf)
    {
        tpfree(call_buf);
    }

    if (NULL!=rply_buf)
    {
        tpfree(rply_buf);
    }
    
    /* release stats counter... */
    tmq_fwd_busy_dec(qname);
    
    return;
}

/**
 * Continues transaction background loop..
 * Try to complete the transactions.
 * @return  SUCCEED/FAIL
 */
expublic int forward_loop(void)
{
    int ret = EXSUCCEED;
    int normal_sleep;
    tmq_msg_t * msg;
    /*
     * We need to get the list of queues to monitor.
     * Note that list can be dynamic. So at some periods we need to refresh
     * the lists we monitor.
     */
    while(!G_forward_req_shutdown)
    {
        msg = NULL;
        
        /* wait for one slot to become free.. */
        ndrx_thpool_wait_one(G_tmqueue_cfg.fwdthpool);
        
        normal_sleep=EXFALSE;
        if (!M_force_sleep)
        {
            /* 2. get the message from Q */
            msg = get_next_msg();
        
            /* 3. run off the thread */
            if (NULL!=msg)
            {
                /* Submit the job to thread */
                if (EXSUCCEED!=tmq_fwd_busy_inc(msg->hdr.qname))
                {
                    EXFAIL_OUT(ret);
                }
                
                ndrx_thpool_add_work(G_tmqueue_cfg.fwdthpool, (void*)thread_process_forward, (void *)msg);            
            }
            else
            {
                normal_sleep=EXTRUE;
            }
        }

        /* go sleep if no msgs, or forced */
        if (normal_sleep || M_force_sleep)
        {
            /* sleep only when did not have a message 
             * So that if we have batch, we try to use all resources...
             */
            NDRX_LOG(log_debug, "background - sleep %d forced=%d", 
                    G_tmqueue_cfg.scan_time, M_force_sleep);
            
            thread_sleep(G_tmqueue_cfg.scan_time);
            
            /* in case of error, forced sleep */
            M_force_sleep=EXFALSE;
        }
    }
    
    /* ask the shutodwn */
    ndrx_G_forward_req_shutdown_ack = EXTRUE;
    
    /* remove any allocated memory... */
    fwd_q_list_rm();
    
out:
    return ret;
}

/**
 * Background processing of the transactions (Complete them).
 * @return 
 */
expublic void * forward_process(void *arg)
{
    NDRX_LOG(log_error, "***********BACKGROUND PROCESS START ********");
    
    tmq_thread_init();
    forward_loop();
    tmq_thread_uninit();
    
    NDRX_LOG(log_error, "***********BACKGROUND PROCESS END **********");
    
    return NULL;
}

/**
 * Initialize background process
 * @return EXSUCCEED/EXFAIL
 */
expublic int forward_process_init(void)
{
    int ret = EXSUCCEED;
    
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);
    
    /* set some small stacks size, 1M should be fine! */
    ndrx_platf_stack_set(&pthread_custom_attr);
    if (EXSUCCEED!=pthread_create(&G_forward_thread, &pthread_custom_attr, 
            forward_process, NULL))
    {
        NDRX_PLATF_DIAG(NDRX_DIAG_PTHREAD_CREATE, errno, "forward_process_init");
        EXFAIL_OUT(ret);
    }
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
