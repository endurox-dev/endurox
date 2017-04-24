/* 
** Kqueue Abstraction Layer (KAL) for BSD
**
** @file sys_kqueue.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
#include <sys/event.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Nothing to init for epoll()
 */
public inline void ndrx_epoll_sys_init(void)
{
    return;
}

/**
 * Nothing to un-init for epoll()
 */
public inline void ndrx_epoll_sys_uninit(void)
{
    return;
}

/**
 * Return the compiled poll mode
 * @return 
 */
public inline char * ndrx_epoll_mode(void)
{
    static char *mode = "kqueue";
    
    return mode;
}
/**
 * Wrapper for epoll_ctl, for standard file descriptors
 * @param epfd
 * @param op
 * @param fd
 * @param event
 * @return 
 */
public inline int ndrx_epoll_ctl(int epfd, int op, int fd, struct ndrx_epoll_event *event)
{   
    int ret = SUCCEED;
    char *fn = "ndrx_epoll_ctl";
    struct kevent ev;
    
    ev.udata = NULL;

    if (EX_EPOLL_CTL_ADD == op)
    {
        NDRX_LOG(log_info, "%s: Add operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);
        
        EV_SET(&ev, fd, EVFILT_READ, event->events, 0, 0, NULL);
        
        return kevent(epfd, &ev, 1, NULL, 0, NULL);
    }
    else if (EX_EPOLL_CTL_DEL == op)
    {
        NDRX_LOG(log_info, "%s: Delete operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);
           
        EV_SET(&ev,	fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        
        return kevent(epfd, &ev, 1, NULL, 0, NULL);
    }
    else
    {
        NDRX_LOG(log_error, "Invalid operation %d", op);
	errno = EINVAL;
        FAIL_OUT(ret);
    }
    
out:

    NDRX_LOG(log_info, "%s return %d", fn, ret);

    return ret;
}

/**
 * epoll_ctl for Posix queue descriptors
 * @param epfd
 * @param op
 * @param fd
 * @param event
 * @return 
 */
public inline int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ndrx_epoll_event *event)
{
    int ret = SUCCEED;
    char *fn = "ndrx_epoll_ctl";
    struct kevent ev;
    void *tmp = fd;
    int real_fd = *((int *)tmp);

    /* for bsd the file descriptor must be dereferenced from pointer */

    if (EX_EPOLL_CTL_ADD == op)
    {
        NDRX_LOG(log_info, "%s: Add operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);

        EV_SET(&ev, real_fd, EVFILT_READ, event->events, 0, 0, NULL);
        ev.udata = fd;

        return kevent(epfd, &ev, 1, NULL, 0, NULL);
    }
    else if (EX_EPOLL_CTL_DEL == op)
    {
        NDRX_LOG(log_info, "%s: Delete operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);

        EV_SET(&ev, real_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);

        return kevent(epfd, &ev, 1, NULL, 0, NULL);
    }
    else
    {
        NDRX_LOG(log_error, "Invalid operation %d", op);
        errno = EINVAL;
        FAIL_OUT(ret);
    }

out:

    NDRX_LOG(log_info, "%s return %d", fn, ret);

    return ret;

}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
public inline int ndrx_epoll_create(int size)
{
    return kqueue();
}

/**
 * Close Epoll set.
 */
public inline int ndrx_epoll_close(int epfd)
{
    return close(epfd);
}

/**
 * Wrapper for epoll_wait
 * @param epfd
 * @param events
 * @param maxevents
 * @param timeout
 * @return 
 */
public inline int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, int maxevents, int timeout)
{
    int ret = SUCCEED;
    int err_saved;
    int numevents = 0;
    char *fn = "ndrx_epoll_wait";
    int i;
    int retpoll;
    struct kevent tevent;	 /* Event triggered */
    struct timespec tout;
    
    if (timeout>0)
    {
    	NDRX_LOG(log_debug, "about to kevent wait %d, timeout %d", epfd, timeout);
        tout.tv_nsec = 0;
        tout.tv_sec = timeout/1000;
        retpoll = kevent(epfd, NULL, 0, &tevent, 1, &tout);
    }
    else
    {
    	NDRX_LOG(log_debug, "about to kevent wait %d, no timeout", epfd);
        retpoll = kevent(epfd, NULL, 0, &tevent, 1, NULL);
    }
    
    err_saved = errno;
    
    
    if (retpoll > 0)
    {
        /* 1 event currently supported... */
        NDRX_LOG(log_debug, "fd=%d, i=%d revents=%u", 
                tevent.ident, tevent.flags);

        numevents++;

        if (tevent.udata!=NULL)
        {
            /* the mqueue hanlder is encoded here (ptr to fd..)*/
            events[numevents-1].data.mqd = (mqd_t)tevent.udata;
            events[numevents-1].is_mqd = TRUE;
        }
        else
        {
           events[numevents-1].data.fd = tevent.ident;
           events[numevents-1].is_mqd = FALSE;
        }
        /* will provide back both flags & flag flags... */
        events[numevents-1].events = tevent.flags;
    }
    else if (retpoll==0)
    {
        goto out;
    }
    else
    {
        FAIL_OUT(ret);
    }
    
out:

    NDRX_LOG(log_info, "%s ret=%d numevents=%d", fn, ret, numevents);

    if (SUCCEED==ret)
    {
        return numevents;
    }
    else
    {
        errno = err_saved;
        return FAIL;
    }
}


/**
 * Return errno for ndrx_poll() operation
 * @return 
 */
public inline int ndrx_epoll_errno(void)
{
    return errno;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
public inline char * ndrx_poll_strerror(int err)
{
    return strerror(err);
}

