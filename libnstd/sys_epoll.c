/* 
** Epoll Abstraction Layer (EAL)
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
#include <sys/epoll.h>


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
    static char *mode = "epoll";
    
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
    return epoll_ctl(epfd, op, fd, (struct epoll_event *) event);
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
    return epoll_ctl(epfd, op, fd, (struct epoll_event *) event);
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
public inline int ndrx_epoll_create(int size)
{
    return epoll_create(size);
}

/**
 * Close Epoll set.
 */
public inline int ndrx_epoll_close(int fd)
{
    return close(fd);
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
    return epoll_wait(epfd, (struct epoll_event *) events, maxevents, timeout);
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

