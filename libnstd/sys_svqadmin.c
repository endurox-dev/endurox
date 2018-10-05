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

#include "atmi_int.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate mqd_t M_adminq = (mqd_t)EXFAIL;
exprivate pthread_t M_evthread;

/*---------------------------Prototypes---------------------------------*/

exprivate void * ndrx_svqadmin_run(void* arg);

/**
 * Perform init on Admin queue
 * @param adminq admin queue already open
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqadmin_init(mqd_t adminq)
{
    int ret = EXSUCCEED;
    
    M_adminq = adminq;
    
    if (EXSUCCEED!=(ret=pthread_create(&M_evthread, NULL, &ndrx_svqadmin_run, NULL)))
    {
        NDRX_LOG(log_error, "Failed to create admin thread: %s", strerror(ret));
        userlog("Failed to create admin thread: %s", strerror(ret));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Terminate the admin thread
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqadmin_deinit(void)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=(ret=pthread_cancel(M_evthread)))
    {
        NDRX_LOG(log_error, "Failed to cancel thread: %s", strerror(ret));
        userlog("Failed to cancel thread: %s", strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    /* join the admin thread / wait for terminate */
    pthread_join(M_evthread, NULL);
    
    NDRX_LOG(log_debug, "Admin thread terminated...");
    
out:
    return ret;
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
    int sz, len;
    int err;
    
    if (EXSUCCEED!=(ret=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)))
    {
        NDRX_LOG(log_error, "Failed to disable thread cancel: %s", strerror(ret));
        userlog("Failed to disable thread cancel: %s", strerror(ret));
        EXFAIL_OUT(ret);
    }
    
    /* Wait for message to arrive
     * and post to main thread if have any..
     */
    while (1)
    {
        qid = M_adminq->qid;
     
        /* Allocate the message size */
        sz = NDRX_MSGSIZEMAX;
        
        if (NULL!=(buf = NDRX_MALLOC(sz)))
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", sz, strerror(err));
            userlog("Failed to malloc %d bytes: %s", sz, strerror(err));
            EXFAIL_OUT(ret);
        }
        
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
        len = msgrcv(qid, buf, NDRX_SVQ_INLEN(sz), 0, 0);
        err = errno;
        
        if (EXSUCCEED!=(ret=pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)))
        {
            NDRX_LOG(log_error, "Failed to disable thread cancel: %s", strerror(ret));
            userlog("Failed to disable thread cancel: %s", strerror(ret));
            EXFAIL_OUT(ret);
        } 
        
        if (EXFAIL==len)
        {
            /* check the error code, if queue removed, then no problem just
             * terminate and that's it
             */
            if (EIDRM==err)
            {
                NDRX_LOG(log_debug, "Admin queue removed, terminating thread...");
                goto out;
            }
            else
            {
                NDRX_LOG(log_debug, "Failed to receive message on admin q: %s",
                        strerror(err));
                userlog("Failed to receive message on admin q: %s",
                        strerror(err));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /* push admin queue event... */
            ndrx_svq_ev_t *ev = NDRX_MALLOC(sizeof(ndrx_svq_ev_t));
            
            if (NULL==ev)
            {
                err = errno;
                NDRX_LOG(log_error, "Failed to malloc event %d bytes: %s",
                        sizeof(ndrx_svq_ev_t), strerror(err));
                userlog("Failed to malloc event %d bytes: %s",
                        sizeof(ndrx_svq_ev_t), strerror(err));
                EXFAIL_OUT(ret);
            }
            
            ev->data = buf;
            ev->datalen = NDRX_SVQ_OUTLEN(len);
            ev->ev = NDRX_SVQ_EV_DATA;
                    
            if (EXSUCCEED!=ndrx_svq_mqd_put_event(M_adminq, ev))
            {
                NDRX_LOG(log_error, "Failed to put admin event");
                userlog("Failed to put admin event");
                EXFAIL_OUT(ret);
            }
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

    if (NULL!=buf)
    {
        NDRX_FREE(buf);
    }

    return NULL;
}

/* vim: set ts=4 sw=4 et smartindent: */