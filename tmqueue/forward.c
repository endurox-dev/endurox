/* 
** Queue forward processing
** We will have a separate thread pool for processing automatic queue .
** the main forward thread will periodically scan the Q and submit the jobs to
** the threads (if any will be free). 
** During the shutdown we will issue for every pool thread 
** 
** @file forward.c
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
#include <xa_cmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/



/*---------------------------Globals------------------------------------*/
public pthread_t G_forward_thread;
public int G_forward_req_shutdown = FALSE;    /* Is shutdown request? */


private pthread_mutex_t M_wait_mutex = PTHREAD_MUTEX_INITIALIZER;
private pthread_cond_t M_wait_cond = PTHREAD_COND_INITIALIZER;


MUTEX_LOCKDECL(M_forward_lock); /* Q Forward operations sync        */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Lock background operations
 */
public void forward_lock(void)
{
    MUTEX_LOCK_V(M_forward_lock);
}

/**
 * Un-lock background operations
 */
public void forward_unlock(void)
{
    MUTEX_UNLOCK_V(M_forward_lock);
}

/**
 * Sleep the thread, with option to wake up (by shutdown).
 * @param sleep_sec
 */
private void thread_sleep(int sleep_sec)
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
public void forward_shutdown_wake(void)
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
private tmq_msg_t * get_next_msg(void)
{
    tmq_msg_t * ret = NULL;
    static fwd_qlist_t *list = NULL;    
    fwd_qlist_t *elt, *tmp;
    
    if (NULL==list || NULL == list->cur->next)
    {
        /* Deallocate the previous DL */
        if (NULL!=list)
        {
            DL_FOREACH_SAFE(list,elt,tmp) 
            {
                DL_DELETE(list,elt);
                free(elt);
            }
        }
        
        /* Generate new list */
        list = tmq_get_fwd_list();
        
        if (NULL!=list)
        {
            list->cur = list;
        }
    }
    
    /*
     * get the message
     */
    while (NULL!=list && NULL!=list->cur)
    {
        /* OK, so we peek for a message */
        if (NULL==(ret=tmq_msg_dequeue_fifo(list->cur->qname, 0, TRUE)))
        {
            NDRX_LOG(log_debug, "Not messages for dequeue");
        }
        else
        {
            NDRX_LOG(log_debug, "Dequeued message");
            goto out;
        }
        list = list->next;
    }
    
out:
    return ret;
}

/**
 * Process of the message
 * @param ptr
 * @param p_finish_off
 */
public void thread_process_forward (void *ptr, int *p_finish_off)
{
    tmq_msg_t * msg = (tmq_msg_t *)ptr;
    
    /* Call the Service & and issue XA commands for update or delete
     *  + If message failed, forward to dead queue (if defined).
     */
    
}

/**
 * Continues transaction background loop..
 * Try to complete the transactions.
 * @return  SUCCEED/FAIL
 */
public int forward_loop(void)
{
    int ret = SUCCEED;
    tmq_msg_t * msg = NULL;
    /*
     * We need to get the list of queues to monitor.
     * Note that list can be dynamic. So at some periods we need to refresh
     * the lists we monitor.
     */
    while(!G_forward_req_shutdown)
    {
        
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
        
        NDRX_LOG(log_debug, "background - sleep %d", 
                G_tmqueue_cfg.scan_time);
        
        if (!G_forward_req_shutdown)
            thread_sleep(G_tmqueue_cfg.scan_time);
    }
    
out:
    return ret;
}

/**
 * Background processing of the transactions (Complete them).
 * @return 
 */
public void * forward_process(void *arg)
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
    
    forward_loop();
    
    NDRX_LOG(log_error, "***********BACKGROUND PROCESS END **********");
}

/**
 * Initialize background process
 * @return
 */
public void forward_process_init(void)
{
    struct sigaction        actions;
    
    pthread_attr_t pthread_custom_attr;
    pthread_attr_init(&pthread_custom_attr);
    
    /* set some small stacks size, 1M should be fine! */
    pthread_attr_setstacksize(&pthread_custom_attr, 2048*1024);
    pthread_create(&G_forward_thread, &pthread_custom_attr, 
            forward_process, NULL);  
}
