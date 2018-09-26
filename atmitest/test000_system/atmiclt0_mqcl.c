/**
 * @brief Simple message client
 *
 * @file atmiclt0_mqcls.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys_mqueue.h>
#include "test000.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int M_ok = 0;
MUTEX_LOCKDECL(M_ok_lock);
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Test exclusive access to queue
 * @param pfx test prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_exlc(char *pfx)
{
    int ret = EXSUCCEED;
    char qstr[128];
    struct mq_attr attr;
    int err;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt", pfx);
    
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;
    
    mqd_t mq1 = (mqd_t)EXFAIL;
    mqd_t mq2 = (mqd_t)EXFAIL;
    
    if ((mqd_t)EXFAIL==(mq1 = ndrx_mq_open(qstr, O_CREAT | O_EXCL, 0644, &attr)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                qstr, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* try open second time should fail! */
    
    if ((mqd_t)EXFAIL!=(mq2 = ndrx_mq_open(qstr, O_CREAT | O_EXCL, 0644, &attr)))
    {
        NDRX_LOG(log_error, "Second time open must fail!: [%s]: %s", 
                qstr, strerror(errno));
        EXFAIL_OUT(ret);
    }
    err = errno;
    
    if (EEXIST!=err)
    {
        NDRX_LOG(log_error, "Unit test failed: expected error %d (EEXIST) got %d", 
                EEXIST, err);
        EXFAIL_OUT(ret);
    }

out:
    
    if (EXSUCCEED!=ndrx_mq_close(mq1))
    {
        NDRX_LOG(log_error, "Failed to close %p: %s", mq1, strerror(errno));
        ret=EXFAIL;
    }

    /* something does not work right here! */
    if (EXSUCCEED!=ndrx_mq_unlink(qstr))
    {
        NDRX_LOG(log_error, "Failed to unlink [%p]: %s", qstr, strerror(errno));
        ret=EXFAIL;
    }

    NDRX_LOG(log_error, "%s returns %d", __func__, ret);
    
    return ret;
}

/**
 * Test exclusive access to queue
 * @param pfx test prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_unlink(char *pfx)
{
    int ret = EXSUCCEED;
    char qstr[128];
    struct mq_attr attr;
    char buffer[TEST_REPLY_SIZE];
    int i;
    mqd_t mq1 = (mqd_t)EXFAIL;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt_unl", pfx);
    
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;
    
    for (i=0; i<1000; i++)
    {
        if ((mqd_t)EXFAIL==(mq1 = ndrx_mq_open(qstr, O_CREAT, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    qstr, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* send one block */
        if (EXFAIL==ndrx_mq_send(mq1, buffer, 
                    TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_error, "Failed to send message: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=ndrx_mq_close(mq1))
        {
            NDRX_LOG(log_error, "Failed to close %p: %s", mq1, strerror(errno));
            ret=EXFAIL;
        }

        if (EXSUCCEED!=ndrx_mq_unlink(qstr))
        {
            NDRX_LOG(log_error, "Failed to unlink [%p]: %s", qstr, strerror(errno));
            ret=EXFAIL;
        }
    }
    
out:

    NDRX_LOG(log_error, "%s returns %d", __func__, ret);
    
    return ret;
}

/**
 * Perform local tests, we will run in threads too. so that we see that
 * threaded mode is ok for our queues.
 */
void *local_test(void *vargp) 
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_info, "create queue + try exclusive access - shall fail properly");
    if (EXSUCCEED!=local_test_exlc((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Test delayed unlink...");
    
    if (EXSUCCEED!=local_test_unlink((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }
    
    /* TODO: test open of non existing queue in not create mode */
    
    /* TODO: receive: timed + blocked */
    /* TODO: receive: timed + non blocked */
    /* TODO: receive: non timed + non blocked */
    
    /* TODO: send: timed + blocked - ok (first msg) */
    /* TODO: send: timed + non blocked - ok (second msg) */
    /* TODO: send: non timed + blocked - ok (third msg) */
    /* TODO: send: non timed + non blocked - ok (forth msg) */
    
    /* TODO: send: timed + blocked fill up the queue, will get timeout */
    /* TODO: send: timed + non blocked fill up the queue, will get EAGAIN in non timeout period (shorter) */
    /* TODO: send: non timed + non blocked fill up the queue, will EAGAIN */
    /* TODO: Test in loop queue open, put msg, close, delete (check that conditionals works */
    
    
out:
    if (EXSUCCEED==ret)
    {
        MUTEX_LOCK_V(M_ok_lock);
        M_ok++;
        MUTEX_UNLOCK_V(M_ok_lock);
    }

    return NULL;
}

/**
 * Send some stuff to test000_server Q
 * TODO: Test cases
 * - queue not found in read mode... - fail
 * @param argc
 * @param argv
 * @return 
 */
int main( int argc , char **argv )
{
    int ret = EXSUCCEED;
    mqd_t mq = (mqd_t)EXFAIL;
    mqd_t mq_srv = (mqd_t)EXFAIL;
    struct mq_attr attr;
    char buffer[TEST_REPLY_SIZE];
    struct   timespec tm;
    int i;
    char *pfx1="th1";
    pthread_t thread_id1;
    
    char *pfx2="th1";
    pthread_t thread_id2;
    
    
    /* we will run some detailed test and after wards the integration test... */
    pthread_create(&thread_id1, NULL, local_test, pfx1); 
    
    /* wait for thread to complete.. */
    pthread_join(thread_id1, NULL); 
    
    if (1!=M_ok)
    {
        NDRX_LOG(log_error, "unit test failed!");
        EXFAIL_OUT(ret);
    }
    
    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;

    /* create the message queue 
     * TODO: use mode flags!
     */
    if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(CL_QUEUE_NAME, O_CREAT, 0644, &attr)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                CL_QUEUE_NAME, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<100; i++)
    {
        ssize_t bytes_read;
        /* receive the message 
         * Maybe have some timed receive
         */
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 5;  /* Set for 20 seconds */

        NDRX_LOG(log_debug, "About to SND!");
        
        /* open server queue */
        if ((mqd_t)EXFAIL==(mq_srv = ndrx_mq_open(SV_QUEUE_NAME, 0, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    SV_QUEUE_NAME, strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        if (EXFAIL==ndrx_mq_send(mq_srv, buffer, 
                TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_error, "Failed to send message: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* close server queue... */
        if (EXFAIL==ndrx_mq_close(mq_srv))
        {
            NDRX_LOG(log_error, "Failed to close server queue: %s", 
                    strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* receive stuff back */
        if (EXFAIL==(bytes_read=ndrx_mq_timedreceive(mq, buffer, 
                TEST_REPLY_SIZE, NULL, &tm)))
        {
            NDRX_LOG(log_error, "Failed to get message: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "Read bytes: %d", bytes_read);
        
    }

out:
    /* cleanup */
    
    if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_close(mq))
    {
        NDRX_LOG(log_error, "Failed to close queue: %s", strerror(errno));
    }
    
    if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_unlink(CL_QUEUE_NAME))
    {
        NDRX_LOG(log_error, "Failed to unlink q: %s", strerror(errno));
    }


    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
