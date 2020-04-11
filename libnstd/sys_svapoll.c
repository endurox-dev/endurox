/**
 * @brief SystemV Queue AIX Poll
 *
 * @file sys_svapoll.c
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
#define _MSGQSUPPORT 1
#endif

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
#include <poll.h>
#include <fcntl.h>
#include <sys/select.h>

#include <sys/time.h>                   /* purely for dbg_timer()       */
#include <sys/stat.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <sys_unix.h>

#include <exhash.h>

#include "nstd_tls.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/*
 * Special ops for message queue...
 */
#define EXHASH_FIND_MQD(head,findptr,out)                                          \
    EXHASH_FIND(hh,head,findptr,sizeof(mqd_t),out)

#define EXHASH_ADD_MQD(head,ptrfield,add)                                          \
    EXHASH_ADD(hh,head,ptrfield,sizeof(mqd_t),add)

/**
 * Find System V Queue ID
 */
#define EXHASH_FIND_QID(head,findptr,out)                                          \
    EXHASH_FIND(hh_qid,head,findptr,sizeof(int),out)

/**
 * Add System V queue ID
 */
#define EXHASH_ADD_QID(head,ptrfield,add)                                          \
    EXHASH_ADD(hh_qid,head,ptrfield,sizeof(int),add)
/**
 * System V Qid map delete
 */
#define EXHASH_DEL_QID(head,delptr)                                                \
    EXHASH_DELETE(hh_qid,head,delptr)

#define EX_POLL_SETS_MAX            1024

#define EX_EPOLL_API_ENTRY      {NSTD_TLS_ENTRY; \
            G_nstd_tls->M_last_err = 0; \
            G_nstd_tls->M_last_err_msg[0] = EXEOS;}

#define ERROR_BUFFER            1024


/** get file descriptor pos  */
#define NDRX_PFD_GET(set, i) ((struct ndrx_pollfd *)((char *)set->polltab + i*sizeof(struct ndrx_pollfd)))

/** get mqueue descr pos */
#define NDRX_PMQ_GET(set, i) ((struct ndrx_pollmsg *)((char *)set->polltab + \
    sizeof(struct ndrx_pollfd)*set->nrfds + i*sizeof(struct ndrx_pollmsg)))

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Hash list of File descriptors we monitor..
 */
struct ndrx_epoll_fds
{
    int fd;
    struct ndrx_epoll_event event;  /**< monitored events               */
    EX_hash_handle hh;              /**< makes this structure hashable  */
};
typedef struct ndrx_epoll_fds ndrx_epoll_fds_t;

/**
 * hash list of queue descriptor
 */
struct ndrx_epoll_mqds
{
    mqd_t mqd;  /**< Enduro/X internal queue handler                    */
    int qid;    /**< System V queue id                                  */
    struct ndrx_epoll_event event;  /**< monitored events               */
    EX_hash_handle hh;              /**< makes this structure hashable  */
    EX_hash_handle hh_qid;          /**< QID Hash for qid->mqd map      */
};
typedef struct ndrx_epoll_mqds ndrx_epoll_mqds_t;

/**
 * Our internal 'epoll' set
 */
struct ndrx_epoll_set
{
    int fd;
    
    ndrx_epoll_fds_t *fds;  /**< hash of file descriptors for monitoring    */
    ndrx_epoll_mqds_t *mqds;/**< hash of message queues for monitoring      */
    ndrx_epoll_mqds_t *mqds_qid;/**< hash of message queues for monitoring  */
    
    int nrfds;              /**< number of FDs in poll                      */
    int nrfmqds;            /** Number of queues polled                     */
    void *polltab;           /**< poll() structure, pre-allocated           */
    
    EX_hash_handle hh;      /**< makes this structure hashable              */
};
typedef struct ndrx_epoll_set ndrx_epoll_set_t;

exprivate ndrx_epoll_set_t *M_psets = NULL; /* poll sets  */
MUTEX_LOCKDECL(M_psets_lock);
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate ndrx_epoll_mqds_t* mqd_find(ndrx_epoll_set_t *pset, mqd_t mqd);

/**
 * Return the compiled poll mode
 * @return 
 */
expublic char * ndrx_epoll_mode(void)
{
    static char *mode = "svapoll";
    return mode;
}

/**
 * Nothing to do
 * @return EXSUCCEED
 */
expublic int ndrx_epoll_sys_init(void)
{    
    return EXSUCCEED;
}

/**
 * Un-initialize polling lib
 * @return
 */
expublic void ndrx_epoll_sys_uninit(void)
{
    /* nothing todo */
}

/**
 * We basically will use unix error codes
 */
exprivate void ndrx_epoll_set_err(int error_code, const char *fmt, ...)
{
    char msg[ERROR_BUFFER+1] = {EXEOS};
    va_list ap;
    
    NSTD_TLS_ENTRY;

    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    NDRX_STRCPY_SAFE(G_nstd_tls->M_last_err_msg, msg);
    G_nstd_tls->M_last_err = error_code;

    NDRX_LOG(log_warn, "ndrx_epoll_set_err: %d (%s) (%s)",
                    error_code, strerror(G_nstd_tls->M_last_err), 
                    G_nstd_tls->M_last_err_msg);
    
}

/**
 * Find FD in hash
 */
exprivate ndrx_epoll_fds_t* fd_find(ndrx_epoll_set_t *pset, int fd)
{
    ndrx_epoll_fds_t*ret = NULL;
    
    EXHASH_FIND_INT( pset->fds, &fd, ret);
    
    return ret;
}

/**
 * FInd q descriptor in hash
 */
exprivate ndrx_epoll_mqds_t* mqd_find(ndrx_epoll_set_t *pset, mqd_t mqd)
{
    ndrx_epoll_mqds_t*ret = NULL;
    
    EXHASH_FIND_MQD( pset->mqds, &mqd, ret);
    
    return ret;
}

/**
 * Find polling set
 * @param epfd epoll set
 * @param dolock if set to !=0 f
 */
exprivate ndrx_epoll_set_t* pset_find(int epfd, int dolock)
{
    ndrx_epoll_set_t *ret = NULL;
    
    if (dolock)
    {
        MUTEX_LOCK_V(M_psets_lock);
    }
    
    EXHASH_FIND_INT( M_psets, &epfd, ret);
    
    if (dolock)
    {
        MUTEX_UNLOCK_V(M_psets_lock);
    }
    
    return ret;
}

/**
 * This synchronizes file descriptors and queues to polltab
 * @param set to update
 */
expublic int ndrx_polltab_sync(ndrx_epoll_set_t* set)
{
    int ret = EXSUCCEED;
    size_t sz;
    int i;
    ndrx_epoll_fds_t *fel, *felt;
    ndrx_epoll_mqds_t *mel, *melt;
    
    /* free up previous image */
    if (NULL!=set->polltab)
    {
        NDRX_FREE(set->polltab);
    }
    
    if (set->nrfds + set->nrfmqds == 0)
    {
        /* nothing to do */
        goto out;
    }
    
    sz = sizeof(struct ndrx_pollfd) * set->nrfds + sizeof(struct ndrx_pollmsg) * set->nrfmqds;
    
    if (NULL==(set->polltab = NDRX_MALLOC(sz)))
    {
        int err = errno;
        ndrx_epoll_set_err(errno, "Failed to alloc new poll tab: %zu bytes: %s ", 
                sz, strerror(err));
        NDRX_LOG(log_error, "Failed to alloc new poll tab: %zu bytes: %s ", 
                sz, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* update records... first comes the fds */
    i=0;
    EXHASH_ITER(hh, set->fds, fel, felt)
    {
        struct ndrx_pollfd * pfd = NDRX_PFD_GET(set, i);
        
        pfd->events=fel->event.events;
        pfd->fd=fel->fd;
        
        i++;
    }
    
    i=0;
    EXHASH_ITER(hh, set->mqds, mel, melt)
    {
        struct ndrx_pollmsg * mfd = NDRX_PMQ_GET(set, i);
        
        mfd->reqevents=mel->event.events;
        mfd->msgid=mel->mqd->qid;
        i++;
    }
    
out:
    return ret;
}

/**
 * Poll ctl for file descriptors
 * @param epfd poll set
 * @param op operation EX_EPOLL_CTL_ADD or EX_EPOLL_CTL_DEL
 * @param fd file descriptor
 * @param event events to use
 * @return EXSUCCED/EXFAIL (error set)
 */
expublic int ndrx_epoll_ctl(int epfd, int op, int fd, struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    ndrx_epoll_set_t* set = NULL;
    ndrx_epoll_fds_t * tmp = NULL;
            
    EX_EPOLL_API_ENTRY;
    
    if (NULL==(set = pset_find(epfd, EXTRUE)))
    {
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        ndrx_epoll_set_err(ENOSYS, "ndrx_epoll set %d not found", epfd);
        EXFAIL_OUT(ret);
    }
    
    if (EX_EPOLL_CTL_ADD == op)
    {
        NDRX_LOG(log_info, "%s: Add operation on ndrx_epoll set %d, fd %d", 
                __func__, epfd, fd);
        
        /* test & add to FD hash */
        if (NULL!=fd_find(set, fd))
        {
            ndrx_epoll_set_err(EINVAL, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            NDRX_LOG(log_error, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                 fd, set->fd);
            EXFAIL_OUT(ret);
        }
        
        if (NULL==(tmp = NDRX_CALLOC(1, sizeof(*tmp))))
        {
            ndrx_epoll_set_err(errno, "Failed to alloc FD hash entry");
            NDRX_LOG(log_error, "Failed to alloc FD hash entry");
            EXFAIL_OUT(ret);
        }
        
        tmp->fd = fd;
        tmp->event = *event;
        EXHASH_ADD_INT(set->fds, fd, tmp);
        
        /* resize/realloc events list, add fd */
        set->nrfds++;
        
        NDRX_LOG(log_info, "set nrfds incremented to %d", set->nrfds);
        
        /* Rebuild mem block.. */
        if (EXSUCCEED!=ndrx_polltab_sync(set))
        {
            EXFAIL_OUT(ret);
        }
    }
    else if (EX_EPOLL_CTL_DEL == op)
    {
        NDRX_LOG(log_info, "%s: Delete operation on ndrx_epoll set %d, fd %d", 
                __func__, epfd, fd);
        
        /* test & add to FD hash */
        if (NULL==(tmp=fd_find(set, fd)))
        {
            ndrx_epoll_set_err(EINVAL, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            NDRX_LOG(log_error, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            EXFAIL_OUT(ret);
        }
        
        /* Remove fd from set->restab & from hash */
        
        EXHASH_DEL(set->fds, tmp);
        NDRX_FREE((char *)tmp);
        
        /* Rebuild mem block.. */
        if (EXSUCCEED!=ndrx_polltab_sync(set))
        {
            EXFAIL_OUT(ret);
        }
        
    }
    else
    {
        ndrx_epoll_set_err(EINVAL, "Invalid operation %d", op);
        NDRX_LOG(log_error, "Invalid operation %d", op);
        
        EXFAIL_OUT(ret);
    }

out:

    NDRX_LOG(log_info, "return %d", ret);

    return ret;
}

/**
 * epoll_ctl for Posix queue descriptors
 * @param epfd poll set
 * @param op operation
 * @param mqd queue descriptor
 * @param event poll events to monitor
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t mqd, struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    ndrx_epoll_set_t* set = NULL;
    ndrx_epoll_mqds_t * tmp = NULL;
    
    EX_EPOLL_API_ENTRY;
    
    if (NULL==(set = pset_find(epfd, EXTRUE)))
    {
        ndrx_epoll_set_err(ENOENT, "ndrx_epoll set %d not found", epfd);
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        
        EXFAIL_OUT(ret);
    }
    
    if (EX_EPOLL_CTL_ADD == op)
    {
        NDRX_LOG(log_info, "%s: Add operation on ndrx_epoll set %d, fd %d", 
                __func__, epfd, mqd);
        
        /* test & add to FD hash */
        if (NULL!=mqd_find(set, mqd))
        {
            ndrx_epoll_set_err(EINVAL, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                    mqd, set->fd);
            NDRX_LOG(log_error, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                    mqd, set->fd);
            EXFAIL_OUT(ret);
        }
        
        if (NULL==(tmp = NDRX_CALLOC(1, sizeof(*tmp))))
        {
            ndrx_epoll_set_err(errno, "Failed to alloc FD hash entry");
            NDRX_LOG(log_error, "Failed to alloc FD hash entry");
            
            EXFAIL_OUT(ret);
        }
        
        tmp->mqd = mqd;
        tmp->qid = mqd->qid;
        tmp->event = *event;
        EXHASH_ADD_MQD(set->mqds, mqd, tmp);
        
        /* add QID */
        EXHASH_ADD_QID(set->mqds_qid, qid, tmp);
        
        /* resize/realloc events list, add fd */
        set->nrfmqds++;
        
        NDRX_LOG(log_info, "set nrfmqds incremented to %d", set->nrfmqds);
        
        /* Rebuild mem block.. */
        if (EXSUCCEED!=ndrx_polltab_sync(set))
        {
            EXFAIL_OUT(ret);
        }

    }
    else if (EX_EPOLL_CTL_DEL == op)
    {
        NDRX_LOG(log_info, "%s: Delete operation on ndrx_epoll set %d, fd %d", 
                __func__, epfd, mqd);
        
        /* test & add to FD hash */
        if (NULL==(tmp=mqd_find(set, mqd)))
        {
            ndrx_epoll_set_err(EINVAL, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    mqd, set->fd);
            
            NDRX_LOG(log_error, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    mqd, set->fd);
            
            EXFAIL_OUT(ret);
        }
        
        /* Remove fd from set->restab & from hash */
        EXHASH_DEL(set->mqds, tmp);
        EXHASH_DEL_QID(set->mqds_qid, tmp);
        NDRX_FREE((char *)tmp);
        
        /* resize/realloc events list, add fd */
        set->nrfmqds--;
        
        NDRX_LOG(log_info, "set nrfmqds decrement to %d", set->nrfmqds);
        
        /* Rebuild mem block.. */
        if (EXSUCCEED!=ndrx_polltab_sync(set))
        {
            EXFAIL_OUT(ret);
        }
        
    }
    else
    {
        ndrx_epoll_set_err(EINVAL, "Invalid operation %d", op);
        
        NDRX_LOG(log_error, "Invalid operation %d", op);
        
        EXFAIL_OUT(ret);
    }

out:

    

    NDRX_LOG(log_info, "return %d", ret);

    return ret;
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
expublic int ndrx_epoll_create(int size)
{
    int ret = EXSUCCEED;
    int i = 1;
    ndrx_epoll_set_t *set;
    
    EX_EPOLL_API_ENTRY;
    
    MUTEX_LOCK_V(M_psets_lock);
    
    while (NULL!=(set=pset_find(i, EXFALSE)) && i < EX_POLL_SETS_MAX)
    {
        i++;
    }
    
    /* we must have free set */
    if (NULL!=set)
    {
        ndrx_epoll_set_err(EMFILE, "Max ndrx_epoll_sets_reached");
        NDRX_LOG(log_error, "Max ndrx_epoll_sets_reached");
                
        
        set = NULL;
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Creating ndrx_epoll set: %d", i);
    
    if (NULL==(set = (ndrx_epoll_set_t *)NDRX_CALLOC(1, sizeof(*set))))
    {
        ndrx_epoll_set_err(errno, "Failed to alloc: %d bytes", sizeof(*set));
        
        NDRX_LOG(log_error, "Failed to alloc: %d bytes", sizeof(*set));

        EXFAIL_OUT(ret);
    }
    
    /* add finally to hash */
    set->fd = i; /* assign the FD num */
    EXHASH_ADD_INT(M_psets, fd, set);
    NDRX_LOG(log_info, "ndrx_epoll_create succeed, fd=%d", i);
    
out:
    
    MUTEX_UNLOCK_V(M_psets_lock); /*  <<< release the lock */

    if (EXSUCCEED!=ret)
    {
        if (NULL!=set)
        {
            NDRX_FREE((char *)set);
        }
        
        return EXFAIL;
        
    }

    return i;
}

/**
 * Close the epoll set
 */
expublic int ndrx_epoll_close(int epfd)
{
    int ret = EXSUCCEED;
    ndrx_epoll_set_t* set = NULL;
    
    ndrx_epoll_fds_t* f, *ftmp;
    ndrx_epoll_mqds_t* m, *mtmp;
    
    NDRX_LOG(log_debug, "ndrx_epoll_close(%d) enter", epfd);
    
    if (NULL==(set = pset_find(epfd, EXTRUE)))
    {
        MUTEX_UNLOCK_V(M_psets_lock); /*  <<< release the lock */
        
        ndrx_epoll_set_err(EINVAL, "ndrx_epoll set %d not found", epfd);
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        
        
        EXFAIL_OUT(ret);
    }
    
    /* Kill file descriptor hash */
    EXHASH_ITER(hh, set->fds, f, ftmp)
    {
        ndrx_epoll_ctl(set->fd, EX_EPOLL_CTL_DEL, f->fd, NULL);
    }
    
    /* Kill message queue descriptor hash */
    EXHASH_ITER(hh, set->mqds, m, mtmp)
    {
        ndrx_epoll_ctl_mq(set->fd, EX_EPOLL_CTL_DEL, m->mqd, NULL);
    }
    
    if (NULL!=set->polltab)
    {
        NDRX_FREE(set->polltab);
    }
    
    MUTEX_LOCK_V(M_psets_lock);
    EXHASH_DEL(M_psets, set);
    NDRX_FREE(set);
    MUTEX_UNLOCK_V(M_psets_lock);
    
out:
    return EXFAIL;
}

/**
 * Wait for event 
 * @param epfd file descriptor
 * @param events where to load the events
 * @param maxevents number events
 * @param timeout how long to wait
 * @return EXUSCCEE
 */
expublic int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char **buf, int *buf_len)
{
    int ret = EXSUCCEED;
    int numevents = 0;
    ndrx_epoll_set_t* set = NULL;
    char *fn = "ndrx_epoll_wait";
    int i, err=0;
    int retpoll;
    struct ndrx_pollfd *pfd;
    struct ndrx_pollmsg *pmq;
    unsigned long nfdmsgs;
    
    EX_EPOLL_API_ENTRY;
    
    /* not returning... */
    *buf_len = EXFAIL;
    
    if (NULL==(set = pset_find(epfd, EXTRUE)))
    {
        ndrx_epoll_set_err(EINVAL, "ndrx_epoll set %d not found", epfd);
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        EXFAIL_OUT(ret);
    }
    
#if 0
    /* reset revents... - seems not needed. */
    for (i=0; i<set->nrfds; i++)
    {
        NDRX_PFD_GET(set, i)->revents = 0;
    }
    
    for (i=0; i<set->nrfmqds; i++)
    {
        NDRX_PMQ_GET(set, i)->rtnevents = 0;
    }
#endif
    
    nfdmsgs=(set->nrfmqds<<16)|(set->nrfds);
    NDRX_LOG(log_debug, "%s: epfd=%d, events=%p, maxevents=%d, timeout=%d - "
                    "about to poll(nrfds=%d ndrmqds=%d) polltab=%p nfdmsgs=%lu",
                    fn, epfd, events, maxevents, timeout, set->nrfds, set->nrfmqds,
                    set->polltab, nfdmsgs);
    
    /* run the poll finally... */
    retpoll = poll( set->polltab, nfdmsgs, timeout);
    err=errno;

    if (retpoll<0)
    {
         NDRX_LOG(log_error, "Poll failure: %s", strerror(err));
         goto out;
    }
    else if (0==retpoll)
    {
         goto out;
    }
    
    /* return file events.. */
    for (i=0; NFDS(retpoll) > 0 && i < set->nrfds && numevents < maxevents; i++)
    {
        pfd = NDRX_PFD_GET(set, i);
        if (pfd->revents)
        {
            /* fill up the event block */
            NDRX_LOG(log_debug, "event no: %d revents: %d fd: %d", 
                    numevents, (int)pfd->revents, pfd->fd);
            events[numevents].data.fd = pfd->fd;
            events[numevents].events = pfd->revents;
            events[numevents].is_mqd = EXFALSE;
            numevents++;
        }
    }
    
    /* return queue events */
    for (i=0; NMSGS(retpoll) > 0 && i < set->nrfmqds && numevents < maxevents; i++)
    {
        pmq = NDRX_PMQ_GET(set, i);
        if (pmq->rtnevents)
        {
            ndrx_epoll_mqds_t *tmqd = NULL;
            EXHASH_FIND_QID( set->mqds_qid, &(pmq->msgid), tmqd);
            
            NDRX_LOG(log_debug, "event no: %d revents: %d mqd: %p (qid %d)", 
                    numevents, (int)pmq->rtnevents, tmqd->mqd, pmq->msgid);

            events[numevents].data.mqd = tmqd->mqd;
            events[numevents].events = pmq->rtnevents;
            events[numevents].is_mqd = EXTRUE;
            numevents++;
        }
    }
    
out:

    NDRX_LOG(log_info, "%s ret=%d numevents=%d", fn, ret, numevents);
    ndrx_poll_strerror(err);

    if (EXSUCCEED==ret)
    {
        return numevents;
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
expublic int ndrx_epoll_errno(void)
{
    NSTD_TLS_ENTRY;
    return G_nstd_tls->M_last_err;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
expublic char * ndrx_poll_strerror(int err)
{
    NSTD_TLS_ENTRY;
    
    snprintf(G_nstd_tls->poll_strerr, ERROR_BUFFER_POLL, "%s (last error: %s)", 
    strerror(err), G_nstd_tls->M_last_err_msg);
    
    return G_nstd_tls->poll_strerr;
}

/**
 * Translate the service name to queue
 * @param[out] send_q output service queue
 * @param[in] q_prefix queue prefix
 * @param[in] svc service name
 * @param[in] resid resource id (in this case it is 
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_epoll_service_translate(char *send_q, char *q_prefix, 
        char *svc, int resid)
{
    /* lookup service in SHM! from resid/qid -> queue */
    snprintf(send_q, NDRX_MAX_Q_SIZE+1, NDRX_SVC_QFMT_SRVID, q_prefix, 
                svc, resid);
    
    return EXSUCCEED;
}

/**
 * Not used by poll
 * @param svcnm
 * @param idx
 * @param mq_exits
 * @return 
 */
expublic mqd_t ndrx_epoll_service_add(char *svcnm, int idx, mqd_t mq_exits)
{
    return mq_exits;
}

/**
 * Not used by poll
 * @return 
 */
expublic int ndrx_epoll_shmdetach(void)
{
    /* detach resources */
    ndrx_svqshm_detach();
    return EXSUCCEED;
}

/**
 * not used by poll
 * @param idx
 * @return 
 */
expublic int ndrx_epoll_shallopenq(int idx)
{
    return EXTRUE;
}
/**
 * Not used by poll
 * @param qstr
 */
expublic void ndrx_epoll_mainq_set(char *qstr)
{
    return;
}

/**
 * Not used by poll
 * @param force
 * @return 
 */
expublic int ndrx_epoll_down(int force)
{
    return ndrx_svqshm_down(force);
}

/* vim: set ts=4 sw=4 et smartindent: */
