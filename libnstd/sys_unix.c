/* 
** Unix Abstraction Layer (UAL)
**
** @file ndebug.c
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

#include <sys/time.h>                   /* purely for dbg_timer()       */
#include <sys/stat.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <mqueue.h>
#include <sys_unix.h>

#include <uthash.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/*
 * Special ops for message queue...
 */
#define HASH_FIND_MQD(head,findptr,out)                                          \
    HASH_FIND(hh,head,findptr,sizeof(mqd_t),out)

#define HASH_ADD_MQD(head,ptrfield,add)                                          \
    HASH_ADD(hh,head,ptrfield,sizeof(mqd_t),add)


#define EX_POLL_SETS_MAX            1024


#define EX_EPOLL_API_ENTRY      {M_last_err = 0; M_last_err_msg[0] = EOS;}

/* The index of the "read" end of the pipe */
#define READ 0

/* The index of the "write" end of the pipe */
#define WRITE 1


#define PIPE_POLL_IDX           0   /* where to pipe sits in poll */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
 * Hash list of File descriptors we monitor..
 */
struct ex_epoll_fds
{
    int fd;
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ex_epoll_fds ex_epoll_fds_t;

/*
 * Hash list of File descriptors we monitor..
 */
struct ex_epoll_mqds
{
    mqd_t md;
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ex_epoll_mqds ex_epoll_mqds_t;

/*
 * Our internal 'epoll' set
 */
struct ex_epoll_set
{
    int fd;
    
    ex_epoll_fds_t *fds; /* hash of file descriptors for monitoring */
    ex_epoll_mqds_t *mqds; /* hash of message queues for monitoring */
    
    int nrfds; /* number of FDs in poll */
    struct pollfd *fdtab; /* poll() structure, pre-allocated*/
    
    int wakeup_pipe[2]; /* wakeup pipe from notification thread */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ex_epoll_set ex_epoll_set_t;


public ex_epoll_set_t *M_psets = NULL; /* poll sets  */

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


/* 
 * - So we need a structure for holding epoll set
 * - We need a hash list for FDs
 * - We need a hash list for mqd_t
 * - Once create is called, we prepare unnamed pipe2() for notify callback.
 */


private int __thread M_last_err = 0;
private char __thread M_last_err_msg[1024];  /* Last error message */
/*---------------------------Prototypes---------------------------------*/


/**
 * We basically will use unix error codes
 */
private void ex_epoll_set_err(int error_code, const char *fmt, ...)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EOS};
    va_list ap;

    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    strcpy(M_last_err_msg, msg);
    M_last_err = error_code;

    NDRX_LOG(log_warn, "ex_epoll_set_err: %d (%s) (%s)",
                    error_code, strerror(M_last_err), M_last_err_msg);
    
}

/**
 * Find FD in hash
 */
private ex_epoll_fds_t* fd_find(ex_epoll_set_t *pset, int fd)
{
    ex_epoll_fds_t*ret = NULL;
    
    HASH_FIND_INT( pset->fds, fd, ret);
    
    return ret;
}

/**
 * FInd q descriptor in hash
 */
private ex_epoll_mqds_t* mqd_find(ex_epoll_set_t *pset, mqd_t mqd)
{
    ex_epoll_mqds_t*ret = NULL;
    
    HASH_FIND_MQD( pset->mqds, mqd, ret);
    
    return ret;
}

/**
 * Find polling set
 */
private ex_epoll_set_t* pset_find(int fd)
{
    ex_epoll_set_t *ret = NULL;
    
    HASH_FIND_INT( M_psets, fd, ret);
    
    return ret;
}

/**
 * Wrapper for epoll_ctl, for standard file descriptors
 * @param epfd
 * @param op
 * @param fd
 * @param event
 * @return 
 */
public int ex_epoll_ctl(int epfd, int op, int fd, struct ex_epoll_event *event)
{
    EX_EPOLL_API_ENTRY;
    
    ex_epoll_set_err(ENOSYS, "Not yet supported.");
    
    return FAIL;
}

/**
 * epoll_ctl for Posix queue descriptors
 * @param epfd
 * @param op
 * @param fd
 * @param event
 * @return 
 */
public int ex_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ex_epoll_event *event)
{
    EX_EPOLL_API_ENTRY;
    
    ex_epoll_set_err(ENOSYS, "Not yet supported.");
    
    return FAIL;
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
public int ex_epoll_create(int size)
{
    int ret = SUCCEED;
    int i = 1;
    ex_epoll_set_t *set;
    
    EX_EPOLL_API_ENTRY;
    
    while (NULL!=(set=pset_find(i)) && i < EX_POLL_SETS_MAX)
    {
        i++;
    }
    
    /* we must have free set */
    if (NULL!=set)
    {
        ex_epoll_set_err(EMFILE, "Max ex_epoll_sets_reached");
        set = NULL;
        FAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Creating ex_epoll set: %d", i);
    
    if (SUCCEED!=(set = (ex_epoll_set_t *)calloc(1, sizeof(*set))))
    {
        ex_epoll_set_err(errno, "Failed to alloc: %d bytes", sizeof(*set));
        FAIL_OUT(ret);
    }
    
    if (pipe2(set->wakeup_pipe, O_NONBLOCK) == -1)
    {
        ex_epoll_set_err(errno, "pipe2 failed");
        FAIL_OUT(ret);
    }
    
    if (NULL==(set->pollfd = calloc(1, sizeof(struct pollfd))))
    {
        ex_epoll_set_err(errno, "calloc for pollfd failed");
        FAIL_OUT(ret);
    }
    
    /* So wait for events here in the pip form Q thread */
    set->fdtab[PIPE_POLL_IDX].fd = set->wakeup_pipe[READ];
    set->fdtab[PIPE_POLL_IDX].events = POLLIN;
    
    
    NDRX_LOG(log_info, "ex_epoll_create succeed, fd=%d", i);
out:

    if (SUCCEED!=ret)
    {
        if (NULL!=set && set->wakeup_pipe[READ])
        {
            close(set->wakeup_pipe[READ]);
        }
        
        if (NULL!=set && set->wakeup_pipe[WRITE])
        {
            close(set->wakeup_pipe[WRITE]);
        }
            
        if (NULL!=set)
        {
            free((char *)set);
        }
        
        return FAIL;
        
    }

    return i;
}

/**
 * Close the epoll set
 */
public int ex_epoll_close(int fd)
{
    
    ex_epoll_set_err(ENOSYS, "Not yet supported.");
    
    return FAIL;
}

/**
 * Wrapper for epoll_wait
 * We do a classical poll here, but before poll, we scan all the queues,
 * so that they are empty before poll() call.
 * if any queue we have something, we return it in evens. Currenly one event per call.
 * @param epfd
 * @param events
 * @param maxevents
 * @param timeout
 * @return 
 */
public int ex_epoll_wait(int epfd, struct ex_epoll_event *events, int maxevents, int timeout)
{
    EX_EPOLL_API_ENTRY;
    
    ex_epoll_set_err(ENOSYS, "Not yet supported.");
    
    return FAIL;
}

/**
 * Return errno for ex_poll() operation
 * @return 
 */
public int ex_epoll_errno(void)
{
    return M_last_err;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
public char * ex_poll_strerror(int err)
{
    static __thread char ret[1024];
    
    sprintf(ret, "%s (last error: %s)", strerror(err), M_last_err;
    
    return ret;
}

