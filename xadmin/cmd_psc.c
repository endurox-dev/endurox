/**
 * @brief `psc' command implementation
 *
 * @file cmd_psc.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate short M_svconly;

/**
 * Print header
 * @return
 */
exprivate void print_hdr(void)
{
    if (M_svconly)
    {
        fprintf(stderr, "Nd Service Name                   Prog SRVID #SUCC #FAIL      MAX     LAST STAT\n");
        fprintf(stderr, "-- ------------------------------ ---- ----- ----- ----- -------- -------- -----\n");
    }
    else
    {
        fprintf(stderr, "Nd Service Name Routine Name Prog Name SRVID #SUCC #FAIL      MAX     LAST STAT\n");
        fprintf(stderr, "-- ------------ ------------ --------- ----- ----- ----- -------- -------- -----\n");
    }
}

/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
expublic int psc_rsp_process(command_reply_t *reply, size_t reply_len)
{
    int i;
    char binary[9+1];
    char svc[12+1];
    char fun[12+1];

    if (NDRXD_CALL_TYPE_SVCINFO==reply->msg_type)
    {
        command_reply_psc_t * psc_info = (command_reply_psc_t*)reply;
        FIX_SVC_NM(psc_info->binary_name, binary, (sizeof(binary)-1));
        for (i=0; i<psc_info->svc_count; i++)
        {
            if (M_svconly)
            {
                /* Feature #230 */
                /*svc    fun     bin*/
                fprintf(stdout, "%2d %-30.30s %-4.4s %5d %5.5s %5.5s %8.8s %8.8s %-5.5s\n",
                       psc_info->nodeid,
                       psc_info->svcdet[i].svc_nm, binary, psc_info->srvid, 
                       ndrx_decode_num(psc_info->svcdet[i].done, 0, 0, 1), 
                       ndrx_decode_num(psc_info->svcdet[i].fail, 1, 0, 1),
                       /*decode_msec(psc_info->svcdet[i].min, 0, 0, 2), - not very interesting */
                       ndrx_decode_msec(psc_info->svcdet[i].max, 0, 0, 2),
                       ndrx_decode_msec(psc_info->svcdet[i].last, 1, 0, 2),
                       (psc_info->svcdet[i].status?"BUSY":"AVAIL"));
            }
            else
            {
                FIX_SVC_NM(psc_info->svcdet[i].svc_nm, svc, (sizeof(svc)-1));
                FIX_SVC_NM(psc_info->svcdet[i].fn_nm, fun, (sizeof(fun)-1));
                                 /*svc    fun     bin*/
                fprintf(stdout, "%2d %-12.12s %-12.12s %-9.9s %5d %5.5s %5.5s %8.8s %8.8s %-5.5s\n",
                       psc_info->nodeid,
                       svc, fun, binary, psc_info->srvid, 
                       ndrx_decode_num(psc_info->svcdet[i].done, 0, 0, 1), 
                       ndrx_decode_num(psc_info->svcdet[i].fail, 1, 0, 1),
                       /*decode_msec(psc_info->svcdet[i].min, 0, 0, 2), - not very interesting */
                       ndrx_decode_msec(psc_info->svcdet[i].max, 0, 0, 2),
                       ndrx_decode_msec(psc_info->svcdet[i].last, 1, 0, 2),
                       (psc_info->svcdet[i].status?"BUSY":"AVAIL"));
            }
        }
    }
    
    return EXSUCCEED;
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_psc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
    
    int ret = EXSUCCEED;
    
    ncloptmap_t clopt[] =
    {
        {'s', BFLD_INT, (void *)&M_svconly, sizeof(M_svconly), 
                                NCLOPT_OPT, "Print services only"},
        {0}
    };
    
    M_svconly = EXFALSE;
            
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    memset(&call, 0, sizeof(call));

    /* Print header at first step! */
    print_hdr();
    /* Then get listing... */
    return cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        &call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        EXFALSE,
                        G_config.listcall_flags);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
