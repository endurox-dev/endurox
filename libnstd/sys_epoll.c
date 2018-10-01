/**
 * @brief Epoll Abstraction Layer (EAL)
 *
 * @file sys_epoll.c
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
expublic inline void ndrx_epoll_sys_init(void)
{
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
expublic inline int ndrx_epoll_ctl(int epfd, int op, int fd, struct ndrx_epoll_event *event)
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
expublic inline int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ndrx_epoll_event *event)
{
    return epoll_ctl(epfd, op, fd, (struct epoll_event *) event);
}

/**
 * Wrapper for epoll_create
 * @param size
 * @return 
 */
expublic inline int ndrx_epoll_create(int size)
{
    return epoll_create(size);
}

/**
 * Close Epoll set.
 */
expublic inline int ndrx_epoll_close(int fd)
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
expublic inline int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char *buf, int *buf_len)
{
    *buf_len = EXFAIL;
    return epoll_wait(epfd, (struct epoll_event *) events, maxevents, timeout);
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
