/* 
** Change queue attributes
**
** @file cmd_mqch.c
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
private char M_qspace[XATMI_SERVICE_NAME_LENGTH+1] = "";
/*---------------------------Prototypes---------------------------------*/

/**
 * Change queue settings (by using config string)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_mqch(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    char qconf[512];
    short srvid, nodeid;
    UBFH *p_ub = NULL;
    char cmd = TMQ_CMD_MQCH;
    char qspacesvc[XATMI_SERVICE_NAME_LENGTH+1]; /* real service name */
    long rsplen;    
    
    ncloptmap_t clopt[] =
    {
        {'n', BFLD_SHORT, (char *)&nodeid, 0, 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Cluster node id"},
        {'i', BFLD_SHORT, (char *)&srvid, 0, 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Server ID"},
        {'q', BFLD_STRING, qconf, sizeof(qconf), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Queue config string"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        FAIL_OUT(ret);
    }
    
    /* we need to init TP subsystem... */
    if (SUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s", tpstrerror(tperrno));
        NDRX_LOG(log_error, "Failed to tpinit(): %s", tpstrerror(tperrno))
        FAIL_OUT(ret);
    }
    
    /* Setup the FB & call the server.. */
    p_ub = (UBFH *)tpalloc("UBF", "", 1024);
         
    if (NULL==p_ub)
    {
        fprintf(stderr, "Failed to alloc call buffer%s", tpstrerror(tperrno));
        NDRX_LOG(log_error,"Failed to alloc call buffer%s", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }

    if (SUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        fprintf(stderr, "Failed to set command: %s", Bstrerror(Berror));
        NDRX_LOG(log_error,"Failed to set command: %s", Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=CBchg(p_ub, EX_DATA, 0, qconf, 0L, BFLD_STRING))
    {
        fprintf(stderr, "Failed to set data: %s", Bstrerror(Berror));
        NDRX_LOG(log_error,"Failed to set data: %s", Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    sprintf(qspacesvc, NDRX_SVC_TMQ, (long)nodeid, (int)srvid);
    
    NDRX_LOG(log_info, "Calling: %s", qspacesvc);

    if (SUCCEED!=tpcall(qspacesvc, (char *)p_ub, 0L, 
            (char **)&p_ub, &rsplen, TPNOTRAN))
    {
        NDRX_LOG(log_error,"Failed: %s", tpstrerror(tperrno));
        
        printf("Failed: %s\n", tpstrerror(tperrno));
    }
    else
    {
        printf("Succeed\n");
    }
    
out:
    return ret;
}

