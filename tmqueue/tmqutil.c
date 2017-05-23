/* 
** Q util
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

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include <xa_cmn.h>
#include <atmi_int.h>
#include <exuuid.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Extract info from msgid.
 * 
 * @param msgid_in
 * @param p_nodeid
 * @param p_srvid
 */
public void tmq_msgid_get_info(char *msgid, short *p_nodeid, short *p_srvid)
{
    *p_nodeid = 0;
    *p_srvid = 0;
    
    memcpy((char *)p_nodeid, msgid+sizeof(exuuid_t), sizeof(short));
    memcpy((char *)p_srvid, msgid+sizeof(exuuid_t)+sizeof(short), sizeof(short));
    
    NDRX_LOG(log_info, "Extracted nodeid=%hd srvid=%hd", 
            *p_nodeid, *p_srvid);
}

/**
 * Generate serialized version of the string
 * @param msgid_in, length defined by constant TMMSGIDLEN
 * @param msgidstr_out
 * @return msgidstr_out
 */
public char * tmq_corid_serialize(char *corid_in, char *corid_str_out)
{
    size_t out_len;
    
    NDRX_DUMP(log_debug, "Original CORID", corid_in, TMCORRIDLEN_STR);
    
    atmi_xa_base64_encode((unsigned char *)corid_in, TMCORRIDLEN_STR, &out_len, corid_str_out);

    corid_str_out[out_len] = EOS;
    
    NDRX_LOG(log_debug, "CORID after serialize: [%s]", corid_str_out);
    
    return corid_str_out;
}
