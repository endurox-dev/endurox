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
 * @param svcnm service name
 * @return "fake" queue descriptor
 */
expublic mqd_t ndrx_epoll_service_add(char *svcnm)
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
    
    /* boot the Auxiliary thread */
    
    return;
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
 * @param fd
 * @param event
 * @return 
 */
expublic inline int ndrx_epoll_ctl(int epfd, int op, int fd, 
        struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    
    /* Add or remove FD from the Aux thread */
    
    switch (op)
    {
        case EX_EPOLL_CTL_ADD:
            break;
        case EX_EPOLL_CTL_DEL:
            break;    
        default:
            NDRX_LOG(log_warn, "Unsupported operation: %d", op);
            errno=EINVAL;
            EXFAIL_OUT(ret);
            break;
    }
    
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
 * @return 
 */
expublic inline int ndrx_epoll_create(int size)
{
    return EXFAIL;
}

/**
 * Close Epoll set.
 */
expublic inline int ndrx_epoll_close(int fd)
{
    return EXFAIL;
}

/**
 * Wrapper for epoll_wait
 * @param epfd
 * @param events
 * @param maxevents
 * @param timeout
 * @return 
 */
expublic inline int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char *buf, int *buf_len)
{
    *buf_len=EXFAIL;
    return EXFAIL;
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
