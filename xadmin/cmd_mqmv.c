/* 
** Move message from one qspace to another qspace + qname
** uses local XA connection
**
** @file cmd_mqmv.c
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
 * Move message to different queue. Also note that caller (xadmin) must be
 * be able to work with XA transaction.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mqmv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    short srvid;
    short nodeid;
    char msgid_str[TMMSGIDLEN_STR+1];
    char msgid[TMMSGIDLEN+1];
    char *buf = NULL;
    TPQCTL qc;
    long len = 1;
    char dest_qspace[XATMI_SERVICE_NAME_LENGTH+1];
    char dest_qname[TMQNAMELEN+1];
    
    ncloptmap_t clopt[] =
    {
        {'n', BFLD_SHORT, (char *)&nodeid, 0,
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Q cluster node id"},
        {'i', BFLD_SHORT, (char *)&srvid, 0, 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Q Server Id"},
        {'m', BFLD_STRING, msgid_str, sizeof(msgid_str), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Message id"},
        {'s', BFLD_STRING, dest_qspace, sizeof(dest_qspace), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Dest Q space"},
        {'q', BFLD_STRING, dest_qname, sizeof(dest_qname), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Dest Q"},
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
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Connect to XA */
    if (EXSUCCEED!=tpopen())
    {
        fprintf(stderr, "Failed to open XA sub-system: %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpbegin(ndrx_get_G_atmi_env()->time_out, 0))
    {
        fprintf(stderr, "Failed to start XA transaction: %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    
    buf = tpalloc("STRING", "", 1);
    
    if (EXSUCCEED!=tpdequeueex(nodeid, srvid, "*N/A*", &qc, (char **)&buf, &len, 0))
    {
        fprintf(stderr, "dequeue failed %s diag: %ld:%s\n", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        NDRX_LOG(log_error, "failed %s diag: %ld:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpenqueue(dest_qspace, dest_qname, &qc, (char *)buf, len, 0))
    {
        fprintf(stderr, "enqueue failed %s diag: %ld:%s\n", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        NDRX_LOG(log_error, "failed %s diag: %ld:%s", 
                tpstrerror(tperrno), qc.diagnostic, qc.diagmsg);
        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        fprintf(stderr, "Commit failed: %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    printf("Committed\n");
    
out:

    if (tpgetlev())
    {
        tpabort(0);
    }

    if (NULL!=buf)
    {
        tpfree(buf);
    }
    
    /* close it anyway... */
    tpclose();

    return ret;
}
