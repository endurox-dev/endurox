/**
 * @brief Command's `pq' - Print Queue backend
 *   This includes backend processing (gathering statistics by sanity callback)
 *   This is server side statistics grabber. command "pq"
 *   We should implemnet also in xadmin local command "pql" which would print
 *   statistics for all prefixed queues (so that we have full view of all
 *   queues in system)
 *   The ndrxd monitor for us is needed, because of we might want to start
 *   services in future automatically of water high or water low...
 *
 * @file cmd_pq.c
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
#ifdef EX_USE_SYSVQ
#include <sys/msg.h>
#endif
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
expublic void pq_reply_mod(command_reply_t *reply, size_t *send_size, mod_param_t *params)
{
    command_reply_pq_t * pq_info = (command_reply_pq_t *)reply;
    bridgedef_svcs_t *svc = (bridgedef_svcs_t *)params->mod_param1;
    
    reply->msg_type = NDRXD_CALL_TYPE_PQ;
    /* calculate new send size */
    *send_size += (sizeof(command_reply_pq_t) - sizeof(command_reply_t));

    /* Copy data to reply structure */
    NDRX_STRCPY_SAFE(pq_info->service, svc->svc_nm);
    memcpy(pq_info->pq_info, svc->pq_info, sizeof(svc->pq_info));
    
    NDRX_LOG(log_debug, "magic: %ld", pq_info->rply.magic);
}

/**
 * Callback to report startup progress
 * @param call
 * @param pm
 * @return
 */
exprivate void pq_progress(command_call_t * call, bridgedef_svcs_t *q_info)
{
    int ret=EXSUCCEED;
    mod_param_t params;
    
    NDRX_LOG(log_debug, "pq_progress enter");
    memset(&params, 0, sizeof(mod_param_t));

    /* pass to reply process model node */
    params.mod_param1 = (void *)q_info;

    if (EXSUCCEED!=simple_command_reply(call, ret, NDRXD_CALL_FLAGS_RSPHAVE_MORE,
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
expublic int cmd_pq (command_call_t * call, char *data, size_t len, int context)
{
    int ret=EXSUCCEED;
    bridgedef_svcs_t *cur, *tmp;
    
    pq_run_santiy(EXFALSE);

    EXHASH_ITER(hh, G_bridge_svc_hash, cur, tmp)
    {
        pq_progress(call, cur);
    }

    if (EXSUCCEED!=simple_command_reply(call, ret, 0L, NULL, NULL, 0L, 0, NULL))
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
expublic int pq_run_santiy(int run_hist)
{
    int ret = EXSUCCEED;
    bridgedef_svcs_t *cur, *tmp;
    int i;
    char q[NDRX_MAX_Q_SIZE+1];
    struct mq_attr att;
    int avg;
    int curmsgs;
    ndrx_shm_resid_t *srvlist = NULL;
    int len;
    /* the services are here: G_bridge_svc_hash - we must loop around 
     * and get every local queue stats, if queue fails to open, then assume 0
     * before that we must shift the array...
     */
    EXHASH_ITER(hh, G_bridge_svc_hash, cur, tmp)
    {
        /* shift the stats (if needed) */
        if (run_hist)
        {
            for (i=PQ_LEN-1; i>1; i--) /* fix for rpi... start with PQ_LEN-1!*/
            {   
                cur->pq_info[i]=cur->pq_info[i-1];
            }
        }
        
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
        /* For poll mode, we need a list of servers, so that we can 
         * request stats for all servers:
         */
        curmsgs = 0;
        
        if (EXSUCCEED==ndrx_shm_get_srvs(cur->svc_nm, &srvlist, &len))
        {
            for (i=0; i<len; i++)
            {
                /* TODO: For System V we could do a direct queue lookup by qid..! */
#if defined(EX_USE_POLL)
                snprintf(q, sizeof(q), NDRX_SVC_QFMT_SRVID, G_sys_config.qprefix, 
                        cur->svc_nm, srvlist[i].resid);
                
                if (EXSUCCEED==ndrx_get_q_attr(q, &att))
                {
                    curmsgs+= att.mq_curmsgs;
                }
#else
                /* System V approach for queues... quick & easy */
                struct msqid_ds buf;
                
                if (EXSUCCEED==msgctl(srvlist[i].resid, IPC_STAT, &buf))
                {
                    curmsgs+= buf.msg_qnum;
                }
                else
                {
                    NDRX_LOG(log_warn, "Failed to get qid %d stats: %s",
                            srvlist[i].resid, strerror(errno));
                }
#endif
            }
            
            NDRX_FREE(srvlist);
        }
#else
        /* now write at POS 0, latest reading of service */
        snprintf(q, sizeof(q), NDRX_SVC_QFMT, G_sys_config.qprefix, cur->svc_nm);
        
        if (EXSUCCEED!=ndrx_get_q_attr(q, &att))
        {
            curmsgs = 0; /* assume 0 */
        }
        else
        {
            curmsgs = att.mq_curmsgs;
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

/* vim: set ts=4 sw=4 et smartindent: */
