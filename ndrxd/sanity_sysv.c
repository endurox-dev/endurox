/**
 * @brief System V specifics for
 *
 * @file sanity_sysv.c
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
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>
#include <bridge_int.h>
#include <atmi_shm.h>
#include <userlog.h>
#include <sys_unix.h>
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Remove any pending calls to request address.
 * Basically we construct here mqd_t and let common function to flush the calls.
 * The common function doesn't do any lockings, thus this approach is fine.
 * @param qid queue id
 * @param qstr queue string
 * @return EXSUCCEED/EXFAIL
 */
exprivate int flush_rqaddr(int qid, char *qstr)
{
    int ret = EXSUCCEED;
    mqd_t mqd = NULL;
    int err;
    
    mqd = NDRX_CALLOC(1, sizeof(struct ndrx_svq_info));
    
    if (NULL==mqd)
    {
        err = errno;
        
        NDRX_LOG(log_error, "Failed to malloc %d bytes", 
                (int)sizeof(struct ndrx_svq_info));
        userlog("Failed to malloc %d bytes", 
                (int)sizeof(struct ndrx_svq_info));
        EXFAIL_OUT(ret);
    }
    
    mqd->qid = qid;
    NDRX_STRCPY_SAFE(mqd->qstr, qstr);
    
    /* lets flush the queue now. */
    if (EXSUCCEED!=remove_service_q(NULL, EXFAIL, mqd, qstr))
    {
        NDRX_LOG(log_error, "Failed to flush [%s]/%d", qstr, qid);
        EXFAIL_OUT(ret);
    }
out:
    
    if (NULL!=mqd)
    {
        NDRX_FREE(mqd);
    }
    return ret;
}

/**
 * System V sanity checks.
 * Includes following steps:
 * - get a copy of SV5 queue maps
 * - scan the shared memory of services, and mark the used sv5 qids.
 * - then scan the list of used qids for the request addresses
 * - remove any qid, that is not present in shm (the removal shall be done
 *  in sv5 library with the write lock present and checking the ctime again
 *  so that we have a real sync). Check the service rqaddr by NDRX_SVQ_MAP_RQADDR
 * @return SUCCEED/FAIL
 */
expublic int do_sanity_check_sysv(void)
{
    int ret=EXSUCCEED;
    ndrx_svq_status_t *svq = NULL;
    int len;
    int i;
    int have_value_3;
    int pos_3;
    bridgedef_svcs_t *cur, *tmp;
    short *srvlist = NULL;
    
    NDRX_LOG(log_debug, "Into System V sanity checks");
    /* Get the list of queues */
    svq = ndrx_svqshm_statusget(&len, G_app_config->rqaddrttl);
    
    if (NULL==svq)
    {
        NDRX_LOG(log_error, "Failed to get System V shared memory status!");
        userlog("Failed to get System V shared memory status!");
        EXFAIL_OUT(ret);
    }
    
    /* Now scan the used services shared memory and updated the 
     * status copy accordingly
     * WELL! We must loop over the local NDRXD list of the services
     * and then update the status. Then we can avoid the locking of
     * the shared memory.
     */
    
    /* We assume shm is OK! */
    EXHASH_ITER(hh, G_bridge_svc_hash, cur, tmp)
    {
        if (EXSUCCEED==ndrx_shm_get_srvs(cur->svc_nm, &srvlist, &len))
        {
            for (i=0; i<len; i++)
            {
                ndrx_svqshm_get_status(svq, srvlist[i], &pos_3, &have_value_3);
                
                if (have_value_3)
                {
                    svq[pos_3].flags |= NDRX_SVQ_MAP_HAVESVC;
                }
            }         
        } /* local servs */
    }
    
    /* Scan for queues which are not any more is service list, 
     * the queue was service, and time have expired for TTL, thus such queues
     * are subject for unlinking...
     * perform that in sync way...
     */
    for (i=0; i<len; i++)
    {
        if ((svq[pos_3].flags & NDRX_SVQ_MAP_RQADDR)
                && !(svq[pos_3].flags & NDRX_SVQ_MAP_HAVESVC)
                && (svq[pos_3].flags & NDRX_SVQ_MAP_SCHEDRM))
        {
            NDRX_LOG(log_info, "qid %d is subject for delete ttl %d", 
                    svq[pos_3].qid, G_app_config->rqaddrttl);
            
            /* Well at this point we shall
             * remove call expublic int remove_service_q(char *svc, short srvid, 
             * mqd_t in_qd, char *in_qstr)!!!
             * because we need to flush the queue of messages...
             * only here we have a issue with lockings
             * we will be in write mode to MAPs, but in mean time we want to
             * perform read ops to the SHM...
             * Thus probably we need some globals in svqshm that indicate
             * that we have already exclusive lock!
             * 
             * But as remove_service_q is not using much of the systemv maps
             * processing, then we could just simple callback from ndrx_svqshm_ctl
             * with qid and queue string. then callback would build simple
             * mqd_t and pass it to remove_service_q for message zapping.
             */
            if (EXSUCCEED==ndrx_svqshm_ctl(NULL, svq[pos_3].qid, 
                    IPC_RMID, G_app_config->rqaddrttl, flush_rqaddr))
            {
                NDRX_LOG(log_error, "Failed to unlink qid %d", svq[pos_3].qid);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    if (NULL!=svq)
    {
        NDRX_FREE(svq);
    }
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
