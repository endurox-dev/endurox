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
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic pthread_t G_forward_thread;
expublic int G_forward_req_shutdown = EXFALSE;    /* Is shutdown request? */


exprivate pthread_mutex_t M_wait_mutex = PTHREAD_MUTEX_INITIALIZER;
exprivate pthread_cond_t M_wait_cond = PTHREAD_COND_INITIALIZER;

exprivate __thread int M_is_xa_open = EXFALSE; /* Init flag for thread. */


MUTEX_LOCKDECL(M_forward_lock); /* Q Forward operations sync        */

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
    wait_time.tv_nsec = now.tv_usec;

    pthread_mutex_lock(&M_wait_mutex);
    rt = pthread_cond_timedwait(&M_wait_cond, &M_wait_mutex, &wait_time);
    pthread_mutex_unlock(&M_wait_mutex);
}

/**
 * Wake up the sleeping thread.
 */
expublic void forward_shutdown_wake(void)
{
    pthread_mutex_lock(&M_wait_mutex);
    pthread_cond_signal(&M_wait_cond);
    pthread_mutex_unlock(&M_wait_mutex);
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
    static __thread fwd_qlist_t *list = NULL;     /* Single threaded but anyway */
    static __thread fwd_qlist_t *cur = NULL;     /* Single threaded but anyway */
    fwd_qlist_t *elt, *tmp;
    
    if (NULL==list || NULL == cur)
    {
        /* Deallocate the previous DL */
        if (NULL!=list)
        {
            DL_FOREACH_SAFE(list,elt,tmp) 
            {
                DL_DELETE(list,elt);
                NDRX_FREE(elt);
            }
        }
        
        /* Generate new list */
        list = tmq_get_qlist(EXTRUE, EXFALSE);
        
        if (NULL!=list)
        {
            cur = list;
        }
    }
    
    /*
     * get the message
     */
    while (NULL!=cur)
    {
        /* OK, so we peek for a message */
        if (NULL==(ret=tmq_msg_dequeue(cur->qname, 0, EXTRUE)))
        {
            NDRX_LOG(log_debug, "Not messages for dequeue");
        }
        else
        {
            NDRX_LOG(log_debug, "Dequeued message");
            goto out;
        }
        cur = cur->next;
    }
    
out:
    return ret;
}

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
    typed_buffer_descr_t *descr;
    char msgid_str[TMMSGIDLEN_STR+1];
    char *fn = "thread_process_forward";
    int tperr;
    union tmq_block cmd_block;
    
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

    
    tmq_msgid_serialize(msg->hdr.msgid, msgid_str); 

    NDRX_LOG(log_info, "%s enter for msgid_str: [%s]", fn, msgid_str);
    
    /* Call the Service & and issue XA commands for update or delete
     *  + If message failed, forward to dead queue (if defined).
     */
    if (EXSUCCEED!=tmq_qconf_get_with_default_static(msg->hdr.qname, &qconf))
    {
        /* might happen if we reconfigure on the fly. */
        NDRX_LOG(log_error, "Failed to get qconf for [%s]", msg->hdr.qname);
        EXFAIL_OUT(ret);
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
        EXFAIL_OUT(ret);
    }

    /* call the service */
    NDRX_LOG(log_info, "Sending request to service: [%s]", qconf.svcnm);
    
    if (EXFAIL == tpcall(qconf.svcnm, call_buf, call_len, (char **)&call_buf, &call_len,0))
    {
        tperr = tperrno;
        NDRX_LOG(log_error, "%s failed: %s", qconf.svcnm, tpstrerror(tperr));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Service answer ok for %s", msgid_str);
    
out:

    memset(&cmd_block, 0, sizeof(cmd_block));

    memcpy(&cmd_block.hdr, &msg->hdr, sizeof(cmd_block.hdr));
    
   /* cmd_block.hdr.flags|=TPQASYNC; async complete to avoid deadlocks... 
    * No need for this: we have seperate thread pool for notify events.
    */

    /* start the transaction */
    if (EXSUCCEED!=tpbegin(G_tmqueue_cfg.dflt_timeout, 0))
    {
        userlog("Failed to start tran: %s", tpstrerror(tperrno));
        NDRX_LOG(log_error, "Failed to start tran!");
        /* unlock the message */
        tmq_unlock_msg_by_msgid(msg->hdr.msgid);
        
        return;
    }

    /* 
     * just unlock the message. Increase the cou
     */
    if (EXSUCCEED==ret)
    {
       /* Remove the message */
        if (msg->qctl.flags & TPQREPLYQ)
        {
            TPQCTL ctl;
            NDRX_LOG(log_warn, "TPQREPLYQ defined, sending answer buffer to "
                    "[%s] q in [%s] namespace", 
                    msg->qctl.replyqueue, msg->hdr.qspace);
            
            /* Send response to reply Q (load the data in FB with call details) */
            memset(&ctl, 0, sizeof(ctl));
                    
            if (EXSUCCEED!=tpenqueue (msg->hdr.qspace, msg->qctl.replyqueue, &ctl, 
                    call_buf, call_len, TPNOTRAN))
            {
                NDRX_LOG(log_error, "Failed to enqueue to replyqueue [%s]: %s", 
                        msg->qctl.replyqueue, tpstrerror(tperrno));
            }
        }
        
        tmq_update_q_stats(msg->hdr.qname, 1, 0);
        
        cmd_block.hdr.command_code = TMQ_STORCMD_DEL;
        
        if (EXSUCCEED!=tmq_storage_write_cmd_block(&cmd_block, 
                "Removing completed message..."))
        {
            userlog("Failed to issue complete/remove command to xa for msgid_str [%s]", 
                    msgid_str);
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
            
            if (msg->qctl.flags & TPQFAILUREQ)
            {
                TPQCTL ctl;
                NDRX_LOG(log_warn, "TPQFAILUREQ defined, sending answer buffer to "
                    "[%s] q in [%s] namespace", 
                    msg->qctl.failurequeue, msg->hdr.qspace);
                

                /* Send response to reply Q (load the data in FB with call details)
                 * Keep the original flags... */
                memcpy(&ctl, &msg->qctl, sizeof(ctl));

                if (EXSUCCEED!=tpenqueue (msg->hdr.qspace, msg->qctl.failurequeue, &ctl, 
                        call_buf, call_len, TPNOTRAN))
                {
                    NDRX_LOG(log_error, "Failed to enqueue to failurequeue [%s]: %s", 
                            msg->qctl.failurequeue, tpstrerror(tperrno));
                }
            }
            
            tmq_update_q_stats(msg->hdr.qname, 0, 1);
            
            cmd_block.hdr.command_code = TMQ_STORCMD_DEL;
        
            if (EXSUCCEED!=tmq_storage_write_cmd_block(&cmd_block, 
                    "Removing expired message..."))
            {
                userlog("Failed to issue complete/remove command to xa for msgid_str [%s]", 
                        msgid_str);
            }
        }
        else
        {
            /* We need to update the message */
            UPD_MSG((&cmd_block.upd), msg);
        
            cmd_block.hdr.command_code = TMQ_STORCMD_UPD;
            
            if (EXSUCCEED!=tmq_storage_write_cmd_block(&cmd_block, 
                    "Update message command"))
            {
                userlog("Failed to issue update command to xa for msgid_str [%s]", 
                        msgid_str);
            }
        }
    }
    
    /* commit the transaction */
    if (EXSUCCEED!=tpcommit(0))
    {
        userlog("Failed to commit: %s", tpstrerror(tperrno));
        NDRX_LOG(log_error, "Failed to commit!");
        tpabort(0);
    }
    
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
    tmq_msg_t * msg;
    /*
     * We need to get the list of queues to monitor.
     * Note that list can be dynamic. So at some periods we need to refresh
     * the lists we monitor.
     */
    while(!G_forward_req_shutdown)
    {
        msg = NULL;
        
        if (thpool_freethreads_nr(G_tmqueue_cfg.fwdthpool) > 0)
        {
            /* 2. get the message from Q */
            msg = get_next_msg();
        }
        
        /* 3. run off the thread */
        if (NULL!=msg)
        {
            /* Submit the job to thread */
            thpool_add_work(G_tmqueue_cfg.fwdthpool, (void*)thread_process_forward, (void *)msg);            
        }
        else
        {
            /* sleep only when did not have a message 
             * So that if we have batch, we try to use all resources...
             */
            NDRX_LOG(log_debug, "background - sleep %d", 
                    G_tmqueue_cfg.scan_time);
            
            if (!G_forward_req_shutdown)
                thread_sleep(G_tmqueue_cfg.scan_time);
        }
    }
    
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
    
   /* 1. Read the transaction records from disk */ 
    /* background_read_log(); */
    
   /* 2. Loop over the transactions and:
    * - Check for in-progress timeouts
    * - Try to abort abortable
    * - Try co commit commitable
    * - Use timers counters from the cli params. 
    */
    
    tmq_thread_init();
    forward_loop();
    tmq_thread_uninit();
    
    NDRX_LOG(log_error, "***********BACKGROUND PROCESS END **********");
    
    return NULL;
}

/**
 * Initialize background process
 * @return
 */
expublic void forward_process_init(void)
{
    struct sigaction        actions;
    
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);
    
    /* set some small stacks size, 1M should be fine! */
    pthread_attr_setstacksize(&pthread_custom_attr, ndrx_platf_stack_get_size());
    pthread_create(&G_forward_thread, &pthread_custom_attr, 
            forward_process, NULL);  
}
