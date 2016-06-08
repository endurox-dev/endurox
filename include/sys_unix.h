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
#include <config.h>
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
#include <sys_mqueue.h>
#include <ndrstandard.h>

#ifdef EX_USE_EPOLL
#include <sys/epoll.h>
#else
#include <poll.h>
#endif


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#ifdef EX_USE_EPOLL

#define EX_EPOLL_CTL_ADD        EPOLL_CTL_ADD
#define EX_EPOLL_CTL_DEL        EPOLL_CTL_DEL
    
    
#define EX_EPOLL_FLAGS          (EPOLLIN | EPOLLERR | EPOLLEXCLUSIVE)

#else

#define EX_EPOLL_CTL_ADD        1
#define EX_EPOLL_CTL_DEL        2
    
#define EX_EPOLL_FLAGS          POLLIN

#endif
    
/**
 * Configure the program name.
 * On same platforms/gcc versions we have __progname external
 * On other systems we will do lookup by ps function
 */    
#ifdef HAVE_PROGNAME
    
extern const char * __progname;

#define EX_PROGNAME __progname
    
#else

#define EX_PROGNAME ndrx_sys_get_proc_name()

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * (E)poll data
 */
typedef union ex_epoll_data {
        void    *ptr;
        int      fd;
        uint32_t u32;
        uint64_t u64;
        mqd_t    mqd;
} ex_epoll_data_t;

/**
 * (E)poll event
 */
struct ex_epoll_event {
        uint32_t     events;    /* Epoll events */
        ex_epoll_data_t data;      /* User data variable */
        
        /* The structure generally is the same as for linux epoll_wait
         * This bellow is extension for non linux version.
         */
        #ifndef EX_USE_EPOLL
        int         is_mqd;      /* Set to TRUE, if call is for message q */
        #endif
};


/**
 * List of posix queues
 */
typedef struct mq_list string_list_t;
struct mq_list
{
    char *qname;
    string_list_t *next;
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* poll ops */
extern void ndrx_epoll_sys_init(void);
extern void ndrx_epoll_sys_uninit(void);
extern char * ndrx_epoll_mode(void);
extern int ndrx_epoll_ctl(int epfd, int op, int fd, struct ex_epoll_event *event);
extern int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ex_epoll_event *event);
extern int ndrx_epoll_create(int size);
extern int ndrx_epoll_close(int fd);
extern int ndrx_epoll_wait(int epfd, struct ex_epoll_event *events, int maxevents, int timeout);
extern int ndrx_epoll_errno(void);
extern char * ndrx_poll_strerror(int err);
extern char * ndrx_sys_get_proc_name(void);
    
extern int ndrx_sys_is_process_running(pid_t pid, char *proc_name);
extern string_list_t* ndrx_sys_mqueue_list_make(char *qpath, int *return_status);
extern void ndrx_string_list_free(string_list_t* list);
    

extern char *ndrx_sys_get_cur_username(void);
extern string_list_t * ndrx_sys_ps_list(char *filter1, char *filter2, char *filter3, char *filter4);
extern string_list_t* ndrx_sys_folder_list(char *path, int *return_status);
#ifdef	__cplusplus
}
#endif

#endif	/* SYS_UNIX_H */

