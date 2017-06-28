/* 
** Internal servers header.
**
** @file srv_int.h
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

#ifndef SRV_INT_H
#define	SRV_INT_H

#ifdef	__cplusplus
extern NDRX_API "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <sys_mqueue.h>
#include <atmi.h>
#include <setjmp.h>
#include <ndrxdcmn.h>
#include <exhash.h>
#include <sys_unix.h>
/*---------------------------Externs------------------------------------*/
extern NDRX_API long G_libatmisrv_flags; /* present in integra.c or standard.c */
extern NDRX_API int G_atmisrv_reply_type; /* ATMI server return value (no long jump) */
/*---------------------------Macros-------------------------------------*/
#define MIN_SVC_LIST_LEN        30
#define SVN_LIST_REALLOC        15

#define RETURN_FAILED             0x00000001
#define RETURN_TYPE_TPRETURN      0x00000002
#define RETURN_TYPE_TPFORWARD     0x00000004
#define RETURN_SVC_FAIL           0x00000008
#define RETURN_TYPE_THREAD        0x00000010  /* processing sent to thread   */

/* Special queue logical numbers */
#define ATMI_SRV_ADMIN_Q            0           /* This is admin queue */
#define ATMI_SRV_REPLY_Q            1           /* This is reply queue */
#define ATMI_SRV_Q_ADJUST           2           /* Adjustment for Q nr */
    
#define ATMI_SRVLIB_NOLONGJUMP     0x00000001 /* Do not use long jump   */

#ifndef EPOLLEXCLUSIVE

#define EPOLLEXCLUSIVE (1 << 28)

#endif

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

   
/**
 * Service name/alias entry.
 */
typedef struct svc_entry svc_entry_t;
struct svc_entry
{
    char svc_nm[XATMI_SERVICE_NAME_LENGTH+1];
    char svc_alias[XATMI_SERVICE_NAME_LENGTH+1];
    svc_entry_t *next, *prev;
};

/**
 * Hash of buffer conversion functions.
 */
typedef struct xbufcvt_entry xbufcvt_entry_t;
struct xbufcvt_entry
{
    char fn_nm[XATMI_SERVICE_NAME_LENGTH+1]; /* function name */
    long xcvtflags; /* Conversion function */
    EX_hash_handle hh;         /* makes this structure hashable */
};

/**
 * Service entry descriptor.
 */
typedef struct svc_entry_fn svc_entry_fn_t;
struct svc_entry_fn
{
    char svc_nm[XATMI_SERVICE_NAME_LENGTH+1]; /* service name */
    char fn_nm[XATMI_SERVICE_NAME_LENGTH+1]; /* function name */
    void (*p_func)(TPSVCINFO *);
    /* listing support */
    svc_entry_fn_t *next, *prev;
    char listen_q[FILENAME_MAX+1]; /* queue on which to listen */
    int is_admin;
    mqd_t q_descr; /* queue descriptor */
    ndrx_stopwatch_t qopen_time;
    long xcvtflags; /* Conversion function */
};

/*
 * Basic call info
 */
struct basic_call_info
{
    char *buf_ptr;
    long len;
    int no;
};
typedef struct basic_call_info call_basic_info_t;

/*
 * Basic server configuration.
 */
struct srv_conf
{
    char binary_name[MAXTIDENT+1];
    int srv_id;
    char err_output[FILENAME_MAX+1];
    int log_work;
    int advertise_all;
    svc_entry_t *svc_list;
    char q_prefix[FILENAME_MAX+1];
    /* Arguments passed after -- */
    int app_argc;
    char **app_argv;
    
    /*<THESE LISTS ARE USED ONLY TILL SERVER GOES ONLINE, STATIC INIT>*/
    svc_entry_fn_t *service_raw_list; /* As from initialization */
    svc_entry_fn_t **service_array; /* Direct array of items */
    /*</THESE LISTS ARE USED ONLY TILL SERVER GOES ONLINE, STATIC INIT>*/
    
    svc_entry_fn_t *service_list; /* Final list used for processing */
    
    int adv_service_count; /* advertised service count. */
    int flags; /* Special flags of the server (see: ndrxdcmn.h:SRV_KEY_FLAGS_BRIDGE) */
    int nodeid; /* Other node id of the bridge */
    int (*p_qmsg)(char *buf, int len, char msg_type); /* Q message processor for bridge */
    /**************** POLLING *****************/
    struct ndrx_epoll_event *events;
    int epollfd;
    /* Information from last call */
    call_basic_info_t last_call;
    jmp_buf call_ret_env;
    int time_out;
    int max_events; /* Max epoll events. */
    
    /* Periodic callback */
    int (*p_periodcb)(void);
    int periodcb_sec; /* Number of seconds for each cb call */
    
    /* Callback used before server goes in poll state */
    int (*p_b4pollcb)(void);
    xbufcvt_entry_t *xbufcvt_tab; /* string hashlist for buffer convert funcs */
};

typedef struct srv_conf srv_conf_t;


/**
 * Server multi threading, context switching
 */
struct server_ctx_info
{
    tp_conversation_control_t G_accepted_connection;
    tp_command_call_t         G_last_call;
    int                       is_in_global_tx;  /* Running in global tx      */
    TPTRANID                  tranid;           /* Transaction ID  (if used) */
};
typedef struct server_ctx_info server_ctx_info_t;

/*---------------------------Globals------------------------------------*/
extern NDRX_API srv_conf_t G_server_conf;
extern NDRX_API shm_srvinfo_t *G_shm_srv;
extern NDRX_API pollextension_rec_t *G_pollext;
extern NDRX_API int G_shutdown_req;
extern NDRX_API int G_shutdown_nr_wait;   /* Number of self shutdown messages to wait */
extern NDRX_API int G_shutdown_nr_got;    /* Number of self shutdown messages got  */
extern NDRX_API void (*___G_test_delayed_startup)(void);

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int sv_open_queue(void);
extern NDRX_API int sv_wait_for_request(void);
extern NDRX_API int unadvertse_to_ndrxd(char *srvcnm);

/* Server specific functions: */
extern NDRX_API void _tpreturn (int rval, long rcode, char *data, long len, long flags);
extern NDRX_API void _tpforward (char *svc, char *data,
                long len, long flags);
extern NDRX_API void _tpcontinue (void);

/* ndrd api */
extern NDRX_API int advertse_to_ndrxd(svc_entry_fn_t *entry);
extern NDRX_API int advertse_to_ndrxd(svc_entry_fn_t *entry);
extern NDRX_API int report_to_ndrxd(void);
extern NDRX_API void ndrx_set_report_to_ndrxd_cb(int (*report_to_ndrxd_callback) (void));
/* Return list of connected bridge nodes. */
extern NDRX_API int ndrxd_get_bridges(char *nodes_out);
extern NDRX_API int pingrsp_to_ndrxd(command_srvping_t *ping);
    
/* Advertise & unadvertise */
extern NDRX_API int dynamic_unadvertise(char *svcname, int *found, svc_entry_fn_t *copy);
extern NDRX_API int	dynamic_advertise(svc_entry_fn_t *entry_new, 
                    char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm);
/* We want to re-advertise the stuff, this could be used for race conditions! */
extern NDRX_API int dynamic_readvertise(char *svcname);

/* Polling extension */
extern NDRX_API pollextension_rec_t * ext_find_poller(int fd);
extern NDRX_API int _tpext_addpollerfd(int fd, uint32_t events, 
        void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1));
extern NDRX_API int _tpext_delpollerfd(int fd);
extern NDRX_API int _tpext_addperiodcb(int secs, int (*p_periodcb)(void));
extern NDRX_API int _tpext_delperiodcb(void);
extern NDRX_API int _tpext_addb4pollcb(int (*p_b4pollcb)(void));
extern NDRX_API int _tpext_delb4pollcb(void);
extern NDRX_API int process_admin_req(char *buf, long len, 
        int *shutdown_req);

/* auto buffer convert: */
extern NDRX_API long xcvt_lookup(char *fn_nm);

extern NDRX_API int build_advertise_list(void);
extern NDRX_API int initialize_atmi_library(void);
extern NDRX_API void un_initialize(void);

extern NDRX_API int array_remove_element(void *arr, int elem, int len, int sz);

#ifdef	__cplusplus
}
#endif

#endif	/* SRV_INT_H */

