/**
 * @brief Emulated message queue. Based on UNIX Network Programming
 *  Volume 2 Second Edition interprocess Communications by W. Richard Stevens
 *  book. This code is only used for MacOS, as there aren't any reasonable
 *  queues available.
 * 
 * @file sys_emqueue.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#ifndef __sys_emqueue_h
#define __sys_emqueue_h

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>
#include <time.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* size of message in file is rounded up for alignment */
#define NDRX_EMQ_MSGSIZE(i) ((((i) + sizeof(long)-1) / sizeof(long)) * sizeof(long))

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/** 
 * opaque interface 
 */
typedef struct emq_info *mqd_t;

/**
 * Queue attributes definition
 */
struct mq_attr
{
    long mq_flags;     /**< MQ flags                        */
    long mq_maxmsg;    /**< max messages per queue          */
    long mq_msgsize;   /**< max message size in bytes       */
    long mq_curmsgs;   /**< number of messages in queue     */
};

/**
 * Message queue header
 */
struct emq_hdr 
{
    struct mq_attr    emqh_attr;  /**< queue attributes             */
    long              emqh_head;  /**< first message index          */
    long              emqh_free;  /**< first free message index     */
    long              emqh_nwait; /**< number of threads waiting    */
    pid_t             emqh_pid;   /**< notification pid             */
    struct sigevent   emqh_event; /**< for emq_notify()             */
    pthread_mutex_t   emqh_lock;  /**< mutex lock                   */
    pthread_cond_t    emqh_wait;  /**< condition var                */
};

/**
 * Message header
 **/
struct emq_msg_hdr
{
    long            msg_next;    /**< next msg index                */
    ssize_t         msg_len;     /**< actual length                 */
    unsigned int    msg_prio;    /**< priority                      */
};

/**
 * Process mapped memory for queue
 */
struct emq_info
{
    
    struct emq_hdr *emqi_hdr;     /**< mapped memory                */
    int            emqi_flags;    /**< flags for this process       */
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern NDRX_API int     emq_close(mqd_t);
extern NDRX_API int     emq_getattr(mqd_t, struct mq_attr *);
extern NDRX_API int     emq_notify(mqd_t, const struct sigevent *);
extern NDRX_API mqd_t   emq_open(const char *, int, ...);
extern NDRX_API ssize_t emq_receive(mqd_t, char *, size_t, unsigned int *);
extern NDRX_API int     emq_send(mqd_t, const char *, size_t, unsigned int);
extern NDRX_API int     emq_setattr(mqd_t, const struct mq_attr *, struct mq_attr *);
extern NDRX_API int     emq_unlink(const char *name);

extern NDRX_API int emq_timedsend(mqd_t emqd, const char *ptr, size_t len, unsigned int prio,
        const struct timespec *__abs_timeout); 

extern  NDRX_API ssize_t emq_timedreceive(mqd_t emqd, char *ptr, size_t maxlen, unsigned int *priop,
        const struct timespec * __abs_timeout);
        
#endif

/* vim: set ts=4 sw=4 et smartindent: */