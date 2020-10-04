/**
 * @brief Bridge commons
 *
 * @file bridge.h
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

#ifndef BRIDGE_H
#define	BRIDGE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <exthpool.h>
#include <pthread.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define BR_QRETRIES_DEFAULT             999999  /**< Default number of retries    */
#define BR_DEFAULT_THPOOL_SIZE          2       /**< Default threadpool size      */
#define BR_THREAD_ENTRY if (!G_thread_init) \
         { \
                if (EXSUCCEED==tpinit(NULL))\
                { \
                    G_thread_init=EXTRUE; \
                } \
                else \
                { \
                    EXFAIL_OUT(ret);\
                } \
         }
    
#define DEFAULT_QUEUE_SIZE          100    /**< max nr of queued messages dflt */
#define DEFAULT_QUEUE_MAXSLEEP      150    /**< Max number milliseconds to sleep */
#define DEFAULT_QUEUE_MINSLEEP      40     /**< Mininum sleep between attempts */
    
#define     PACK_TYPE_TONDRXD   1   /**< Send message NDRXD                   */
#define     PACK_TYPE_TOSVC     2   /**< Send to service, use timer (their)   */
#define     PACK_TYPE_TORPLYQ   3   /**< Send to reply q, use timer (internal)*/
    
/* List of queue actions: */
#define QUEUE_ACTION_BLOCK         0   /**< Block the traffic                 */
#define QUEUE_ACTION_DROP          1   /**< Drop the msg                      */
#define QUEUE_ACTION_IGNORE        2   /**< Ignore the condition              */
    
/* List of action flags: */
#define QUEUE_FLAG_ACTION_BLKIGN        1 /* Global queue full - block, svc queue full ignore */
#define QUEUE_FLAG_ACTION_BLKDROP       2 /* Global queue full - block, svc queue full - drop */
#define QUEUE_FLAG_ACTION_DROPDROP      3 /* Global queue full - drop, svc queue full - drop  */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*
 * Bridge ndrx_config.handler
 */
typedef struct
{
    int nodeid;                   /**< External node id */
    exnetcon_t net;               /**< Network handler, might be client or server...  */
    exnetcon_t *con;              /**< Real working connection  */
    char svc[XATMI_SERVICE_NAME_LENGTH+1];  /**< Service name used by this bridge */
    long long timediff;           /**< Bridge time correction       */
    int common_format;            /**< Common platform format. */
    int qretries;                 /**< Queue Resubmit retries */
    int qsize;                    /**< Number of messages stored in memory before blocking */
    int qsizesvc;                 /**< Single service queue size                */
    int qttl;                     /**< Number of miliseconds for message to live in queue */
    int qmaxsleep;                /**< Max number of millisecionds to sleep between attempts */
    int qminsleep;                /**< Min number of millisecionds to sleep between attempts */
    
    int qfullaction;              /**< Action for full queue                  */
    int qfullactionsvc;           /**< Action One service queue full          */
    
    int threadpoolbufsz;          /**< Threadpool buffer size, lock after full */
    int threadpoolsize;           /**< Thread pool size */
    int check_interval;           /**< connection checking interval             */
    threadpool thpool_tonet;      /**< Thread pool by it self */
    /* Support #502, we get deadlock when both nodes all threads attempt to send
     * and there is no one who performs receive, all sockets become full */
    threadpool thpool_fromnet;    /**< Thread pool by it self */
    
    threadpool thpool_queue;    /**< Queue runner */
    
} bridge_cfg_t;

typedef struct in_msg in_msg_t;
struct in_msg
{
    int pack_type;
    char destqstr[NDRX_MAX_Q_SIZE+1];  /**< Destination queue to which sent msg */
    char *buffer;
    int len;
    int tries;                    /**< number of attempts for sending msg to Q */
    ndrx_stopwatch_t addedtime;   /**< Time in Q                               */
    ndrx_stopwatch_t updatetime;  /**< Last time when msg was processed        */
    int next_try_ms;              /**< When the next attempt is scheduled      */
    in_msg_t *prev, *next;
};


typedef struct in_msg_hash in_msg_hash_t;
struct in_msg_hash
{
    char qstr[NDRX_MAX_Q_SIZE+1];/**< Posix queue name string                */
    int nrmsg;                   /**< current number of messages per posix q */
    in_msg_t  *msgs;             /**< DL list of messages                    */
    EX_hash_handle hh;
};

/**
 * Message received from XATMI, for sending to network, by thread
 */
typedef struct
{
    char *buf;
    int len;
    char msg_type;
    
} xatmi_brmessage_t;

/**
 * Message received from network and submitted to thread
 */
typedef struct
{
    char *buf;
    int len;
    exnetcon_t *net;
    cmd_br_net_call_t *call; /* Intermediate field */
} net_brmessage_t;

/*---------------------------Globals------------------------------------*/
extern bridge_cfg_t G_bridge_cfg;
extern __thread int G_thread_init;
extern pthread_mutex_t ndrx_G_global_br_lock;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int br_submit_to_ndrxd(command_call_t *call, int len);
extern int br_submit_to_service(tp_command_call_t *call, int len);
extern int br_submit_to_service_notif(tp_notif_call_t *call, int len);
extern int br_got_message_from_q(char **buf, int len, char msg_type);
extern int br_submit_reply_to_q(tp_command_call_t *call, int len);
    
extern int br_process_msg(exnetcon_t *net, char **buf, int len);
extern int br_send_to_net(char *buf, int len, char msg_type, int command_id);

extern int br_calc_clock_diff(command_call_t *call);
extern int br_send_clock(void);
extern void br_clock_adj(tp_command_call_t *call, int is_out);

extern int br_tpcall_pushstack(tp_command_call_t *call);
extern int br_get_conv_cd(char msg_type, char *buf, int *p_pool);
extern int br_chk_limit(void);

extern int br_netin_setup(void);
extern void br_netin_shutdown(void);

extern int br_process_error(char *buf, int len, int err, in_msg_t* from_q, 
        int pack_type, char *destqstr, in_msg_hash_t * qhash);

extern void br_tempq_init(void);
extern int br_add_to_q(char *buf, int len, int pack_type, char *destq);

#ifdef	__cplusplus
}
#endif

#endif	/* TPEVSV_H */

/* vim: set ts=4 sw=4 et smartindent: */
