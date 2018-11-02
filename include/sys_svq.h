/**
 * @brief Enduro/X System V message queue support
 *   Also in case of system exit, we will use "atexit(3)" call to register
 *   termination handlers.
 *
 * @file sys_svq.h
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

#ifndef SYS_SYSVQ_H__
#define SYS_SYSVQ_H__

/*------------------------------Includes--------------------------------------*/
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>
#include <time.h>
#include <sys_primitives.h>
#include <atmi.h>
#include <nstopwatch.h>
#include <nstd_shm.h>

/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

#define NDRX_SVQ_EV_NONE    0   /**< No event received, just normal msg */
#define NDRX_SVQ_EV_TOUT    1   /**< Timeout event                      */
#define NDRX_SVQ_EV_DATA    2   /**< Main thread got some data          */
#define NDRX_SVQ_EV_FD      3   /**< File descriptor got something      */

#define NDRX_SVQ_SIG SIGUSR2    /**< Signal used for timeout wakeups    */

#define NDRX_SVQ_MAP_ISUSED       0x00000001  /**< Queue is used              */
#define NDRX_SVQ_MAP_WASUSED      0x00000002  /**< Queue was used (or is used)*/
#define NDRX_SVQ_MAP_EXPIRED      0x00000004  /**< Queue expired by ctime     */
#define NDRX_SVQ_MAP_SCHEDRM      0x00000008  /**< Schedule queue removal     */
#define NDRX_SVQ_MAP_RQADDR       0x00000010  /**< This is request address q  */
#define NDRX_SVQ_MAP_HAVESVC      0x00000020  /**< Have service in shm        */


#define NDRX_SVQ_MONF_SYNCFD      0x00000001  /**< Perform monitoring on FDs  */

/** For quick access to  */
#define NDRX_SVQ_INDEX(MEM, IDX) ((ndrx_svq_map_t*)(((char*)MEM)+(int)(sizeof(ndrx_svq_map_t)*IDX)))
#define NDRX_SVQ_STATIDX(MEM, IDX) ((ndrx_svq_status_t*)(((char*)MEM)+(int)(sizeof(ndrx_svq_status_t)*IDX)))

/** Match timeout event */
#define NDRX_SVQ_TOUT_MATCH(X, Y) (X->stamp_seq == Y->stamp_seq && \
                        0==memcmp( &(X->stamp_time), &(Y->stamp_time), \
                        sizeof(X->stamp_time)))

#define NDRX_SVQ_MON_TOUT         1 /**< Request for timeout                  */
#define NDRX_SVQ_MON_ADDFD        2 /**< Add file descriptor for ev monitoring*/
#define NDRX_SVQ_MON_RMFD         3 /**< Remove file descriptor for ev mon    */
#define NDRX_SVQ_MON_TERM         4 /**< Termination handler calls us         */
#define NDRX_SVQ_MON_CLOSE        5 /**< Queue unlink request                 */

#define NDRX_SVQ_INLEN(X)       (X-sizeof(long))    /**< System V input len   */
#define NDRX_SVQ_OUTLEN(X)       (X+sizeof(long))   /**< System V output len  */

/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

/**
 * Shared memory entry for service
 */
typedef struct ndrx_svq_map ndrx_svq_map_t;
struct ndrx_svq_map
{
    char qstr[NDRX_MAX_Q_SIZE+1];       /**< Posix queue name string    */
    int qid;                            /**< System V Queue id          */
    short flags;                        /**< See NDRX_SVQ_MAP_STAT_*    */
    /* put stopwatch for last create time */
    ndrx_stopwatch_t ctime;             /**< change time                */
};

/**
 * List of expired queues (it doesn't mean they will be removed,
 * that is later confirmed by ndrxd when scanning services)
 */
typedef struct
{
    int qid;                            /**< System V Queue id          */
    short flags;                        /**< See NDRX_SVQ_MAP_STAT_*    */
    char qstr[NDRX_MAX_Q_SIZE+1];       /**< Posix queue name string    */
    ndrx_stopwatch_t ctime;             /**< change time                */
} ndrx_svq_status_t;

/**
 * Message queue attributes
 */
struct mq_attr 
{
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
};

typedef struct ndrx_svq_ev ndrx_svq_ev_t;
/**
 * Event queue, either timeout, data or waken up by poller
 */
struct ndrx_svq_ev
{
    int ev;                 /**< Event code received                        */
    
    ndrx_stopwatch_t stamp_time; /**< timestamp for timeout waiting         */
    unsigned long stamp_seq;/**< stamp sequence                             */
    
    int fd;                 /**< Linked file descriptor generating FD event */
    uint32_t revents;       /**< events occurred                            */
    
    size_t datalen;         /**< data event len, by admin thread            */
    char *data;             /**< event data                                 */
    
    ndrx_svq_ev_t *next, *prev;/**< Linked list of event enqueued           */
};

/**
 * Queue entry
 */
struct ndrx_svq_info
{
    char qstr[NDRX_MAX_Q_SIZE+1];/**< Posix queue name string               */
    int qid;                    /**< System V Queue ID                      */
    /* Locks for synchronous or other event wakeup */
    
    /* Using spinlocks for better performance  */
    pthread_spinlock_t rcvlock;    /**< Data receive lock, msgrcv              */
    pthread_spinlock_t rcvlockb4;  /**< Data receive lock, before going msgrcv */
    ndrx_svq_ev_t *eventq;      /**< Events queued for this ipc q              */
    pthread_mutex_t barrier;     /**< Border lock after msgrcv woken up        */
    pthread_mutex_t qlock;      /**< Queue lock (event queue)                  */

    /* Timeout matching.
     * All the timeout events are enqueued to thread and thread is waken up
     * if needed. If not then event will be discarded because of stamps
     * does not match.
     */
    pthread_spinlock_t stamplock;/**< Stamp change lock                     */
    ndrx_stopwatch_t stamp_time;/**< timestamp for timeout waiting          */
    unsigned long stamp_seq;    /**< stamp sequence                         */
    
    mode_t mode;                /**< permissions on new queue               */
    struct mq_attr attr;        /**< attributes for the queue, if creating  */
    
    /**
     * thread operating with queue... 
     * Also note that one thread might operate with multiple queues.
     * but only one queue will be blocked at the same time.
     * WARNING !!! This needs to be set every time we enter in wait mode...
     * cannot be set initially because threads might be switched
     * in high level, Object API modes.
     */
    pthread_t thread;
    
    char *self;                /**< ptr to self for hash                    */
    EX_hash_handle hh;         /**< delete hash                             */

};
typedef struct ndrx_svq_info * mqd_t;


/**
 * Command block for monitoring thread
 */
typedef struct
{
    int cmd;                    /**< See NDRX_SVQ_MON_* commands            */
    struct timespec abs_timeout;/**< timeout value when the wait shell tout */
    int flags;                  /**< See NDRX_SVQ_MONF*                     */
    
    /* Data for timeout request: */
    ndrx_stopwatch_t stamp_time;/**< timestamp for timeout waiting          */
    unsigned long stamp_seq;    /**< stamp sequence                         */
    
    int fd;                     /** file descriptor for related cmds        */
    uint32_t events;            /** events requested for fd monitor         */
    mqd_t mqd;                  /** message queue requesting an event       */
    
    /* in case of mqd delete, we shall sync off with the back thread */
    pthread_mutex_t *del_lock;   /** delete lock                             */
    pthread_cond_t *del_cond;     /** conditional variable for delete         */
    
} ndrx_svq_mon_cmd_t;

/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

extern NDRX_API int ndrx_svq_close(mqd_t mqd);
extern NDRX_API int ndrx_svq_getattr(mqd_t, struct mq_attr * attr);
extern NDRX_API int ndrx_svq_notify(mqd_t, const struct sigevent *);
extern NDRX_API mqd_t ndrx_svq_open(const char *pathname, int oflag, mode_t mode, 
                struct mq_attr *attr);
extern NDRX_API ssize_t ndrx_svq_receive(mqd_t, char *, size_t, unsigned int *);
extern NDRX_API int ndrx_svq_send(mqd_t, const char *, size_t, unsigned int);
extern NDRX_API int ndrx_svq_setattr(mqd_t, const struct mq_attr *attr, struct mq_attr *oattr);
extern NDRX_API int ndrx_svq_unlink(const char *name);

extern NDRX_API  int ndrx_svq_timedsend(mqd_t mqd, const char *ptr, size_t len, unsigned int prio,
        const struct timespec *__abs_timeout); 

extern NDRX_API ssize_t ndrx_svq_timedreceive(mqd_t mqd, char *ptr, size_t maxlen, unsigned int *priop,
        const struct timespec * __abs_timeout);

extern NDRX_API void ndrx_svq_set_lock_timeout(int secs);
extern NDRX_API int ndrx_svq_mqd_put_event(mqd_t mqd, ndrx_svq_ev_t *ev);
extern NDRX_API void ndrx_svq_delref_add(mqd_t qd);
extern NDRX_API int ndrx_svq_event_sndrcv(mqd_t mqd, char *ptr, size_t *maxlen, 
        struct timespec *abs_timeout, ndrx_svq_ev_t **ev, int is_send, int syncfd);
extern NDRX_API void ndrx_svq_event_exit(int detatch);

extern NDRX_API int ndrx_svq_moncmd_term(void);
extern NDRX_API int ndrx_svq_moncmd_close(mqd_t mqd);
extern NDRX_API int ndrx_svq_moncmd_addfd(mqd_t mqd, int fdm, uint32_t events);
extern NDRX_API int ndrx_svq_moncmd_rmfd(int fd);
extern NDRX_API mqd_t ndrx_svq_mainq_get(void);

extern NDRX_API int ndrx_svq_event_init(void);

/* internals... */
extern NDRX_API int ndrx_svqshm_init(int attach_only);
extern NDRX_API int ndrx_svqshm_attach(void);
extern NDRX_API int ndrx_svqshm_down(int force);
extern NDRX_API void ndrx_svqshm_detach(void);
extern NDRX_API int ndrx_svqshm_shmres_get(ndrx_shm_t **map_p2s, ndrx_shm_t **map_s2p, 
        ndrx_sem_t **map_sem, int *queuesmax);
extern NDRX_API int ndrx_svqshm_get(char *qstr, mode_t mode, int oflag);
extern NDRX_API int ndrx_svqshm_get_qid(int in_qid, char *out_qstr, int out_qstr_len);
extern NDRX_API int ndrx_svqshm_ctl(char *qstr, int qid, int cmd, int arg1,
        int (*p_deletecb)(int qid, char *qstr));

extern NDRX_API ndrx_svq_status_t* ndrx_svqshm_statusget(int *len, int ttl);

extern NDRX_API string_list_t* ndrx_sys_mqueue_list_make_svq(char *qpath, int *return_status);

extern NDRX_API int ndrx_svqshm_get_status(ndrx_svq_status_t *status, 
        int qid, int *pos, int *have_value);

extern NDRX_API int ndrx_svqadmin_init(mqd_t adminq);
extern NDRX_API int ndrx_svqadmin_deinit(void);

#endif

/* vim: set ts=4 sw=4 et smartindent: */
