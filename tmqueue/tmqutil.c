/* 
** Q util
**
** @file xasrvutil.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include <uuid/uuid.h>
#include <xa_cmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
MUTEX_LOCKDECL(M_msgid_gen_lock); /* Thread locking for xid generation  */
/*---------------------------Prototypes---------------------------------*/

/**
 * Setup queue header
 * @param hdr header to setup
 * @param qname queue name
 */
public int tmq_setup_cmdheader_newmsg(tmq_cmdheader_t *hdr, char *qname, 
        short srvid, short nodeid)
{
    int ret = SUCCEED;
    
    strcpy(hdr->qname, qname);
    hdr->command_code = TMQ_CMD_NEWMSG;
    strncpy(hdr->magic, TMQ_MAGIC, TMQ_MAGIC_LEN);
    hdr->nodeid = nodeid;
    hdr->srvid = srvid;
    tmq_msgid_gen(hdr->msgid);
    
out:
    return ret;
}


/**
 * Generate new transaction id, native form (byte array)
 * Note this initializes the msgid.
 * @param msgid value to return
 */
public void tmq_msgid_gen(char *msgid)
{
    uuid_t uuid_val;
    short node_id = (short) G_atmi_env.our_nodeid;
    short srv_id = (short) G_srv_id;
   
    memset(msgid, 0, TMMSGIDLEN);
    
    /* Do the locking, so that we get unique xids... */
    MUTEX_LOCK_V(M_msgid_gen_lock);
    uuid_generate(uuid_val);
    MUTEX_UNLOCK_V(M_msgid_gen_lock);
    
    memcpy(msgid, uuid_val, sizeof(uuid_t));
    /* Have an additional infos for transaction id... */
    memcpy(msgid  
            +sizeof(uuid_t)  
            ,(char *)&(node_id), sizeof(short));
    memcpy(msgid  
            +sizeof(uuid_t) 
            +sizeof(short)
            ,(char *)&(srv_id), sizeof(short));    
    
    NDRX_LOG(log_error, "MSGID: struct size: %d", sizeof(uuid_t)+sizeof(short)+ sizeof(short));
}

/**
 * Generate serialized version of the string
 * @param msgid_in, length defined by constant TMMSGIDLEN
 * @param msgidstr_out
 * @return msgidstr_out
 */
public char * tmq_msgid_serialize(char *msgid_in, char *msgid_str_out)
{
    size_t out_len;
    
    NDRX_DUMP(log_debug, "Original MSGID", msgid_in, TMMSGIDLEN);
    
    atmi_xa_base64_encode(msgid_in, strlen(msgid_in), &out_len, msgid_str_out);
    msgid_str_out[out_len] = EOS;
    
    NDRX_LOG(log_debug, "MSGID after serialize: [%s]", msgid_str_out);
    
    return msgid_str_out;
}

/**
 * Get binary message id
 * @param msgid_str_in, length defined by constant TMMSGIDLEN
 * @param msgid_out
 * @return msgid_out 
 */
public char * tmq_msgid_deserialize(char *msgid_str_in, char *msgid_out)
{
    size_t tot_len;
    
    NDRX_LOG(log_debug, "Serialized MSGID: [%s]", msgid_str_in);
    
    memset(msgid_out, 0, TMMSGIDLEN);
        
    atmi_xa_base64_decode(msgid_str_in, strlen(msgid_str_in), &tot_len, msgid_out);
    
    NDRX_DUMP(log_debug, "Deserialized MSGID", msgid_out, TMMSGIDLEN);
    
    return msgid_out;
}
