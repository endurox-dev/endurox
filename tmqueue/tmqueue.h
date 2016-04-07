/* 
** Q for EnduroX
**
** @file tmqueue.h
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

#ifndef TMQUEUE_H
#define	TMQUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa_cmn.h>
#include <atmi.h>
#include <utlist.h>
#include <uthash.h>
#include "thpool.h"
    
/*---------------------------Externs------------------------------------*/
extern pthread_t G_bacground_thread;
extern int G_bacground_req_shutdown;    /* Is shutdown request? */
/*---------------------------Macros-------------------------------------*/
#define SCAN_TIME_DFLT          10  /* Every 10 sec try to complete TXs */
#define MAX_TRIES_DFTL          100 /* Try count for transaction completion */
#define TOUT_CHECK_TIME         1   /* Check for transaction timeout, sec   */
#define THREADPOOL_DFLT         10   /* Default number of threads spawned   */

/* Basically we have a two forms of MSGID
 * 1. Native, 32 byte binary byte array
 * 2. String, Base64, 
 *  */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * TM config handler
 */
typedef struct
{
    long dflt_timeout; /* how long monitored transaction can be open        */
    char data_dir[PATH_MAX]; /* Where to write tx log files                 */
    int scan_time;      /* Number of seconds retries */
    char qspace[XATMI_SERVICE_NAME_LENGTH+1];
    
    int threadpoolsize; /* thread pool size */
    
    threadpool thpool;
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
 * Common command header
 */
typedef struct
{
    char magic[4];          /* File magic               */
    short srvid;
    short nodeid;
    char qspace[TMQNAMELEN+1];
    short command_code;     /* command code             */
    char msgid[TMMSGIDLEN]; /* message_id               */
} tmq_cmdheader_t;

/** 
 * Command: qmessage 
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    TPQCTL qctl;          /* Queued message        */
    unsigned char status;   /* Status of the message */
    long trycounter;        /* try counter           */
    long long timestamp;    /* timestamp, YYYYMMDDHHMISSfff (with milliseconds) */
    long long trytstamp;    /* Last try timestamp */
    /* Message log (stored only in file)  */
    long len;               /* msg len               */
    char msg[0];            /* msg                   */
} tmq_msg_t;

/**
 * Command: delmsg
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    
} tmq_msg_del_t;

/** 
 * Command: updcounter
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    unsigned char status;   /* Status of the message */
    long trycounter;        /* try counter           */
    long long trytstamp;    /* Last try timestamp */
} tmq_msg_upd_t;

/**
 * Data block
 */
union tmq_block {
    tmq_msg_t msg;
    tmq_msg_del_t del;
    tmq_msg_upd_t upd;
};  


/**
 * Memory based message.
 */
typedef struct tmq_memmsg tmq_memmsg_t;
struct tmq_memmsg
{
    char msgid[TMMSGIDLEN+1]; /* we might store msgid in string format... */
    tmq_msg_t msg;
    /* We should have hash handler of message hash */
    UT_hash_handle hh; /* makes this structure hashable        */
    /* We should also have a linked list handler   */
    tmq_memmsg_t *next;
    tmq_memmsg_t *prev;
};

/**
 * List of queues (for queued messages)
 */
typedef struct tmq_qhash tmq_qhash_t;
struct tmq_qhash
{
    char qname[TMQNAMELEN+1];
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
    int is_automatic; /* Is this automatic queue */
    int retry_max; /* Retry count for sending */
    
    int initial_wait; /* How long to wait for initial sending  */
    int retry_step_wait; /* Increase wait seconds for each retry */
    int retry_max_wait; /* Max time to wait for retry */
    
    int is_memory; /* is queue memory only */
    
    UT_hash_handle hh; /* makes this structure hashable        */
};

/*---------------------------Globals------------------------------------*/
extern tmqueue_cfg_t G_tmqueue_cfg;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/* Q api */
extern int tmq_printqueue(UBFH *p_ub, int cd);
extern int tmq_enqueue(UBFH *p_ub);
extern int tmq_dequeue(UBFH *p_ub);

/* Background API */
extern int background_read_log(void);
extern void background_wakeup(void);
extern void background_process_init(void);
extern void background_lock(void);
extern void background_unlock(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TMQUEUE_H */

