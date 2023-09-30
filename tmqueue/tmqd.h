/**
 * @brief Transaction Managed Queue - daemon header
 *
 * @file tmqd.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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

#ifndef TMQD_H
#define	TMQD_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <sys_unix.h>
#include <xa_cmn.h>
#include <atmi.h>
#include <exhash.h>
#include <exthpool.h>
#include <rbtree.h>
#include "tmqueue.h"
    
/*---------------------------Externs------------------------------------*/
extern pthread_t G_forward_thread;
extern int volatile G_forward_req_shutdown;          /**< Is shutdown request? */
extern int volatile ndrx_G_forward_req_shutdown_ack; /**< Is shutdown acked?   */
/*---------------------------Macros-------------------------------------*/
#define SCAN_TIME_DFLT          10  /**< Every 10 sec try to complete TXs */
#define MAX_TRIES_DFTL          100 /**< Try count for transaction completion */
#define THREADPOOL_DFLT         10  /**< Default number of threads spawned   */
#define TXTOUT_DFLT             30  /**< Default XA transaction timeout      */
#define SES_TOUT_DFLT           180  /**< Default XA transaction timeout      */

#define TMQ_MODE_FIFO           'F' /**< fifo q mode                        */
#define TMQ_MODE_LIFO           'L' /**< lifo q mode                        */

/* Autoq flags: */
#define TMQ_AUTOQ_MANUAL        'N' /**< Not automatic Q                    */
#define TMQ_AUTOQ_AUTO          'Y' /**< Automatic                          */
#define TMQ_AUTOQ_AUTOTX        'T' /**< Automatic, transactional           */
#define TMQ_AUTOQ_ALLFLAGS      "NYT" /**< list of all flasg                */
#define TMQ_AUTOQ_ISAUTO(X) ((TMQ_AUTOQ_AUTO==X) || (TMQ_AUTOQ_AUTOTX==X))


#define TMQ_TXSTATE_ACTIVE      0   /**< Active message                     */
#define TMQ_TXSTATE_PREPARED    1   /**< Prepared msg                       */
#define TMQ_TXSTATE_COMMITTED   2   /**< Committed msg                      */

#define TMQ_QUEUE_SERVICE       "@" /**< Special name when service matches queue name */

#define TMQ_ARGS_COMMIT         "Cc"    /**< Sync after commit              */

#define TMQ_SYNC_NONE           0       /**< NO sync needed                 */
#define TMQ_SYNC_TPACALL        1       /**< Sync on tpacall                */
#define TMQ_SYNC_TPCOMMIT       2       /**< Sync on tpcommit () if auto=T  */

#define NDRX_TMQ_LOC_UNKNOWN    0x0000  /**< Unknown location               */
#define NDRX_TMQ_LOC_INFL       0x0001  /**< Inflight queue                 */
#define NDRX_TMQ_LOC_FUTQ       0x0002  /**< Future queue                   */
#define NDRX_TMQ_LOC_CURQ       0x0004  /**< Current queue                  */
#define NDRX_TMQ_LOC_CORQ       0x0008  /**< Correlator queue               */
#define NDRX_TMQ_LOC_MSGIDHASH  0x0010  /**< Message id hash                */
/**
 * Extract tmq_memmsg_t from the correltion tree node 
 */
#define TMQ_COR_GETMSG(ptr) ((tmq_memmsg_t *)((char *)ptr - EXOFFSET(tmq_memmsg_t, cor)))

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * TM ndrx_config.handler
 */
typedef struct
{
    long dflt_timeout;  /**< service call transaction timeout (forwarder)    */
    
    long ses_timeout;  /**< global session timeout, 
                        * how long uncompleted transaction may hang          */
    
    int scan_time;      /**< Number of seconds retries                       */
    
    int tout_check_time; /**< seconds used for detecting transaction timeout */
    
    char qconfig[PATH_MAX+1]; /**< Queue config file                        */
    int threadpoolsize;       /**< thread pool size                         */
    threadpool thpool;        /**< threads for service                      */
    
    int notifpoolsize;        /**< Notify thread pool size                  */
    threadpool notifthpool;   /**< Notify (loop back) threads for service (callbacks from TMSRV) */
    
    int fwdpoolsize;          /**< forwarder thread pool size               */
    threadpool fwdthpool;     /**< threads for forwarder                    */
    
    threadpool shutdownseq;   /**< Shutdown sequencer, we have simpler just to use pool instead of threads  */
    
    long fsync_flags;         /**< special flags for disk sync              */
    
    int no_chkrun;           /**< If set to true, do not trigger queue run  */

    long vnodeid;            /**< Node id, command id used for failovers    */

    int chkdisk_time;        /**< Time by which disk checking shall be excuted (enabled if > 0) */
    
} tmqueue_cfg_t;

/** correlator message queue hash */
typedef struct tmq_cormsg tmq_corhash_t;

/** hash of queues */
typedef struct tmq_qhash tmq_qhash_t;

/** queue configuration */
typedef struct tmq_qconfig tmq_qconfig_t;

/**
 * Memory based message.
 * Includes all the links to keep track of the message in the queues.
 * such as:
 *  - red-black tree of current/future run messages
 *  - linked list of in-flight messages (message locked, not available for dequeue)
 *  - red-black tree of correlator based messages
 * Structure is built every time message is enqueued or read from disk.
 */
typedef struct tmq_memmsg tmq_memmsg_t;
struct tmq_memmsg
{
    ndrx_rbt_node_t cur;    /**< handle in future or now list */
    ndrx_rbt_node_t cor;    /**< handle in correlator list    */

    char msgid_str[TMMSGIDLEN_STR+1]; /**< we might store msgid in string format... */
    char corrid_str[TMCORRIDLEN_STR+1]; /**< hash for correlator              */
    /** We should have hash handler of message hash                           */
    EX_hash_handle hh; /**< makes this structure hashable (for msgid)         */

    /** handlers for in-flight q (used for current message listing)           */
    tmq_memmsg_t *next;
    tmq_memmsg_t *prev;

    /** backlink to correlator q, so that we know where to remove             */
    tmq_corhash_t *corhash;

    tmq_qconfig_t *qconf;   /**< configuration used for this message Q        */
    tmq_qhash_t *qhash;     /**< queue entry                                  */

    tmq_msg_t *msg;         /**< disk message                                 */
    
/**
 * NDRX_TMQ_LOC_UNK  0x0000
 * NDRX_TMQ_LOC_INFL 0x0001
 * NDRX_TMQ_LOC_FUT  0x0002
 * NDRX_TMQ_LOC_CUR  0x0004
 * NDRX_TMQ_LOC_COR  0x0008
 * 
 * flags|=NDRX_TMQ_LOC_INFL;
 * 
 * flags&=~NDRX_TMQ_LOC_CUR;
 * flags&=~NDRX_TMQ_LOC_COR;
 * flags&=~NDRX_TMQ_LOC_FUT;
 * 
 * 
 * delete:
 *  if flags & NDRX_TMQ_LOC_INFL=> delete from NDRX_TMQ_LOC_INFL
 *  if flags & NDRX_TMQ_LOC_FUT=> delete from fut;
 * 
 * 
 */
    /* where are we? */
    short qstate;

};

/**
 * Messages correlated
 */
struct tmq_cormsg
{
    char corrid_str[TMCORRIDLEN_STR+1]; /**< hash for correlator               */
    /** queue by correlation, CDL, next2, prev2 
    tmq_memmsg_t *corq; */

    ndrx_rbt_tree_t corq; /**< queue uses standard sorting (insert time)      */

    EX_hash_handle hh; /**< makes this structure hashable        */
};

/**
 * List of queues (for queued messages)
 */
struct tmq_qhash
{
    char qname[TMQNAMELEN+1];
    long succ;      /**< Succeeded auto messages                 */
    long fail;      /**< failed auto messages                    */
    
    long numenq;    /**< Enqueued messages (even locked)         */
    long numdeq;    /**< Dequeued messages (removed, including aborts)     */

    EX_hash_handle hh; /**< makes this structure hashable        */

    ndrx_rbt_tree_t q;     /**< Currently available messages        */
    ndrx_rbt_tree_t q_fut; /**< future messages (not yet ready for proc) */

    /** 
     * in-flight messages (in process) 
     * this is linked list head
     */
    tmq_memmsg_t *q_infligh;

    tmq_corhash_t *corhash; /**< hash of correlators                */
};

/**
 * Queue configuration.
 * There will be special Q: "@DEFAULT" which contains the settings for default
 * (unknown queue)
 */
struct tmq_qconfig
{
    char qname[TMQNAMELEN+1];
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1]; /**< optional service name to call */
    char autoq;      /**< Is this automatic queue                       */
    int tries;       /**< Retry count for sending                       */
    int waitinit;    /**< How long to wait for initial sending (sec)    */
    int waitretry;   /**< How long to wait between retries (sec)        */
    int waitretrymax;/**< Max wait  (sec)                               */
    int memonly;    /**< is queue memory only                           */
    char mode;      /**< queue mode fifo/lifo                           */
    int txtout;     /**< transaction timeout (override if > -1)         */
    char errorq[TMQNAMELEN];     /**< Error queue name, optional        */
    int workers;   /**< Max number of busy forward workers              */
    int sync;      /**< Sync forward sending                            */
    
    EX_hash_handle hh; /**< makes this structure hashable               */
};

/**
 * List of Qs to forward
 */
typedef struct fwd_qlist fwd_qlist_t;
struct fwd_qlist
{
    char qname[TMQNAMELEN+1];
    long succ; /**< Succeeded auto messages                 */
    long fail; /**< failed auto messages                    */
    
    long numenq; /**< Succeeded auto messages               */
    long numdeq; /**< failed auto messages                  */
    int workers;    /**< number of configured workers       */
    int sync;       /**< is queue synchronized?             */
    fwd_qlist_t *next;
    fwd_qlist_t *prev;
};


typedef struct fwd_msg fwd_msg_t;

/**
 * Forward statistics
 */
typedef struct {
    
    char qname[TMQNAMELEN+1];
    int busy;
    NDRX_SPIN_LOCKDECL(busy_spin); /**< add/cmp/del ops         */
    
    /*
     * - have have spinlock for adding/checking/removing msg from list.
     * - have a mutex for workers to sleep & wait for signal/broadcast
     * - have a cond variable for sleeping on
     */
    NDRX_SPIN_LOCKDECL(sync_spin); /**< add/cmp/del ops         */
    MUTEX_LOCKDECLN(sync_mut);  /**< wait mut                   */
    pthread_cond_t   sync_cond; /**< wait cond                  */
            
    fwd_msg_t *sync_head;       /**< head msg if used for sync  */
    
    EX_hash_handle hh; /**< makes this structure hashable       */
    
} fwd_stats_t;

/**
 * Forward message entry
 */
struct fwd_msg {
    fwd_stats_t *stats; /**< ptr to stats block of the queue    */
    tmq_msg_t   *msg;   /**< message entry to forward           */
    int     sync;       /**< do we run in sync mode?            */
    unsigned long seq;  /**< sequence number                    */
    fwd_msg_t *prev;
    fwd_msg_t *next;
    
};

/*---------------------------Globals------------------------------------*/
extern tmqueue_cfg_t G_tmqueue_cfg;
extern tmq_memmsg_t *G_msgid_hash;
extern void (*G_tmq_chkdisk_th)(void *ptr, int *p_finish_off);
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* init */
extern void tmq_thread_init(void);
extern void tmq_thread_uninit(void);
extern void tmq_thread_shutdown(void *ptr, int *p_finish_off);


/* Q api */
extern int tmq_mqlq(UBFH *p_ub, int cd);
extern int tmq_mqlc(UBFH *p_ub, int cd);
extern int tmq_mqlm(UBFH *p_ub, int cd);

extern int tmq_mqrc(UBFH *p_ub);
extern int tmq_mqch(UBFH *p_ub);

extern int tmq_enqueue(UBFH *p_ub, int *int_diag);
extern int tmq_dequeue(UBFH **pp_ub, int *int_diag);

/* Background API */
extern int background_read_log(void);
extern void forward_shutdown_wake(void);
extern int forward_process_init(void);
extern void forward_lock(void);
extern void forward_unlock(void);
extern void thread_shutdown(void *ptr, int *p_finish_off);

/* Q space api: */
extern int tmq_reload_conf(char *cf);
extern int tmq_qconf_addupd(char *qconfstr, char *name);
extern int tmq_dum_add(char *tmxid);
extern int tmq_msg_add(tmq_msg_t **msg, int is_recovery, TPQCTL *diag, int *int_diag);
extern int tmq_unlock_msg(union tmq_upd_block *b);
extern tmq_msg_t * tmq_msg_dequeue(char *qname, long flags, int is_auto, long *diagnostic, 
            char *diagmsg, size_t diagmsgsz, char *corrid_str, int *int_diag);
extern tmq_msg_t * tmq_msg_dequeue_by_msgid(char *msgid, long flags, long *diagnostic, 
        char *diagmsg, size_t diagmsgsz, int *int_diag);
extern int tmq_unlock_msg_by_msgid(char *msgid, int chkrun);

extern int tmq_msgid_exists(char *msgid_str);

extern int tmq_load_msgs(void);
extern fwd_qlist_t *tmq_get_qlist(int auto_only, int incl_def);
extern int tmq_qconf_get_with_default_static(char *qname, tmq_qconfig_t *qconf_out);
extern tmq_qconfig_t * tmq_qconf_get_with_default(char *qname, int *p_is_defaulted);
extern int tmq_build_q_def(char *qname, int *p_is_defaulted, char *out_buf, size_t out_bufsz);
extern tmq_memmsg_t *tmq_get_msglist(char *qname);
    
extern int tmq_update_q_stats(char *qname, long succ_diff, long fail_diff);
extern void tmq_get_q_stats(char *qname, long *p_msgs, long *p_locked);
extern int q_msg_sort(tmq_memmsg_t *q1, tmq_memmsg_t *q2);

extern int tmq_cor_msg_add(tmq_memmsg_t *mmsg);
extern void tmq_cor_msg_del(tmq_memmsg_t *mmsg);
extern tmq_corhash_t * tmq_cor_find(tmq_qhash_t *qhash, char *corrid_str);

/* Red-black tree support: */
extern void tmq_rbt_combine_cur(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg);
extern void tmq_rbt_combine_fut(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg);
extern void tmq_rbt_combine_cor(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg);
extern int tmq_rbt_cmp_cur(const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg);
extern int tmq_rbt_cmp_cor(const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg);
extern int tmq_rbt_cmp_fut (const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg);

extern int tmq_fwd_busy_cnt(char *qname, fwd_stats_t **p_stats);
extern void tmq_fwd_busy_inc(fwd_stats_t *p_stats);
extern void tmq_fwd_busy_dec(fwd_stats_t *p_stats);
extern int tmq_fwd_stat_init(void);

extern void tmq_fwd_sync_add(fwd_msg_t *fwd);
extern void tmq_fwd_sync_del(fwd_msg_t *fwd);
extern int tmq_fwd_sync_cmp(fwd_msg_t *fwd);
extern void ndrx_forward_chkrun(tmq_memmsg_t *msg);;

extern void tmq_fwd_sync_wait(fwd_msg_t *fwd);
extern void tmq_fwd_sync_notify(fwd_msg_t *fwd);

/* inflight routines */
extern int ndrx_infl_addmsg(tmq_qconfig_t * qconf, tmq_qhash_t *qhash, tmq_memmsg_t *mmsg);
extern int ndrx_infl_mov2infl(tmq_memmsg_t *mmsg);
extern int ndrx_infl_mov2cur(tmq_memmsg_t *mmsg);
extern int ndrx_infl_delmsg(tmq_memmsg_t *mmsg);
extern int ndrx_infl_fut2cur(tmq_qhash_t *qhash);

#ifdef	__cplusplus
}
#endif

#endif	/* TMQD_H */

/* vim: set ts=4 sw=4 et smartindent: */
