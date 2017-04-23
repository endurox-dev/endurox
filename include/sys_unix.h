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
#include <ndrx_config.h>
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
#include <sys_mqueue.h>
#include <exhash.h>
    
#ifdef EX_OS_DARWIN
#include <sys/types.h>
#include <sys/_types/_timespec.h>
#include <mach/mach.h>
#include <mach/clock.h>
#endif

#if defined(EX_USE_EPOLL)
#include <sys/epoll.h>
#elif defined(EX_USE_KQUEUE)
#include <sys/event.h>
#else
#include <poll.h>
#endif


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if defined(EX_USE_EPOLL)

#define EX_EPOLL_CTL_ADD        EPOLL_CTL_ADD
#define EX_EPOLL_CTL_DEL        EPOLL_CTL_DEL
    
#define EX_EPOLL_FLAGS          (EPOLLIN | EPOLLERR | EPOLLEXCLUSIVE)

#elif defined (EX_USE_KQUEUE)

#define EX_EPOLL_CTL_ADD        1
#define EX_EPOLL_CTL_DEL        2
    
/* #define EX_EPOLL_FLAGS          EV_ADD | EV_ONESHOT | EV_ENABLE */
#define EX_EPOLL_FLAGS          EV_ADD
    
#else

#define EX_EPOLL_CTL_ADD        1
#define EX_EPOLL_CTL_DEL        2
    
#define EX_EPOLL_FLAGS          POLLIN

#endif

#ifdef EX_OS_DARWIN

/* The opengroup spec isn't clear on the mapping from REALTIME to CALENDAR
 being appropriate or not.
 http://pubs.opengroup.org/onlinepubs/009695299/basedefs/time.h.html */

/* XXX only supports a single timer */
#define TIMER_ABSTIME -1
#define CLOCK_REALTIME CALENDAR_CLOCK
#define CLOCK_MONOTONIC SYSTEM_CLOCK

#endif
/******************************************************************************/
/***************************** PROGNAME ***************************************/
/******************************************************************************/
/**
 * Configure the program name.
 * On same platforms/gcc versions we have __progname external
 * On other systems we will do lookup by ps function
 */    
#ifdef HAVE_PROGNAME
    
extern NDRX_API const char * __progname;

#define EX_PROGNAME __progname


#elif EX_OS_AIX

#define EX_PROGNAME ndrx_sys_get_proc_name_getprocs()

#else

/* the worst option: to use ps, it generates sigchild...
 * and for libraries that is not good.
 */
#define EX_PROGNAME ndrx_sys_get_proc_name_by_ps()

#endif

/******************************************************************************/
/***************************** PROCESS TESTING ********************************/
/******************************************************************************/
/**
 * The best option is linux procfs, we generally fallback to kill -0
 * but better would be to use OS options for providing pid + procname testing 
 * for existence. Because OS might reuse the PID.
 */
#ifdef EX_OS_LINUX

#define ndrx_sys_is_process_running ndrx_sys_is_process_running_procfs

#elif EX_OS_CYGWIN

/* Same as for linux */
#define ndrx_sys_is_process_running ndrx_sys_is_process_running_procfs

#else

#define ndrx_sys_is_process_running ndrx_sys_is_process_running_by_kill

#endif
 
/******************************************************************************/

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * (E)poll data
 */
typedef union ndrx_epoll_data {
        void    *ptr;
        int      fd;
        uint32_t u32;
        uint64_t u64;
        mqd_t    mqd;
} ndrx_epoll_data_t;

/**
 * (E)poll event
 */
struct ndrx_epoll_event {
        uint32_t     events;    /* Epoll events */
        ndrx_epoll_data_t data;      /* User data variable */
        
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
typedef struct string_list string_list_t;
struct string_list
{
    char *qname;
    string_list_t *next;
};


typedef struct string_hash string_hash_t;
struct string_hash
{
    char *str;
    EX_hash_handle hh;
};


#ifdef EX_OS_DARWIN
typedef int clockid_t;
#endif
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef EX_OS_DARWIN
extern int clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif

#if defined(EX_OS_DARWIN) || defined(EX_OS_FREEBSD)
extern NDRX_API FILE * fmemopen(void *buffer, size_t len, const char *mode);
#endif

/* poll ops */
extern NDRX_API void ndrx_epoll_sys_init(void);
extern NDRX_API void ndrx_epoll_sys_uninit(void);
extern NDRX_API char * ndrx_epoll_mode(void);
extern NDRX_API int ndrx_epoll_ctl(int epfd, int op, int fd, struct ndrx_epoll_event *event);
extern NDRX_API int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ndrx_epoll_event *event);
extern NDRX_API int ndrx_epoll_create(int size);
extern NDRX_API int ndrx_epoll_close(int fd);
extern NDRX_API int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, int maxevents, int timeout);
extern NDRX_API int ndrx_epoll_errno(void);
extern NDRX_API char * ndrx_poll_strerror(int err);

/* string generics: */
extern NDRX_API void ndrx_string_list_free(string_list_t* list);
extern NDRX_API int ndrx_sys_string_list_add(string_list_t**list, char *string);

extern NDRX_API void ndrx_string_hash_free(string_hash_t *h);
extern NDRX_API int ndrx_string_hash_add(string_hash_t **h, char *str);
extern NDRX_API string_hash_t * ndrx_string_hash_get(string_hash_t *h, char *str);

extern NDRX_API char *ndrx_sys_get_cur_username(void);
extern NDRX_API string_list_t * ndrx_sys_ps_list(char *filter1, char *filter2, char *filter3, char *filter4);
extern NDRX_API string_list_t* ndrx_sys_folder_list(char *path, int *return_status);

/* gen unix: */
extern NDRX_API char * ndrx_sys_get_proc_name_by_ps(void);
extern NDRX_API int ndrx_sys_is_process_running_by_kill(pid_t pid, char *proc_name);
extern NDRX_API int ndrx_sys_is_process_running_by_ps(pid_t pid, char *proc_name);

/* sys_linux.c: */
extern NDRX_API int ndrx_sys_is_process_running_procfs(pid_t pid, char *proc_name);

/* sys_aix.c: */
extern NDRX_API char * ndrx_sys_get_proc_name_getprocs(void);

/* provided by: sys_<platform>.c */
extern NDRX_API string_list_t* ndrx_sys_mqueue_list_make(char *qpath, int *return_status);

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_UNIX_H */

