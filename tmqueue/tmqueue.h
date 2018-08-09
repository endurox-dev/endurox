/**
 * @brief Q for EnduroX, shared header between XA driver and TMQ server
 *
 * @file tmqueue.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <utlist.h>
#include <exhash.h>
#include "thpool.h"
    
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* Basically we have a two forms of MSGID
 * 1. Native, 32 byte binary byte array
 * 2. String, Base64, 
 */
#define TMQ_DEFAULT_Q           "@" /* Symbol for default Q */

#define TMQ_MAGIC               "ETMQ" /* magic of tmq record      */
#define TMQ_MAGIC_LEN           4      /* the len of message magic */

#define TMQ_STORCMD_NEWMSG          'N'      /* Command code - new message */
#define TMQ_STORCMD_UPD             'U'      /* Command code - update msg  */
#define TMQ_STORCMD_DEL             'D'      /* Command code - delete msg  */
#define TMQ_STORCMD_UNLOCK          'L'      /* Command code - unlock msg  */
    

/**
 * Status codes of the message
 */
#define TMQ_STATUS_ACTIVE       'A'      /* Message is active          */
#define TMQ_STATUS_DONE         'D'      /* Message is done            */
#define TMQ_STATUS_EXPIRED      'E'      /* Message is expired  or tries exceeded  */
#define TMQ_STATUS_SUSPENDED    'S'      /* Message is suspended       */
    
    
#define TMQ_SYS_ASYNC_CPLT    0x00000001    /* Complete message in async mode */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Common command header
 */
typedef struct
{
    char magic[4];          /* File magic                            */
    short srvid;
    short nodeid;
    char qname[TMQNAMELEN+1];
    char qspace[XATMI_SERVICE_NAME_LENGTH+1];
    char command_code;     /* command code, see TMQ_CMD*             */
    char msgid[TMMSGIDLEN]; /* message_id                            */
    long flags;             /* Copy of message flags                 */
} tmq_cmdheader_t;

/** 
 * Command: qmessage 
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    TPQCTL qctl;            /* Queued message */
    uint64_t lockthreadid;  /* Locked thread id */
    char status;            /* Status of the message */
    long trycounter;        /* try counter */
    short buftyp;           /* ATMI buffer type id */
    long msgtstamp;         /* epoch up to second */
    long msgtstamp_usec;    /* 1/10^6 sec */
    int msgtstamp_cntr;     /* Message counter for same time interval */
    long trytstamp;         /* epoch up to second */
    long trytstamp_usec;    /* 1/10^6 sec */
    
    /* Message log (stored only in file) */
    long len;               /* msg len */
    char msg[0];            /* the memory segment for structure shall be large 
                             * enough to handle the message len 
                             * indexed by the array   */
} tmq_msg_t;

/**
 * Command: delmsg
 */
typedef struct
{
    tmq_cmdheader_t hdr;
    
} tmq_msg_del_t;

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
};

/**
 * Update block (either update or delete)
 */
union tmq_upd_block {
    tmq_cmdheader_t hdr;
    tmq_msg_upd_t upd;
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
 
/* util, shared between driver & daemon */
extern int tmq_setup_cmdheader_newmsg(tmq_cmdheader_t *hdr, char *qname, 
        short nodeid, short srvid, char *qspace, long flags);
extern void tmq_msgid_gen(char *msgid);
extern char * tmq_msgid_serialize(char *msgid_in, char *msgid_str_out);
extern char * tmq_msgid_deserialize(char *msgid_str_in, char *msgid_out);
extern void tmq_msgid_get_info(char *msgid, short *p_nodeid, short *p_srvid);
extern char * tmq_corid_serialize(char *corid_in, char *corid_str_out);
    
/* From storage driver: */
extern int tmq_storage_write_cmd_newmsg(tmq_msg_t *msg);
extern int tmq_storage_write_cmd_block(union tmq_block *p_block, char *descr);
extern int tmq_storage_get_blocks(int (*process_block)(union tmq_block **p_block), 
        short nodeid, short srvid);
   
    
#ifdef	__cplusplus
}
#endif

#endif	/* TMQUEUE_H */

/* vim: set ts=4 sw=4 et smartindent: */
