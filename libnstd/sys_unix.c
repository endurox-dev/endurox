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
typedef struct ex_epoll_fds ex_epoll_fds_t;

/*
 * Our internal 'epoll' set
 */
struct ex_epoll_set
{
    int fd;
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct ex_epoll_set ex_epoll_set_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


/* 
 * - So we need a structure for holding epoll set
 * - We need a hash list for FDs
 * - We need a hash list for mqd_t
 * - Once create is called, we prepare unnamed pipe2() for notify callback.
 */

/*---------------------------Prototypes---------------------------------*/

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
    return FAIL;
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
public int ex_epoll_create(int size)
{
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
    return FAIL;
}

/**
 * Return errno for ex_poll() operation
 * @return 
 */
public int ex_epoll_errno(void)
{
    return FAIL;
}

/**
 * Wrapper for strerror
 * @param err
 * @return 
 */
public char * ex_poll_strerror(int err)
{
    static __thread char ret[] = "Not implemented.";
    
    return ret;
}

