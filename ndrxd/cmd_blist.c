/**
 * @brief Return list of bridge admin queues
 *
 * @file cmd_blist.c
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
#include <exhash.h>
#include <errno.h>
#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi_shm.h>

#include "cmd_processor.h"
#include <bridge_int.h>
#include <utlist.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Modify reply according the data.
 * @param call
 * @param pm
 */
expublic void blist_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_blist_t * blist_info = (command_reply_blist_t *)reply;
    
    reply->msg_type = NDRXD_CALL_TYPE_BLIST;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_blist_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    NDRX_STRCPY_SAFE(blist_info->qstr, (char *)params->mod_param1);
    
    NDRX_LOG(log_debug, "magic: %ld", blist_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param qstr - queue to send away
 * @return
 */
exprivate int blist_progress(command_call_t * call, char *qstr)
{
    int ret=EXSUCCEED;
    mod_param_t params;
    
    NDRX_LOG(log_debug, "blist_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)qstr;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_CALL_FLAGS_RSPHAVE_MORE,
                            /* hook up the reply */
                            &params, blist_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
        EXFAIL_OUT(ret);
    }
    
out:
    NDRX_LOG(log_debug, "blist_progress exit %d", ret);
}

/**
 * Call of the blist command
 * @param args
 * @return
 */
expublic int cmd_blist (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    pm_node_t *p_pm;
    
    DL_FOREACH(G_process_model, p_pm)
    {
        /* only connected bridges being listed */
        if ( (p_pm->flags & SRV_KEY_FLAGS_BRIDGE)
                && (p_pm->flags & SRV_KEY_FLAGS_CONNECTED))
        {
            if (EXSUCCEED!=blist_progress(call, get_srv_admin_q(p_pm)))
            {
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    
    NDRX_LOG(log_warn, "cmd_blist returns with status %d", ret);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
