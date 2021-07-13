/**
 * @brief Transaction support for queue
 *
 * @file qtran.h
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

#ifndef QTRAN_H
#define	QTRAN_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa_cmn.h>
#include <atmi.h>
#include <utlist.h>
#include <exhash.h>
#include <exthpool.h>
#include "tmqueue.h"
    
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Command block entry in the log
 */    
typedef struct qtran_log_cmd qtran_log_cmd_t;
struct qtran_log_cmd
{
    char command_code;          /**< Command code                          */
    int  seqno;                 /**< Command sequence number               */
    tmq_msg_t *p_msg;           /**< message pointer                       */
    
    char cmd_status;            /**< status according to XA_RM_STATUS*     */
    
    union tmq_upd_block b;            /**< Update block (largest metadata store  */
        
    qtran_log_cmd_t *prev, *next;   /**< DL list of locked / related msgs  */
};

/**
 * Queue transaction support
 */
struct qtran_log
{
     char tmxid[NDRX_XID_SERIAL_BUFSIZE+1]; /**< tmxid, serialized */
     
    /* Log the date & time with transaction is open                     */
    unsigned long long t_start;     /**< when tx started                */
    unsigned long long t_update;    /**< wehn tx updated (last)         */
    
    short   txstage;  /**< In what state we are                         */
    
    /* background processing: */
    long trycount;              /**< Number of attempts                 */
    
    /* Have a timer for active transaction (to watch for time-outs)     */
    ndrx_stopwatch_t ttimer;    /**< transaction timer */
    
    int is_background;          /**< Is background responsible for tx?  */
    uint64_t    lockthreadid;   /**< Thread ID, locked the log entry    */
    
    
    int seqno;                  /**< Sequence counter                   */
    int is_abort_only;          /**< Is transaction broken?             */
    
    qtran_log_cmd_t *cmds;      /**< List of queue commands within tran */
    
    EX_hash_handle hh;          /**< makes this structure hashable      */
};
typedef struct qtran_log qtran_log_t;


/**
 * Transaction copy list
 */
typedef struct qtran_log_list qtran_log_list_t;
struct qtran_log_list
{
    qtran_log_t p_tl;/* copy of transaction */
    qtran_log_list_t *next;
};


/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

extern qtran_log_t * tmq_log_get_entry(char *tmxid, int dowait, int *locke);
extern int tmq_log_start(char *tmxid);
extern int tmq_log_addcmd(char *tmxid, int seqno, union tmq_upd_block *b, tmq_msg_t * p_msg,
        char entry_status);
extern void tmq_remove_logfree(qtran_log_t *p_tl, int hash_rm);
extern qtran_log_list_t* tmq_copy_hash2list(int copy_mode);
extern void tmq_tx_hash_lock(void);
extern void tmq_tx_hash_unlock(void);
extern int tmq_log_unlock(qtran_log_t *p_tl);
extern int tmq_log_next(qtran_log_t *p_tl);

#ifdef	__cplusplus
}
#endif

#endif	/* QTRAN_H */

/* vim: set ts=4 sw=4 et smartindent: */
