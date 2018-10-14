/**
 * @brief Bridge support functions
 *
 * @file brsupport.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <sys_mqueue.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>


#include <atmi.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi_int.h>
#include <atmi_shm.h>

#include <ndrxdcmn.h>
#include "gencall.h"
#include "atmi_tls.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Dump node stack
 * @param stack
 */
expublic void br_dump_nodestack(char *stack, char *msg)
{
    int i=0;
    int nodeid;
    char node_stack_str[CONF_NDRX_NODEID_COUNT*4];
    node_stack_str[0] = EXEOS;
    char tmp[10];
    int len = strlen(stack);
    
    for (i=0; i<len; i++)
    {
        nodeid = stack[i];
        if (i+1!=len)
            sprintf(tmp, "%d->", nodeid);
        else
            sprintf(tmp, "%d", nodeid);
        strcat(node_stack_str, tmp);
    }
    NDRX_LOG(log_info, "%s: [%s]", msg, node_stack_str);
    
}

/**
 * Fill reply queue, either local (if no nodes in stack), or another birdge service.
 * @param call
 * @return 
 */
expublic int fill_reply_queue(char *nodestack, 
            char *org_reply_to, char *reply_to)
{
    int i;
    int len;
    int nodeid;
    int ret=EXSUCCEED;
    ATMI_TLS_ENTRY;
    
    *reply_to = EXEOS;
    /* So we are going to do reply, we should scan the reply stack
     * chose the bridge, and reduce the callstack
     */
    br_dump_nodestack(nodestack, "Node stack before bridge select");
    
    if ((len = strlen(nodestack)) > 0 )
    {
        NDRX_LOG(log_debug, "Selecting bridge for reply");
        for (i=0; i<len; i++)
        {
            nodeid = nodestack[i];
            if (ndrx_shm_bridge_is_connected(nodeid))
            {
                /* get the bridge service first... */
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
                int is_bridge;
                char tmpsvc[MAXTIDENT+1];
                
                snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, nodeid);
                
                if (EXSUCCEED!=ndrx_shm_get_svc(tmpsvc, reply_to, &is_bridge, NULL))
                {
                    NDRX_LOG(log_error, "Failed to get bridge svc: [%s]", 
                            tmpsvc);
                    EXFAIL_OUT(ret);
                }
#else
                /* epoll/fdpoll/kqueue mode direct call: */
                sprintf(reply_to, NDRX_SVC_QBRDIGE, G_atmi_tls->G_atmi_conf.q_prefix, nodeid);
#endif
                
                nodestack[i] = EXEOS;
                br_dump_nodestack(nodestack, "Node stack after bridge select");
                break;
            }
        }
        
        if (EXEOS==*reply_to)
        {
            ret=EXFAIL;
            NDRX_LOG(log_error, "No bridge to send reply to!");
        }
    }
    /* If have something in node stack */
    else
    {
        /* this was local call, so reply on q */
        strcpy(reply_to, org_reply_to);
    }
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
