/* 
** Bridge commons
**
** @file bridge.h
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

#ifndef BRIDGE_H
#define	BRIDGE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <thpool.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define BR_DEFAULT_THPOOL_SIZE          5 /* Default threadpool size */
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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*
 * Bridge ndrx_config.handler
 */
typedef struct
{
    int nodeid;                   /* External node id */
    exnetcon_t net;               /* Network handler, might be client or server...  */
    exnetcon_t *con;              /* Real working connection  */
    char svc[XATMI_SERVICE_NAME_LENGTH+1];  /* Service name used by this bridge */
    long long timediff;           /* Bridge time correction       */
    int common_format;            /* Common platform format. */
    char gpg_recipient[33];       /* PGP Encryption recipient */
    char gpg_signer[33];          /* PGP Encryption signer */
    int threadpoolsize;           /* Thread pool size */
    threadpool thpool;            /* Thread pool by it self */
} bridge_cfg_t;

typedef struct in_msg in_msg_t;
struct in_msg
{
    int pack_type;
    char buffer[ATMI_MSG_MAX_SIZE];
    int len;
    ndrx_stopwatch_t trytime;  /* Time in Q */
    in_msg_t *prev, *next;
};

/**
 * Message received from XATMI, for sending to network, by thread
 */
typedef struct
{
    char *buf;
    int len;
    char msg_type;
    int threaded; /* Do we run in threaded mode? */
    
} xatmi_brmessage_t;


/**
 * Message received from network and submitted to thread
 */
typedef struct
{
    char *buf;
    int len;
    exnetcon_t *net;
    int threaded; /* Do we run in threaded mode? */
    int buf_malloced; /* buffer is malloced? */
    cmd_br_net_call_t *call; /* Intermediate field */
} net_brmessage_t;

/*---------------------------Globals------------------------------------*/
extern bridge_cfg_t G_bridge_cfg;
extern __thread int G_thread_init;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int br_submit_to_ndrxd(command_call_t *call, int len, in_msg_t* from_q);
extern int br_submit_to_service(tp_command_call_t *call, int len, in_msg_t* from_q);
extern int br_submit_to_service_notif(tp_notif_call_t *call, int len, in_msg_t* from_q);
extern int br_got_message_from_q(char *buf, int len, char msg_type);
extern int br_submit_reply_to_q(tp_command_call_t *call, int len, in_msg_t* from_q);
    
extern int br_process_msg(exnetcon_t *net, char *buf, int len);
extern int br_send_to_net(char *buf, int len, char msg_type, int command_id);

extern int br_calc_clock_diff(command_call_t *call);
extern int br_send_clock(void);
extern void br_clock_adj(tp_command_call_t *call, int is_out);

extern int br_tpcall_pushstack(tp_command_call_t *call);
extern int br_get_conv_cd(char msg_type, char *buf, int *p_pool);

#ifdef	__cplusplus
}
#endif

#endif	/* TPEVSV_H */

