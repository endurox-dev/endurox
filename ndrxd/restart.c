/* 
** Logic that needs to be processed in case of ndrxd restart.
**
** @file restart.c
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
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <ntimer.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>

#include "userlog.h"
#include "sys_unix.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
private int request_info(char *qname);

/**
 * Do actions required for restart.
 */
public int do_restart_actions(void)
{
    int ret=SUCCEED;
    mq_list_t* qlist = NULL;
    mq_list_t* elt = NULL;
    static char    server_prefix[NDRX_MAX_Q_SIZE+1];
    static int     server_prefix_len;
    
    /* Load app config */
    if (SUCCEED!=(ret = load_active_config(&G_app_config, &G_process_model,
            &G_process_model_hash, &G_process_model_pid_hash)))
    {
        NDRX_LOG(log_error, "Failed to load active configuration - "
                "cannot restart!");
        goto out;
    }

    sprintf(server_prefix, NDRX_ADMIN_FMT_PFX, G_sys_config.qprefix);
    server_prefix_len=strlen(server_prefix);
    NDRX_LOG(log_debug, "server_prefix=[%s]/%d", server_prefix, 
                        server_prefix_len);

    NDRX_LOG(log_warn, "Scanning process queues for info gathering");

    /* Do the directory listing here... and perform the check! */
    
    qlist = ex_sys_mqueue_list_make(G_sys_config.qpath, &ret);

    if (SUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        FAIL_OUT(ret);
    }

    LL_FOREACH(qlist,elt)
    {
        if (0==strncmp(elt->qname, server_prefix, server_prefix_len)) 
        {
            NDRX_LOG(log_warn, "Requesting info from: [%s]", elt->qname);
            ret=request_info(elt->qname);
        }
        
        /* Check the status of above run */
        if (SUCCEED!=ret)
            goto out;
    }

    /* Reset wait timer for learning */
    n_timer_reset(&(G_sys_config.time_from_restart));

out:

    if (NULL!=qlist)
    {
        ex_sys_mqueue_list_free(qlist);
    }

    return ret;
}

/**
 * Send info request to server.
 */
private int request_info(char *qname)
{
    int ret=SUCCEED;
    command_call_t call;
    memset(&call, 0, sizeof(call));
    
    if (SUCCEED!=(cmd_generic_call(NDRXD_COM_SRVINFO_RQ, NDRXD_SRC_ADMIN,
                NDRXD_CALL_TYPE_GENERIC,
                &call, sizeof(call),
                G_command_state.listenq_str,
                G_command_state.listenq,
                FAIL,
                qname,
                0, NULL,
                NULL,
                NULL,
                NULL,
                FALSE)))
    {
        /* Will ignore any error */
        NDRX_LOG(log_error, "Failed to call: [%s]", qname);
    }
    
out:
    return ret;
}
/**
 * Do checking about shall we switch to normal mode.
 * @return 
 */
public int do_restart_chk(void)
{
    int ret=SUCCEED;
    int delta;
    
    if ((delta=n_timer_get_delta_sec(&(G_sys_config.time_from_restart))) > G_app_config->restart_to_check)
    {
        NDRX_LOG(log_warn, "Restart learning time spent over... "
                "switching to normal state!");
        G_sys_config.restarting = FALSE;
    }
    else
    {
        NDRX_LOG(log_warn, "Still learning for %d secs, limit: %d", 
                delta, G_app_config->restart_to_check);
    }
    
out:
    return ret;    
}


