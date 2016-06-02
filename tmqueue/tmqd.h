/* 
** Transaction Managed Queue - daemon header
**
** @file tmqd.h
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

#ifndef TMQD_H
#define	TMQD_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa_cmn.h>
#include <atmi.h>
#include <utlist.h>
#include <uthash.h>
#include "thpool.h"
#include "tmqueue.h"
    
/*---------------------------Externs------------------------------------*/
extern pthread_t G_forward_thread;
extern int G_forward_req_shutdown;    /* Is shutdown request? */
/*---------------------------Macros-------------------------------------*/
#define SCAN_TIME_DFLT          10  /* Every 10 sec try to complete TXs */
#define MAX_TRIES_DFTL          100 /* Try count for transaction completion */
#define TOUT_CHECK_TIME         1   /* Check for transaction timeout, sec   */
#define THREADPOOL_DFLT         10   /* Default number of threads spawned   */
#define TXTOUT_DFLT             30   /* Default XA transaction timeout      */

#define TMQ_MODE_FIFO           'F' /* fifo q mode */
#define TMQ_MODE_LIFO           'L' /* lifo q mode */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * TM config handler
 */
typedef struct
{
    long dflt_timeout; /* how long monitored transaction can be open        */
    /*char data_dir[PATH_MAX];  Where to write tx log files  - NO NEED, handed by XA driver */
    int scan_time;      /* Number of seconds retries */
    char qspace[XATMI_SERVICE_NAME_LENGTH+1]; /* where the Q files live */
    char qspacesvc[XATMI_SERVICE_NAME_LENGTH+1]; /* real service name */
    char qconfig[PATH_MAX+1]; /* Queue config file  */
    
    int threadpoolsize; /* thread pool size */
    
    
    threadpool thpool; /* threads for service */
    
    int notifpoolsize; /* Notify thread pool size */
    threadpool notifthpool; /* Notify (loop back) threads for service (callbacks from TMSRV) */
    
    int fwdpoolsize; /* forwarder thread pool size */
    threadpool fwdthpool; /* threads for forwarder */
    
} tmqueue_cfg_t;

/**
 * Server thread struct
 */
struct thread_server
{
    char *context_data; /* malloced by enduro/x */
    int cd;
    char *buffer; /* buffer data, managed by enduro/x */
};
/* note we must malloc this struct too. */
typedef struct thread_server thread_server_t;

/**
 * Memory based message.
 */
typedef struct tmq_memmsg tmq_memmsg_t;
struct tmq_memmsg
{
    char msgid_str[TMMSGIDLEN_STR+1]; /* we might store msgid in string format... */
    char corid_str[TMCORRIDLEN_STR+1]; /* hash for correlator            */
    /* We should have hash handler of message hash */
    UT_hash_handle hh; /* makes this structure hashable (for msgid)        */
    UT_hash_handle h2; /* makes this structure hashable (for corid)        */
    /* We should also have a linked list handler   */
    tmq_memmsg_t *next;
    tmq_memmsg_t *prev;
    
    tmq_msg_t *msg;
};

/**
 * List of queues (for queued messages)
 */
typedef struct tmq_qhash tmq_qhash_t;
struct tmq_qhash
{
    char qname[TMQNAMELEN+1];
    long succ; /* Succeeded auto messages */
    long fail; /* failed auto messages */
    
    long numenq; /* Enqueued messages (even locked)                   */
    long numdeq; /* Dequeued messages (removed, including aborts)     */
    
    UT_hash_handle hh; /* makes this structure hashable        */
    tmq_memmsg_t *q;
};

/**
 * Qeueue configuration.
 * There will be special Q: "@DEFAULT" which contains the settings for default
 * (unknown queue)
 */
typedef struct tmq_qconfig tmq_qconfig_t;
struct tmq_qconfig
{
    char qname[TMQNAMELEN+1];
    
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1]; /* optional service name to call */
    
    int autoq; /* Is this automatic queue */
    int tries; /* Retry count for sending */
    int waitinit; /* How long to wait for initial sending (sec) */
    int waitretry; /* How long to wait between retries (sec) */
    int waitretryinc; /* Wait increment between retries (sec) */
    int waitretrymax; /* Max wait  (sec) */
    int memonly; /* is queue memory only */
    char mode; /* queue mode fifo/lifo*/
    
    UT_hash_handle hh; /* makes this structure hashable        */
};

/**
 * List of Qs to forward
 */
typedef struct fwd_qlist fwd_qlist_t;
struct fwd_qlist
{
    char qname[TMQNAMELEN+1];
    long succ; /* Succeeded auto messages */
    long fail; /* failed auto messages */
    
    long numenq; /* Succeeded auto messages */
    long numdeq; /* failed auto messages */
    
    fwd_qlist_t *next;
    fwd_qlist_t *prev;
};

/*---------------------------Globals------------------------------------*/
extern tmqueue_cfg_t G_tmqueue_cfg;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* Q api */
extern int tmq_mqlq(UBFH *p_ub, int cd);
extern int tmq_mqlc(UBFH *p_ub, int cd);
extern int tmq_mqlm(UBFH *p_ub, int cd);

extern int tmq_mqrc(UBFH *p_ub);
extern int tmq_mqch(UBFH *p_ub);


extern int tmq_enqueue(UBFH *p_ub);
extern int tmq_dequeue(UBFH **pp_ub);
extern int tex_mq_notify(UBFH *p_ub);

/* Background API */
extern int background_read_log(void);
extern void forward_shutdown_wake(void);
extern void forward_process_init(void);
extern void forward_lock(void);
extern void forward_unlock(void);
extern void thread_shutdown(void *ptr, int *p_finish_off);

/* Q space api: */
extern int tmq_reload_conf(char *cf);
extern int tmq_qconf_addupd(char *qconfstr);
extern int tmq_msg_add(tmq_msg_t *msg, int is_recovery);
extern int tmq_unlock_msg(union tmq_upd_block *b);
extern tmq_msg_t * tmq_msg_dequeue(char *qname, long flags, int is_auto);
extern tmq_msg_t * tmq_msg_dequeue_by_msgid(char *msgid, long flags);
extern tmq_msg_t * tmq_msg_dequeue_by_corid(char *corid, long flags);
extern int tmq_unlock_msg_by_msgid(char *msgid);
extern int tmq_load_msgs(void);
extern fwd_qlist_t *tmq_get_qlist(int auto_only, int incl_def);
extern int tmq_qconf_get_with_default_static(char *qname, tmq_qconfig_t *qconf_out);
extern int tmq_build_q_def(char *qname, int *p_is_defaulted, char *out_buf);
extern tmq_memmsg_t *tmq_get_msglist(char *qname);
    
extern int tmq_update_q_stats(char *qname, long succ_diff, long fail_diff);
extern void tmq_get_q_stats(char *qname, long *p_msgs, long *p_locked);
    
#ifdef	__cplusplus
}
#endif

#endif	/* TMQD_H */

