/**
 * @brief Q util
 *
 * @file tmqutil.c
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
#include <exuuid.h>
#include <exbase64.h>
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
expublic char * tmq_corid_serialize(char *corid_in, char *corid_str_out)
{
    size_t out_len = 0;
    
    NDRX_DUMP(log_debug, "Original CORID", corid_in, TMCORRIDLEN);
    
    ndrx_xa_base64_encode((unsigned char *)corid_in, TMCORRIDLEN, &out_len, 
            corid_str_out);

    /* corid_str_out[out_len] = EXEOS; */
    
    NDRX_LOG(log_debug, "CORID after serialize: [%s]", corid_str_out);
    
    return corid_str_out;
}

/**
 * Finalize file commands (before notifications unlock q space)
 * Also only one type of ops are supported either batch remove or batch rename
 * @param p_ub UBF 
 * @return 
 */
expublic int tmq_finalize_files(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    BFLDOCC occ, occs = Boccur(p_ub, EX_QFILECMD);
    char cmd;
    char *fname1, *fname2;
    
    /* rename -> mandatory. Also if on remove file does not exists, this is the
     *  same as removed OK
     * first unlink -> mandatory
     * second unlink -> optional 
     */
    
    for (occ=0; occ<occs; occ++)
    {
        if (EXSUCCEED!=Bget(p_ub, EX_QFILECMD, occ, &cmd, NULL))
        {
            NDRX_LOG(log_error, "Failed to get EX_QFILECMD at occ %d: %s", 
                    occ, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        fname1 = Bfind(p_ub, EX_QFILENAME1, occ, NULL);

        if (NULL==fname1)
        {
            NDRX_LOG(log_error, "Failed to find EX_QFILENAME1 occ %d: %s", 
                    occ, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        if (TMQ_FILECMD_UNLINK==cmd)
        {            
            NDRX_LOG(log_debug, "Unlinking file [%s]", fname1);

            if (EXSUCCEED!=unlink(fname1))
            {
                if (ENOENT!=errno)
                {
                    int err = errno;
                    NDRX_LOG(log_error, "Failed to unlinking file [%s] occ %d: %s", 
                            fname1, occ, strerror(err));
                    userlog("Failed to unlinking file [%s] occ %d: %s", 
                            fname1, occ, strerror(err));
                    
                    if (0==occ)
                    {
                        /* this is critical file */
                        EXFAIL_OUT(ret);
                    }
                    
                }
            }
        }
        else if (TMQ_FILECMD_RENAME==cmd)
        {
            fname2 = Bfind(p_ub, EX_QFILENAME2, occ, NULL);
            
            if (NULL==fname1)
            {
                NDRX_LOG(log_error, "Failed to find EX_QFILENAME2 occ %d: %s", 
                        occ, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            NDRX_LOG(log_debug, "About to rename: [%s] -> [%s]",
                    fname1, fname2);
            if (EXSUCCEED!=rename(fname1, fname2))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to rename file [%s] -> [%s] occ %d: %s", 
                        fname1, fname2, occ, strerror(err));
                userlog("Failed to rename file [%s] -> [%s] occ %d: %s", 
                        fname1, fname2, occ, strerror(err));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Unsupported file command %c", cmd);
            EXFAIL_OUT(ret);   
        }
    }
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
