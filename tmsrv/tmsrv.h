/* 
** Transaction Monitor for XA
**
** @file tmsrv.h
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

#ifndef TMSRV_H
#define	TMSRV_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa_cmn.h>
#include "thpool.h"
/*---------------------------Externs------------------------------------*/
extern pthread_t G_bacground_thread;
extern int G_bacground_req_shutdown;    /* Is shutdown request? */
/*---------------------------Macros-------------------------------------*/
#define SCAN_TIME_DFLT          10  /* Every 10 sec try to complete TXs */
#define MAX_TRIES_DFTL          100 /* Try count for transaction completion */
#define TOUT_CHECK_TIME         1   /* Check for transaction timeout, sec   */
#define THREADPOOL_DFLT         10   /* Default number of threads spawned   */

#define COPY_MODE_FOREGROUND        0x1       /* Copy foreground elements */
#define COPY_MODE_BACKGROUND        0x2       /* Copy background elements */
#define COPY_MODE_ACQLOCK           0x4       /* Should we do locking?    */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
 * TM ndrx_config.handler
 */
typedef struct
{
    long dflt_timeout; /* how long monitored transaction can be open        */
    char tlog_dir[PATH_MAX]; /* Where to write tx log files                 */
    int scan_time;      /* Number of seconds retries */
    long max_tries;      /* Number of tries for running session for single 
                         * transaction, until stop processing it 
                         * (in this process session) */
    int tout_check_time; /* seconds used for detecting transaction timeout   */
    int threadpoolsize; /* thread pool size */
    
    threadpool thpool;
} tmsrv_cfg_t;

struct thread_server
{
    char *context_data; /* malloced by enduro/x */
    int cd;
    char *buffer; /* buffer data, managed by enduro/x */
};
/* note we must malloc this struct too. */
typedef struct thread_server thread_server_t;


extern tmsrv_cfg_t G_tmsrv_cfg;

extern void atmi_xa_new_xid(XID *xid);

extern int tms_unlock_entry(atmi_xa_log_t *p_tl);
extern atmi_xa_log_t * tms_log_get_entry(char *tmxid);
extern int tms_log_start(atmi_xa_tx_info_t *xai, int txtout, long tmflags);
extern int tms_log_addrm(atmi_xa_tx_info_t *xai, short rmid, int *p_is_already_logged);
extern int tms_open_logfile(atmi_xa_log_t *p_tl, char *mode);
extern int tms_is_logfile_open(atmi_xa_log_t *p_tl);
extern void tms_close_logfile(atmi_xa_log_t *p_tl);
extern void tms_remove_logfile(atmi_xa_log_t *p_tl);
extern int tms_log_info(atmi_xa_log_t *p_tl);
extern int tms_log_stage(atmi_xa_log_t *p_tl, short stage);
extern int tms_log_rmstatus(atmi_xa_log_t *p_tl, short rmid, 
        char rmstatus, int  rmerrorcode, short  rmreason);
extern int tms_load_logfile(char *logfile, char *tmxid, atmi_xa_log_t **pp_tl);
extern int tm_chk_tx_status(atmi_xa_log_t *p_tl);
extern atmi_xa_log_list_t* tms_copy_hash2list(int copy_mode);
extern void tms_tx_hash_lock(void);
extern void tms_tx_hash_unlock(void);
extern int tms_log_cpy_info_to_fb(UBFH *p_ub, atmi_xa_log_t *p_tl);
        
extern int tm_rollback_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);

extern int tm_drive(atmi_xa_tx_info_t *p_xai, atmi_xa_log_t *p_tl, int master_op,
                        short rmid);

/* Prepare API */
extern int tm_prepare_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);
extern int tm_prepare_remote_call(atmi_xa_tx_info_t *p_xai, short rmid);
extern int tm_prepare_combined(atmi_xa_tx_info_t *p_xai, short rmid);

/* Rollback API */
extern int tm_rollback_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);
extern int tm_rollback_remote_call(atmi_xa_tx_info_t *p_xai, short rmid);
extern int tm_rollback_combined(atmi_xa_tx_info_t *p_xai, short rmid);

/* Commit API */
extern int tm_commit_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);
extern int tm_commit_remote_call(atmi_xa_tx_info_t *p_xai, short rmid);
extern int tm_commit_combined(atmi_xa_tx_info_t *p_xai, short rmid);

extern int tm_tpbegin(UBFH *p_ub);
extern int tm_tpcommit(UBFH *p_ub);
extern int tm_tpabort(UBFH *p_ub);

extern int tm_tmprepare(UBFH *p_ub);
extern int tm_tmcommit(UBFH *p_ub);
extern int tm_tmabort(UBFH *p_ub);
extern int tm_tmregister(UBFH *p_ub);

/* Background API */
extern int background_read_log(void);
extern void background_wakeup(void);
extern void background_process_init(void);
extern void background_lock(void);
extern void background_unlock(void);

/* Admin functions */
extern int tm_tpprinttrans(UBFH *p_ub, int cd);
extern int tm_aborttrans(UBFH *p_ub);
extern int tm_committrans(UBFH *p_ub);


#ifdef	__cplusplus
}
#endif

#endif	/* TMSRV_H */

