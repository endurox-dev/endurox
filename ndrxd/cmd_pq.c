/* 
** Command's `pq' - Print Queue backend
** This includes backend processing (gathering statistics by sanity callback)
** This is server side statistics grabber. command "pq"
** We should implemnet also in xadmin local command "pql" which would print
** statistics for all prefixed queues (so that we have full view of all
** queues in system)
** The ndrxd monitor for us is needed, because of we might want to start
** services in future automatically of water high or water low...
**
** @file cmd_pq.c
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
#include <uthash.h>
#include <errno.h>
#include <ndrstandard.h>

#include <ndebug.h>
#include <userlog.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <atmi_shm.h>

#include "cmd_processor.h"
#include <bridge_int.h>
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
public void pq_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_pq_t * pq_info = (command_reply_pq_t *)reply;
    bridgedef_svcs_t *svc = (bridgedef_svcs_t *)params->mod_param1;
    
    reply->msg_type = NDRXD_CALL_TYPE_PQ;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_pq_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    strcpy(pq_info->service, svc->svc_nm);
    memcpy(pq_info->pq_info, svc->pq_info, sizeof(svc->pq_info));
    
    NDRX_LOG(log_debug, "magic: %ld", pq_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
private void pq_progress(command_call_t * call, bridgedef_svcs_t *q_info)
{
    int ret=SUCCEED;
    mod_param_t params;
    
    NDRX_LOG(log_debug, "pq_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)q_info;

    if (SUCCEED!=simple_command_reply(call, ret, NDRXD_REPLY_HAVE_MORE,
                            /* hook up the reply */
                            &params, pq_reply_mod, 0L, 0, NULL))
    {
        userlog("Failed to send progress back to [%s]", call->reply_queue);
    }
    

    NDRX_LOG(log_debug, "pq_progress exit");
}

/**
 * Call to psc command
 * @param args
 * @return
 */
public int cmd_pq (command_call_t * call, char *data, size_t len, int context)
{
    int ret=SUCCEED;
    bridgedef_svcs_t *cur, *tmp;
    
    pq_run_santiy(FALSE);

    HASH_ITER(hh, G_bridge_svc_hash, cur, tmp)
    {
        pq_progress(call, cur);
    }

    if (SUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
    {
        userlog("Failed to send reply back to [%s]", call->reply_queue);
    }
    NDRX_LOG(log_warn, "cmd_pq returns with status %d", ret);
    
out:
    return ret;
}

/**
 * Append queue statistics.
 * @param run_hist - shift the history
 * @return SUCCEED/FAIL
 */
public int pq_run_santiy(int run_hist)
{
    int ret = SUCCEED;
    bridgedef_svcs_t *cur, *tmp;
    int i;
    char q[NDRX_MAX_Q_SIZE+1];
    struct mq_attr att;
    int avg;
    int curmsgs;
    short *srvlist = NULL;
    int len;
    /* the services are here: G_bridge_svc_hash - we must loop around 
     * and get every local queue stats, if queue fails to open, then assume 0
     * before that we must shift the array...
     */
    HASH_ITER(hh, G_bridge_svc_hash, cur, tmp)
    {
        /* shift the stats (if needed) */
        if (run_hist)
        {
            for (i=PQ_LEN-1; i>1; i--) /* fix for rpi... start with PQ_LEN-1!*/
            {   
                cur->pq_info[i]=cur->pq_info[i-1];
            }
        }
#ifdef EX_USE_EPOLL
        /* now write at POS 0, latest reading of service */
        sprintf(q, NDRX_SVC_QFMT, G_sys_config.qprefix, cur->svc_nm);
        
        if (SUCCEED!=ndrx_get_q_attr(q, &att))
        {
            curmsgs = 0; /* assume 0 */
        }
        else
        {
            curmsgs = att.mq_curmsgs;
        }
#else
        /* For poll mode, we need a list of servers, so that we can 
         * request stats for all servers:
         */
        curmsgs = 0;
        if (SUCCEED==ndrx_shm_get_srvs(cur->svc_nm, &srvlist, &len))
        {
            for (i=0; i<len; i++)
            {
                sprintf(q, NDRX_SVC_QFMT_SRVID, G_sys_config.qprefix, 
                        cur->svc_nm, srvlist[i]);
                
                if (SUCCEED==ndrx_get_q_attr(q, &att))
                {
                    curmsgs+= att.mq_curmsgs;
                }
            }
            
            free(srvlist);
        }
        
#endif
        
        cur->pq_info[1] = curmsgs;
        
        /* Now calculate the average */
        avg = 0;
        for (i=1; i<PQ_LEN; i++) 
        {   
            avg+=cur->pq_info[i];
        }
        cur->pq_info[0] = avg/(PQ_LEN-1);
        
    }
    
out:
    return ret;
}

