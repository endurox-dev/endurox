/* 
** Tmsrv server - transaction monitor, XA util part
**
** @file xasrvutil.c
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

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include <exuuid.h>
#include <xa_cmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
MUTEX_LOCKDECL(M_xid_gen_lock); /* Thread locking for xid generation    */
/*---------------------------Prototypes---------------------------------*/

/**
 * Generate new transaction id
 * @param xid
 */
expublic void atmi_xa_new_xid(XID *xid)
{
    exuuid_t uuid_val;
    atmi_lib_env_t *atmi_env = ndrx_get_G_atmi_env();
    unsigned char rmid =  (unsigned char)atmi_env->xa_rmid;
    short node_id = (short) atmi_env->our_nodeid;
    short srv_id = (short) G_srv_id;
    
    /* Do the locking, so that we get unique xids... */
    MUTEX_LOCK_V(M_xid_gen_lock);
   
    /* we will include here following additional data:
     * - cluster node_id
     * - endurox server_id
     */
    
    xid->formatID = NDRX_XID_FORMAT_ID;
    xid->gtrid_length = sizeof(exuuid_t);
    xid->bqual_length = sizeof(unsigned char) + sizeof(short) + sizeof(short);
    memset(xid->data, 0, XIDDATASIZE); /* this is not necessary, but... */
    exuuid_generate(uuid_val);
    memcpy(xid->data, uuid_val, sizeof(exuuid_t));
    memcpy(xid->data + sizeof(exuuid_t), &(atmi_env->xa_rmid), sizeof(unsigned char));
    /* Have an additional infos for transaction id... */
    memcpy(xid->data  
            +sizeof(exuuid_t)  
            +sizeof(unsigned char)
            ,(char *)&(node_id), sizeof(short));
    memcpy(xid->data  
            +sizeof(exuuid_t) 
            +sizeof(unsigned char)
            +sizeof(short)
            ,(char *)&(srv_id), sizeof(short));
    
    MUTEX_UNLOCK_V(M_xid_gen_lock);
}

