/**
 * @brief System V Queue Time-out and event handling
 *
 * @file sys_svqevent.c
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
#include <fcntl.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <pthread.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>

#include <utlist.h>
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/** The index of the "read" end of the pipe */
#define READ 0
/** The index of the "write" end of the pipe */
#define WRITE 1
#define PIPE_POLL_IDX           0   /**< where to pipe sits in poll */
/**< default timeout when no thread have been registered for processing  */
#define DFLT_TOUT               60

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef struct ndrx_svq_mqd_hash ndrx_svq_mqd_hash_t;
/**
 * Hash of queues and their timeouts assigned and stamps...
 */
struct ndrx_svq_mqd_hash
{
    void *mqd;                  /**< Queue handler hashed               */
    
    ndrx_stopwatch_t stamp_time;/**< timestamp for timeout waiting      */
    unsigned long stamp_seq;    /**< stamp sequence                     */
    
    struct timespec abs_timeout;/**< actual future timeout time         */

    EX_hash_handle hh;          /**< make hashable                      */
};

typedef struct ndrx_svq_fd_hash ndrx_svq_fd_hash_t;
/**
 * Hash of queues and their timeouts assigned and stamps...
 */
struct ndrx_svq_fd_hash
{
    int fd;                     /**< hashed file descriptor             */
    mqd_t mqd;                  /**< Queue handler hashed               */

    EX_hash_handle hh;          /**< make hashable                      */
};

/**
 * System V Event monitor data
 */
typedef struct
{
    int nrfds;              /**< number of FDs in poll                  */
    struct pollfd *fdtab;   /**< poll() structure, pre-allocated        */
    struct pollfd *fdtabmo; /**< Real monitoirng table                  */
    
    int evpipe[2];          /**< wakeup pipe from notification thread   */
    pthread_t evthread;     /**< Event thread                           */
    
    /** 
     * Message queue descriptor hash list 
     * used for timeout processing and queue removal.
     */
    ndrx_svq_mqd_hash_t *mqdhash; 
    
    ndrx_svq_fd_hash_t *fdhash;  /**< File descriptor hash list         */
    
} ndrx_svq_evmon_t;


/*---------------------------Globals------------------------------------*/

/* have a thread handler for tout monitoring thread! */

exprivate ndrx_svq_evmon_t M_mon;        /**< event monitor data          */
exprivate int M_shutdown = EXFALSE;      /**< is shutdown requested?      */
exprivate int M_alive = EXFALSE;         /**< is monitoring thread alive? */

/* we need two hash lists
 * - the one goes by mqd to list/update timeout registrations
 *   from this list we calculate the next wakeup too...
 * - needs a hash with file descriptors, as in this list there
 *   will file descriptors. We need to know to which thread we shall
 *   deliver this event for wakeup...
 * 
 * For both hashes we need following API:
 * - add/update
 * - delete mqd or fd (loop over...)
 */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Add FD to polling structure
 * @param fd file descriptor to add for polling
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_fd_hash_addpoll(int fd, uint32_t events)
{
    int ret = EXSUCCEED;
    
    /* resize/realloc events list, add fd */
    M_mon.nrfds++;

    NDRX_LOG(log_info, "set nrfds incremented to %d, events: %d", 
        M_mon.nrfds, (int)events);

    if (NULL==(M_mon.fdtab=NDRX_REALLOC(M_mon.fdtab, sizeof(struct pollfd)*M_mon.nrfds)))
    {
        NDRX_LOG(log_error, "Failed to realloc %d/%d", 
                M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(M_mon.fdtabmo=NDRX_REALLOC(M_mon.fdtabmo, sizeof(struct pollfd)*M_mon.nrfds)))
    {
        NDRX_LOG(log_error, "Failed to realloc (mo) %d/%d", 
                M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds);
        EXFAIL_OUT(ret);
    }

    M_mon.fdtab[M_mon.nrfds-1].fd = fd;
    M_mon.fdtab[M_mon.nrfds-1].events = events;
    M_mon.fdtab[M_mon.nrfds-1].revents = 0;
    
out:
    return ret;
}

/**
 * Remove FD from polling struct
 * @param fd file descriptor
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_fd_hash_delpoll(int fd)
{
    int ret = EXSUCCEED;
    int i;
    
    for (i = 0; i < M_mon.nrfds; i++)
    {
        if (M_mon.fdtab[i].fd == fd)
        {
            /* kill the element */
            if (i!=M_mon.nrfds-1 && M_mon.nrfds>1)
            {
                memmove(&M_mon.fdtab[i], &M_mon.fdtab[i+1], 
                        sizeof(struct pollfd)*(M_mon.nrfds-i-1));
            }

            M_mon.nrfds--;

            NDRX_LOG(log_info, "set nrfds decremented to %d fdtab=%p", 
                     M_mon.nrfds, M_mon.fdtab);

            if (0==M_mon.nrfds)
            {
                NDRX_LOG(log_warn, "set->nrfds == 0, => free");
                NDRX_FREE((char *)M_mon.fdtab);
                NDRX_FREE((char *)M_mon.fdtabmo);
            }
            else if (NULL==(M_mon.fdtab=NDRX_REALLOC(M_mon.fdtab, 
                    sizeof(struct pollfd)*M_mon.nrfds)))
            {
                int err = errno;
                userlog("Failed to realloc %d/%d: %s", 
                        M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds, 
                        strerror(err));

                NDRX_LOG(log_error, "Failed to realloc %d/%d: %s", 
                        M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds, 
                        strerror(err));

                EXFAIL_OUT(ret);
            }
            else if (NULL==(M_mon.fdtabmo=NDRX_REALLOC(M_mon.fdtabmo, 
                    sizeof(struct pollfd)*M_mon.nrfds)))
            {
                int err = errno;
                userlog("Failed to realloc mo %d/%d: %s", 
                        M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds, 
                        strerror(err));

                NDRX_LOG(log_error, "Failed to realloc mo %d/%d: %s", 
                        M_mon.nrfds, sizeof(struct pollfd)*M_mon.nrfds, 
                        strerror(err));

                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    return ret;
}

/**
 * Check queue for FD presence in hash
 * @param fd queue descriptor ptr
 * @return q descriptor or NULL
 */
exprivate ndrx_svq_fd_hash_t * ndrx_svq_fd_hash_find(int fd)
{
    ndrx_svq_fd_hash_t *ret = NULL;
    
    EXHASH_FIND_INT( (M_mon.fdhash), &fd, ret);
    
    NDRX_LOG(log_dump, "checking fd %d -> %p", fd, ret);
    
    return ret;
}

/**
 * Add queue to timeout monitor
 * @param fd message queue descriptor
 * @param stamp_time timestamp when we are going to expire
 * @param stamp_seq sequence number for expiry
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_fd_hash_add(int fd, mqd_t mqd, uint32_t events)
{
    int ret = EXSUCCEED;
    ndrx_svq_fd_hash_t * el;
    
    el = ndrx_svq_fd_hash_find(fd);
    
    if (NULL==el)
    {
        el = NDRX_CALLOC(1, sizeof(ndrx_svq_fd_hash_t));

        NDRX_LOG(log_dump, "Registering %d as int", fd);

        if (NULL==el)
        {
            int err = errno;

            NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
            userlog("Failed to alloc: %s", strerror(err));

            EXFAIL_OUT(ret);
        }

        el->fd  = fd;
        el->mqd = mqd;
        
        ndrx_svq_fd_hash_addpoll(fd, events);
        EXHASH_ADD_INT((M_mon.fdhash), fd, el);
    }
    else
    {
        /* just update entry...  */
        el->mqd = mqd;
    }
    
out:
    
    return ret;
}

/**
 * Delete single entry from queue hash
 * @param fd queue ptr
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_fd_hash_del(int fd)
{
    int ret = EXSUCCEED;
    ndrx_svq_fd_hash_t *el;

    /* remove from polling array!  */
    if (EXSUCCEED!=ndrx_svq_fd_hash_delpoll(fd))
    {
        EXFAIL_OUT(ret);
    }
    
    /* remove form FD registry */
    EXHASH_FIND_INT( (M_mon.fdhash), &fd, el);
    
    if (NULL!=el)
    {
        EXHASH_DEL((M_mon.fdhash), el);
        NDRX_FREE(el);
    }
    
out:
    return ret;
}

/**
 * Delete record from fdhash by matching the queue descriptor
 * @param mqd queue descriptor ptr
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_fd_hash_delbymqd(mqd_t mqd)
{
    int ret = EXSUCCEED;
    ndrx_svq_fd_hash_t *e=NULL, *et=NULL;
    
    /* remove from polling array */
    EXHASH_ITER(hh, M_mon.fdhash, e, et)
    {
        if (e->mqd == mqd)
        {
            /* delete from polling struct */
            if (EXSUCCEED!=ndrx_svq_fd_hash_delpoll(e->fd))
            {
                EXFAIL_OUT(ret);
            }
            
            /* delete the entry by it self */
            EXHASH_DEL(M_mon.fdhash, e);
            NDRX_FREE(e);
            break;
        }
    }
    
out:
    return ret;
}

/**
 * Check queue descriptor presence in hash
 * @param mqd queue descriptor ptr
 * @return q descriptor or NULL
 */
exprivate ndrx_svq_mqd_hash_t * ndrx_svq_mqd_hash_find(mqd_t mqd)
{
    ndrx_svq_mqd_hash_t *ret = NULL;
    
    EXHASH_FIND_PTR( (M_mon.mqdhash), ((void **)&mqd), ret);
    
    NDRX_LOG(log_dump, "checking mqd %p -> %p", mqd, ret);
    
    return ret;
}

/**
 * Add queue to timeout monitor
 * @param mqd message queue descriptor
 * @param stamp_time timestamp when we are going to expire
 * @param stamp_seq sequence number for expiry
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_mqd_hash_add(mqd_t mqd, ndrx_stopwatch_t *stamp_time, 
        unsigned long stamp_seq, struct timespec *abs_timeout)
{
    int ret = EXSUCCEED;
    ndrx_svq_mqd_hash_t * el;
    
    el = ndrx_svq_mqd_hash_find(mqd);
    
    if (NULL==el)
    {
        el = NDRX_CALLOC(1, sizeof(ndrx_svq_mqd_hash_t));

        NDRX_LOG(log_dump, "Registering %p as mqd_t", mqd);

        if (NULL==el)
        {
            int err = errno;

            NDRX_LOG(log_error, "Failed to alloc: %s", strerror(err));
            userlog("Failed to alloc: %s", strerror(err));

            EXFAIL_OUT(ret);
        }

        el->mqd  = (void *)mqd;
        el->stamp_seq = stamp_seq;
        el->stamp_time = *stamp_time;
        el->abs_timeout = *abs_timeout;
        EXHASH_ADD_PTR((M_mon.mqdhash), mqd, el);
    }
    else
    {
        /* just update entry...  */
        el->stamp_seq = stamp_seq;
        el->stamp_time = *stamp_time;
        el->abs_timeout = *abs_timeout;
    }
    
out:
    
    return ret;
}

/**
 * Delete single entry from queue hash
 * @param mqd queue ptr
 */
exprivate void ndrx_svq_mqd_hash_del(mqd_t mqd)
{
    ndrx_svq_mqd_hash_t *ret = NULL;
    ndrx_svq_ev_t *elt, *tmp;
    /* Remove queue completely */
    
    NDRX_LOG(log_debug, "Closing queue %p qstr:[%s] qid:%d", 
            mqd, mqd->qstr, mqd->qid);
    
    /* for service queues, the ndrxd will which are subject for removal
     * and for which timestamp is older than configure time frame
     * this is due to fact, that queue might be open, but not yet reported
     * to ndrxd or shm.
     * thus here we just remove directly.
     */
            
    /*
    NDRX_LOG(log_dump, "Unregistering %p as mqd_t from timeout mon", mqd);
    */
    
    /* Remove any un-processed queued events... */
    DL_FOREACH_SAFE(mqd->eventq,elt,tmp)
    {
        DL_DELETE(mqd->eventq, elt);
        NDRX_FREE(elt);
    }
    
    /* remove from timeout hash */
    EXHASH_FIND_PTR( (M_mon.mqdhash), ((void **)&mqd), ret);
    
    if (NULL!=ret)
    {
        EXHASH_DEL((M_mon.mqdhash), ret);
        NDRX_FREE(ret);
    }
}

/**
 * Remove from MQ hash and remove linked file descriptors 
 * @param mqd queue descriptor to remove
 */
exprivate int ndrx_svq_mqd_hash_delfull(mqd_t mqd)
{
    int ret = EXSUCCEED;
    
    /* remove FD firstly, if any */
    if (EXSUCCEED!=ndrx_svq_fd_hash_delbymqd(mqd))
    {
        EXFAIL_OUT(ret);
    }
    
    ndrx_svq_mqd_hash_del(mqd);
    
out:
    return ret;
}

/* TODO: We need a full delete where we scan the FD hash and remove any related
 * MQDs and vice versa
 */

/**
 * Find next timeout time - time that we shall spend in sleep...
 * @return timeout for sleeping in poll mode
 */
exprivate int ndrx_svq_mqd_hash_findtout(void)
{
    int tout = DFLT_TOUT;
    long tmp;
    ndrx_svq_mqd_hash_t * r, *rt;
    struct timespec abs_timeout;
    struct timeval  timeval;
    
    gettimeofday (&timeval, NULL);
    abs_timeout.tv_sec = timeval.tv_sec;
    abs_timeout.tv_nsec = timeval.tv_usec*1000;
    
    /* loop over the hash and compare stuff for current time,
     * get delta
     */
    EXHASH_ITER(hh, (M_mon.mqdhash), r, rt)
    {
        tmp = ndrx_timespec_get_delta(&(r->abs_timeout), &abs_timeout);
        
        if (tmp < 1)
        {
            /* so we get msgs with delay, ma */
            NDRX_LOG(log_debug, "For mqd %p timeout less than 0 (%ld) "
                    "- default to 1 msec - slow system?",
                    r->mqd, tmp);
            tmp = 1;
        }
        
        /* calculate delta time 
         * get the seconds from diff
         */
        tmp = ndrx_ceil(tmp, 1000);
        
        NDRX_LOG(log_debug, "Requesting for mqd %p to %d sec",
                r->mqd, tmp);
        
        if (tmp < tout)
        {
            tout = tmp;
        }
    }
    
    NDRX_LOG(log_debug, "Next timeout requested to %d", tout);
    
    return (int)tout;
}

/**
 * Dispatch any pending timeouts
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_mqd_hash_dispatch(void)
{
    int ret = EXSUCCEED;
    ndrx_svq_mqd_hash_t * r, *rt;
    long delta;
    struct timespec abs_timeout;
    struct timeval  timeval;
    ndrx_svq_ev_t * ev = NULL;
    int err;
    
    gettimeofday (&timeval, NULL);
    abs_timeout.tv_sec = timeval.tv_sec;
    abs_timeout.tv_nsec = timeval.tv_usec*1000;
    
    EXHASH_ITER(hh, (M_mon.mqdhash), r, rt)
    {
        delta = ndrx_timespec_get_delta(&(r->abs_timeout), &abs_timeout);
        
        if (delta <= 0)
        {
            NDRX_LOG(log_warn, "Timeout condition: mqd %p time spent: %ld", 
                        r->mqd, delta);
            
            /* lets put the event to the message queue... 
             * firstly we need to allocate the event.
             */            
            if (NULL==(ev = NDRX_MALLOC(sizeof(ndrx_svq_ev_t))))
            {
                err = errno;
                NDRX_LOG(log_error, "Failed to allocate ndrx_svq_ev_t: %s", 
                            strerror(err));
                userlog("Failed to allocate ndrx_svq_ev_t: %s", 
                            strerror(err));
                EXFAIL_OUT(ret);
            }

            ev->ev = NDRX_SVQ_EV_TOUT;
            ev->data = NULL;
            ev->stamp_seq = r->stamp_seq;
            ev->stamp_time = r->stamp_time;

            if (EXSUCCEED!=ndrx_svq_mqd_put_event(r->mqd, ev))
            {
                NDRX_LOG(log_error, "Failed to put event for %p typ %d",
                        r->mqd, ev->ev);
                EXFAIL_OUT(ret);
            }
            
            /* delete timeout object from hash as no more relevant... */
            EXHASH_DEL((M_mon.mqdhash), r);
            NDRX_FREE(r);
            
        }
    }
out:
    
    if (EXSUCCEED!=ret && NULL!=ev)
    {
        NDRX_FREE(ev);
    }

    return ret;
}

/**
 * What about admin msg thread? If we get admin message, then
 * we need to forward the message to main thread, not?
 * this down mixing we will do at poller level..
 * 
 * @param mqd message queue descriptor
 * @param ev allocate event structure
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_mqd_put_event(mqd_t mqd, ndrx_svq_ev_t *ev)
{
    int ret = EXSUCCEED;
    int l2, l1;
    int sigs = 0;

    /* Append messages to Q: */
    pthread_mutex_lock(&(mqd->qlock));
    DL_APPEND(mqd->eventq, ev);
    pthread_mutex_unlock(&(mqd->qlock));
    
    /* now emit the wakeup call */
    /* put borderlock */	
    pthread_mutex_lock(&(mqd->barrier));

    l1=pthread_mutex_trylock(&(mqd->rcvlockb4));
    l2=pthread_mutex_trylock(&(mqd->rcvlock));


    if (0==l1)
    {
        /* nothing todo, not locked... 
         * thus......
         * assume that it will pick the msg up in next loop
         */
        /* fprintf(stderr, "we locked b4 area, thus process is in workload\n"); */
        pthread_mutex_unlock(&(mqd->rcvlockb4));

        if (0==l2)
        {
            pthread_mutex_unlock(&(mqd->rcvlock));
        }

    }
    else
    {

        /* both locked...
         * so we will signal the thread
         */

        /* unlock our fired, so that it can step forward
         */
        if (0==l1)
        {
            pthread_mutex_unlock(&(mqd->rcvlockb4));
        }

        if (0==l2)
        {
            pthread_mutex_unlock(&(mqd->rcvlock));
        }

        /* reseync on Q lock so that main thread goes into both locked state*/
        pthread_mutex_lock(&(mqd->qlock));
        pthread_mutex_unlock(&(mqd->qlock));

        /* now loop the locking until we get both locked 
         * or both non locked
         */
        while (1)
        {
            /* try lock again... */
            l1=pthread_mutex_trylock(&(mqd->rcvlockb4));
            l2=pthread_mutex_trylock(&(mqd->rcvlock));

            /* both not locked, then we need to interrupt the thread */

            if (0==l1 && 0==l2)
            {
                /* then it will process or queued event anyway... */

                pthread_mutex_unlock(&(mqd->rcvlockb4));
                pthread_mutex_unlock(&(mqd->rcvlock));

                break;
            }
            else if (0!=l1 && 0!=l2)
            {
                /*Maybe try to send only 1 signal?*/

                if (0==sigs)
                {
                    if (0!=pthread_kill(mqd->thread, NDRX_SVQ_SIG))
                    {
                        int err = errno;
                        NDRX_LOG(log_error, "pthread_kill(%d) failed: %s", 
                                NDRX_SVQ_SIG, strerror(err));
                        userlog("pthread_kill(%d) failed: %s", 
                                NDRX_SVQ_SIG, strerror(err));
                        EXFAIL_OUT(ret);
                    }
                    sigs++;
                }
            }

            if (0==l1)
            {
                pthread_mutex_unlock(&(mqd->rcvlockb4));
            }

            if (0==l2)
            {
                pthread_mutex_unlock(&(mqd->rcvlock));
            }

            /* TODO: Maybe do yeald... */
            usleep(1);

        }
    }

    pthread_mutex_unlock(&(mqd->barrier));
    
out:
    return ret;        
}

/**
 * Wakup signal handling
 * @param sig
 */
exprivate void ndrx_svq_signal_action(int sig)
{
    /* nothing todo, just ignore */
    NDRX_LOG(log_debug, "Signal action");
    return;
}

/**
 * Thread used for monitoring the pipe of incoming timeout requests
 * doing polling for the given time period and sending the events
 * to queue threads.
 * @param arg
 * @return 
 */
exprivate void * ndrx_svq_timeout_thread(void* arg)
{
    int retpoll;
    int ret = EXSUCCEED;
    int timeout;
    int i, moc;
    int err;
    ndrx_svq_mon_cmd_t cmd;
    ndrx_svq_ev_t *ev;
    int syncfd = EXFALSE;
    /* we shall receive unnamed pipe
     * in thread.
     * 
     * During the operations, via pipe we might receive following things:
     * 1) request for timeout monitoring.
     * 2) request for adding particular FD in monitoring sub-set
     * 3) request for deleting particular FD from monitoring sub-set
     */
    
    /* wait for event... */
    while (!M_shutdown)
    {
        timeout = ndrx_svq_mqd_hash_findtout();
        
        if (EXFAIL==timeout || 0==timeout)
        {
            NDRX_LOG(log_error, "Invalid System V poller timeout!");
            userlog("Invalid System V poller timeout!");
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "About to poll for: %d nrfds=%d",
                timeout, M_mon.nrfds);
        
        moc=0;
        for (i=0; i<M_mon.nrfds; i++)
        {
            /* M_mon.fdtab[i].revents = 0; 
             
             we know that first is command pipe the others are FDs
             */
            if (syncfd || i<1)
            {
                M_mon.fdtabmo[moc].revents = 0;
                M_mon.fdtabmo[moc].fd = M_mon.fdtab[i].fd;
                M_mon.fdtabmo[moc].events = M_mon.fdtab[i].events;
                
                NDRX_LOG(log_error, "moc=%d %p %p fd=%d", moc, 
                        M_mon.fdtabmo, M_mon.fdtab, M_mon.fdtabmo[moc].fd);
                moc++;
            }
            
        }
        
        retpoll = poll( M_mon.fdtabmo, moc, timeout*1000);
        
        syncfd = EXFALSE;
        
        NDRX_LOG(log_debug, "poll() ret = %d pid = %d", retpoll, (int)getpid());
        if (EXFAIL==retpoll)
        {
            err = errno;
            
            if (err==EINTR)
            {
                NDRX_LOG(log_warn, "poll() interrupted... ignore...: %s", 
                        strerror(err));
            }
            else
            {
                NDRX_LOG(log_error, "System V auxiliary event poll() got error: %s", 
                        strerror(err));
                userlog("System V auxiliary event poll() got error: %s", 
                        strerror(err));
                EXFAIL_OUT(ret);
            }
        }
        else if (0==retpoll)
        {
            /* 
             * we got timeout, so lets scan the hash lists to detect
             */
            NDRX_LOG(log_debug, "Got timeout...");
            
            /* 
             * now detect queues/threads to which this event shall be submitted
             */
            if (EXSUCCEED!=ndrx_svq_mqd_hash_dispatch())
            {
                NDRX_LOG(log_error, "Failed to dispatch events - general failure "
                        "- reboot to avoid lockups!");
                userlog("Failed to dispatch events - general failure "
                        "- reboot to avoid lockups!");
                
                /* better reboot process, so that we avoid any lockups! */
                exit(EXFAIL);
            }
            
        }
        else for (i=0; i<moc; i++)
        {
            if (M_mon.fdtabmo[i].revents)
            {
                NDRX_LOG(log_debug, "%d fd=%d revents=%d", i, 
                        M_mon.fdtabmo[i].fd, (int)M_mon.fdtabmo[i].revents);
                if (PIPE_POLL_IDX==i)
                {
                    /* We got something from command pipe, thus 
                     * we alter our selves
                     */
                    
                    /* receive the command first... */
                    if (EXFAIL==read(M_mon.evpipe[READ], &cmd, sizeof(cmd)))
                    {
                        err = errno;
                        
                        NDRX_LOG(log_error, "Failed to receive command block by "
                                "System V monitoring thread: %s", strerror(err));
                        userlog("Failed to receive command block by "
                                "System V monitoring thread: %s", strerror(err));
                        EXFAIL_OUT(ret);
                    }
                    
                    NDRX_LOG(log_debug, "Got command: %d %p pid=%d flags=%d", 
                            cmd.cmd, cmd.mqd, (int)getpid(), cmd.flags);
                    
                    /* next time monitor file descriptors too */
                    if (NDRX_SVQ_MONF_SYNCFD & cmd.flags)
                    {
                        syncfd = EXTRUE;
                    }
                    
                    switch (cmd.cmd)
                    {
                        case NDRX_SVQ_MON_TOUT:
                            
                            if (cmd.abs_timeout.tv_sec > 0 || 
                                    cmd.abs_timeout.tv_nsec > 0)
                            {
                                NDRX_LOG(log_debug, "register timeout %p "
                                        "stamp_seq=%ld sec %ld nsec %ld",
                                        cmd.mqd, cmd.stamp_seq, 
                                        (long)cmd.abs_timeout.tv_sec,
                                        (long)cmd.abs_timeout.tv_nsec);

                                if (EXSUCCEED!=ndrx_svq_mqd_hash_add(cmd.mqd, 
                                        &(cmd.stamp_time), cmd.stamp_seq, &(cmd.abs_timeout)))
                                {
                                    NDRX_LOG(log_error, "Failed to register timeout for %p!",
                                            cmd.mqd);
                                    userlog("Failed to register timeout for %p!",
                                            cmd.mqd);
                                    EXFAIL_OUT(ret);
                                }
                            }
                            
                            break;
                        case NDRX_SVQ_MON_ADDFD:
                            
                            NDRX_LOG(log_info, "register fd %d for %p events %ld", 
                                    cmd.fd, cmd.mqd, (long)cmd.events);
                            if (EXSUCCEED!=ndrx_svq_fd_hash_add(cmd.fd, cmd.mqd, 
                                    cmd.events))
                            {
                                NDRX_LOG(log_error, "Failed to register timeout for %p!",
                                        cmd.mqd);
                                userlog("Failed to register timeout for %p!",
                                        cmd.mqd);
                                EXFAIL_OUT(ret);
                            }
                            
                            break;
                            
                        case NDRX_SVQ_MON_RMFD:
                            NDRX_LOG(log_info, "Deregister fd %d from polling", 
                                    cmd.fd);
                            ndrx_svq_fd_hash_del(cmd.fd);
                            break;
                        case NDRX_SVQ_MON_TERM:
                            NDRX_LOG(log_info, "Terminate request...");
                            goto out;
                            break;
                        case NDRX_SVQ_MON_CLOSE:
                            
                            /*
                             * This is close, not unlink...
                             */    
                            NDRX_LOG(log_info, "Close queue command mqd: %p qstr: [%s]/%d",
                                    cmd.mqd, cmd.mqd->qstr, cmd.mqd->qid);
                         
                            if (EXSUCCEED!=ndrx_svq_mqd_hash_delfull(cmd.mqd))
                            {
                                ret = EXFAIL;
                            }

                            pthread_mutex_destroy(&cmd.mqd->rcvlock);
                            pthread_mutex_destroy(&cmd.mqd->rcvlockb4);
                            pthread_mutex_destroy(&cmd.mqd->barrier);
                            pthread_mutex_destroy(&cmd.mqd->qlock);
                            
                            NDRX_FREE(cmd.mqd);

                            if (EXSUCCEED!=ret)
                            {
                                goto out;
                            }
                            
                            break;
                    }
                    
                }
                else
                {
                    ndrx_svq_fd_hash_t *fdh =  ndrx_svq_fd_hash_find(M_mon.fdtabmo[i].fd);
                    
                    if (NULL==fdh)
                    {
                        NDRX_LOG(log_error, "File descriptor %d not registered"
                                " int System V poller - FAIL", M_mon.fdtabmo[i].fd);
                        userlog("File descriptor %d not registered"
                                " int System V poller - FAIL", M_mon.fdtabmo[i].fd);
                        EXFAIL_OUT(ret);
                    }
                            
                    /* this one is from related file descriptor... 
                     * thus needs to put an event.
                     */
                   if (NULL==(ev = NDRX_MALLOC(sizeof(*ev))))
                   {
                       err = errno;
                       NDRX_LOG(log_error, "Failed to malloc ndrx_svq_ev_t: %s", 
                               strerror(err));
                       userlog("Failed to malloc ndrx_svq_ev_t: %s", 
                               strerror(err));
                       EXFAIL_OUT(ret);
                   }
                   
                   ev->data = NULL;
                   ev->datalen = 0;
                   ev->ev = NDRX_SVQ_EV_FD;
                   ev->fd = M_mon.fdtabmo[i].fd;
                   ev->revents = M_mon.fdtabmo[i].revents;
                   
                   /* get queue descriptor  
                    * the data is deallocated by target thread
                    */
                   if (EXSUCCEED!=ndrx_svq_mqd_put_event(fdh->mqd, ev))
                   {
                       NDRX_LOG(log_error, "Failed to put FD event for %p - FAIL", 
                               fdh->mqd);
                       EXFAIL_OUT(ret);
                   }
                }
            } /* if got revents */
        } /* for events... */
    } /* while not shutdown... */
    
out:
    
    if (NULL!=M_mon.fdtab)
    {
        NDRX_FREE(M_mon.fdtab);
    }

    if (NULL!=M_mon.fdtabmo)
    {
        NDRX_FREE(M_mon.fdtabmo);
    }

    /* if we get "unknown" error here, then we have to shutdown the whole app
     * nothing todo, as we are totally corrupted
     */
    
    M_alive = EXFALSE;

    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "System V Queue monitoring thread faced "
                "unhandled error - terminate!");
        userlog("System V Queue monitoring thread faced unhandled error - terminate!");
        exit(ret);
    }
    
    return NULL;
}

/**
 * Terminate the poller thread
 */
expublic void ndrx_svq_event_exit(int detach)
{
    NDRX_LOG(log_debug, "Terminating event thread...");
    if (M_alive)
    {
        ndrx_svq_moncmd_term();
        if (pthread_self()!=M_mon.evthread)
        {
            NDRX_LOG(log_debug, "Join evthread...");
            pthread_join(M_mon.evthread, NULL);
            NDRX_LOG(log_debug, "Join evthread... (done)");
        }
    }
    
    if (detach)
    {
        /* detach resources */
        ndrx_svqshm_detach();
    }   
}

/**
 * Terminate resources..
 */
expublic void ndrx_svq_event_atexit(void)
{
    ndrx_svq_event_exit(EXTRUE);
}

/**
 * Prepare event thread for forking
 * This will terminate the event thread
 */
exprivate void event_fork_prepare(void)
{
    NDRX_LOG(log_debug, "Preparing System V Aux thread for fork");
    
    ndrx_svq_event_exit(EXFALSE);
    
    /* Close pipes */
    if (EXSUCCEED!=close(M_mon.evpipe[READ]))
    {
        NDRX_LOG(log_error, "Failed to close READ PIPE %d: %s",
                M_mon.evpipe[READ], strerror(errno));
    }
    
    if (EXSUCCEED!=close(M_mon.evpipe[WRITE]))
    {
        NDRX_LOG(log_error, "Failed to close WRITE PIPE %d: %s",
                M_mon.evpipe[READ], strerror(errno));
    }
    
}

/**
 * Resume after fork 
 */
exprivate void event_fork_resume(void)
{
    int err;
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "Restoring System V Aux thread after fork %d", (int)getpid());
    
    /* create pipes */
    /* O_NONBLOCK */
    if (EXFAIL==pipe(M_mon.evpipe))
    {
        err = errno;
        NDRX_LOG(log_error, "pipe failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==fcntl(M_mon.evpipe[READ], F_SETFL, 
                fcntl(M_mon.evpipe[READ], F_GETFL) | O_NONBLOCK))
    {
        err = errno;
        NDRX_LOG(log_error, "fcntl READ pipe set O_NONBLOCK failed: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }

    if (NULL==(M_mon.fdtab = NDRX_CALLOC(M_mon.nrfds, sizeof(struct pollfd))))
    {
        err = errno;
        NDRX_LOG(log_error, "calloc for pollfd failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(M_mon.fdtabmo = NDRX_CALLOC(M_mon.nrfds, sizeof(struct pollfd))))
    {
        err = errno;
        NDRX_LOG(log_error, "calloc for pollfd mo failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* create thread */
    
    /* So wait for events here in the pip form Q thread */
    M_mon.fdtab[PIPE_POLL_IDX].fd = M_mon.evpipe[READ];
    M_mon.fdtab[PIPE_POLL_IDX].events = POLLIN;
    
    M_mon.nrfds = 1; /* initially only pipe wait */
    
    /* startup tup the thread */
    NDRX_LOG(log_debug, "System V Monitoring pipes fd read:%d write:%d",
                            M_mon.evpipe[READ], M_mon.evpipe[WRITE]);
    
    M_alive=EXTRUE;
    if (EXSUCCEED!=(ret=pthread_create(&(M_mon.evthread), NULL, 
            ndrx_svq_timeout_thread, NULL)))
    {
        M_alive=EXFALSE;
        NDRX_LOG(log_error, "Failed to create System V Auch thread: %s",
                strerror(ret));
        userlog("Failed to create System V Auch thread: %s",
                strerror(ret));
        EXFAIL_OUT(ret);
    }
    
out:
                            
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "System V AUX thread resume after fork failed - abort!");
        userlog("System V AUX thread resume after fork failed - abort!");
        abort();
    }

}

/**
 * Setup basics for Event handling for System V queues
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_event_init(void)
{
    int err;
    int ret = EXSUCCEED;
    static int first = EXTRUE;
   /* 
    * Signal handling 
    */
    struct sigaction act;
    
    NDRX_LOG(log_debug, "About to init System V Eventing sub-system");
    memset(&act, 0, sizeof(act));
    /* define the signal action */

    /* make "act" an empty signal set */
    sigemptyset(&act.sa_mask);

    act.sa_handler = ndrx_svq_signal_action;
    act.sa_flags = SA_NODEFER;

    if (EXSUCCEED!=sigaction(NDRX_SVQ_SIG, &act, 0))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to init sigaction: %s" ,strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* At this moment we need to bootstrap a timeout monitoring thread.. 
     * the stack size of this thread could be small one because it
     * will mostly work with dynamically allocated memory..
     * need to open unnamed pipe too for incoming commands to thread.
     */
    memset(&M_mon, 0, sizeof(M_mon));
    
    /* O_NONBLOCK */
    if (EXFAIL==pipe(M_mon.evpipe))
    {
        err = errno;
        NDRX_LOG(log_error, "pipe failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }

    if (EXFAIL==fcntl(M_mon.evpipe[READ], F_SETFL, 
                fcntl(M_mon.evpipe[READ], F_GETFL) | O_NONBLOCK))
    {
        err = errno;
        NDRX_LOG(log_error, "fcntl READ pipe set O_NONBLOCK failed: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }

    M_mon.nrfds = 1; /* initially only pipe wait */
    if (NULL==(M_mon.fdtab = NDRX_CALLOC(M_mon.nrfds, sizeof(struct pollfd))))
    {
        err = errno;
        NDRX_LOG(log_error, "calloc for pollfd failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(M_mon.fdtabmo = NDRX_CALLOC(M_mon.nrfds, sizeof(struct pollfd))))
    {
        err = errno;
        NDRX_LOG(log_error, "calloc for pollfd mo failed: %s", strerror(err));
        EXFAIL_OUT(ret);
    }

    /* So wait for events here in the pip form Q thread */
    M_mon.fdtab[PIPE_POLL_IDX].fd = M_mon.evpipe[READ];
    M_mon.fdtab[PIPE_POLL_IDX].events = POLLIN;
    
    /* startup tup the thread */
    NDRX_LOG(log_debug, "System V Monitoring pipes fd read:%d write: %d",
                            M_mon.evpipe[READ], M_mon.evpipe[WRITE]);
    
    if (EXSUCCEED!=(ret=pthread_create(&(M_mon.evthread), NULL, ndrx_svq_timeout_thread, NULL)))
    {
        NDRX_LOG(log_error, "Failed to create monitoring thread: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* register fork handlers */
    M_alive=EXTRUE;
        
    if (first)
    {
        /* have exit handler */
        if (EXSUCCEED!=atexit(ndrx_svq_event_atexit))
        {
            err = errno;
            NDRX_LOG(log_error, "Failed to register ndrx_svq_event_exit with atexit(): %s",
                    strerror(err));
            userlog("Failed to register ndrx_svq_event_exit with atexit(): %s",
                    strerror(err));
            EXFAIL_OUT(ret);
        }
    
        if (EXSUCCEED!=(ret=pthread_atfork(event_fork_prepare, 
                event_fork_resume, event_fork_resume)))
        {
            M_alive=EXFALSE;
            NDRX_LOG(log_error, "Failed to register fork handlers: %s", strerror(ret));
            userlog("Failed to register fork handlers: %s", strerror(ret));
            EXFAIL_OUT(ret);
        }
        /* after xadmin un-inits we might get some extra threads...
         * due to mulitple calls... */
        first = EXFALSE;
    }

out:
    errno = err;
    return ret;
}

/**
 * Submit monitor command
 * @param cmd filled command block
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svq_moncmd_send(ndrx_svq_mon_cmd_t *cmd)
{
    int ret = EXSUCCEED;
    int err = 0;
    
    if (EXFAIL==write (M_mon.evpipe[WRITE], (char *)cmd, 
            sizeof(ndrx_svq_mon_cmd_t)))
    {
        err = errno;
        NDRX_LOG(log_error, "Error ! write fail: %s", strerror(err));
        userlog("Error ! write fail: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
out:
    errno = err;
    return ret;
}

/**
 * register timeout for the monitor
 * @param mqd message queue ptr
 * @param stamp_time timestamp
 * @param stamp_seq sequence number
 * @param abs_timeout absolute timestamp
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_moncmd_tout(mqd_t mqd, ndrx_stopwatch_t *stamp_time, 
        unsigned long stamp_seq, struct timespec *abs_timeout, int syncfd)
{
    ndrx_svq_mon_cmd_t cmd;
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.cmd = NDRX_SVQ_MON_TOUT;
    cmd.mqd = mqd;
    cmd.stamp_time = *stamp_time;
    cmd.stamp_seq = stamp_seq;
    cmd.abs_timeout = *abs_timeout;
    
    if (syncfd)
    {
        cmd.flags|=NDRX_SVQ_MONF_SYNCFD;
    }
    
    return ndrx_svq_moncmd_send(&cmd);
}

/**
 * Add file descriptor to event monitor
 * @param mqd message queue ptr
 * @param fd file descriptor
 * @param[in] events to poll for
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_moncmd_addfd(mqd_t mqd, int fd, uint32_t events)
{
    ndrx_svq_mon_cmd_t cmd;
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.cmd = NDRX_SVQ_MON_ADDFD;
    cmd.mqd = mqd;
    cmd.fd = fd;
    cmd.events = events;
    
    return ndrx_svq_moncmd_send(&cmd);
}

/**
 * Remove file descriptor.
 * @param fd file descriptor to remove from monitor
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_moncmd_rmfd(int fd)
{
    ndrx_svq_mon_cmd_t cmd;
    
    memset(&cmd, 0, sizeof(cmd));
    
    
    cmd.cmd = NDRX_SVQ_MON_RMFD;
    cmd.fd = fd;
    
    return ndrx_svq_moncmd_send(&cmd);
}

/**
 * Terminate the monitor thread
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_moncmd_term(void)
{
    ndrx_svq_mon_cmd_t cmd;
    int ret;
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.cmd = NDRX_SVQ_MON_TERM;
    
    ret=ndrx_svq_moncmd_send(&cmd);
    
    /* wait for thread to kill up */
    
    return ret;
}

/**
 * Remove message queue from the monitor.
 * Well what will happen if Queue is closed and mqd deleted?
 * we will not be able to delete them from hashes...
 * thus close shall finish off any timeouts
 * @param mqd message queue descriptor to remove
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_moncmd_close(mqd_t mqd)
{
    int ret;
    ndrx_svq_mon_cmd_t cmd;
    
    memset(&cmd, 0, sizeof(cmd));
    
    cmd.cmd = NDRX_SVQ_MON_CLOSE;
    cmd.mqd = mqd;

    
    /* perform sync off */
    ret = ndrx_svq_moncmd_send(&cmd);
    
}

/**
 * Send/Receive data with timeout and other events option
 * @param mqd queue descriptor (already validated)
 * @param ptr data ptr 
 * @param maxlen buffer size for receive. For sending it is data len
 * @param abs_timeout absolute timeout passed to posix q
 * @param ev if event received, then pointer is set to dequeued event.
 * @param p_ptr allocate data buffer for NDRX_SVQ_EV_DATA event.
 * @param is_send do we send? if EXFALSE, we do receive
 * @param[in] syncfd Tell poller thread that we start to wait for FD events
 *  otherwise sync thread won't monitor FDs, due to fact that while we are
 *  not processed the FD, we will gate again FD wakups for the same events.
 * @return EXSUCCEED (got message from q)/EXFAIL (some event received)
 */
expublic int ndrx_svq_event_msgrcv(mqd_t mqd, char *ptr, size_t *maxlen, 
        struct timespec *abs_timeout, ndrx_svq_ev_t **ev, int is_send, int syncfd)
{
    int ret = EXSUCCEED;
    int err;
    int msgflg;
    int len;
    
    /* set the flag value */
    if (mqd->attr.mq_flags & O_NONBLOCK)
    {
        msgflg = IPC_NOWAIT;
    }
    else
    {
        msgflg = 0;
    }
    
    *ev = NULL; /* no event... */
    len = *maxlen;
    /* to avoid the deadlocks, we shall register the timeout here...
     * if we do it after the "rcvlockb4" then we might 
     * but then if we got any applied event, we shall change the
     * timestamp and sequence as the timeout request is not more
     * valid and shall be discarded next time...
     * or just do no thing, as on next entry we will update the stamp
     * and the processed one will be invalid
     */
    
    /* update time stamps */
    pthread_mutex_lock(&(mqd->stamplock));
    mqd->stamp_seq++;
    ndrx_stopwatch_reset(&(mqd->stamp_time));
    pthread_mutex_unlock(&(mqd->stamplock));
    
    /* register timeout: */

    /* if abs timeout is set to zero, then there is no timeout expected.. */
    NDRX_LOG(log_debug, "timeout tv_sec=%ld tv_nsec=%ld", 
        (long)abs_timeout->tv_sec, (long)abs_timeout->tv_nsec);
    
    if (0!=abs_timeout->tv_nsec || 0!=abs_timeout->tv_sec || syncfd)
    {
        if (EXSUCCEED!=ndrx_svq_moncmd_tout(mqd, &(mqd->stamp_time), mqd->stamp_seq, 
            abs_timeout, syncfd))
        {
            err = EFAULT;
            NDRX_LOG(log_error, "Failed to request timeout to ndrx_svq_moncmd_tout()");
            userlog("Failed to request timeout to ndrx_svq_moncmd_tout()");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "timeout not needed... mqd=%p", mqd);
    }
    
    /* Also we shall here check the queue? */

    /* Lock here before process... */
    pthread_mutex_lock(&(mqd->rcvlockb4));

    /* We need pre-read lock...? as it is flushing the queue
     * but we assume that it will still process the queue...! */
    
    /* lock queue  */
    pthread_mutex_lock(&(mqd->qlock));

    /* process incoming events... if any queued... 
     * if have any event, interrupt the waiting and return back to caller
     * Also check that event is relevant for us -> i.e timestamps matches...
     */
    if (NULL!=mqd->eventq)
    {
        *ev = mqd->eventq;
        DL_DELETE(mqd->eventq, mqd->eventq);
        pthread_mutex_unlock(&(mqd->qlock));
        
        NDRX_LOG(log_info, "Got event in q %p: %d", mqd, (*ev)->ev);
        EXFAIL_OUT(ret);
    }

    /* Chain the lockings, so that Q lock would wait until both
     *  are locked
     */
    /* set thread id.. */
    mqd->thread = pthread_self();
    
    /* here is no interrupt, as pthread locks are imune to signals */
    pthread_mutex_lock(&(mqd->rcvlock));    
    /* unlock queue  */
    pthread_mutex_unlock(&(mqd->qlock));

    /* So we will do following by tout thread.
     * Lock Q, put msgs inside unlock
     * if rcvlockb4 && rcvlock are locked, then send kill signals
     * to process...
     * send until both are unlocked or stamp is changed.
     */
    if (is_send)
        ret=msgsnd (mqd->qid, ptr, NDRX_SVQ_INLEN(len), msgflg);
    else
        ret=msgrcv (mqd->qid, ptr, NDRX_SVQ_INLEN(len), 0, msgflg);
    
    err=errno;
    pthread_mutex_unlock(&(mqd->rcvlock));
    pthread_mutex_unlock(&(mqd->rcvlockb4));


    pthread_mutex_lock(&(mqd->barrier));
    pthread_mutex_unlock(&(mqd->barrier));

    /* if have message return it first..  */
    if (EXFAIL!=ret)
    {
        pthread_mutex_unlock(&(mqd->qlock));
        
        /* OK, we got a message from queue */
        if (!is_send)
        {
            *maxlen = NDRX_SVQ_OUTLEN(ret);
        }
            
        ret=EXSUCCEED;
        goto out;
    }
    else
    {
        NDRX_LOG(log_error, "MQ op fail qid:%d len:%d msgflg: %d: %s", 
                mqd->qid, len, msgflg, strerror(err));
        
        if (EINTR!=err)
        {
            EXFAIL_OUT(ret);
        }
    }
    
    
    ret=EXSUCCEED;
    
    
    /* lock queue  */
    pthread_mutex_lock(&(mqd->qlock));	
    
    /* if have event queued, return it second */
    if (NULL!=mqd->eventq)
    {
        *ev = mqd->eventq;
        DL_DELETE(mqd->eventq, mqd->eventq);
        pthread_mutex_unlock(&(mqd->qlock));
        
        NDRX_LOG(log_info, "Got event in q %p: %d", mqd, (*ev)->ev);
        
        /* failed to receive, got event! */
        EXFAIL_OUT(ret);
    }

    /* unlock queue  */
    pthread_mutex_unlock(&(mqd->qlock));
    
out:
    
    errno = err;

    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
