/* 
** XA Enduro/X Common header
**
** @file xa_cmn.h
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
#ifndef XA_CMN
#define	XA_CMN

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <sys_mqueue.h>
#include <xa.h>
    
#include <ndrxdcmn.h>
#include <stdint.h>
#include <ntimer.h>
#include <uthash.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* Internal XA commands */
/* buffer usage: */
#define ATMI_XA_TPBEGIN             'b' /* Begin global transaction             */
#define ATMI_XA_TPCOMMIT            'c' /* Commit global transaction            */
#define ATMI_XA_TPABORT             'a' /* Abort global transaction             */
    
#define ATMI_XA_TPJOIN              'j' /* Process is joining to txn, under new RM
                                        or process doesn't know that resorce is involed */
#define ATMI_XA_PRINTTRANS          'p' /* Print transactions to admin pt       */
#define ATMI_XA_ABORTTRANS          'r' /* Abo[r]t transaction, admin util      */
#define ATMI_XA_COMMITTRANS          'm' /* Co[m]mit transaction, admin util     */
    
/* Register new resource handler - remote process sends us info about working under same TXN */
#define ATMI_XA_TMREGISTER          'R' /* Register new resource under txn...   */
#define ATMI_XA_TMPREPARE           'P' /* Sends prepare statement to slave RM  */    
#define ATMI_XA_TMCOMMIT            'C' /* Sends commit to remove RM            */
#define ATMI_XA_TMABORT             'A' /* Master TM sends us Abort local tx    */
    
/* Transaction status per RM */
#define XA_RM_STATUS_NULL           0  /* NULL                                  */
#define XA_RM_STATUS_NONE           'n' /* Non transaction                      */
#define XA_RM_STATUS_IDLE           'i' /* Idle state, according to book        */
#define XA_RM_STATUS_ACTIVE         'j' /* RM is in joined state, book: atctive */
#define XA_RM_STATUS_PREP           'p' /* RM is in prepared state              */
#define XA_RM_STATUS_ABORTED        'a' /* RM is in abort state                 */
#define XA_RM_STATUS_ABORT_HEURIS   'b' /* Aborted houristically                */
#define XA_RM_STATUS_ABORT_HAZARD   'd' /* Aborted, hazard                      */
#define XA_RM_STATUS_COMMITTED      'c' /* Committed                            */
#define XA_RM_STATUS_COMMITTED_RO   'r' /* Committed, was read only             */
#define XA_RM_STATUS_COMMIT_HEURIS  'h' /* Committed, Heuristically             */
#define XA_RM_STATUS_COMMIT_HAZARD  'z' /* Hazrad, committed or aborted         */
    
/* Transaction Stages */
/* The lowest number of RM outcomes, denotes the more exact Result */
#define XA_TX_STAGE_NULL                     0   /* Transaction does not exists */
#define XA_TX_STAGE_ACTIVE                   5   /* Transaction is in active processing */
/* Abort base: */
#define XA_TX_STAGE_ABORTING                 20   /* Aborting                   */
#define XA_TX_STAGE_ABORTED_HAZARD           25   /* Abort, Hazard              */
#define XA_TX_STAGE_ABORTED_HEURIS           30   /* Aborted, Heuristically     */
#define XA_TX_STAGE_ABORTED                  35   /* Finished ok                */

/* Entered in preparing stage, with possiblity to fall back to Abort... */
#define XA_TX_STAGE_PREPARING                40   /* Doing prepare              */
    
/* Commit base 
 * TODO: Might think, if first commit fails (with no TX), then we still migth roll back
 * all the stuff.
 */
#define XA_TX_STAGE_COMMITTING               50   /* Prepared                   */
#define XA_TX_STAGE_COMMITTED_HAZARD         55   /* Commit, hazard             */
#define XA_TX_STAGE_COMMITTED_HEURIS         65   /* Commit Heuristically       */
#define XA_TX_STAGE_COMMITTED                70   /* Commit OK                  */
    
#define XA_TX_STAGE_MAX_NEVER                100  /* Upper never stage          */

#define XA_TX_COPY(X,Y)\
    X->tmtxflags = Y->tmtxflags;\
    strcpy(X->tmxid, Y->tmxid);\
    X->tmrmid = Y->tmrmid;\
    X->tmnodeid = Y->tmnodeid;\
    X->tmsrvid = Y->tmsrvid;\
    strcpy(X->tmknownrms, Y->tmknownrms);
    
#define XA_TX_ZERO(X)\
    X->tmtxflags = 0;\
    X->tmxid[0] = EOS;\
    X->tmrmid = 0;\
    X->tmnodeid = 0;\
    X->tmsrvid = 0;\
    X->tmknownrms[0] = EOS;
    
    
#define TMTXFLAGS_DYNAMIC_REG      0x00000001  /* TX initiator uses dyanmic reg */
#define TMTXFLAGS_RMIDKNOWN        0x00000002  /* RMID already registered       */
    
#define XA_OP_NOP                       0
#define XA_OP_START                     1
#define XA_OP_END                       2
#define XA_OP_PREPARE                   3
#define XA_OP_COMMIT                    4
#define XA_OP_ROLLBACK                  5
#define XA_OP_FORGET                    6
#define XA_OP_OPEN                      7
#define XA_OP_RECOVER                   8
#define XA_OP_CLOSE                     9
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/*
 * Hash entry of call descriptor related with tx
 */
struct atmi_xa_tx_cd
{
    int cd;
    UT_hash_handle hh;         /* makes this structure hashable */
};

typedef struct atmi_xa_tx_cd atmi_xa_tx_cd_t;
    
struct atmi_xa_tx_info
{
    ATMI_XA_TX_INFO_FIELDS;
    
    
    int is_tx_suspended; /* Is current transaction suspended?   */
    int is_tx_initiator; /* Are current process transaction intiator? */
    int is_ax_reg_called; /* Have work done, needs xa_end()! */
    
    atmi_xa_tx_cd_t *call_cds;  /* hash list of call descriptors involved in tx 
                                 * (checked for commit/abort/tpreturn) */
    atmi_xa_tx_cd_t *conv_cds;  /* hash list of conversation open */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct atmi_xa_tx_info atmi_xa_tx_info_t;

/* Current thread transaction info block */
struct atmi_xa_curtx
{
    int is_xa_open;      /* Is xa_open for current thread           */
    atmi_xa_tx_info_t *tx_tab; /* transaction descriptors, table   */
    
    /* TODO: We should have hash list here with open transactions
     * And we should have a pointer to current transaction.
     * Hash is done by transaction id.
     * This is needed for suspend/resume purposes, so that we suspen one
     * and continue with other.
     * is_in_tx, is_tx_suspended, is_tx_initiator should be moved to txinfo!!!
     */
    atmi_xa_tx_info_t *txinfo; /* we need a ptr to current transaction */
    
};
typedef struct atmi_xa_curtx atmi_xa_curtx_t;

/**
 * Resource monitor status during the prepare-commit phase
 */
struct atmi_xa_rm_status
{
    char rmstatus; /* RM=1 index is 0 */
    int  rmerrorcode; /* ATMI error code */
    short  rmreason; /* Reason code of RM */
};
typedef struct atmi_xa_rm_status atmi_xa_rm_status_t;

/**
 * TM's journal about transaction
 * This structure should be hashable.
 */
struct atmi_xa_log
{
#if 0
    /*  transaction id: */
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    short tmrmid; /* initial resource manager id */
    short tmnodeid; /* initial node id */
    short tmsrvid; /* initial TM server id */
#endif
    ATMI_XA_TX_INFO_FIELDS; /* tmknownrms not used!!!  */

    /* Log the date & time with transaction is open*/
    unsigned long long t_start; /* when tx started */
    unsigned long long t_update; /* wehn tx updated (last) */
    
    short   txstage;  /* In what state we are */
    
    /* the list of RMs (the ID is index) statuses.
     * 0x0 indicates that RM is not in use.
     */
    atmi_xa_rm_status_t rmstatus[NDRX_MAX_RMS]; /* RM=1 index is 0 */
    
    char fname[PATH_MAX+1]; /* Full file name of the transaction log file */
    FILE *f; /* the transaction file descriptor (where stuff is logged) */
    
    /* background processing: */
    long trycount;       /* Number of attempts */
    /* Have a timer for active transaction (to watch for time-outs)  */
    ndrx_timer_t ttimer;   /* transaction timer */
    long txtout;  /* Number of seconds for timeout */
    
    int is_background;  /* Is background responsible for tx? */
    uint64_t    lockthreadid;   /* Thread ID, locked the log entry */
    
    UT_hash_handle hh;         /* makes this structure hashable */
};
typedef struct atmi_xa_log atmi_xa_log_t;


typedef struct atmi_xa_log_list atmi_xa_log_list_t;
struct atmi_xa_log_list
{
    atmi_xa_log_t p_tl;/* copy of transaction */
    /* Linked list 
    atmi_xa_log_dl_t *prev;*/
    atmi_xa_log_list_t *next;
};

/**
 * XA State (Global transaction & branch) driving
 */
struct rmstate_driver
{
    short   txstage;      /* Global transaction stage          */
    char    rmstatus;     /* Current RM status, 1              */
    int     op;           /* XA Operation                      */
 
    int     min_retcode;  /* Return code of XA function        */
    int     max_retcode;  /* Return code of XA function        */
    char    next_rmstatus;/* New resource manager status       */
    short   next_txstage; /* Vote next global transaction stage*/
};
typedef struct rmstate_driver rmstatus_driver_t;

/**
 * List of operations to do for particular tx state/rm status:
 */
struct txaction_driver
{
    short   txstage;      /* Global transaction stage          */
    char    rmstatus;     /* Current RM status, 1              */
    int     op;           /* XA Operation                      */
};
typedef struct txaction_driver txaction_driver_t;

/**
 * State descriptors
 */
struct txstate_descriptor
{
    short   txstage;          /* Global transaction stage            */
    short   txs_stage_min;    /* minimums state to say at that level */
    short   txs_min_complete; /* minimum complete stage              */
    short   txs_max_complete; /* maximum complete stage              */
    char    descr[64];        /* stage description                   */
    int     allow_jump;       /* Allow jump to different group       */
};
typedef struct txstate_descriptor txstage_descriptor_t;


struct txstate2tperrno
{
    short   txstage;          /* Global transaction stage            */
    int     master_op;        /* Master operation (commit or abort)  */
    int     tpe;              /* tperrno */
};
typedef struct txstate2tperrno txstate2tperrno_t;


/*---------------------------Globals------------------------------------*/
extern NDRX_API atmi_xa_curtx_t __thread G_atmi_xa_curtx;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
    
/* ATMI XA lib */
extern NDRX_API int atmi_xa_init(void);
extern NDRX_API void atmi_xa_uninit(void);
extern NDRX_API int atmi_xa_open_entry(void);
extern NDRX_API int atmi_xa_close_entry(void);
extern NDRX_API int atmi_xa_start_entry(XID *xid, long flags);
extern NDRX_API int atmi_xa_end_entry(XID *xid);
extern NDRX_API int atmi_xa_rollback_entry(XID *xid, long flags);
extern NDRX_API int atmi_xa_prepare_entry(XID *xid, long flags);
extern NDRX_API int atmi_xa_commit_entry(XID *xid, long flags);
extern NDRX_API UBFH* atmi_xa_call_tm_generic(char cmd, int call_any, short rmid,
                    atmi_xa_tx_info_t *p_xai);
extern NDRX_API UBFH* atmi_xa_call_tm_generic_fb(char cmd, char *svcnm_spec, int call_any, short rmid, 
        atmi_xa_tx_info_t *p_xai, UBFH *p_ub);

/* interface to ATMI lib/utils */
extern NDRX_API char * atmi_xa_serialize_xid(XID *xid, char *xid_str_out);
extern NDRX_API void atmi_xa_xid_get_info(XID *xid, short *p_nodeid, short *p_srvid);
extern NDRX_API void atmi_xa_xid_str_get_info(char *xid_str, short *p_nodeid, short *p_srvid);
extern NDRX_API XID* atmi_xa_deserialize_xid(unsigned char *xid_str, XID *xid_out);
extern NDRX_API int atmi_xa_load_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);
extern NDRX_API void atmi_xa_print_knownrms(int dbglev, char *msg, char *tmknownrms);
extern NDRX_API int atmi_xa_update_known_rms(char *dst_tmknownrms, char *src_tmknownrms);
extern NDRX_API int atmi_xa_is_current_rm_known(char *tmknownrms);
extern NDRX_API void atmi_xa_curtx_del(atmi_xa_tx_info_t *p_txinfo);

extern NDRX_API UBFH * atmi_xa_alloc_tm_call(char cmd);
extern NDRX_API int atmi_xa_set_curtx_from_tm(UBFH *p_ub);
extern NDRX_API int atmi_xa_set_curtx_from_xai(atmi_xa_tx_info_t *p_xai);
extern NDRX_API int atmi_xa_curtx_set_cur_rmid(atmi_xa_tx_info_t *p_xai);
extern NDRX_API void atmi_xa_reset_curtx(void);
extern NDRX_API void atmi_xa_print_knownrms(int dbglev, char *msg, char *tmknownrms);
extern NDRX_API int atmi_xa_read_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai);
extern NDRX_API XID* atmi_xa_get_branch_xid(atmi_xa_tx_info_t *p_xai);
extern NDRX_API void atmi_xa_cpy_xai_to_call(tp_command_call_t *call, atmi_xa_tx_info_t *p_xai);

/* CD registration with transaction: */
extern NDRX_API int atmi_xa_cd_reg(atmi_xa_tx_cd_t **cds, int cd);
extern NDRX_API atmi_xa_tx_cd_t * atmi_xa_cd_find(atmi_xa_tx_cd_t **cds, int cd);
extern NDRX_API int atmi_xa_cd_isanyreg(atmi_xa_tx_cd_t **cds);
extern NDRX_API void atmi_xa_cd_unreg(atmi_xa_tx_cd_t **cds, int cd);
extern NDRX_API int atmi_xa_cd_unregall(atmi_xa_tx_cd_t **cds);

/* API sections */
extern NDRX_API int _tpopen(void);
extern NDRX_API int _tpclose(void);
extern NDRX_API int _tpbegin(unsigned long timeout, long flags);
extern NDRX_API int _tpcommit(long flags);
extern NDRX_API int _tpabort(long flags);
extern NDRX_API int _tpsuspend (TPTRANID *tranid, long flags);
extern NDRX_API int _tpresume (TPTRANID *tranid, long flags);

extern NDRX_API int _tp_srv_join_or_new_from_call(tp_command_call_t *call, int is_ax_reg_callback);
extern NDRX_API int _tp_srv_join_or_new(atmi_xa_tx_info_t *p_xai, int is_ax_reg_callback,
                    int *p_is_known);
extern NDRX_API int _tp_srv_disassoc_tx(void);
extern NDRX_API int _tp_srv_tell_tx_fail(void);

/* State driving */
extern NDRX_API rmstatus_driver_t* xa_status_get_next_by_op(short txstage, char rmstatus, 
                                                    int op, int op_retcode);
extern NDRX_API rmstatus_driver_t* xa_status_get_next_by_new_status(short   txstage, 
                                                    char next_rmstatus);
extern NDRX_API int xa_status_get_op(short txstage, char rmstatus);
extern NDRX_API txstage_descriptor_t* xa_stage_get_descr(short txstage);
extern NDRX_API int xa_txstage2tperrno(short txstage, int master_op);


extern NDRX_API atmi_xa_curtx_t *ndrx_get_G_atmi_xa_curtx(void);

#ifdef	__cplusplus
}
#endif

#endif	/* XA_CMN */

