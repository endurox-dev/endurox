/* 
** Request for re-advertise
**
** @file cmd_readv.c
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

/**
 * Request for service re-advertise
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_readv(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=SUCCEED;
    command_dynadvertise_t call;
    short srvid=FAIL;
    char svcnm[MAXTIDENT+1]={EOS};
    
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_SHORT, (void *)&srvid, 0, 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Server ID"},
        {'s', BFLD_STRING, (void *)svcnm, sizeof(svcnm), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Service name"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv))
    {
        fprintf(stderr, "Invalid options, see `help'.");
        FAIL_OUT(ret);
    }
    
    memset(&call, 0, sizeof(call));
    
    call.srvid = FAIL;
    strncpy(call.svc_nm, svcnm, sizeof(call.svc_nm)-1);
    
    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                        NDRXD_CALL_TYPE_GENERIC,
                        (command_call_t *)&call, sizeof(call),
                        G_config.reply_queue_str,
                        G_config.reply_queue,
                        G_config.ndrxd_q,
                        G_config.ndrxd_q_str,
                        argc, argv,
                        p_have_next,
                        G_call_args,
                        FALSE);
out:
        return ret;
}
