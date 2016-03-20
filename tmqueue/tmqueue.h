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
#include "thpool.h"
/*---------------------------Externs------------------------------------*/
extern pthread_t G_bacground_thread;
extern int G_bacground_req_shutdown;    /* Is shutdown request? */
/*---------------------------Macros-------------------------------------*/
#define SCAN_TIME_DFLT          10  /* Every 10 sec try to complete TXs */
#define MAX_TRIES_DFTL          100 /* Try count for transaction completion */
#define TOUT_CHECK_TIME         1   /* Check for transaction timeout, sec   */
#define THREADPOOL_DFLT         10   /* Default number of threads spawned   */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*
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

struct thread_server
{
    char *context_data; /* malloced by enduro/x */
    int cd;
    char *buffer; /* buffer data, managed by enduro/x */
};
/* note we must malloc this struct too. */
typedef struct thread_server thread_server_t;


extern tmqueue_cfg_t G_tmqueue_cfg;

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

