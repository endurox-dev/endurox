/* 
** Dump the message to stdout.
**
** @file cmd_mqdm.c
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
 * Dump the messsage to stdout.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_mqdm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    short srvid;
    short nodeid;
    char msgid_str[TMMSGIDLEN_STR+1];
    char msgid[TMMSGIDLEN+1];
    char *buf = NULL;
    UBFH *p_ub = NULL;
    UBFH *p_ub2 = NULL;
    TPQCTL qc;
    long len = 1;
    char typ[64];
    long blen; 
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
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, "Invalid options, see `help'.");
        FAIL_OUT(ret);
    }
    
    /* Have a number in FB! */
    memset(&qc, 0, sizeof(qc));

    tmq_msgid_deserialize(msgid_str, msgid);
    
    qc.flags|= TPQPEEK;
    qc.flags|= TPQGETBYMSGID;
    
    memcpy(qc.msgid, msgid, TMMSGIDLEN);
    
    buf = tpalloc("STRING", "", 1);
    
    if (SUCCEED!=tpdequeueex(nodeid, srvid, "*N/A*", &qc, (char **)&buf, &len, 0))
    {
        fprintf(stderr, "failed %s diag: %ld:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        FAIL_OUT(ret);
    }
    
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", "", 1024)))
    {
        NDRX_LOG(log_error, "Failed to alloc: %s", strerror(errno));
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tmq_tpqctl_to_ubf_deqrsp(p_ub, &qc))
    {
        NDRX_LOG(log_error, "Failed to build tpqctl struct ubf");
        FAIL_OUT(ret);
    }
    
    printf("*************** TPQCTL ***************\n");
    
    Bprint(p_ub);
    
    printf("*************** MESSAGE **************\n");
    
    
    /* Check the buffer & do the actions */
    if (FAIL==(blen=tptypes(buf, typ, NULL)))
    {
        NDRX_LOG(log_error, "Failed to detect buffer type");
        FAIL_OUT(ret);
    }
    
    printf("* Buffer type = %s\n", typ);
    
    if (0==strcmp(typ, "UBF"))
    {
        Bprint((UBFH *)buf);
    }
    else
    {
        /* Do the hex dump */
        STDOUT_DUMP(log_info, "Binary message contents", buf, len);
    }
    
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=buf)
    {
        tpfree(buf);
    }

    return ret;
}

