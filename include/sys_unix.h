/**
 * @brief Unix Abstraction Layer
 *
 * @file sys_unix.h
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
#ifndef SYS_UNIX_H
#define	SYS_UNIX_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdint.h>
#include <regex.h>
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

#if defined(EX_USE_SYSVQ) 
/* Feature #281 */
#include <sys_svq.h>
#include <poll.h>
#elif defined(EX_USE_EPOLL)
#include <sys/epoll.h>
#elif defined(EX_USE_KQUEUE)
#include <sys/event.h>
#else
#include <poll.h>
#endif

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if defined(EX_USE_SYSVQ)
    
#define EX_EPOLL_CTL_ADD        1
#define EX_EPOLL_CTL_DEL        2
    
#define EX_EPOLL_FLAGS          POLLIN
    
#elif defined(EX_USE_EPOLL)

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
    
#define EX_KILL_SLEEP_SECS 2
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
 * Memory infos...
 */
typedef struct ndrx_proc_info ndrx_proc_info_t;
struct ndrx_proc_info
{
    long rss;
    long vsz;
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


#if 1==EX_OS_SUNOS

extern NDRX_API ssize_t sol_mq_timedreceive(mqd_t mqdes, char *msg_ptr,
     size_t  msg_len,  unsigned  *msg_prio, struct
     timespec *abs_timeout);
extern NDRX_API int sol_mq_timedsend(mqd_t mqdes, char *msg_ptr, size_t len, 
                              unsigned int msg_prio, struct timespec *abs_timeout);
extern NDRX_API int sol_mq_unlink(char *name);
extern NDRX_API int sol_mq_setattr(mqd_t mqdes,
                       struct mq_attr * newattr,
                       struct mq_attr * oldattr);
extern NDRX_API int sol_mq_send(mqd_t mqdes, char *msg_ptr, size_t msg_len,
                    unsigned int msg_prio);
extern NDRX_API ssize_t sol_mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, 
                                unsigned int *msg_prio);
extern NDRX_API mqd_t sol_mq_open(char *name, int oflag, mode_t mode, struct mq_attr *attr);
extern NDRX_API int sol_mq_notify(mqd_t mqdes, struct sigevent * sevp);
extern NDRX_API int sol_mq_getattr(mqd_t mqdes, struct mq_attr * attr);
extern NDRX_API int sol_mq_close(mqd_t mqdes);


#endif

#if defined(EX_OS_DARWIN) || defined(EX_OS_FREEBSD)
extern NDRX_API FILE * fmemopen(void *buffer, size_t len, const char *mode);
#endif

/* poll ops */
extern NDRX_API void ndrx_epoll_mainq_set(char *qstr);
extern NDRX_API mqd_t ndrx_epoll_service_add(char *svcnm, int idx, mqd_t mq_exits);
extern NDRX_API int ndrx_epoll_service_translate(char *send_q, char *q_prefix, 
        char *svc, int resid);
extern NDRX_API int ndrx_epoll_shallopensvc(int idx);
extern NDRX_API int ndrx_epoll_resid_get(void);

extern NDRX_API int ndrx_epoll_sys_init(void);
extern NDRX_API void ndrx_epoll_sys_uninit(void);
extern NDRX_API char * ndrx_epoll_mode(void);
extern NDRX_API int ndrx_epoll_ctl(int epfd, int op, int fd, struct ndrx_epoll_event *event);
extern NDRX_API int ndrx_epoll_ctl_mq(int epfd, int op, mqd_t fd, struct ndrx_epoll_event *event);
extern NDRX_API int ndrx_epoll_create(int size);
extern NDRX_API int ndrx_epoll_close(int fd);
extern NDRX_API int ndrx_epoll_wait(int epfd, struct ndrx_epoll_event *events, 
        int maxevents, int timeout, char *buf, int *buf_len);
extern NDRX_API int ndrx_epoll_errno(void);
extern NDRX_API char * ndrx_poll_strerror(int err);

/* used by System V, dummies for others pollers/queues: */
extern NDRX_API int ndrx_epoll_resid_get(void);
extern NDRX_API int ndrx_epoll_down(void);
extern NDRX_API int ndrx_epoll_shmdetach(void);
extern NDRX_API int ndrx_epoll_service_translate(char *send_q, char *q_prefix, 
        char *svc, int resid);
extern NDRX_API void ndrx_epoll_mainq_set(char *qstr);
extern NDRX_API int ndrx_epoll_shallopensvc(int idx);
extern NDRX_API mqd_t ndrx_epoll_service_add(char *svcnm, int idx, mqd_t mq_exits);

/* string generics: */
extern NDRX_API void ndrx_string_list_free(string_list_t* list);

extern NDRX_API void ndrx_string_hash_free(string_hash_t *h);
extern NDRX_API int ndrx_string_hash_add(string_hash_t **h, char *str);
extern NDRX_API string_hash_t * ndrx_string_hash_get(string_hash_t *h, char *str);

extern NDRX_API char *ndrx_sys_get_cur_username(void);
extern NDRX_API int ndrx_sys_get_hostname(char *out_hostname, long out_bufsz);
extern NDRX_API string_list_t * ndrx_sys_ps_list(char *filter1, char *filter2, 
        char *filter3, char *filter4, char *regex1);
extern NDRX_API string_list_t * ndrx_sys_ps_getchilds(pid_t ppid);
extern NDRX_API string_list_t* ndrx_sys_folder_list(char *path, int *return_status);

extern NDRX_API int ndrx_proc_pid_get_from_ps(char *psout, pid_t *pid);
extern NDRX_API int ndrx_proc_ppid_get_from_ps(char *psout, pid_t *ppid);

extern NDRX_API void ndrx_proc_kill_list(string_list_t *list);
extern NDRX_API int ndrx_proc_children_get_recursive(string_list_t **list, pid_t pid);
extern NDRX_API int ndrx_proc_get_infos(pid_t pid, ndrx_proc_info_t *p_infos);
extern NDRX_API int ndrx_sys_cmdout_test(char *fmt, pid_t pid, regex_t *p_re);
extern NDRX_API void ndrx_sys_banner(void);

/* gen unix: */
extern NDRX_API char * ndrx_sys_get_proc_name_by_ps(void);
extern NDRX_API int ndrx_sys_is_process_running_by_kill(pid_t pid, char *proc_name);
extern NDRX_API int ndrx_sys_is_process_running_by_ps(pid_t pid, char *proc_name);
extern NDRX_API int ndrx_sys_is_process_running_by_pid(pid_t pid);


/* sys_linux.c: */
extern NDRX_API int ndrx_sys_is_process_running_procfs(pid_t pid, char *proc_name);

/* sys_aix.c: */
extern NDRX_API char * ndrx_sys_get_proc_name_getprocs(void);

/* provided by: sys_<platform>.c */
extern NDRX_API string_list_t* ndrx_sys_mqueue_list_make_pl(char *qpath, int *return_status);
/* emulated message queue: */
extern NDRX_API string_list_t* ndrx_sys_mqueue_list_make_emq(char *qpath, int *return_status);



/** Test environmental variables of the PID 
 Sample regex: NDRX_SVCLOPT.{0,2}-k 0myWI5nu.*-i [0-9]+ --
 */
extern NDRX_API int ndrx_sys_env_test(pid_t pid, regex_t *p_re);

#ifdef EX_USE_SYSVQ

#define ndrx_sys_mqueue_list_make ndrx_sys_mqueue_list_make_svq

#elif EX_USE_EMQ

#define ndrx_sys_mqueue_list_make ndrx_sys_mqueue_list_make_emq

#else

#define ndrx_sys_mqueue_list_make ndrx_sys_mqueue_list_make_pl

#endif


#ifdef	__cplusplus
}
#endif

#endif	/* SYS_UNIX_H */

/* vim: set ts=4 sw=4 et smartindent: */
