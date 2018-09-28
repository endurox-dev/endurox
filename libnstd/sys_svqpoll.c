/**
 * @brief System V Queue polling
 *  here we will work with assumption that there is only one poller sub-system
 *  per process.
 *
 * @file sys_svqpoll.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>
#include <sys/epoll.h>
#include <sys_mqueue.h>
#include <sys_svq.h>
#include <sys/fcntl.h>

#include "atmi_int.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Hash of queues and their timeouts assigned and stamps...
 */
typedef struct
{
    char svnm[MAXTIDENT+1];     /**< Service name                       */
    mqd_t mqd;                  /**< Queue handler hashed               */
    int typ;                    /**< either fake service or admin Q     */
    EX_hash_handle hh;          /**< make hashable                      */
} ndrx_svq_pollsvc_t;

/*---------------------------Globals------------------------------------*/

/**
 * main polling queue. All the service requests are made to this queue.
 */
exprivate char M_mainqstr[NDRX_MAX_Q_SIZE+1] = "";

/**
 * Main queue object used for polling
 */
exprivate mqd_t M_mainq = NULL;

/**
 * Service mapping against the fake queue descriptors
 */
exprivate ndrx_svq_pollsvc_t * M_svcmap = NULL;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Set main polling queue. The queue is open once the
 * @param qstr full queue string for the main polling interface
 */
expublic void ndrx_epoll_mainq_set(char *qstr)
{
    NDRX_STRCPY_SAFE(M_mainq, qstr);
    NDRX_LOG(log_debug, "Setting System V main dispatching queue: [%s]", qstr);
}

/**
 * Register service queue with poller interface
 * for admin we use fake service "@ADMINSVC".
 * @param svcnm service name
 * @param typ Service type, see ATMI_SRV_* constants
 * @return "fake" queue descriptor or NULL in case of error
 */
expublic mqd_t ndrx_epoll_service_add(char *svcnm, int typ)
{
    int ret = EXSUCCEED;
    mqd_t mq = NULL;
    ndrx_svq_pollsvc_t *el = NULL;
    int err;
    
    /* allocate pointer of 1 byte */
    /* register the queue in the hash */
    
    if (NULL==(ret=NDRX_MALLOC(1)))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to malloc 1 byte: %s", strerror(err));
        userlog("Failed to malloc 1 byte: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(el=NDRX_MALLOC(sizeof(*el))))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
                sizeof(*el), strerror(err));
        userlog("Failed to malloc %d bytes: %s", sizeof(*el),
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    el->mqd = mq;
    NDRX_STRCPY_SAFE(el->svnm, svnm);
    
    /* register service name into cache... */
    EXHASH_ADD_STR( M_svcmap, svnm, el );
    
    NDRX_LOG(log_debug, "[%s] mapped to %p", el->svnm, el->mqd);
    
out:
    if (EXSUCCEED!=ret && NULL!=mq)
    {
        NDRX_FREE((char *)mq);
    }
    return ret;
}

/**
 * Nothing to init for epoll()
 */
expublic inline void ndrx_epoll_sys_init(void)
{
    int ret = EXSUCCEED;
    /* boot the Auxiliary thread */
    if (EXSUCCEED!=ndrx_svqshm_init(void))
    {
        NDRX_LOG(log_error, "Failed to init System V Aux thread/SHM");
        EXFAIL_OUT(ret);
    }
    
    return ret;
}

/**
 * Nothing to un-init for epoll()
 */
expublic inline void ndrx_epoll_sys_uninit(void)
{
    return;
}

/**
 * Return the compiled poll mode
 * @return 
 */
expublic inline char * ndrx_epoll_mode(void)
{
    static char *mode = "svpoll";
    
    return mode;
}

/**
 * Wrapper for epoll_ctl, for standard file descriptors
 * @param epfd do not care about epfd, we use only one poler
 * @param op operation EX_EPOLL_CTL_ADD or EX_EPOLL_CTL_DEL
 * @param fd file descriptor to monitor
 * @param event not used
 * @return EXSUCCEED/EXFAIL
 */
expublic inline int ndrx_epoll_ctl(int epfd, int op, int fd, 
        struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    int err = 0;
    
    /* Add or remove FD from the Aux thread */
    
    switch (op)
    {
        case EX_EPOLL_CTL_ADD:
            if (EXSUCCEED!=(ret = ndrx_svq_moncmd_addfd(M_mainq, fd)))
            {
                err = errno;
                NDRX_LOG(log_error, "Failed to add fd %d to mqd %p for polling: %s",
                        fd, M_mainq);
            }
            break;
        case EX_EPOLL_CTL_DEL:
            if (EXSUCCEED!=(ret = ndrx_svq_moncmd_rmfd(fd)))
            {
                err = errno;
                NDRX_LOG(log_error, "Failed to remove fd %d to mqd %p for polling: %s",
                        fd, M_mainq);
            }
            break;    
        default:
            NDRX_LOG(log_warn, "Unsupported operation: %d", op);
            errno=EINVAL;
            EXFAIL_OUT(ret);
            break;
    }
out:
    errno = err;
    return ret;
}

/**
 * epoll_ctl for Posix queue descriptors
 * @param epfd not used
 * @param op operation EX_EPOLL_CTL_DEL
 * @param fd queue descriptor
 * @param event not used...
 * @return EXSUCCEED/EXFAIL
 */
expublic inline int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, 
        struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    ndrx_svq_pollsvc_t *el, *elt;
    
    /* just remove service from dispatching... 
     * for adding it is done by service queue open function
     * ndrx_epoll_service_add()
     */
    
    if (EX_EPOLL_CTL_DEL==op)
    {
        exprivate ndrx_svq_pollsvc_t * M_svcmap = NULL;
        EXHASH_ITER(hh, M_svcmap, el, elt)
        {
            if (el->mqd==fd)
            {
                EXHASH_DEL(M_svcmap, el);
                NDRX_FREE(el);
            }
        }
    }
    
    return ret;
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return Fake FD(1) or EXFAIL
 */
expublic inline int ndrx_epoll_create(int size)
{
    int ret = 1;
    int err;
    struct mq_attr attr;
    
    memset(&attr, 0, sizeof(attr));
    
    /* at this point we will open the M_mainq 
     * by using th rq addr provided..
     */
    M_mainq = ndrx_mq_open(M_mainqstr, O_CREAT, 0660,  &attr);
    
    if ((mqd_t)EXFAIL==M_mainq)
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to open main System V poller queue!");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Close Epoll set.
 */
expublic inline int ndrx_epoll_close(int fd)
{
    ndrx_svq_pollsvc_t *el, *elt;
    /* Close main poller Q erase mapping hashes */
    ndrx_mq_close(M_mainq);
    
    EXHASH_ITER(hh, M_svcmap, el, elt)
    {
        EXHASH_DEL(M_svcmap, el);
        NDRX_FREE(el);
    }
    
    return EXFAIL;
}

/**
 * Wrapper for epoll_wait
 * TODO: needs to provide back the identifier that we got msg for admin Q
 * or for admin we could use special service name like @ADMIN so that
 * we can find it in standard hash list? but the MQD needs to be kind of special
 * one so that we do not remove it by our selves at the fake
 * @param epfd not used, poll set
 * @param events events return events struct 
 * @param maxevents max number of events can be loaded
 * @param timeout timeout in milliseconds
 * @return 0 - timeout, -1 - FAIL, 1 - have one event
 */
expublic inline int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char *buf, int *buf_len)
{
    int ret = EXSUCCEED;
    ssize_t rcvlen = *buf_len;
    ndrx_svq_ev_t *ev = NULL;
    int err = 0;
    
    /* set thread handler - for interrupts */
    M_mainq->thread = pthread_self();
    
    /* TODO: Calculate the timeout? */
    
    if (EXSUCCEED!=ndrx_svq_event_msgrcv( M_mainq, buf, &rcvlen, 
            (struct timespec *)__abs_timeout, &ev, EXFALSE))
    {
        err = errno;
        if (NULL!=ev)
        {
            switch (ev->ev)
            {
                    
                case NDRX_SVQ_EV_TOUT:
                    NDRX_LOG(log_debug, "Timed out");
                    err = EAGAIN;
                    break;
                case NDRX_SVQ_EV_DATA:
                    
                    /* TODO: Admin thread sends something to us... */
                    
                    break;
                case NDRX_SVQ_EV_FD:
                    NDRX_LOG(log_info, "File descriptor %d sends us something "
                            "revents=%d bytes %d", ev->fd, (int)ev->revents, 
                            (int)ev->datalen);
                    events[0].is_mqd = EXFALSE;
                    events[0].events = ev->revents;
                    events[0].data.fd = ev->fd;

                    /* Copy off the data - todo think about zero copy...*/
                    if (*buf_len < ev->datalen)
                    {
                        NDRX_LOG(log_error, "Receive from FD %d bytes, but max buffer %d",
                                ev->datalen, *buf_len);
                        userlog("Receive from FD %d bytes, but max buffer %d",
                                ev->datalen, *buf_len);

                        err=EBADFD;
                        EXFAIL_OUT(ret);
                    }

                    *buf_len = ev->datalen;
                    memcpy(buf, ev->data, *buf_len);
                    break;
                    
                default:
                    NDRX_LOG(log_error, "Unexpected event: %d", ev->ev);
                    err = EBADF;
                    break;
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
    else
    {
        
        tp_command_generic_t *gen_command = (tp_command_generic_t *)buf;
        /* we got a message! */
        NDRX_LOG(log_debug, "Got message from main SysV queue %d "
                "bytes gencommand: %hd", rcvlen, gen_command->command_id);
        
        
        /* understand what type of message this is - to which queue we shall map?
         
         case ATMI_COMMAND_CONNECT:
            case ATMI_COMMAND_TPCALL:
            case ATMI_COMMAND_CONNRPLY:
            case ATMI_COMMAND_TPREPLY:

            tp_command_call_t *call

            use  tp_command_call_t to get service name
            for other cases just use any service we have advertised, if not services available, drop the message
         
         
         */
        
    }
    
out:
    
            
    if (NULL!=ev)
    {
        
        if (NULL!=ev->data)
        {
            NDRX_FREE(ev->data);
        }
        
        NDRX_FREE(ev);
    }
    
    errno = err;
    
    if (EXSUCCEED==ret)
    {
        if (0==err)
        {
            return 1; /* have one event */
        }
        else
        {
            return 0;   /* timeout */
        }
    }
    else
    {
        return EXFAIL;
    }
}

/**
 * Return errno for ndrx_poll() operation
 * @return 
 */
expublic inline int ndrx_epoll_errno(void)
{
    return errno;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
expublic inline char * ndrx_poll_strerror(int err)
{
    return strerror(err);
}

/* vim: set ts=4 sw=4 et smartindent: */
