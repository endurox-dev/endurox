/* 
** `psc' command implementation
**
** @file cmd_psc.c
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

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Print header
 * @return
 */
private void print_hdr(void)
{
    fprintf(stderr, "Nd Service Name Routine Name Prog Name SRVID #SUCC #FAIL MAX      LAST     STAT\n");
    fprintf(stderr, "-- ------------ ------------ --------- ----- ----- ----- -------- -------- -----\n");
}

/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
public int psc_rsp_process(command_reply_t *reply, size_t reply_len)
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
            FIX_SVC_NM(psc_info->svcdet[i].svc_nm, svc, (sizeof(svc)-1));
            FIX_SVC_NM(psc_info->svcdet[i].fn_nm, fun, (sizeof(fun)-1));
                             /*svc    fun     bin*/
           fprintf(stdout, "%-2d %-12.12s %-12.12s %-9.9s %-5d %-5.5s %-5.5s %-8.8s %-8.8s %-5.5s\n",
                   psc_info->nodeid,
                   svc, fun, binary, psc_info->srvid, 
                   nstdutil_decode_num(psc_info->svcdet[i].done, 0, 0, 1), 
                   nstdutil_decode_num(psc_info->svcdet[i].fail, 1, 0, 1),
                   /*decode_msec(psc_info->svcdet[i].min, 0, 0, 2), - not very interesting */
                   decode_msec(psc_info->svcdet[i].max, 0, 0, 2),
                   decode_msec(psc_info->svcdet[i].last, 1, 0, 2),
                   (psc_info->svcdet[i].status?"BUSY":"AVAIL"));
        }
    }
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_psc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    command_call_t call;
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
                        FALSE);
}

