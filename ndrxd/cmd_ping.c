/**
 * @brief Ping command related routines (i.e send ping from ndrxd and get reply from server)
 *
 * @file cmd_ping.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
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
expublic int srv_send_ping (pm_node_t *p_pm)
{
    int ret=EXSUCCEED;
    
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
    ndrx_stopwatch_reset(&p_pm->pingroundtrip);
    
    /* Call the server */
    if (EXSUCCEED!=(ret = cmd_generic_callfl(NDRXD_COM_SRVPING_RQ, NDRXD_SRC_ADMIN,
            NDRXD_CALL_TYPE_GENERIC,
            (command_call_t *)&ping, sizeof(ping),
            G_command_state.listenq_str,
            G_command_state.listenq,
            (mqd_t)EXFAIL,
            get_srv_admin_q(p_pm),
            0, NULL,
            NULL,
            NULL,
            NULL,
            EXFALSE,
            TPNOBLOCK)))
    {
        NDRX_LOG(log_error, "Failed to send ping command to server id=%d!", 
                ping.srvid);
        ret=EXFAIL;
        goto out;
    }
    
out:
   
    NDRX_LOG(log_info, "srv_send_ping returns with status %d", ret);
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
expublic int cmd_srvpingrsp (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    
    command_srvping_t * ping = (command_srvping_t *)call;
    pm_node_t *p_pm = get_pm_from_srvid(ping->srvid);
    
    if (NULL==p_pm)
    {
        NDRX_LOG(log_error, "No such server with id: %d", ping->srvid);
        ret=EXFAIL;
        goto out;
    }
    else if (p_pm->pingseq == ping->seq)
    {
        NDRX_LOG(log_info, "Server id=%d ok, binary: [%s] ping reply seq: %d, rsptime: %s", 
                ping->srvid, p_pm->binary_name, ping->seq, 
                ndrx_stopwatch_decode(&p_pm->pingroundtrip, 0));
        
        /* Well... we should not count the time unless the
         * ping has been issued.
         *  */
        p_pm->rspstwatch = SANITY_CNT_START;
        p_pm->pingstwatch = SANITY_CNT_IDLE;
    }
    else
    {
        NDRX_LOG(log_error, "WARNING: Server id=%d, binary: [%s] ping "
                "seq out of order! Expected: %d got: %d", 
                ping->srvid, p_pm->binary_name, p_pm->pingseq, ping->seq);
    }
    
out:
    /* Ignore the error */
    return EXSUCCEED;
}
