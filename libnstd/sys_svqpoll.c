/**
 * @brief System V Queue polling
 *   here we will work with assumption that there is only one poller sub-system
 *   per process.
 *
 * @file sys_svqpoll.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>
#include <sys_mqueue.h>
#include <sys_svq.h>
#include <fcntl.h>

#include <atmi.h>
#include <atmi_int.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Hash of queues and their timeouts assigned and stamps...
 */
typedef struct
{
    char svcnm[MAXTIDENT+1];     /**< Service name                       */
    mqd_t mqd;                  /**< Queue handler hashed               */
    int idx;                    /**< either fake service or admin Q     */
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

/**
 * This is used for special cases such as bridges
 */
exprivate int M_accept_any = EXFALSE;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Remove all resources used by polling sub-system or queuing
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_epoll_down(int force)
{
    int ret = EXSUCCEED;
    
    ret=ndrx_svqshm_down(force);
    
out:
    return ret;
}

/**
 * Detach from shared memory resources (used by both epoll & simple queues)
 * @return EXUSCCEED
 */
expublic int ndrx_epoll_shmdetach(void)
{
    /* terminate aux thread */
    ndrx_svq_event_exit(EXTRUE);
    
    return EXSUCCEED;
}

expublic void ndrx_YOPT(char *func, char *file, long line)
{
	static int first = TRUE;
	static struct ndrx_svq_info sv;
	
	if (first)
	{
		first = FALSE;
	}
	else 
	{
		NDRX_DUMP_DIFF(log_error,"YOPT DIFF",&sv,M_mainq,sizeof(struct ndrx_svq_info));
		
	}
	NDRX_LOG(log_error, "%s:%s:%ld M_mainq=%p eventq=%p", func,
		file, line, M_mainq, M_mainq->eventq);
	memcpy(&sv, (struct ndrx_svq_info *)M_mainq, sizeof(struct ndrx_svq_info));
}

/**
 * Get Main queue / basically RQADDR!
 * @return main queue of current service poller
 */
expublic mqd_t ndrx_svq_mainq_get(void)
{
    return M_mainq;
}

/**
 * Return main poller resource id, installed into SHM
 * @return 
 */
expublic int ndrx_epoll_resid_get(void)
{
    return M_mainq->qid;
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
    return ndrx_svqshm_get_qid(resid, send_q, NDRX_MAX_Q_SIZE+1);
}

/**
 * Set main polling queue. The queue is open once the
 * @param qstr full queue string for the main polling interface
 */
expublic void ndrx_epoll_mainq_set(char *qstr)
{
    NDRX_STRCPY_SAFE(M_mainqstr, qstr);
    NDRX_LOG(log_debug, "Setting System V main dispatching queue: [%s]", qstr);
}

/**
 * This test whether we need to open service queue 
 * @param idx advertise service index
 * @return EXTRUE - open q, EXFALSE - do not open Q
 */
expublic int ndrx_epoll_shallopenq(int idx)
{
    if (ATMI_SRV_ADMIN_Q==idx || ATMI_SRV_REPLY_Q==idx)
    {
        return EXTRUE;
    }
        
    return EXFALSE;
}

/**
 * Get service definition from service name
 * @param svcnm service name to lookup
 * @return NULL - not found or ptr to service definition
 */
exprivate ndrx_svq_pollsvc_t * ndrx_epoll_getsvc(char *svcnm)
{
    ndrx_svq_pollsvc_t *ret = NULL;
    ndrx_svq_pollsvc_t *elt = NULL;
    
    EXHASH_FIND_STR(M_svcmap, svcnm, ret);
    
    if (NULL==ret && M_accept_any)
    {
        EXHASH_ITER(hh, M_svcmap, ret, elt)
        {
            if (ret->idx > ATMI_SRV_REPLY_Q)
            {
                NDRX_LOG(log_debug, "Accepting any msg svcnm [%s] mapped to [%s]",
                        svcnm, ret->svcnm);
                goto out;
            }
        }
        ret = NULL;
    }
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to find queue definition for [%s] service",
                svcnm);
    }
    
out:
    return ret;
}

/**
 * Get first non admin queue
 * @return NULL - not found, or queue found
 */
exprivate ndrx_svq_pollsvc_t * ndrx_epoll_getfirstna(void)
{
    ndrx_svq_pollsvc_t *el = NULL, *elt;
    
    EXHASH_ITER(hh, M_svcmap, el, elt)
    {
        if (ATMI_SRV_ADMIN_Q!=el->idx)
        {
            break;
        }
    }
    
out:
    return el;
}

/**
 * Register service queue with poller interface.
 * add logical names for admin and reply services
 * returns the used queue id back.
 * @param svcnm service name
 * @param idx queue index (admin 0 /reply 1/service > 1)
 * @param mq_exists Existing queue defintion (for admin queues)
 * @return logical/real queue descriptor or NULL in case of error
 */
expublic mqd_t ndrx_epoll_service_add(char *svcnm, int idx, mqd_t mq_exits)
{
    int ret = EXSUCCEED;
    mqd_t mq = NULL;
    ndrx_svq_pollsvc_t *el = NULL;
    int err;
    char *adminsvc = NDRX_SVC_ADMIN;
    char *replysvc = NDRX_SVC_REPLY;
    
    if (NULL==(el=NDRX_MALLOC(sizeof(*el))))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
                sizeof(*el), strerror(err));
        userlog("Failed to malloc %d bytes: %s", sizeof(*el),
                strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* allocate pointer of 1 byte */
    /* register the queue in the hash */
    if (ATMI_SRV_ADMIN_Q==idx)
    {
        svcnm = adminsvc;
        mq = mq_exits;
        
        /*
         * At this point we shall create a server command monitoring
         * thread!!!
         */
        NDRX_LOG(log_debug, "About to init admin thread...");
        if (EXSUCCEED!=ndrx_svqadmin_init(mq))
        {
            NDRX_LOG(log_error, "Failed to init admin queue");
            userlog("Failed to init admin queue");
            EXFAIL_OUT(ret);
        }
        
    }
    else if (ATMI_SRV_REPLY_Q==idx)
    {
        svcnm = replysvc;
        mq = mq_exits;
    }
    else if ((mqd_t)EXFAIL!=mq_exits)
    {
        mq = mq_exits;
    }
    else if (NULL==(mq=NDRX_MALLOC(1)))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to malloc 1 byte: %s", strerror(err));
        userlog("Failed to malloc 1 byte: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (0==strncmp(svcnm, NDRX_SVC_BRIDGE, NDRX_SVC_BRIDGE_STATLEN))
    {
        NDRX_LOG(log_info, "Accepting any msg");
        M_accept_any = EXTRUE;
    }
    
    el->mqd = mq;
    el->idx = idx;
    
    NDRX_STRCPY_SAFE(el->svcnm, svcnm);
    
    /* register service name into cache... */
    EXHASH_ADD_STR( M_svcmap, svcnm, el );
    
    NDRX_LOG(log_debug, "[%s] mapped to %p idx %d", el->svcnm, el->mqd, el->idx);
    
out:
    
    if (EXSUCCEED!=ret && NULL!=mq)
    {
        NDRX_FREE((char *)mq);
        mq=NULL;
    }

    if (EXSUCCEED!=ret && NULL!=el)
    {
        NDRX_FREE((char *)el);
    }

    errno=err;
    return mq;
}

/**
 * Ini System V polling thread
 * @return EXSUCCEED/EXFAIL, errno set.
 */
expublic int ndrx_epoll_sys_init(void)
{
    int ret = EXSUCCEED;
    /* boot the Auxiliary thread */
    if (EXSUCCEED!=ndrx_svqshm_init(EXFALSE))
    {
        NDRX_LOG(log_error, "Failed to init System V Aux thread/SHM");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Nothing to un-init for epoll()
 */
expublic void ndrx_epoll_sys_uninit(void)
{
    return;
}

/**
 * Return the compiled poll mode
 * @return 
 */
expublic char * ndrx_epoll_mode(void)
{
    static char *mode = "SystemV";
    
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
expublic int ndrx_epoll_ctl(int epfd, int op, int fd, 
        struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    int err = 0;
    
    /* Add or remove FD from the Aux thread */
    
    switch (op)
    {
        case EX_EPOLL_CTL_ADD:
            if (EXSUCCEED!=(ret = ndrx_svq_moncmd_addfd(M_mainq, fd, event->events)))
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
expublic int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, 
        struct ndrx_epoll_event *event)
{
    int ret = EXSUCCEED;
    ndrx_svq_pollsvc_t *el, *elt;
    
    /* just remove service from dispatching... 
     * for adding it is done by service queue open function
     * ndrx_epoll_service_add()
     */
    NDRX_LOG(log_debug, "Op %d on mqd=%p from poller", op, fd);
    if (EX_EPOLL_CTL_DEL==op)
    {
        EXHASH_ITER(hh, M_svcmap, el, elt)
        {
            if (el->mqd==fd)
            {
                /* if this is admin queue, then terminate admin thread! */
                if (ATMI_SRV_ADMIN_Q==el->idx)
                {
                    if (EXSUCCEED!=ndrx_svqadmin_deinit())
                    {
                        NDRX_LOG(log_error, "Failed to terminate ADMIN thread!");
                        EXFAIL_OUT(ret);
                    }
                }
                
                EXHASH_DEL(M_svcmap, el);
                /* if this this is service Q (it is virtual svc q) */
                if (!ndrx_epoll_shallopenq(el->idx))
                {
                    NDRX_LOG(log_info, "Free up virtual mqd %p", el->mqd);
                    NDRX_FREE((char *)el->mqd);
                }
                NDRX_FREE(el);
            }
        }
    }
    
out:
    return ret;
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return Fake FD(1) or EXFAIL
 */
expublic int ndrx_epoll_create(int size)
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
    
    /* Set RQADDR flag for the queue */    
    if (EXSUCCEED!=ndrx_svqshm_ctl(NULL, M_mainq->qid, IPC_SET, 
            NDRX_SVQ_MAP_RQADDR, NULL))
    {
        NDRX_LOG(log_error, "Failed to mark qid %d as request address type", 
                M_mainq->qid);
        EXFAIL_OUT(ret);
    }
    
    
out:
    return ret;
}

/**
 * Close Epoll set.
 */
expublic int ndrx_epoll_close(int fd)
{
    ndrx_svq_pollsvc_t *el, *elt;
    /* Close main poller Q erase mapping hashes 
    ndrx_mq_close(M_mainq);
     * THIS WILL BE DONE BY atexit()...
     * */
    
    EXHASH_ITER(hh, M_svcmap, el, elt)
    {
        EXHASH_DEL(M_svcmap, el);
        NDRX_FREE(el);
    }
    
    return EXSUCCEED;
}

/**
 * Wrapper for epoll_wait
 * Needs to provide back the identifier that we got msg for admin Q
 * or for admin we could use special service name like @ADMIN so that
 * we can find it in standard hash list? but the MQD needs to be kind of special
 * one so that we do not remove it by our selves at the fake
 * @param epfd not used, poll set
 * @param events events return events struct 
 * @param maxevents max number of events can be loaded
 * @param timeout timeout in milliseconds
 * @param buf preloaded message
 * @param buf_len preloaded buffer size on input max size, on output actual msgs sz
 * @return 0 - timeout, -1 - FAIL, 1 - have one event
 */
expublic int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char *buf, int *buf_len)
{
    int ret = EXSUCCEED;
    ssize_t rcvlen = *buf_len;
    ndrx_svq_ev_t *ev = NULL;
    int err = 0;
    int gottout = EXFALSE;
    ndrx_svq_pollsvc_t *svc;
    tp_command_call_t *call;
    tp_command_generic_t *gen_command;
    struct timespec tm;
    
    /* set thread handler - for interrupts */
    M_mainq->thread = pthread_self();
    
    /* as we do not have non timed receive available for  */
    
    if (-1==timeout)
    {
        /* no timeout required */
        tm.tv_sec = 0;
        tm.tv_nsec = 0;
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += (timeout / 1000);  /* Set timeout, passed in msec, uses as sec */
    }
    
    if (EXFAIL==ndrx_svq_event_sndrcv( M_mainq, buf, &rcvlen, 
            &tm, &ev, EXFALSE, EXTRUE))
    {
        err = errno;
        if (NULL!=ev)
        {
            switch (ev->ev)
            {
                case NDRX_SVQ_EV_TOUT:
                    NDRX_LOG(log_debug, "Timed out");
                    gottout = EXTRUE;
                    break;
                case NDRX_SVQ_EV_DATA:
                    
                    /* Admin thread sends something to us... */
                    NDRX_LOG(log_info, "Admin queue sends us something "
                            "bytes %d", (int)ev->datalen);
                    
                    events[0].is_mqd = EXTRUE;
                    
                    /* Copy off the data - todo think about zero copy...*/
                    if (*buf_len < ev->datalen)
                    {
                        NDRX_LOG(log_error, "Receive from FD %d bytes, but max buffer %d",
                                ev->datalen, *buf_len);
                        userlog("Receive from FD %d bytes, but max buffer %d",
                                ev->datalen, *buf_len);

                        err=EMSGSIZE;
                        EXFAIL_OUT(ret);
                    }

                    *buf_len = ev->datalen;
                    memcpy(buf, ev->data, *buf_len);
                    
                    /* free up the event block? already done at exit... */
                    
                    /* Lookup admin Queue ID */
                    if (NULL==(svc=ndrx_epoll_getsvc(NDRX_SVC_ADMIN)))
                    {
                        err=EFAULT;
                        NDRX_LOG(log_error, "Missing admin [%s] queue def!",
                                NDRX_SVC_ADMIN);
                        EXFAIL_OUT(ret);
                    }
                    
                    events[0].data.mqd = svc->mqd;
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

                        err=EMSGSIZE;
                        EXFAIL_OUT(ret);
                    }

                    if (ev->datalen > 0)
                    {
                        *buf_len = ev->datalen;
                        memcpy(buf, ev->data, *buf_len);
                    }
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
                NDRX_LOG(log_debug, "msgrcv(qid=%d) failed: %s", M_mainq->qid, 
                    strerror(err));
                err = EAGAIN;
            }
            else
            {
                NDRX_LOG(log_error, "msgrcv(qid=%d) failed: %s", M_mainq->qid, 
                    strerror(err));
            }
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        gen_command = (tp_command_generic_t *)buf;
        
        /* we got a message! */
        NDRX_LOG(log_debug, "Got message from main SysV queue %d "
                "bytes gencommand: %hd", rcvlen, gen_command->command_id);
        
        switch (gen_command->command_id)
        {
            case ATMI_COMMAND_CONNECT:
            case ATMI_COMMAND_TPCALL:
            case ATMI_COMMAND_CONNRPLY:
            case ATMI_COMMAND_TPREPLY:
                
                call = (tp_command_call_t *)buf;
                
                NDRX_LOG(log_debug, "Lookup service: [%s]", call->name);
                if (NULL==(svc=ndrx_epoll_getsvc(call->name)))
                {
                    err=EAGAIN;
                    NDRX_DUMP(log_error, "!!! Missing queue def - dumpg", buf, rcvlen);
                    NDRX_LOG(log_error, "!!! Missing queue def for [%s] data "
                            "len %d- dropping "
                            "msg - is all servers on RQADDR serving all services?",
                            call->name, rcvlen);
                    userlog("!!! Missing queue def for [%s] "
                            "data len %d - dropping msg - is "
                            "all servers on RQADDR serving all services?",
                            call->name, rcvlen);
                    EXFAIL_OUT(ret);
                }
                events[0].data.mqd = svc->mqd;
                break;
            default:
                /* any non admin Q will be fine here!*/
                if (NULL==(svc=ndrx_epoll_getfirstna()))
                {
                    err=EFAULT;
                    NDRX_LOG(log_error, "Cannot find any non admin Q for event");
                    userlog("Cannot find any non admin Q for event");
                    EXFAIL_OUT(ret);
                }
                
                events[0].data.mqd = svc->mqd;
                
                break;
        }
        
        events[0].is_mqd = EXTRUE;
        
        NDRX_LOG(log_debug, "event mapped to %p (mqd subst)",
                events[0].data.mqd);
        *buf_len = rcvlen;
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
    
    if (EXSUCCEED==ret)
    {
        if (gottout)
        {
            errno = EAGAIN;
            return 0; /* got timeout */
        }
        else
        {
            return 1;   /* received something useful */
        }
    }
    else
    {
        errno = err;
        return EXFAIL;
    }
}

/**
 * Return errno for ndrx_poll() operation
 * @return 
 */
expublic int ndrx_epoll_errno(void)
{
    return errno;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
expublic char * ndrx_poll_strerror(int err)
{
    return strerror(err);
}

/* vim: set ts=4 sw=4 et smartindent: */
