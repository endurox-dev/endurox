/* 
** File Descriptor based Poll Abstraction Layer (FAL)
**
** @file sys_epoll.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define EX_POLL_SETS_MAX            1024

#define ERROR_BUFFER            1024

#define EX_EPOLL_API_ENTRY      {NSTD_TLS_ENTRY; \
            G_nstd_tls->M_last_err = 0; \
            G_nstd_tls->M_last_err_msg[0] = EOS;}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
 * Our internal 'epoll' set
 */
struct ndrx_epoll_set
{
    int fd;
    
    struct pollfd *fdtab;   /* set off poll FDs */
    int nrfds; /* number of FDs in poll */
    
    EX_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ndrx_epoll_set ndrx_epoll_set_t;

/*---------------------------Globals------------------------------------*/

private ndrx_epoll_set_t *M_psets = NULL; /* poll sets  */
MUTEX_LOCKDECL(M_psets_lock);

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Find polling set (must be locked externally...)
 * @param epfd poll set to lookup
 * @return Poll set struct
 */
private ndrx_epoll_set_t* pset_find(int epfd)
{
    ndrx_epoll_set_t *ret = NULL;
    
    EXHASH_FIND_INT( M_psets, &epfd, ret);
    
    return ret;
}

/**
 * Find FD in hash
 * @param pset  POLL Set
 * @param fd    File Descriptor to find
 * @return FD or FAIL
 */
private int fd_find(ndrx_epoll_set_t *pset, int fd)
{
    int i;
    
    for (i=0; i<pset->nrfds; i++)
    {
        if (fd==pset->fdtab[i].fd)
        {
            return i;
        }
    }
    
    return FAIL;
}


/**
 * We basically will use unix error codes
 */
private void ndrx_epoll_set_err(int error_code, const char *fmt, ...)
{
    char msg[ERROR_BUFFER+1] = {EOS};
    va_list ap;
    
    NSTD_TLS_ENTRY;

    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    strcpy(G_nstd_tls->M_last_err_msg, msg);
    G_nstd_tls->M_last_err = error_code;

    NDRX_LOG(log_warn, "ndrx_epoll_set_err: %d (%s) (%s)",
                    error_code, strerror(G_nstd_tls->M_last_err), 
                    G_nstd_tls->M_last_err_msg);
    
}

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
    static char *mode = "fdpoll";
    
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
    ndrx_epoll_set_t* set = NULL;
    char *fn = "ndrx_epoll_ctl";
    
    EX_EPOLL_API_ENTRY;
    
    MUTEX_LOCK_V(M_psets_lock);
    
    if (NULL==(set = pset_find(epfd)))
    {
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        ndrx_epoll_set_err(ENOSYS, "ndrx_epoll set %d not found", epfd);
        FAIL_OUT(ret);
    }
    
    if (EX_EPOLL_CTL_ADD == op)
    {
        NDRX_LOG(log_info, "%s: Add operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);
        
        /* test & add to FD hash */
        if (FAIL!=fd_find(set, fd))
        {

            ndrx_epoll_set_err(EINVAL, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            NDRX_LOG(log_error, "fd %d already exists in ndrx_epoll set (epfd %d)", 
                 fd, set->fd);
            FAIL_OUT(ret);
        }
        
        /* resize/realloc events list, add fd */
        set->nrfds++;
        
        NDRX_LOG(log_info, "set nrfds incremented to %d", set->nrfds);
        
        if (NULL==(set->fdtab=NDRX_REALLOC(set->fdtab, sizeof(struct pollfd)*set->nrfds)))
        {
            ndrx_epoll_set_err(errno, "Failed to realloc %d/%d", 
                    set->nrfds, sizeof(struct pollfd)*set->nrfds);
            NDRX_LOG(log_error, "Failed to realloc %d/%d", 
                    set->nrfds, sizeof(struct pollfd)*set->nrfds);
            FAIL_OUT(ret);
        }
        
        set->fdtab[set->nrfds-1].fd = fd;
        set->fdtab[set->nrfds-1].events = event->events;
        
    }
    else if (EX_EPOLL_CTL_DEL == op)
    {
        int found = FALSE;
        int i;
        NDRX_LOG(log_info, "%s: Delete operation on ndrx_epoll set %d, fd %d", fn, epfd, fd);
        
        /* test & add to FD hash */
        if (FAIL==fd_find(set, fd))
        {
            ndrx_epoll_set_err(EINVAL, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            NDRX_LOG(log_error, "fd %d not found in ndrx_epoll set (epfd %d)", 
                    fd, set->fd);
            FAIL_OUT(ret);
        }
        
        /* Remove fd from set->fdtab & from hash */
        
        for (i = 0; i < set->nrfds; i++)
        {
            if (set->fdtab[i].fd == fd)
            {
                /* kill the element */
                if (i!=set->nrfds-1 && set->nrfds>1)
                {
                    /* It is not last element, thus we can move something... */
                    memmove(&set->fdtab[i], &set->fdtab[i+1], 
                            sizeof(struct pollfd)*(set->nrfds-i-1));
                }
                
                set->nrfds--;
                
                NDRX_LOG(log_info, "set nrfds decremented to %d", set->nrfds);
                
                if (0==set->nrfds)
                {
                    NDRX_LOG(log_warn, "set->nrfds == 0, => free");
                    NDRX_FREE((char *)set->fdtab);
                }
                else if (NULL==(set->fdtab=NDRX_REALLOC(set->fdtab, 
                        sizeof(struct pollfd)*set->nrfds)))
                {
                    ndrx_epoll_set_err(errno, "Failed to realloc %d/%d", 
                            set->nrfds, sizeof(struct pollfd)*set->nrfds);
                    
                    NDRX_LOG(log_error, "Failed to realloc %d/%d", 
                            set->nrfds, sizeof(struct pollfd)*set->nrfds);
                    
                    FAIL_OUT(ret);
                }
                
                found = TRUE;
                
                break;
            }
        }
        
        if (!found)
        {
            NDRX_LOG(log_warn, "File descriptor %d was requested for removal, "
                    "but not found epoll set %d!", fd, epfd);
        }
    }
    else
    {
        ndrx_epoll_set_err(EINVAL, "Invalid operation %d", op);
        NDRX_LOG(log_error, "Invalid operation %d", op);
        
        FAIL_OUT(ret);
    }

out:

    MUTEX_UNLOCK_V(M_psets_lock);

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
    return ndrx_epoll_ctl(epfd, op, (int)fd, event);
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
public inline int ndrx_epoll_create(int size)
{
    int ret = SUCCEED;
    int i = 1;
    ndrx_epoll_set_t *set;
    
    EX_EPOLL_API_ENTRY;
    
    while (NULL!=(set=pset_find(i)) && i < EX_POLL_SETS_MAX)
    {
        i++;
    }
    
    /* we must have free set */
    if (NULL!=set)
    {
        ndrx_epoll_set_err(EMFILE, "Max ndrx_epoll_sets_reached");
        NDRX_LOG(log_error, "Max ndrx_epoll_sets_reached");
                
        
        set = NULL;
        FAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Creating ndrx_epoll set: %d", i);
    
    if (NULL==(set = (ndrx_epoll_set_t *)NDRX_CALLOC(1, sizeof(*set))))
    {
        ndrx_epoll_set_err(errno, "Failed to alloc: %d bytes", sizeof(*set));
        
        NDRX_LOG(log_error, "Failed to alloc: %d bytes", sizeof(*set));

        FAIL_OUT(ret);
    }
    
    set->nrfds = 0;
    set->fd = i; /* assign the FD num */
    
    /* add finally to hash */
    MUTEX_LOCK_V(M_psets_lock);
    EXHASH_ADD_INT(M_psets, fd, set);
    MUTEX_UNLOCK_V(M_psets_lock);
    
    NDRX_LOG(log_info, "ndrx_epoll_create succeed, fd=%d", i);
    
out:

    if (SUCCEED!=ret)
    {
            
        if (NULL!=set)
        {
            NDRX_FREE((char *)set);
        }
        
        return FAIL;        
    }

    return i;
}

/**
 * Close Epoll set.
 */
public inline int ndrx_epoll_close(int epfd)
{
    int ret = SUCCEED;
    ndrx_epoll_set_t* set = NULL;
    
    NDRX_LOG(log_debug, "ndrx_epoll_close(%d) enter", epfd);
            
    MUTEX_LOCK_V(M_psets_lock);
    
    NDRX_LOG(log_debug, "ndrx_epoll_close(%d) enter (after lock", epfd);
    
    if (NULL==(set = pset_find(epfd)))
    {
        MUTEX_UNLOCK_V(M_psets_lock); /*  <<< release the lock */
        
        ndrx_epoll_set_err(EINVAL, "ndrx_epoll_close set %d not found", epfd);
        NDRX_LOG(log_error, "ndrx_epoll_close set %d not found", epfd);
        
        
        FAIL_OUT(ret);
    }
    MUTEX_UNLOCK_V(M_psets_lock);
    
    while (set->nrfds > 0)
    {
        ndrx_epoll_ctl(set->fd, EX_EPOLL_CTL_DEL, set->fdtab[0].fd, NULL);
    }
    
    MUTEX_LOCK_V(M_psets_lock);
    EXHASH_DEL(M_psets, set);
    NDRX_FREE(set);
    MUTEX_UNLOCK_V(M_psets_lock);
    
out:
    return FAIL;
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
    int numevents = 0;
    ndrx_epoll_set_t* set = NULL;
    char *fn = "ndrx_epoll_wait";
    int i;
    int retpoll;
    
    EX_EPOLL_API_ENTRY;
    
    /*  !!!! LOCKED AREA !!!! */
    MUTEX_LOCK_V(M_psets_lock);
    
    if (NULL==(set = pset_find(epfd)))
    {
        MUTEX_UNLOCK_V(M_psets_lock); /*  <<< release the lock */
        
        ndrx_epoll_set_err(EINVAL, "ndrx_epoll set %d not found", epfd);
        
        NDRX_LOG(log_error, "ndrx_epoll set %d not found", epfd);
        
        FAIL_OUT(ret);
    }
    
    MUTEX_UNLOCK_V(M_psets_lock); /*  <<< release the lock */
    
    /*  !!!! LOCKED AREA, end !!!! */
    
    /* Reset received events... */
   
    
    for (i=0; i<set->nrfds; i++)
    {
        set->fdtab[i].revents = 0;
        NDRX_LOG(log_debug, "poll i=%d, fd=%d, events=%d", 
                i, set->fdtab[i].fd, set->fdtab[i].events);
    }

    NDRX_LOG(log_debug, "%s: epfd=%d, events=%p, maxevents=%d, "
                    "timeout=%d - about to poll(nrfds=%d)",
                    fn, epfd, events, maxevents, timeout, set->nrfds);
        
    retpoll = poll( set->fdtab, set->nrfds, timeout);
    
    NDRX_LOG(log_debug, "retpoll=%d", retpoll);
    
    for (i=0; i<set->nrfds; i++)
    {
        NDRX_LOG(log_debug, "fd=%d, i=%d revents=%d", 
                set->fdtab[i].fd, i, set->fdtab[i].revents);
        if (set->fdtab[i].revents && maxevents > numevents)
        {
            NDRX_LOG(log_debug, "Returning...");

            numevents++;
            
            events[numevents-1].data.fd = set->fdtab[i].fd;
            events[numevents-1].events = set->fdtab[i].revents;
            events[numevents-1].is_mqd = FAIL;
	}
    }
    
out:

    NDRX_LOG(log_info, "%s ret=%d numevents=%d", fn, ret, numevents);

    if (SUCCEED==ret)
    {
        return numevents;
    }
    else
    {
        return FAIL;
    }
}

/**
 * Return errno for ndrx_poll() operation
 * @return 
 */
public int ndrx_epoll_errno(void)
{
    NSTD_TLS_ENTRY;
    return G_nstd_tls->M_last_err;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
public char * ndrx_poll_strerror(int err)
{
    NSTD_TLS_ENTRY;
    
    sprintf(G_nstd_tls->poll_strerr, "%s (last error: %s)", 
    strerror(err), G_nstd_tls->M_last_err_msg);
    
    return G_nstd_tls->poll_strerr;
}

