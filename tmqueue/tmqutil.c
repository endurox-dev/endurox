/**
 * @brief Q util
 *
 * @file tmqutil.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <qcommon.h>

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include <xa_cmn.h>
#include <atmi_int.h>
#include <exbase64.h>
#include <nstdutil.h>
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
expublic void tmq_msgid_get_info(char *msgid, short *p_nodeid, short *p_srvid)
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
expublic char * tmq_corrid_serialize(char *corrid_in, char *corrid_str_out)
{
    size_t out_len = 0;
    
    NDRX_DUMP(log_debug, "Original CORRID", corrid_in, TMCORRIDLEN);
    
    ndrx_xa_base64_encode((unsigned char *)corrid_in, TMCORRIDLEN, &out_len, 
            corrid_str_out);

    /* corrid_str_out[out_len] = EXEOS; */
    
    NDRX_LOG(log_debug, "CORRID after serialize: [%s]", corrid_str_out);
    
    return corrid_str_out;
}

/* vim: set ts=4 sw=4 et smartindent: */
