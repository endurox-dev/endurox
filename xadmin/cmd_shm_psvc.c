/* 
** `shm_psvc' command implementation - SHM - print services
**
** @file cmd_shm_psvc.c
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

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>

#include "nstopwatch.h"
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
    fprintf(stderr, "Slot   Service Name Nsrv Flags CSrvs TClst CMAX CNODES\n");
    fprintf(stderr, "------ ------------ ---- ----- ----- ----- ---- --------------------------------\n");
}

/**
 * Generate cluster map
 * @param reply
 * @return 
 */
private char *gen_clstr_map(command_reply_shm_psvc_t * reply)
{
    static char map[CONF_NDRX_NODEID_COUNT+1];
    char tmp[6];
    int i;
    map[0] = EOS;
    
    for (i=0; i<CONF_NDRX_NODEID_COUNT; i++)
    {
        if (reply->cnodes[i].srvs < 10)
        {
            sprintf(tmp, "%d", reply->cnodes[i].srvs);
        }
        else
        {
            strcpy(tmp, "+");
        }
        strcat(map, tmp);
    }
    
    return map;
}

/**
 * Process response back.
 * @param reply
 * @param reply_len
 * @return
 */
public int shm_psvc_rsp_process(command_reply_t *reply, size_t reply_len)
{
    int i;
    char binary[9+1];
    char svc[12+1];
    
    if (NDRXD_CALL_TYPE_PM_SHM_PSVC==reply->msg_type)
    {
        command_reply_shm_psvc_t * shm_psvc_info = (command_reply_shm_psvc_t*)reply;
        
        FIX_SVC_NM(shm_psvc_info->service, svc, (sizeof(svc)-1));
                
        fprintf(stdout, "%-6d %-12.12s %-4.4s %-5d %-5d %-5d %-4d %-*.*s\n", 
            shm_psvc_info->slot, 
            svc,
            ndrx_decode_num(shm_psvc_info->srvs, 0, 0, 1),
            shm_psvc_info->flags,
            shm_psvc_info->csrvs, shm_psvc_info->totclustered,
            shm_psvc_info->cnodes_max_id, 
            CONF_NDRX_NODEID_COUNT,
            CONF_NDRX_NODEID_COUNT, gen_clstr_map(shm_psvc_info));
        
        /* This is poll mode, provide info about individual serves: */
        if (shm_psvc_info->srvids[0])
        {
            fprintf(stdout, "\t\t\t\t\t\tSRVIDS: ");
            for (i=0; i<shm_psvc_info->srvs-shm_psvc_info->csrvs; i++)
            {
                fprintf(stdout, "%hd ", shm_psvc_info->srvids[i]);
            }
            fprintf(stdout, "\n");
        }
    }
    
    return SUCCEED;
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_shm_psvc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
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

