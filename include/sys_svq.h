/**
 * @brief Enduro/X System V message queue support
 *
 * @file sys_svsvq.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <atmi.h>
#include <nstopwatch.h>

/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

#define NDRX_SVQ_EV_TOUT    1   /**< Timeout event                      */
#define NDRX_SVQ_EV_DATA    2   /**< Main thread got some data          */
#define NDRX_SVQ_EV_FD      3   /**< File descriptor got something      */

#define SIG SIGUSR2             /**< Signal used for timeout wakeups    */

#define NDRX_SVQ_MAP_ISUSED       0x00000001  /**< Queue is used              */
#define NDRX_SVQ_MAP_WASUSED      0x00000002  /**< Queue was used (or is used)*/

/** For quick access to  */
#define NDRX_SVQ_INDEX(MEM, IDX) ((ndrx_svq_map_t*)(((char*)MEM)+(int)(sizeof(ndrx_svq_map_t)*IDX)))

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
 * Message queue attributes
 */
struct mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
};

/**
 * Queue entry
 */
struct ndrx_svq_info
{
    char qstr[NDRX_MAX_Q_SIZE+1];/**< Posix queue name string               */
    int qid;                    /**< System V Queue ID                      */
    /* Locks for synchronous or other event wakeup */
    pthread_mutex_t rcvlock;    /**< Data receive lock, msgrcv              */
    pthread_mutex_t rcvlockb4;  /**< Data receive lock, before going msgrcv */
    pthread_mutex_t border;     /**< Border lock after msgrcv woken up      */
   
    pthread_mutex_t qlock;      /**< Queue lock (event queue)               */

    /* Timeout matching.
     * All the timeout events are enqueued to thread and thread is waken up
     * if needed. If not then event will be discarded because of stamps
     * does not match.
     */
    time_t stamp_time;          /**< timestamp for timeout waiting          */
    unsigned long stamp_seq;    /**< stamp sequence                         */
    
    
    mode_t mode;                /**< permissions on new queue               */
    struct mq_attr attr;        /**< attributes for the queue, if creating  */
    
    /**
     * thread operating with queue... 
     * Also note that one thread might operate with multiple queues.
     * but only one queue will be blocked at the same time.
     */
    pthread_t thread;
};
typedef struct ndrx_svq_info *mqd_t;


typedef struct ndrx_svq_ev ndrx_svq_ev_t;
/**
 * Event queue, either timeout, data or waken up by poller
 */
struct ndrx_svq_ev
{
    int ev;                 /**< Event code received                        */

    char *data;             /**< Associate data received                    */
    long datalen;           /**< Assocate data len                          */
    
    int fd;                 /**< Linked file descriptor generating FD event */
    ndrx_svq_ev_t *next;    /**< Linked list of event enqueued              */
};


/* TODO: We need a structure for mapping the Posix Q names -> System V 
 * and vice versa
 */


/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

extern int ndrx_svq_close(mqd_t);
extern int ndrx_svq_getattr(mqd_t, struct mq_attr *);
extern int ndrx_svq_notify(mqd_t, const struct sigevent *);
extern mqd_t   ndrx_svq_open(const char *pathname, int oflag, mode_t mode, 
                struct mq_attr *attr);
extern ssize_t ndrx_svq_receive(mqd_t, char *, size_t, unsigned int *);
extern int ndrx_svq_send(mqd_t, const char *, size_t, unsigned int);
extern int ndrx_svq_setattr(mqd_t, const struct mq_attr *, struct mq_attr *);
extern int     ndrx_svq_unlink(const char *name);

extern int ndrx_svq_timedsend(mqd_t mqd, const char *ptr, size_t len, unsigned int prio,
        const struct timespec *__abs_timeout); 

extern  ssize_t ndrx_svq_timedreceive(mqd_t mqd, char *ptr, size_t maxlen, unsigned int *priop,
        const struct timespec * __abs_timeout);

extern void ndrx_svq_set_lock_timeout(int secs);


/* internals... */

extern int ndrx_svqshm_get(char *qstr, int oflag);
        
#endif
