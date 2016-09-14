/* 
** Enduro/X ATMI library thread local storage
**
** @file atmi_tls.h
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
#ifndef ATMI_TLS_H
#define	ATMI_TLS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <pthread.h>
#include <ndrstandard.h>
#include <tperror.h>
#include <xa.h>
#include <atmi_int.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define ATMI_TLS_MAGIG          0x39617cde
    
#define ATMI_TLS_ENTRY  if (NULL==G_atmi_tls) {ndrx_atmi_tls_new(TRUE, TRUE);};
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
    call_descriptor_state_t G_call_state[MAX_ASYNC_CALLS];
    tp_conversation_control_t G_tp_conversation_status[MAX_CONNECTIONS];
    tp_conversation_control_t G_accepted_connection;
    
    /* tpcall.c */
    long M_svc_return_code;/*=0; */
    int tpcall_first; /* = TRUE; */
    ndrx_timer_t tpcall_start;
    int tpcall_cd; /* = 0; */
    
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
    int global_tx_suspended; /* suspend the global txn */
    TPTRANID tranid;
    
    int is_auto; /* is this auto-allocated (thus do the auto-free) */
    /* mutex lock (so that no two parallel threads work with same tls */
    pthread_mutex_t mutex; /* initialize later with PTHREAD_MUTEX_INITIALIZER */
    
} atmi_tls_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API __thread atmi_tls_t *G_atmi_tls; /* Enduro/X standard library TLS */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
#ifdef	__cplusplus
}
#endif

#endif	/* ATMI_CONTEXT_H */

