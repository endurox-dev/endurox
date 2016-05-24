/* 
** Unix Abstraction Layer
**
** @file sys_unix.h
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
#ifndef SYS_UNIX_H
#define	SYS_UNIX_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
#include <mqueue.h>
#include <ndrstandard.h>

/* gcc only: */
#ifdef EX_OS_LINUX
#include <sys/epoll.h>
#endif


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#ifdef EX_OS_LINUX

#define EX_EPOLL_CTL_ADD        EPOLL_CTL_ADD
#define EX_EPOLL_CTL_DEL        EPOLL_CTL_DEL
    
    
#define EX_EPOLL_FLAGS          (EPOLLIN | EPOLLERR | EPOLLEXCLUSIVE)

#else

#define EX_EPOLL_CTL_ADD        1
#define EX_EPOLL_CTL_DEL        2

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef union ex_epoll_data {
        void    *ptr;
        int      fd;
        uint32_t u32;
        uint64_t u64;
        mqd_t    mqd;
} ex_epoll_data_t;

struct ex_epoll_event {
        uint32_t     events;    /* Epoll events */
        ex_epoll_data_t data;      /* User data variable */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern int ex_epoll_ctl(int epfd, int op, int fd, struct ex_epoll_event *event);
extern int ex_epoll_ctl_mq(mqd_t epfd, int op, int fd, struct ex_epoll_event *event);
extern int ex_epoll_create(int size);
extern int ex_epoll_wait(int epfd, struct ex_epoll_event *events, int maxevents, int timeout);
extern int ex_epoll_errno(void);
extern char * ex_poll_strerror(int err);

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_UNIX_H */

