/* 
** Ping command related routines (i.e send ping from ndrxd and get reply from server)
**
** @file cmd_ping.c
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

#include <ndrstandard.h>

#include "ndebug.h"
#include "userlog.h"
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi.h>
#include <cmd_processor.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Unadvertise service, call from xadmin.
 * @param args
 * @return 
 */
public int srv_send_ping (pm_node_t *p_pm)
{
    int ret=SUCCEED;
    
    command_srvping_t ping;
    
    memset(&ping, 0, sizeof(ping));
    if (PING_MAX_SEQ==p_pm->pingseq || 0==p_pm->pingseq)
    {
        p_pm->pingseq=1;
    }
    else
    {
        p_pm->pingseq++;
    }
    ping.seq = p_pm->pingseq;
    ping.srvid = p_pm->srvid;
    n_timer_reset(&p_pm->pingroundtrip);
    
    /* Call the server */
    if (SUCCEED!=(ret = cmd_generic_callfl(NDRXD_COM_SRVPING_RQ, NDRXD_SRC_ADMIN,
            NDRXD_CALL_TYPE_GENERIC,
            (command_call_t *)&ping, sizeof(ping),
            G_command_state.listenq_str,
            G_command_state.listenq,
            (mqd_t)FAIL,
            get_srv_admin_q(p_pm),
            0, NULL,
            NULL,
            NULL,
            NULL,
            FALSE,
            TPNOBLOCK)))
    {
        NDRX_LOG(log_error, "Failed to send ping command to server id=%d!", 
                ping.srvid);
        ret=FAIL;
        goto out;
    }
    
out:
   
    NDRX_LOG(log_warn, "srv_send_ping returns with status %d", ret);
    return ret;
}

/**
 * Server ping response comman processing
 * @param call
 * @param data
 * @param len
 * @param context
 * @return  
 */
public int cmd_srvpingrsp (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    
    command_srvping_t * ping = (command_srvping_t *)call;
    pm_node_t *p_pm = get_pm_from_srvid(ping->srvid);
    
    if (NULL==p_pm)
    {
        NDRX_LOG(log_error, "No such server with id: %d", ping->srvid);
        ret=FAIL;
        goto out;
    }
    else if (p_pm->pingseq == ping->seq)
    {
        NDRX_LOG(log_error, "Server id=%d ok, binary: [%s] ping reply seq: %d, rsptime: %s", 
                ping->srvid, p_pm->binary_name, ping->seq, n_timer_decode(&p_pm->pingroundtrip, 0));
        p_pm->rsptimer = SANITY_CNT_START;
    }
    else
    {
        NDRX_LOG(log_error, "WARNING: Server id=%d, binary: [%s] ping "
                "seq out of order! Expected: %d got: %d", 
                ping->srvid, p_pm->binary_name, p_pm->pingseq, ping->seq);
    }
    
out:
    /* Ignore the error */
    return SUCCEED;
}
