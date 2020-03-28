/**
 * @brief System V Queue polling
 *   Basically we will operate with special shared memory registry
 *   where will be mapping between string name -> queue id
 *   and vice versa queue id -> string
 *   the registry is protected by read/write System V semaphore.
 *   The shared mem by it self is based on Posix shared mem.
 *   For System V, there will be new parameter used NDRX_QUEUES_MAX
 *   that will device the limits of the shared memory for queues.
 *
 * @file sys_svq.c
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

/*---------------------------Includes-----------------------------------*/

#include <ndrx_config.h>

#ifdef EX_OS_AIX
/* This is for aix to active extended poll */
#define _MSGQSUPPORT
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

#include <ndrstandard.h>

#include <nstopwatch.h>
#include <nstd_tls.h>
#include <exhash.h>
#include <ndebug.h>
#include <sys_svq.h>


#include "sys_unix.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define VALIDATE_MQD if ( NULL==mqd || (mqd_t)EXFAIL==mqd)\
    {\
        NDRX_LOG(log_error, "Invalid queue descriptor %p", mqd);\
        errno = EINVAL;\
        EXFAIL_OUT(ret);\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * For SystemV:
 * Close queue. Basically we remove the dynamic data associated with queue
 * the queue id by it self continues to live on... until it is unlinked
 * 
 * For svapoll: just remove the allocated block.
 * @param mqd queue descriptor
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_close(mqd_t mqd)
{
    NDRX_LOG(log_debug, "close %p mqd", mqd);
    
#ifdef EX_USE_SYSVQ
    if (NULL!=mqd && (mqd_t)EXFAIL!=mqd)
    {   
        /* close the queue 
         * we will put the Q in hash locally so while it is in pipe
         * (in kernel space), the address sanitizer might see it as leaked
         * ptr. Thus hash will keep the local pointer.
         * This is really needed for sanitizer only...
         */
        mqd->self = (char *)mqd;
        ndrx_svq_delref_add(mqd);
        
        if (EXSUCCEED!=ndrx_svq_moncmd_close(mqd))
        {
            NDRX_LOG(log_error, "Failed to close queue %p", mqd);
            userlog("Failed to close queue %p", mqd);
        }
        
        /*
         * free will be done by backend thread..
        NDRX_FREE(mqd);
         */
        
        return EXSUCCEED;
    }
    else
    {
        NDRX_LOG(log_error, "Invalid mqd %p!", mqd);
        errno = EBADF;
        return EXFAIL;
    }
#endif
    
#ifdef EX_USE_SVAPOLL
    NDRX_FREE(mqd);
#endif
}

/**
 * Get attributes of the queue
 * @param mqd queue descriptor / ptr to descr block
 * @param attr queue stats
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_getattr(mqd_t mqd, struct mq_attr *attr)
{
    int ret = EXSUCCEED;
    int err = 0;
    struct msqid_ds buf;
    
    VALIDATE_MQD;
    
    if (NULL==attr)
    {
        NDRX_LOG(log_error, "Invalid attr is null", mqd);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    memcpy(attr, &(mqd->attr), sizeof(*attr));
    
    /* read the queue stats */
    if (EXSUCCEED!=msgctl(mqd->qid, IPC_STAT, &buf))
    {
        err = errno;
        NDRX_LOG(log_debug, "Failed to get queue qid %d stats: %s",
                mqd->qid, strerror(err));
        userlog("Failed to get queue qid %d stats: %s",
                mqd->qid, strerror(err));
    }
    
    attr->mq_curmsgs = (long)buf.msg_qnum;

out:
    errno = err;
    return ret;
}

/**
 * Perform notification - not supported
 * @param mqd queue descriptor 
 * @param notification signal event descr
 * @return EXFAIL
 */
expublic int ndrx_svq_notify(mqd_t mqd, const struct sigevent *notification)
{
    NDRX_LOG(log_error, "mq_notify() not supported by System V queue emulation!");
    userlog("mq_notify() not supported by System V queue emulation!");
    return EXFAIL;
}

/**
 * Open queue this, will include following steps:
 * - build queue object
 * - register into hash list
 * - Generate next queue id, according shm
 * - if fails, something then queue must be removed from shm
 * @param pathname queue path
 * @param oflag queue flags
 * @param mode unix permissions mode
 * @param mq_attr attributes
 * @return queue descriptor or EXFAIL
 */
expublic mqd_t ndrx_svq_open(const char *pathname, int oflag, mode_t mode, 
        struct mq_attr *attr)
{
    /* Allocate queue object */
    mqd_t mq = (mqd_t)EXFAIL;
    int ret = EXSUCCEED;
    int errno_save;
    
    NDRX_LOG(log_debug, "enter");
    mq = NDRX_CALLOC(1, sizeof(struct ndrx_svq_info));
    
    if (NULL==mq)
    {
        errno_save=errno;
        NDRX_LOG(log_error, "Failed to malloc %d bytes queue descriptor", 
                (int)sizeof(struct ndrx_svq_info));
        userlog("%s: Failed to malloc %d bytes queue descriptor", 
                __func__, (int)sizeof(struct ndrx_svq_info));
        mq = (mqd_t)EXFAIL;
        EXFAIL_OUT(ret);
    }
    
    /* so we have an options:
     * - if we create a Q, then alloc new ID
     * - if queue already exists SHM, then we can use that ID directly
     */
    if (EXFAIL==(mq->qid = ndrx_svqshm_get((char *)pathname, mode, oflag)))
    {
        errno_save=errno;
        EXFAIL_OUT(ret);
    }
    
    /* mq->thread = pthread_self(); - set only in timed functions */
    NDRX_STRCPY_SAFE(mq->qstr, pathname);
    mq->mode = mode;
    if (NULL!=attr)
    {
        memcpy(&(mq->attr), attr, sizeof (*attr));
        
        /* initial attribs does not include flags */
        if (oflag & O_NONBLOCK)
        {
            mq->attr.mq_flags|=O_NONBLOCK;
            NDRX_LOG(log_debug, "Opening in non blocked mode");
        }
    }
#ifdef EX_USE_SYSVQ
    /* Init mutexes... */
    pthread_spin_init(&mq->rcvlock, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&mq->rcvlockb4, PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&mq->stamplock, PTHREAD_PROCESS_PRIVATE);
    pthread_mutex_init(&mq->barrier, NULL);
    pthread_mutex_init(&mq->qlock, NULL);
#endif
    
out:
    
    if (EXSUCCEED!=ret)
    {
        if (NULL!=(mqd_t)EXFAIL)
        {
            NDRX_FREE((char *)mq);
            mq = (mqd_t)EXFAIL;
        }
    }

    NDRX_LOG(log_debug, "return %p/%ld", mq, (long)mq);
    
    errno=errno_save;
    return mq;
}

/**
 * Timed queue receive operation
 * @param mqd queue descriptor
 * @param ptr data ptr
 * @param maxlen buffer size
 * @param priop not used
 * @param __abs_timeout absolute time out according to mq_timedreceive(3)
 * @return data len received
 */

expublic ssize_t ndrx_svq_timedreceive(mqd_t mqd, char *ptr, size_t maxlen, 
        unsigned int *priop, const struct timespec *__abs_timeout)
{
    
#ifdef EX_USE_SYSVQ
    ssize_t ret = maxlen;
    ndrx_svq_ev_t *ev = NULL;
    int err = 0;
    
    /* set thread handler - for interrupts */
    mqd->thread = pthread_self();
    
    if (EXSUCCEED!=ndrx_svq_event_sndrcv( mqd, ptr, &ret, 
            (struct timespec *)__abs_timeout, &ev, EXFALSE, EXFALSE))
    {
        err = errno;
        if (NULL!=ev)
        {
            if (NDRX_SVQ_EV_TOUT==ev->ev)
            {
                NDRX_LOG(log_warn, "Timed out");
                err = ETIMEDOUT;
            }
            else
            {
                NDRX_LOG(log_error, "Unexpected event: %d", ev->ev);
                err = EBADF;
            }
        }
        else
        {
            /* translate the error codes */
            if (ENOMSG==err)
            {
                NDRX_LOG(log_debug, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                    strerror(err));
                err = EAGAIN;
            }
            else
            {
                NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                    strerror(err));
            }
        }
        
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (NULL!=ev)
    {
        NDRX_FREE(ev);
    }
    
    errno = err;
    return ret;
#endif
    
#ifdef EX_USE_SVAPOLL
    /* in case of SVAPOLL
     * in loop while time left wait for even in poll
     * once we get something we try to receive,
     * if noting to receive, go to sleep (if time left)
     * if no time left, just return TPETIME.
     * In the same way if poll gives timeout, just return TPETIME.
     */
    
    int wait_left;
    struct timeval  timeval;
    int ret;
    int err;
    long *l;
    
    VALIDATE_MQD;
    
    NDRX_LOG(log_debug, "receiving msg mqd=%p, ptr=%p, maxlen=%d flags: %ld qid: %d",
                mqd, ptr, (int)maxlen, mqd->attr.mq_flags, mqd->qid);
    
    if (maxlen<sizeof(long))
    {
        NDRX_LOG(log_error, "Invalid message size, the minimum is %d but got %d", 
                (int)sizeof(long), (int)maxlen);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    l = (long *)ptr;    
    *l = 1;
    
    ret=msgrcv(mqd->qid, ptr, NDRX_SVQ_INLEN(maxlen), 0, IPC_NOWAIT);
    
    /* if blocked mode is requested... */
    if (ret == EXFAIL && (ENOMSG!=errno || mqd->attr.mq_flags & O_NONBLOCK))
    {
        /* if no msg, then continue with bellow */
        err = errno;
        NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
        
        /* translate to posix */
        if (ENOMSG==err)
        {
            err = EAGAIN;
        }
        
        errno = err;
        
                    
        goto out;
    }
    else 
    {
        /* got result */
        goto out;
    }
    
    gettimeofday (&timeval, NULL);
    wait_left = __abs_timeout->tv_sec - timeval.tv_sec;    
    
    /* wait for message...
     * firstly attempt to send, if NO MSG, then wait on POLL
     */
    while (wait_left>0)
    {
        /* do poll on queue.. */
        struct ndrx_pollmsg msgs[1];
        
        msgs[0].msgid=mqd->qid;
        msgs[0].rtnevents = POLLIN;
        msgs[0].reqevents = 0;
        
        ret = poll((void *)&msgs, (1<<16)|(0), wait_left);
        
        if (ret>0)
        {
            /* OK, can try to receive something */
            if (EXFAIL==(ret = msgrcv(mqd->qid, ptr, NDRX_SVQ_INLEN(maxlen), 0, IPC_NOWAIT)))
            {
                err = errno;
                /* translate the error codes */
                if (ENOMSG==err)
                {
                    NDRX_LOG(log_debug, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
                    /* OK try again, some else downloaded msg.. */
                    errno = EAGAIN;
                }
                else
                {
                    NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
                    errno = err;
                    
                    /* termiante the process.. */
                    break;
                }
            }
            else if (0==ret)
            {
                errno = ETIMEDOUT;
                break;
            }
            else
            {
                err = errno;
                
                /* this is poll error */
                NDRX_LOG(log_error, "poll (qid=%d) failed (tout: %d): %s", mqd->qid, 
                        wait_left, strerror(err));
                errno = err;

                /* terminate the receive */
                break;
            }

            gettimeofday (&timeval, NULL);
            wait_left = __abs_timeout->tv_sec - timeval.tv_sec;    
        }
    }

out:    
    if (ret>=0)
    {
        ret=NDRX_SVQ_OUTLEN(ret);
    }
    
    return ret;
    
#endif
    
}


/**
 * Sned message with timeout option
 * @param mqd message queue descriptor
 * @param ptr data ptr including mtype - long
 * @param len data len including mtype - long
 * @param prio message priority, not used
 * @param __abs_timeout time out..., from this we calculate the time diff
 *  for setting the delta time we shall spend in Q
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_timedsend(mqd_t mqd, const char *ptr, size_t len, 
        unsigned int prio, const struct timespec *__abs_timeout)
{
    
#ifdef EX_USE_SYSVQ
    ssize_t ret = len;
    ndrx_svq_ev_t *ev = NULL;
    int err = 0;
    long *l = (long *)ptr;
    
    *l = 1; /* set default mtype.. */
    /* set thread handler - for interrupts */
    mqd->thread = pthread_self();
    
    if (EXSUCCEED!=ndrx_svq_event_sndrcv( mqd, (char *)ptr, &ret, 
            (struct timespec *)__abs_timeout, &ev, EXTRUE, EXFALSE))
    {
        err = errno;
        if (NULL!=ev)
        {
            if (NDRX_SVQ_EV_TOUT==ev->ev)
            {
                NDRX_LOG(log_warn, "Timed out");
                err = ETIMEDOUT;
            }
            else
            {
                NDRX_LOG(log_error, "Unexpected event: %d", ev->ev);
                err = EBADF;
            }
        }
        else
        {
            /* translate the error codes */
            if (ENOMSG==err)
            {
                NDRX_LOG(log_debug, "msgsnd(qid=%d) failed: %s", mqd->qid, 
                    strerror(err));
                err = EAGAIN;
            }
            else
            {
                NDRX_LOG(log_error, "msgsnd(qid=%d) failed: %s", mqd->qid, 
                    strerror(err));
            }
        }
        
        EXFAIL_OUT(ret);
    }
    
    ret = EXSUCCEED;
out:
    
    if (NULL!=ev)
    {
        NDRX_FREE(ev);
    }
    
    errno = err;
    return ret;
    
#endif
    
#ifdef EX_USE_SVAPOLL
    
    /* Try to send, if queue full, wait on poll
     * if poll says ok, try to send,.. again if full, wait on poll
     * until is sent or process times out..
     */
    
    int wait_left;
    struct timeval  timeval;
    int ret;
    int err;
    long *l;
    
    VALIDATE_MQD;
    
    if (len<sizeof(long))
    {
        NDRX_LOG(log_error, "Invalid message size, the minimum is %d but got %d", 
                (int)sizeof(long), (int)len);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    l = (long *)ptr;    
    *l = 1;
    
    ret = msgsnd(mqd->qid, ptr, NDRX_SVQ_INLEN(len), IPC_NOWAIT);
    
    /* so if other error, or we get blocking condition when not requested */
    if (ret == EXFAIL && (EAGAIN!=errno || mqd->attr.mq_flags & O_NONBLOCK))
    {
        /* if no msg, then continue with bellow */
        err = errno;
        NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
        errno = err;
                    
        goto out;
    }
    else 
    {
        /* got result */
        goto out;
    }
    
    gettimeofday (&timeval, NULL);
    wait_left = __abs_timeout->tv_sec - timeval.tv_sec;    
    
    /* wait for message...
     * firstly attempt to send, if NO MSG, then wait on POLL
     */
    while (wait_left>0)
    {
        /* do poll on queue.. */
        struct ndrx_pollmsg msgs[1];
        
        msgs[0].msgid=mqd->qid;
        msgs[0].rtnevents = POLLOUT;
        msgs[0].reqevents = 0;
        
        ret = poll((void *)&msgs, (1<<16)|(0), wait_left);
        
        if (ret>0)
        {
            /* OK, can try to receive something */
            if (EXFAIL==(ret = msgsnd(mqd->qid, ptr, NDRX_SVQ_INLEN(len), IPC_NOWAIT)))
            {
                err=errno;
                
                /* translate the error codes */
                if (EAGAIN==err)
                {
                    NDRX_LOG(log_debug, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
                    /* OK try again, some else downloaded msg.. */
                    errno = err;
                }
                else
                {
                    NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                        strerror(err));
                    errno = err;
                    
                    /* termiante the process.. */
                    break;
                }
            }
            else if (0==ret)
            {
                errno = ETIMEDOUT;
                break;
            }
            else
            {
                err = errno;
                
                /* this is poll error */
                NDRX_LOG(log_error, "poll (qid=%d) failed (tout: %d): %s", mqd->qid, 
                        wait_left, strerror(err));
                errno = err;

                /* termiante the receive */
                break;
            }

            gettimeofday (&timeval, NULL);
            wait_left = __abs_timeout->tv_sec - timeval.tv_sec;    
        }
    }

out:    
    /* in case of error, errno is set! */
    
    if (ret>=0)
    {
        ret=NDRX_SVQ_OUTLEN(ret);
    }
    
    return ret;
    
#endif
}

/**
 * Just send msg, no timeout control.
 * This are also not used by polling interfaces...
 * @param mqd message queue descriptor
 * @param ptr data ptr to send. Note that structures include first field as
 *  long, due to System V IPC requirements!
 * @param len the full message size includes first long field. The
 *  function will do the internal wrappings by it self
 * @param prio priority, not used.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_send(mqd_t mqd, const char *ptr, size_t len, 
        unsigned int prio)
{
    int ret = EXSUCCEED;
    long *l;
    int msgflg;
    
    NDRX_LOG(log_debug, "sending msg mqd=%p, qid=%d, ptr=%p, len=%d",
                mqd, mqd->qid, ptr, (int)len);
    
    VALIDATE_MQD;
    
    if (len<sizeof(long))
    {
        NDRX_LOG(log_error, "Invalid message size, the minimum is %d but got %d", 
                (int)sizeof(long), (int)len);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    l = (long *)ptr;    
    *l = 1;
    
    if (mqd->attr.mq_flags & O_NONBLOCK)
    {
        msgflg = IPC_NOWAIT;
    }
    else
    {
        msgflg = 0;
    }
    
    ret = msgsnd(mqd->qid, ptr, NDRX_SVQ_INLEN(len), msgflg);
    
    /* no logging here, as we need to keep errno */
    
out:
    return ret;
}

/**
 * Receive message from queue, no timeout.
 * This are also not used by polling interfaces...
 * @param mqd message queue descriptor
 * @param ptr data ptr (this will include initial long field - message type)
 * @param maxlen max buffer size with out initial long
 * @param priop not used
 * @return EXSUCCEED/EXFAIL
 */
expublic ssize_t ndrx_svq_receive(mqd_t mqd, char *ptr, size_t maxlen, 
        unsigned int *priop)
{
    int ret = EXSUCCEED;
    long *l;
    int msgflg;
    
    VALIDATE_MQD;
    
    NDRX_LOG(log_debug, "receiving msg mqd=%p, ptr=%p, maxlen=%d flags: %ld qid: %d",
                mqd, ptr, (int)maxlen, mqd->attr.mq_flags, mqd->qid);
    
    if (maxlen<sizeof(long))
    {
        NDRX_LOG(log_error, "Invalid message size, the minimum is %d but got %d", 
                (int)sizeof(long), (int)maxlen);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    l = (long *)ptr;    
    *l = 1;
    
    if (mqd->attr.mq_flags & O_NONBLOCK)
    {
        msgflg = IPC_NOWAIT;
    }
    else
    {
        msgflg = 0;
    }
    
    if (EXFAIL==(ret = msgrcv(mqd->qid, ptr, NDRX_SVQ_INLEN(maxlen), 0, msgflg)))
    {
        int err = errno;
        
        /* translate the error codes */
        if (ENOMSG==err)
        {
            NDRX_LOG(log_debug, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                strerror(err));
            err = EAGAIN;
        }
        else
        {
            NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", mqd->qid, 
                strerror(err));
        }
        
        errno = err;
    }
    
    /* no logging here, as we need to keep errno */
    if (ret>=0)
    {
        ret=NDRX_SVQ_OUTLEN(ret);
    }
    
out:
    return ret;
}

/**
 * set queue attributes
 * @param mqd queue descriptor
 * @param attr new queue stat
 * @param oattr old queue stat (returned if not NULL)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_setattr(mqd_t mqd, const struct mq_attr *attr, 
        struct mq_attr *oattr)
{
    int ret = EXSUCCEED;
    
    VALIDATE_MQD;
    
    if (NULL==attr)
    {
        NDRX_LOG(log_error, "Invalid attr is null", mqd);
        errno = EINVAL;
        EXFAIL_OUT(ret);
    }
    
    /* old ret old attribs */
    if (NULL!=oattr)
    {
        memcpy(oattr, &(mqd->attr), sizeof(*oattr));
    }
    
    memcpy(&(mqd->attr), attr, sizeof(*attr));
    
out:
    return ret;
}

/**
 * Unlink queue should simply lookup the tables and remove the id
 * @param pathname queue string
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_unlink(const char *pathname)
{
    /* for ndrxd service queue unlinks we shall use ndrx_svqshm_ctl directly */
    
    /* TODO: we might need a condition variable to be sent in command
     * so that back thread can update it once delete is fine...! */
    
    return ndrx_svqshm_ctl((char *)pathname, EXFAIL, IPC_RMID, EXFAIL, NULL);
    
}

/* vim: set ts=4 sw=4 et smartindent: */
