/**
 * @brief Q for EnduroX, shared header between XA driver and TMQ server
 *
 * @file tmqueue.h
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

#ifndef TMQUEUE_H
#define	TMQUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa_cmn.h>
#include <atmi.h>
#include <exhash.h>
#include <exthpool.h>
    
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* Basically we have a two forms of MSGID
 * 1. Native, 32 byte binary byte array
 * 2. String, Base64, 
 */
#define TMQ_DEFAULT_Q           "@"         /**< Symbol for default Q       */

#define TMQ_MAGIC               "ETQ2"      /**< magic of tmq record        */
#define TMQ_MAGIC2              "END2"      /**< magic of tmq record, end   */
#define TMQ_MAGIC_LEN           4           /**< the len of message magic   */

#define TMQ_MAGICBASE           "ETQ"       /**< magic without version      */
#define TMQ_MAGICBASE2          "END"       /**< magic without version, end */
#define TMQ_MAGICBASE_LEN       3           /**< magic without version      */
    
#define TMQ_STORCMD_NEWMSG          'N'     /**< Command code - new message */
#define TMQ_STORCMD_UPD             'U'     /**< Command code - update msg  */
#define TMQ_STORCMD_DEL             'D'     /**< Command code - delete msg  */
#define TMQ_STORCMD_UNLOCK          'L'     /**< Command code - unlock msg  */
#define TMQ_STORCMD_DUM             'M'     /**< Command code - dummy msg
                                            for transaction identification  */
    

/**
 * Status codes of the message
 */
#define TMQ_STATUS_ACTIVE       'A'         /**< Message is active          */
#define TMQ_STATUS_DONE         'D'         /**< Message is done            */
#define TMQ_STATUS_EXPIRED      'E'         /**< Message is expired  or tries exceeded  */
#define TMQ_STATUS_SUSPENDED    'S'         /**< Message is suspended       */
    
    
#define TMQ_SYS_ASYNC_CPLT    0x00000001    /**< Complete message in async mode */
    
/**
 * List of tmq specific internal errors
 * @defgroup tmq_errors
 * @{
 */

#define TMQ_ERR_VERSION          1          /**< Version error              */
#define TMQ_ERR_EOF              2          /**< File is truncated          */
#define TMQ_ERR_CORRUPT          3          /**< File contents are corrupted*/
    
/** @} */ /* end of tmq_errors */
    
    
#define TMQ_HOUSEKEEP_DEFAULT   (90*60)     /**< houskeep 1 hour 30 min     */

#define TMQ_INT_DIAG_EJOIN      0x00000001  /**< Got join transaction join error */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Common command header
 */
typedef struct
{
    char magic[4];          /**< File magic   1                             */
    short srvid;
    short nodeid;
    /* TODO: Consider adding rmid, if running different queue spaces in 
     * the same folder
     */
    char qname[TMQNAMELEN+1];
    char qspace[XATMI_SERVICE_NAME_LENGTH+1];
    char command_code;      /**< command code, see TMQ_CMD*                 */
    char msgid[TMMSGIDLEN]; /**< message_id                                 */
    long flags;             /**< Copy of message flags                      */
    char reserved[64];      /**< Reversed space for future upgrades         */
    char magic2[4];         /**< File magic                                 */
} tmq_cmdheader_t;

/** 
 * Command: qmessage 
 */
typedef struct
{
    /** Lets have first 512 bytes of dynamic infos:
     * so that update fits in one sector update, if in future
     * we perform optimizations:
     */
    tmq_cmdheader_t hdr;
    uint64_t lockthreadid;  /**< Locked thread id                           */
    char status;            /**< Status of the message                      */
    long trycounter;        /**< try counter                                */
    short buftyp;           /**< ATMI buffer type id                        */
    long msgtstamp;         /**< epoch up to second                         */
    long msgtstamp_usec;    /**< 1/10^6 sec                                 */
    int msgtstamp_cntr;     /**< Message counter for same time interval     */
    long trytstamp;         /**< epoch up to second                         */
    long trytstamp_usec;    /**< 1/10^6 sec                                 */
    
    TPQCTL qctl;            /**< Queued message                             */
    /* Message log (stored only in file)                                    */
    long len;               /**< msg len                                    */
    char msg[0];            /**< the memory segment for structure shall be large 
                             * enough to handle the message len 
                             * indexed by the array                         */
} tmq_msg_t;

/**
 * Command: delmsg
 */
typedef struct
{
    tmq_cmdheader_t hdr;
} tmq_msg_del_t;

/**
 * Dummy command
 * Transaction marker
 */
typedef struct
{
    tmq_cmdheader_t hdr;
} tmq_msg_dum_t;


/**
 * Command: unlock
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    
} tmq_msg_unl_t;

/** 
 * Command: updcounter
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    char status;   /* Status of the message */
    long trycounter;        /* try counter */
    long trytstamp;
    long trytstamp_usec;
    
} tmq_msg_upd_t;

#define UPD_MSG(DEST, SRC)  NDRX_LOG(log_debug, "status [%c] -> [%c]",\
                    DEST->status, SRC->status);\
            DEST->status = SRC->status;\
            NDRX_LOG(log_debug, "trycounter [%ld] -> [%ld]",\
                    DEST->trycounter, SRC->trycounter);\
            DEST->trycounter = SRC->trycounter;\
            NDRX_LOG(log_debug, "trycounter [%ld] -> [%ld]",\
                    DEST->trytstamp, SRC->trytstamp);\
            DEST->trytstamp = SRC->trytstamp;\
            NDRX_LOG(log_debug, "trycounter_usec [%ld] -> [%ld]",\
                    DEST->trytstamp, SRC->trytstamp);\
            DEST->trytstamp_usec = SRC->trytstamp_usec;

/**
 * Data block
 */
union tmq_block {
    tmq_cmdheader_t hdr;
    tmq_msg_t msg;
    tmq_msg_del_t del;
    tmq_msg_upd_t upd;
    tmq_msg_dum_t dum;
};

/**
 * Update block (either update or delete)
 */
union tmq_upd_block {
    tmq_cmdheader_t hdr;
    tmq_msg_del_t del;
    tmq_msg_upd_t upd;
    tmq_msg_dum_t dum;
};

/*---------------------------Globals------------------------------------*/

extern char ndrx_G_qspace[];    /**< Name of the queue space            */
extern char ndrx_G_qspacesvc[]; /**< real service name                  */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
 
/* util, shared between driver & daemon */
extern int tmq_setup_cmdheader_newmsg(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags);
extern void tmq_msgid_gen(char *msgid);
extern char * tmq_msgid_serialize(char *msgid_in, char *msgid_str_out);
extern char * tmq_msgid_deserialize(char *msgid_str_in, char *msgid_out);
extern void tmq_msgid_get_info(char *msgid, short *p_nodeid, short *p_srvid);
extern char * tmq_corrid_serialize(char *corrid_in, char *corrid_str_out);
extern int tmq_finalize_files(UBFH *p_ub);
extern void tmq_set_tmqueue(
    int setting
    , int (*p_tmq_setup_cmdheader_dum)(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags)
    , int (*p_tmq_dum_add)(char *tmxid)
    , int (*p_tmq_unlock_msg)(union tmq_upd_block *b));
    
/* From storage driver: */
extern size_t tmq_get_block_len(char *data);
extern int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg, int *int_diag);
extern int tmq_storage_write_cmd_block(char *p_block, char *descr, char *cust_tmxid, 
        int *int_diag);
extern int tmq_storage_get_blocks(int (*process_block)(char *tmxid, 
        union tmq_block **p_block, int state, int seqno), short nodeid, short srvid);

/* transaction management: */
extern int ndrx_xa_qminiservce(UBFH *p_ub, char cmd);

extern int tmq_setup_cmdheader_dum(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags);
   
extern int tmq_sort_queues(void);
extern int tmq_lock_msg(char *msgid);

#ifdef	__cplusplus
}
#endif

#endif	/* TMQUEUE_H */

/* vim: set ts=4 sw=4 et smartindent: */
