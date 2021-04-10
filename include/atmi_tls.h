/**
 * @brief Enduro/X ATMI library thread local storage
 *
 * @file atmi_tls.h
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
#ifndef ATMI_TLS_H
#define	ATMI_TLS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
#include <ndrstandard.h>
#include <xa.h>
#include <atmi_int.h>
#include <xa_cmn.h>
#include <tperror.h>
#include <nstd_tls.h>
#include <ubf_tls.h>
#include <tx.h>
#include <setjmp.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define ATMI_TLS_MAGIG          0x39617cde
    
#define ATMI_TLS_ENTRY  if (NDRX_UNLIKELY(NULL==G_atmi_tls)) \
    {G_atmi_tls=(atmi_tls_t *)ndrx_atmi_tls_new(NULL, EXTRUE, EXTRUE);};
    
#define NDRX_NDRXD_PING_SEQ_MAX         32000 /**< max ping sequence to reset */
    
/* Feature #139 mvitolin, 09/05/2017 */
#define NDRX_SVCINSTALL_NOT		0 /**< Not doing service install      */
#define NDRX_SVCINSTALL_DO		1 /**< Installing new service to SHM  */
#define NDRX_SVCINSTALL_OVERWRITE	2 /**< Overwrite the already installed data */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
        
/**
 * Memory base Q (current with static buffer)
 * but we could migrate to dynamically allocated buffers...
 */
typedef struct tpmemq tpmemq_t;
struct tpmemq
{    
    char *buf;
    size_t len;
    size_t data_len;
    /* Linked list */
    tpmemq_t *prev;
    tpmemq_t *next;
};

/**
 * TLS malloc'd block for qdisk_xa
 * Not moving all setting just purely to TLS, as this seems to
 * take a lot space and tls is used by all processes, thus it would
 * greatly increase memory footprint.
 */
typedef struct
{    
    /* Per thread data: */
    int is_reg; /* Dynamic registration done? */
    /*
     * Due to fact that we might have multiple queued messages per resource manager
     * we will name the transaction files by this scheme:
     * - <XID_STR>-1|2|3|4|..|N
     * we will start the processing from N back to 1 so that if we crash and retry
     * the operation, we can handle all messages in system.
     */
    char filename_base[PATH_MAX+1]; /* base name of the file */
    char filename_active[PATH_MAX+1]; /* active file name */
    char filename_prepared[PATH_MAX+1]; /* prepared file name */
    
} ndrx_qdisk_tls_t;

/**
 * ATMI library TLS
 * Here we will have a trick, if we get at TLS, then we must automatically suspend
 * the global transaction if one in progress. To that if we move to different
 * thread we can restore it...
 */
typedef struct
{
    int magic;
    
    /* atmi.c */    
    tp_command_call_t G_last_call;
    /* conversation.c */
    int conv_cd;/*=1;  first available */
    /* unsigned callseq; - will be shared with tpcalls... global ...*//* = 0; */ 
    
    /* init.c */
    int G_atmi_is_init;/* = 0;  Is environment initialised */
    atmi_lib_conf_t G_atmi_conf; /* ATMI library configuration */
    tp_conversation_control_t G_tp_conversation_status[MAX_CONNECTIONS];
    tp_conversation_control_t G_accepted_connection;
    
    
    /* tpcall.c */
    long M_svc_return_code;/*=0; */
    int tpcall_first; /* = TRUE; */
    ndrx_stopwatch_t tpcall_start;
    call_descriptor_state_t G_call_state[MAX_ASYNC_CALLS];
    int tpcall_get_cd; /* first available, we want test overlap!*/
    /* unsigned tpcall_callseq; */
    /*int tpcall_cd;  = 0; */
    
    /* We need a enqueued list of messages */
    tpmemq_t *memq; /* Message enqueued in memory... */
    /* EX_SPIN_LOCKDECL(memq_lock); - not needed this TLS for one thread 
     * thus memq is processed by same thread only*/
    
    /* tperror.c */
    char M_atmi_error_msg_buf[MAX_TP_ERROR_LEN+1]; /* = {EOS}; */
    int M_atmi_error;/* = TPMINVAL; */
    short M_atmi_reason;/* = NDRX_XA_ERSN_NONE;  XA reason, default */
    char errbuf[MAX_TP_ERROR_LEN+1];
    /* xa.c */
    int M_is_curtx_init;/* = FALSE; */
    atmi_xa_curtx_t G_atmi_xa_curtx;

    /* xautils.c */
    XID xid; /* handler for new XID */
    int global_tx_suspended; /* TODO: suspend the global txn */
    TPTRANID tranid; /* suspended transaction id - suspended global tx... */
    
    int is_auto; /* is this auto-allocated (thus do the auto-free) */
    /* mutex lock (so that no two parallel threads work with same tls */
    MUTEX_VAR(mutex); /* initialize later with PTHREAD_MUTEX_INITIALIZER */
    
    /* have the transport for other's TLSes 
     * used by tpgetctxt() and tpsetctxt()
     */
    nstd_tls_t *p_nstd_tls;
    ubf_tls_t *p_ubf_tls;
    
    int is_associated_with_thread; /* Is current context associated with thread? */
    
    /* unsolicited notification processing */
    void (*p_unsol_handler) (char *data, long len, long flags);
    
    TPINIT client_init_data;
    
    int ndrxd_ping_seq;     /**< NDRXD daemon ping sequence sent */
    
    
    /*  TX Specification related: */
    
    /**
     * Shall tx_commit() return when transaction is completed
     * or return when logged for completion?
     * Default: Wait for complete.
     */
    COMMIT_RETURN tx_commit_return;

   /**
    * Shall caller start new transaction when commit/abort is called?
    * Default NO
    */
   TRANSACTION_CONTROL tx_transaction_control;

   /**
    * What is transaction timeout? In seconds.
    */
   TRANSACTION_TIMEOUT tx_transaction_timeout;
   
   ndrx_ctx_priv_t integpriv;    /**< integration  private data               */
   
   jmp_buf call_ret_env;    /**< for MT server where to return                */
   
   /* hook tpacall no service... */
    int (*pf_tpacall_noservice_hook)(char *svc, char *data,
                long len, long flags); 
    
    buffer_obj_t  nullbuf; /**< so that we have place where to set call infos */
    
    /** default priority setting used for calls */
    int prio;
    long prio_flags;
    int prio_last;
    
    
    int tmnull_is_open; /**< Null swith is open ? */
    int tmnull_rmid; /**< Null switch RMID */
    
    /* Shared between threads: */
    int qdisk_is_open;
    int qdisk_rmid;
    
    ndrx_qdisk_tls_t *qdisk_tls;
    
} atmi_tls_t;
/*---------------------------Globals------------------------------------*/
extern NDRX_API __thread atmi_tls_t *G_atmi_tls; /* Enduro/X standard library TLS */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CONTEXT_H */

/* vim: set ts=4 sw=4 et smartindent: */
