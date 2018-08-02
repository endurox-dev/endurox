/* 
 * @brief Remove message from Qspace
 *
 * @file cmd_mqrm.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include <ndrx.h>
#include <qcommon.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Remove the message from Q
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mqrm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    short srvid;
    short nodeid;
    char msgid_str[TMMSGIDLEN_STR+1];
    char msgid[TMMSGIDLEN+1];
    char *buf = NULL;
    TPQCTL qc;
    long len = 1;
    ncloptmap_t clopt[] =
    {
        {'n', BFLD_SHORT, (char *)&nodeid, 0,
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Q cluster node id"},
        {'i', BFLD_SHORT, (char *)&srvid, 0, 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Q Server Id"},
        {'m', BFLD_STRING, msgid_str, sizeof(msgid_str), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Message id"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    /* Have a number in FB! */
    memset(&qc, 0, sizeof(qc));

    tmq_msgid_deserialize(msgid_str, msgid);
    
    qc.flags|= TPQGETBYMSGID;
    
    memcpy(qc.msgid, msgid, TMMSGIDLEN);
    
    buf = tpalloc("STRING", "", 1);
    
    if (EXSUCCEED!=tpdequeueex(nodeid, srvid, "*N/A*", &qc, (char **)&buf, &len, 0))
    {
        fprintf(stderr, "failed %s diag: %ld:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        NDRX_LOG(log_error, "failed %s diag: %ld:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        
        EXFAIL_OUT(ret);
    }
    
    
    printf("Succeed\n");
    
out:

    if (NULL!=buf)
    {
        tpfree(buf);
    }

    return ret;
}
