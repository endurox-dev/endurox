/**
 * @brief Simple message client
 *
 * @file atmiclt0_mqcl.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys_mqueue.h>
#include "test000.h"
#include "sys_unix.h"
#include <ndrstandard.h>
#include <ndebug.h>
#include <errno.h>
#include <nstopwatch.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define ATTRCMP(X, Y) X.mq_curmsgs == Y.mq_curmsgs && X.mq_flags == Y.mq_flags && \
        X.mq_maxmsg == Y.mq_maxmsg && X.mq_msgsize==Y.mq_msgsize
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
        if ((mqd_t)EXFAIL==(mq1 = ndrx_mq_open(qstr, O_CREAT | O_RDWR, 0644, &attr)))
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
            NDRX_LOG(log_error, "Failed to unlink [%s]: %s", qstr, strerror(errno));
            ret=EXFAIL;
        }
    }
    
out:

    NDRX_LOG(log_error, "%s returns %d", __func__, ret);
    
    return ret;
}

/**
 * Test non existing queue open with out create
 * @param pfx test prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_nonexists(char *pfx)
{
    int ret = EXSUCCEED;
    char qstr[128];
    struct mq_attr attr;
    int i;
    mqd_t mq1 = (mqd_t)EXFAIL;
    int err;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt_none", pfx);
    
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;
    
    for (i=0; i<1000; i++)
    {
        if ((mqd_t)EXFAIL!=(mq1 = ndrx_mq_open(qstr, 0, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Queue opened for some reason but shall not: [%s]", 
                    qstr);
            EXFAIL_OUT(ret);
        }
        err = errno;
        
        if (err!=ENOENT)
        {
            NDRX_LOG(log_error, "Expected error %d (ENOENT) got %d", ENOENT, err);
            EXFAIL_OUT(ret);
        }
    }
    
out:

    NDRX_LOG(log_error, "%s returns %d", __func__, ret);
    
    return ret;
}

/**
 * Receive tests with different modes
 * @param pfx queue prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_receive(char *pfx)
{
    int ret = EXSUCCEED;
    struct mq_attr attr, attrnew, attrold;
    char buffer[TEST_REPLY_SIZE];
    struct   timespec tm;
    int i;
    char qstr[128];
    mqd_t mq = (mqd_t)EXFAIL;
    int err;
    ssize_t bytes_read;
    ndrx_stopwatch_t t;
    int tim;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt_rcv", pfx);
    
    for (i=0; i<4; i++)
    {
        /* initialize the queue attributes */
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = TEST_REPLY_SIZE;
        attr.mq_curmsgs = 0;

        /* create the message queue */
        if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(qstr, O_CREAT, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    SV_QUEUE_NAME, strerror(errno));
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> receive: timed + blocked");

        ndrx_stopwatch_reset(&t);
        /* receive the message 
         * Maybe have some timed receive
         */
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 2;  /* Set for 20 seconds */

        if (EXSUCCEED==(bytes_read=ndrx_mq_timedreceive(mq, buffer, 
                TEST_REPLY_SIZE, NULL, &tm)))
        {
            NDRX_LOG(log_error, "Got message at len %d but expected error!", bytes_read);
            EXFAIL_OUT(ret);
        }

        /* the error shall be timeout... */

        err = errno;

        if (ETIMEDOUT!=err)
        {
            NDRX_LOG(log_error, "Expected %d (EAGAIN) error but got %d", ETIMEDOUT, err);
            EXFAIL_OUT(ret);
        }

        /* test the timeout */
        tim = ndrx_stopwatch_get_delta_sec(&t);

        if (tim<2 ||tim > 3)
        {
            NDRX_LOG(log_error, "Expected timeout 2 spent %d", tim);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> receive: timed + non blocked");

        /* we should get EAGAIN and time shall be less than second */
        memcpy(&attrnew, &attr, sizeof(attr));

        attrnew.mq_flags = O_NONBLOCK;

        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attrnew, &attrold))
        {
            NDRX_LOG(log_error, "Failed to set new attr: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* compare the attribs with initial, must match */

        NDRX_LOG(log_debug, "oattr: mq_curmsgs=%ld mq_flags=%ld mq_maxmsg=%ld, mq_msgsize=%ld", 
                attrold.mq_curmsgs, attrold.mq_flags, attrold.mq_maxmsg, attrold.mq_msgsize);
        
        NDRX_LOG(log_debug, "nattr: mq_curmsgs=%ld mq_flags=%ld mq_maxmsg=%ld, mq_msgsize=%ld", 
                attr.mq_curmsgs, attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize);

        if (!(ATTRCMP(attr, attrold)))
        {
            NDRX_LOG(log_error, "Org attrs does not match!");
            EXFAIL_OUT(ret);
        }

        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 2;  /* Set for 20 seconds */
        ndrx_stopwatch_reset(&t);

        if (EXSUCCEED==(bytes_read=ndrx_mq_timedreceive(mq, buffer, 
                TEST_REPLY_SIZE, NULL, &tm)))
        {
            NDRX_LOG(log_error, "Got message at len %d but expected error!", 
                    bytes_read);
            EXFAIL_OUT(ret);
        }

        /* the error shall be timeout... */

        err = errno;

        if (EAGAIN!=err)
        {
            NDRX_LOG(log_error, "Expected %d (EAGAIN) error but got %d", EAGAIN, err);
            EXFAIL_OUT(ret);
        }

        /* test the timeout */
        tim = ndrx_stopwatch_get_delta_sec(&t);

        if (0!=tim)
        {
            NDRX_LOG(log_error, "Expected spent 0 as non blocked q, but got %d", 
                    tim);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> receive: non timed + non blocked");

        if (EXSUCCEED==(bytes_read=ndrx_mq_receive(mq, buffer, 
                TEST_REPLY_SIZE, NULL)))
        {
            NDRX_LOG(log_error, "Got message with len %d but expected error!", 
                    bytes_read);
            EXFAIL_OUT(ret);
        }

        /* the error shall be timeout... */

        err = errno;

        if (EAGAIN!=err)
        {
            NDRX_LOG(log_error, "Expected %d (EAGAIN) error but got %d", 
                    EAGAIN, err);
            EXFAIL_OUT(ret);
        }

        /* cleanup */

        if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_close(mq))
        {
            NDRX_LOG(log_error, "Failed to close queue: %s", strerror(errno));
            ret=EXFAIL;

        }

        if (EXSUCCEED!=ndrx_mq_unlink(qstr))
        {
            NDRX_LOG(log_error, "Failed to unlink [%p]: %s", 
                    qstr, strerror(errno));
            ret=EXFAIL;
        }
    }

out:
    return ret;

}

/**
 * Send tests with different modes
 * @param pfx queue prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_send(char *pfx)
{
    int ret = EXSUCCEED;
    struct mq_attr attr, attrnew, attrold;
    char buffer[TEST_REPLY_SIZE];
    struct   timespec tm;
    int i;
    char qstr[128];
    mqd_t mq = (mqd_t)EXFAIL;
    int err;
    ndrx_stopwatch_t t;
    int tim;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt_snd", pfx);
    /* unlink the queue if something left from pervious tests... */
    ndrx_mq_unlink(qstr);
    
    for (i=0; i<4; i++)
    {
        /* initialize the queue attributes */
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = TEST_REPLY_SIZE;
        attr.mq_curmsgs = 0;

        /* create the message queue */
        if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(qstr, O_CREAT | O_RDWR, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    SV_QUEUE_NAME, strerror(errno));
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> send: timed + blocked - ok (first msg)");

        ndrx_stopwatch_reset(&t);
        /* receive the message 
         * Maybe have some timed receive
         */
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 2;  /* Set for 20 seconds */

        if (EXSUCCEED!=ndrx_mq_timedsend(mq, buffer, 
                TEST_REPLY_SIZE, 0, &tm))
        {
            NDRX_LOG(log_error, "Failed to send 1: %s!", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* test the timeout */
        tim = ndrx_stopwatch_get_delta_sec(&t);

        if (0!=tim)
        {
            NDRX_LOG(log_error, "Expected send time 0, but got: %d", tim);
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, ">>> send: timed + non blocked - ok (second msg)");

        /* we should get EAGAIN and time shall be less than second */
        memcpy(&attrnew, &attr, sizeof(attr));

        attrnew.mq_flags = O_NONBLOCK;

        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attrnew, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }

        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 2;  /* Set for 20 seconds */
        ndrx_stopwatch_reset(&t);

        if (EXSUCCEED!=ndrx_mq_timedsend(mq, buffer, 
                TEST_REPLY_SIZE, 0, &tm))
        {
            NDRX_LOG(log_error, "Failed to send 2: %s!", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* test the timeout */
        tim = ndrx_stopwatch_get_delta_sec(&t);

        if (0!=tim)
        {
            NDRX_LOG(log_error, "Expected spent 0 as non blocked q, but got %d", 
                    tim);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> send: non timed + blocked - ok (third msg)");

        attrnew.mq_flags = 0;

        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attrnew, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr 2: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_mq_send(mq, buffer, 
                TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_error, "Failed to send 3: %s!", strerror(errno));
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> send: non timed + non blocked - ok (forth msg)");

        attrnew.mq_flags = O_NONBLOCK;

        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attrnew, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr 2: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_mq_send(mq, buffer, 
                TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_error, "Failed to send 4: %s!", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, ">>> Test queue attributes...");
        memset(&attrold, 0, sizeof(attrold));
        if (EXSUCCEED!=ndrx_mq_getattr(mq, &attrold))
        {
            NDRX_LOG(log_error, "Failed to get queue attribs: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* there must be 4 msgs */
        if (4!=attrold.mq_curmsgs)
        {
            NDRX_LOG(log_error, "Expected 4 msgs on queue but got: %d", attrold.mq_curmsgs);
            EXFAIL_OUT(ret);
        }
        
        if (10!=attrold.mq_maxmsg)
        {
            NDRX_LOG(log_error, "Expected maxmsg 10 but got %d", attrold.mq_maxmsg);
            EXFAIL_OUT(ret);
        }
        
        if (TEST_REPLY_SIZE!=attrold.mq_msgsize)
        {
            NDRX_LOG(log_error, "Expected msgsize %d but got %d", 
                    TEST_REPLY_SIZE, attrold.mq_msgsize);
            EXFAIL_OUT(ret);
        }
        
        /* cleanup */
        if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_close(mq))
        {
            NDRX_LOG(log_error, "Failed to close queue: %s", strerror(errno));
            ret=EXFAIL;

        }

        if (EXSUCCEED!=ndrx_mq_unlink(qstr))
        {
            NDRX_LOG(log_error, "Failed to unlink [%p]: %s", 
                    qstr, strerror(errno));
            ret=EXFAIL;
        }
    }

out:
    return ret;

}

/**
 * Test what happens if queue is full
 * @param pfx queue prefix
 * @return EXSUCCEED/EXFAIL
 */
int local_test_qfull(char *pfx)
{
    int ret = EXSUCCEED;
    struct mq_attr attr;
    char buffer[TEST_REPLY_SIZE];
    struct   timespec tm;
    int i;
    char qstr[128];
    mqd_t mq = (mqd_t)EXFAIL;
    int err;
    ndrx_stopwatch_t t;
    int tim;
    
    snprintf(qstr, sizeof(qstr), "/%s_test000_clt_full", pfx);
    /* unlink the queue if something left from pervious tests... */
    ndrx_mq_unlink(qstr);
    
    for (i=0; i<4; i++)
    {
        /* initialize the queue attributes */
        attr.mq_flags = O_NONBLOCK;
        /* this does not matter, for system v the kernel config dictates the limit */
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = TEST_REPLY_SIZE;
        attr.mq_curmsgs = 0;

        /* create the message queue */
        if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(qstr, O_CREAT | O_RDWR | O_NONBLOCK, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    SV_QUEUE_NAME, strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* fill it up... */
        
        NDRX_LOG(log_info, ">>> send: timed + non blocked fill up the queue, "
                "will get EAGAIN in non timeout period (shorter)");
        
        while (EXSUCCEED==ndrx_mq_send(mq, buffer, 
                TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_debug, "msg sent...");
        }
        err = errno;
        
        /* now it test the error code */
        if (EAGAIN!=err)
        {
            NDRX_LOG(log_error, "Expected error %d (EAGAIN) but got %d",
                    EAGAIN, err);
            EXFAIL_OUT(ret);
        }
        
        /* test the blocked mode, should get some timeout... */
        NDRX_LOG(log_debug, ">>> send: timed + blocked fill up the queue, "
                "will get timeout");
        
        /* switch to blocked */
        attr.mq_flags = 0;
        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attr, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr 1: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }

        ndrx_stopwatch_reset(&t);
        /* receive the message 
         * Maybe have some timed receive
         */
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 2;  /* Set for 20 seconds */

        if (EXSUCCEED==ndrx_mq_timedsend(mq, buffer, 
                TEST_REPLY_SIZE, 0, &tm))
        {
            NDRX_LOG(log_error, "The queue is full but msg sent for some error reason!");
            EXFAIL_OUT(ret);
        }
        err = errno;
        NDRX_LOG(log_error, "got err: %s", strerror(errno));
        /* now it test the error code */
        if (ETIMEDOUT!=err)
        {
            NDRX_LOG(log_error, "Expected error %d (EAGAIN) but got %d",
                    ETIMEDOUT, err);
            EXFAIL_OUT(ret);
        }
        
        /* test the timeout */
        tim = ndrx_stopwatch_get_delta_sec(&t);

        if (2!=tim)
        {
            NDRX_LOG(log_error, "Expected send time 2, but got: %d", tim);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, ">>> send: non timed + non blocked fill "
                "up the queue, will EAGAIN");
        
        
        attr.mq_flags = O_NONBLOCK;
        if (EXSUCCEED!=ndrx_mq_setattr(mq, &attr, NULL))
        {
            NDRX_LOG(log_error, "Failed to set new attr 2: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED==ndrx_mq_send(mq, buffer, 
                TEST_REPLY_SIZE, 0))
        {
            NDRX_LOG(log_error, "Sending shall fail, but was ok!");
            EXFAIL_OUT(ret);
        }
        
        err = errno;
        
        /* now it test the error code */
        if (EAGAIN!=err)
        {
            NDRX_LOG(log_error, "Expected error %d (EAGAIN) but got %d",
                    EAGAIN, err);
            EXFAIL_OUT(ret);
        }
        
        /* cleanup */
        if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_close(mq))
        {
            NDRX_LOG(log_error, "Failed to close queue: %s", strerror(errno));
            ret=EXFAIL;

        }

        if (EXSUCCEED!=ndrx_mq_unlink(qstr))
        {
            NDRX_LOG(log_error, "Failed to unlink [%p]: %s", 
                    qstr, strerror(errno));
            ret=EXFAIL;
        }
    }

out:
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
    
    NDRX_LOG(log_info, "test open of non existing queue in not create mode");
    
    if (EXSUCCEED!=local_test_nonexists((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }
    
    /* receive: timed + blocked */
    
    if (EXSUCCEED!=local_test_receive((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=local_test_send((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=local_test_qfull((char *)vargp))
    {
        EXFAIL_OUT(ret);
    }

out:
    if (EXSUCCEED==ret)
    {
        MUTEX_LOCK_V(M_ok_lock);
        M_ok++;
        NDRX_LOG(log_debug, "Thread finished OK (%d)!", M_ok);
        MUTEX_UNLOCK_V(M_ok_lock);
    }
    else
    {
        NDRX_LOG(log_debug, "Thread failed!");
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
    char buffer_rcv[TEST_REPLY_SIZE];
    struct   timespec tm;
    int i, j;
    char *pfx1="th1";
    pthread_t thread_id1;
    
    char *pfx2="th2";
    pthread_t thread_id2;
    
    
    /* we will run some detailed test and after wards the integration test... */
    pthread_create(&thread_id1, NULL, local_test, pfx1); 
    pthread_create(&thread_id2, NULL, local_test, pfx2); 
    
    /* wait for thread to complete.. */
    pthread_join(thread_id1, NULL); 
    pthread_join(thread_id2, NULL); 
    
    if (2!=M_ok)
    {
        NDRX_LOG(log_error, "unit test failed! %d", M_ok);
        EXFAIL_OUT(ret);
    }
    
    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;

    /* create the message queue 
     * use mode flags!
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
        
        for (j=1; j<TEST_REPLY_SIZE; j++)
        {
            buffer[j] = (char)((i+j) & 0xff);
        }

        NDRX_LOG(log_debug, "About to SND!");
        
        /* open server queue */
        if ((mqd_t)EXFAIL==(mq_srv = ndrx_mq_open(SV_QUEUE_NAME, O_RDWR, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    SV_QUEUE_NAME, strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_DUMP(log_debug, "Sending data", buffer, TEST_REPLY_SIZE);
        
        
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
        if (EXFAIL==(bytes_read=ndrx_mq_timedreceive(mq, buffer_rcv, 
                TEST_REPLY_SIZE, NULL, &tm)))
        {
            NDRX_LOG(log_error, "Failed to get message: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "Read bytes: %d", bytes_read);
        NDRX_DUMP(log_debug, "Got data", buffer_rcv, bytes_read);
        
        if (TEST_REPLY_SIZE!=bytes_read)
        {
            NDRX_LOG(log_error, "Invalid size received, expected %d but got %d",
                    TEST_REPLY_SIZE, bytes_read);
            EXFAIL_OUT(ret);
        }
        
        /* compare the message, byte by byte 
         * skip the message type...
         */
        for (j=sizeof(long); j<TEST_REPLY_SIZE; j++)
        {
            unsigned char expected = ((unsigned char)buffer[j])+1;
            unsigned char readb = (unsigned char)buffer_rcv[j];
            
            if (expected!=readb)
            {
                NDRX_LOG(log_error, "Expected %x got %x at %d",
                        (int)expected, (int)readb, j);
                EXFAIL_OUT(ret);
            }
        }
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
