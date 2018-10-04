/**
 * @brief System V XATMI service administrative message dispatching
 *  to System V main thread via queue and including a wakeup call.
 *  This could be done when admin queue is removed from polling - or polling
 *  is closed, we could send shutdown notification to this queue waiter...
 *  or we could just cancel the thread?
 *  we could allow to cancel the thread during the msgrcv.
 *  Also we should put the pthread_cleanup_push() to unlock the resources.
 *  
 *
 * @file sys_svqadmin.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

/*---------------------------Includes-----------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/ipc.h>
#include <sys/msg.h>

#include <ndrstandard.h>

#include <nstopwatch.h>
#include <nstd_tls.h>
#include <exhash.h>
#include <ndebug.h>
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate mqd_t M_adminq = (mqd_t)EXFAIL;
exprivate char *M_buf = NULL; /* Allocated event buffer */
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform init on Admin queue
 * @param adminq admin queue already open
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqadmin_init(mqd_t *adminq)
{
    
}

/**
 * Free up resource
 * @param arg
 */
exprivate void cleanup_handler(void *arg)
{
    if (NULL!=arg)
    {
        NDRX_FREE(arg);
    }
}

/**
 * Run the admin queue. How about messages, we shall allocate them
 * as they will be free'd by the main thread, not?
 * @param arg
 * @return 
 */
exprivate void * ndrx_svqadmin_run(void* arg)
{
    int ret = EXSUCCEED;
    int qid;
    char *buf = NULL;
    
    if (EXSUCCEED!=(ret=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)))
    {
        NDRX_LOG(log_error, "Failed to disable thread cancel: %s", strerror(ret));
        userlog("Failed to disable thread cancel: %s", strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    /* TODO: Wait for message to arrive
     * and post to main thread if have any..
     */
    
    while (1)
    {
        qid = M_adminq->qid;
     
        /* Allocate the message size */
        buf = NDRX_MALLOC(XXX);
        
        pthread_cleanup_push(cleanup_handler, buf);
        
        if (EXSUCCEED!=(ret=pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)))
        {
            NDRX_LOG(log_error, "Failed to enable thread cancel: %s", strerror(ret));
            userlog("Failed to enable thread cancel: %s", strerror(ret));
            EXFAIL_OUT(ret);
        }
        
        /* read the message, well we could read it directly from MQD 
         * then we do not need any locks..
         */
        
        if (EXSUCCEED!=(ret=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)))
        {
            NDRX_LOG(log_error, "Failed to disable thread cancel: %s", strerror(ret));
            userlog("Failed to disable thread cancel: %s", strerror(ret));
            EXFAIL_OUT(ret);
        } 
        
        pthread_cleanup_pop(1);
    }
out:
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "Admin thread failed! Abort as cannot "
                "guarantee application stability!");
        userlog("Admin thread failed! Abort as cannot "
                "guarantee application stability!");
        abort();
    }
    return NULL;
}

/* vim: set ts=4 sw=4 et smartindent: */