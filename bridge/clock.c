/**
 * @brief Clock related routines.
 *
 * @file clock.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "bridge.h"
#include "../libatmisrv/srv_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CLOCK_DEBUG     1
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Initialize clock diff.
 * @param call
 * @return 
 */
expublic int br_calc_clock_diff(command_call_t *call)
{
    int ret=EXSUCCEED;
    ndrx_stopwatch_t our_time;
    cmd_br_time_sync_t *their_time = (cmd_br_time_sync_t *)call;
    
    /* So we have their time let timer lib, to get diff */
    ndrx_stopwatch_reset(&our_time);
    
    
    G_bridge_cfg.timediff = ndrx_stopwatch_diff(&their_time->time, &our_time);
    NDRX_LOG(log_warn, "Monotonic clock time correction between us "
            "and node %d is: %lld msec (%d) ", 
            call->caller_nodeid, G_bridge_cfg.timediff, sizeof(G_bridge_cfg.timediff));
    
    return ret;
}


/**
 * Send or clock to server.
 * @return 
 */
expublic int br_send_clock(void)
{
    char *fn = "br_send_clock";
    cmd_br_time_sync_t ourtime;
    int ret=EXSUCCEED;
    
    NDRX_LOG(log_debug, "%s - enter", fn);
    
    memset(&ourtime, 0, sizeof(ourtime));
    
    cmd_generic_init(NDRXD_COM_BRCLOCK_RQ, NDRXD_SRC_BRIDGE, NDRXD_CALL_TYPE_BRBCLOCK,
                            (command_call_t *)&ourtime, ndrx_get_G_atmi_conf()->reply_q_str);
    ndrx_stopwatch_reset(&ourtime.time);
    
    ret=br_send_to_net((char*)&ourtime, sizeof(ourtime), BR_NET_CALL_MSG_TYPE_NDRXD, 
            ourtime.call.command);
    
out:

    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
    
}

/**
 * Adjust clock in packet.
 * @return 
 */
expublic void br_clock_adj(tp_command_call_t *call, int is_out)
{
    N_TIMER_DUMP(log_info, "Call timer: ", call->timer);    
#if CLOCK_DEBUG
    ndrx_stopwatch_t our_time;
    ndrx_stopwatch_reset(&our_time);
    NDRX_LOG(log_debug, "Original call age: %lld ms", 
            ndrx_stopwatch_diff(&call->timer, &our_time));
#endif
    if (is_out)
    {
        ndrx_stopwatch_plus(&call->timer, G_bridge_cfg.timediff);
    }
    else
    {
        ndrx_stopwatch_minus(&call->timer, G_bridge_cfg.timediff);
    }
        
    NDRX_LOG(log_debug, "Clock diff: %lld ms", G_bridge_cfg.timediff);
    N_TIMER_DUMP(log_info, "Adjusted call timer: ", call->timer);
    
#if CLOCK_DEBUG
    NDRX_LOG(log_debug, "New call age: %lld ms", 
            ndrx_stopwatch_diff(&call->timer, &our_time));
    NDRX_LOG(log_debug, "Clock based call age (according to tstamp): %d", 
            time(NULL) - call->timestamp);
#endif
        
}
/* vim: set ts=4 sw=4 et smartindent: */
