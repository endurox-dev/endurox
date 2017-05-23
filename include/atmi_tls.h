/* 
** Enduro/X ATMI library thread local storage
**
** @file atmi_tls.h
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define ATMI_TLS_MAGIG          0x39617cde
    
#define ATMI_TLS_ENTRY  if (NDRX_UNLIKELY(NULL==G_atmi_tls)) \
    {G_atmi_tls=(atmi_tls_t *)ndrx_atmi_tls_new(TRUE, TRUE);};
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
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
    unsigned callseq;/* = 0; */
    
    /* init.c */
    int G_atmi_is_init;/* = 0;  Is environment initialised */
    atmi_lib_conf_t G_atmi_conf; /* ATMI library configuration */
    tp_conversation_control_t G_tp_conversation_status[MAX_CONNECTIONS];
    tp_conversation_control_t G_accepted_connection;
    
    
    /* tpcall.c */
    long M_svc_return_code;/*=0; */
    int tpcall_first; /* = TRUE; */
    ndrx_timer_t tpcall_start;
    call_descriptor_state_t G_call_state[MAX_ASYNC_CALLS];
    int tpcall_get_cd; /* first available, we want test overlap!*/
    /* unsigned tpcall_callseq; */
    /*int tpcall_cd;  = 0; */
    
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
    pthread_mutex_t mutex; /* initialize later with PTHREAD_MUTEX_INITIALIZER */
    
    /* have the transport for other's TLSes 
     * used by tpgetctxt() and tpsetctxt()
     */
    nstd_tls_t *p_nstd_tls;
    ubf_tls_t *p_ubf_tls;
    
    int is_associated_with_thread; /* Is current context associated with thread? */
    
} atmi_tls_t;
/*---------------------------Globals------------------------------------*/
extern NDRX_API __thread atmi_tls_t *G_atmi_tls; /* Enduro/X standard library TLS */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CONTEXT_H */

