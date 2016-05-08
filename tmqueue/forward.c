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
 * Read the logfiles from the disk (if any we have there...)
 * @return 
 */
public int forward_read_q(void)
{
    int ret=SUCCEED;
    
out:
    return ret;
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
 * Continues transaction background loop..
 * Try to complete the transactions.
 * @return  SUCCEED/FAIL
 */
public int forward_loop(void)
{
    int ret = SUCCEED;
    atmi_xa_log_list_t *tx_list;
    atmi_xa_log_list_t *el, *tmp;
    atmi_xa_tx_info_t xai;
    atmi_xa_log_t *p_tl;
    
    memset(&xai, 0, sizeof(xai));
    
    while(!G_forward_req_shutdown)
    {
        
        forward_lock();
        
        forward_unlock();
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
