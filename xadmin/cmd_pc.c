/* 
** `pc' (print clients) command implementation
**
** @file cmd_pc.c
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
#include <cpm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print the processes
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
private int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = SUCCEED;
    char output[CPM_OUTPUT_SIZE];
    
    if (SUCCEED!=Bget(p_ub, EX_CPMOUTPUT, 0, (char *)output, 0L))
    {
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }

    printf("%s\n", output);
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
private int call_cpm(char *svcnm)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, CPM_DEF_BUFFER_SZ);
    int ret=SUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_CPMCOMMAND, 0, CPM_CMD_PC, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CPMCOMMAND to %s!", EX_CPMCOMMAND);        
        FAIL_OUT(ret);
    }
    
    if (FAIL == (cd = tpconnect(svcnm,
                                    (char *)p_ub,
                                    0,
                                    TPNOTRAN |
                                    TPRECVONLY)))
    {
        NDRX_LOG(log_error, "Connect error [%s]", tpstrerror(tperrno) );
        
        ret = FAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Connected OK, cd = %d", cd );

    while (recv_continue)
    {
        recv_continue=0;
        if (FAIL == tprecv(cd,
                            (char **)&p_ub,
                            0L,
                            0L,
                            &revent))
        {
            ret = FAIL;
            tp_errno = tperrno;
            if (TPEEVENT == tp_errno)
            {
                    if (TPEV_SVCSUCC == revent)
                            ret = SUCCEED;
                    else
                    {
                        NDRX_LOG(log_error,
                                 "Unexpected conv event %lx", revent );
                        FAIL_OUT(ret);
                    }
            }
            else
            {
                NDRX_LOG(log_error, "recv error %d", tp_errno  );
                FAIL_OUT(ret);
            }
        }
        else
        {
            /*
            fprintf(stderr, "Response: \n");
            Bfprint(p_ub, stderr);
            */
            if (SUCCEED!=print_buffer(p_ub, svcnm))
            {
                FAIL_OUT(ret);
            }
            rcv_count++;
            recv_continue=1;
        }
    }

out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
/*
    tpdiscon(cd);
*/
    return ret;
}

/**
 * Print Clients
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_pc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    
    call_cpm(NDRX_SVC_CPM);
    
out:
    return ret;
}

