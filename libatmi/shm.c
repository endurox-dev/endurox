/**
 * @brief Common shared memory routines for EnduroX. - ATMI level..
 *   Generally we do not use any global vars because this stuff should
 *   generic one.
 *   SHM Example see here:
 *   http://mij.oltrelinux.com/devel/unixprg/
 *   General rule regarding the SVCINFO shared memory area is, that we do not
 *   Delete any services out from that area. That ensures that we can alwasy search
 *   The data up!
 *
 * @file shm.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <atmi_tls.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_shm_t G_srvinfo;
expublic ndrx_shm_t G_svcinfo;
expublic ndrx_shm_t G_brinfo;     /* Info about bridges */

expublic int G_max_servers   = EXFAIL;         /* max servers         */
expublic int G_max_svcs      = EXFAIL;         /* max svcs per server */

int M_init = EXFALSE;                 /* no init yet done */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialise prefix part, that is needed for shm...
 * WARNING ! Not thread safe.
 * MT protected by:
 * - called by ndrxd (single thread)
 * - called by tp_internal_init(), locked protected
 * @param ndrx_prefix
 * @return 
 */
expublic int ndrx_shm_init(char *q_prefix, int max_servers, int max_svcs)
{
    memset(&G_srvinfo, 0, sizeof(G_srvinfo));
    memset(&G_svcinfo, 0, sizeof(G_svcinfo));
    memset(&G_brinfo, 0, sizeof(G_brinfo));

    G_svcinfo.fd = EXFAIL;
    G_svcinfo.key = G_atmi_env.ipckey + NDRX_SHM_SVCINFO_KEYOFSZ;
    
    G_srvinfo.fd = EXFAIL;
    G_srvinfo.key = G_atmi_env.ipckey + NDRX_SHM_SRVINFO_KEYOFSZ;
    
    G_brinfo.fd = EXFAIL;
    G_brinfo.key = G_atmi_env.ipckey + NDRX_SHM_BRINFO_KEYOFSZ;
    
    
    snprintf(G_srvinfo.path, sizeof(G_srvinfo.path), NDRX_SHM_SRVINFO, q_prefix);
    snprintf(G_svcinfo.path, sizeof(G_svcinfo.path), NDRX_SHM_SVCINFO, q_prefix);
    snprintf(G_brinfo.path,  sizeof(G_brinfo.path), NDRX_SHM_BRINFO,  q_prefix);
    
    G_max_servers = max_servers;
    G_max_svcs = max_svcs;

    /* Initialise sizes */

    G_srvinfo.size = sizeof(shm_srvinfo_t)*max_servers;
    NDRX_LOG(log_debug, "G_srvinfo.size = %d (%d * %d)",
                    G_srvinfo.size, sizeof(shm_srvinfo_t), max_servers);
    
    G_svcinfo.size = SHM_SVCINFO_SIZEOF *max_svcs;
    NDRX_LOG(log_debug, "G_svcinfo.size = %d (%d * %d)",
                    G_svcinfo.size, SHM_SVCINFO_SIZEOF, max_svcs);
   
    G_brinfo.size = sizeof(int)*CONF_NDRX_NODEID_COUNT;
    NDRX_LOG(log_debug, "G_brinfo.size = %d (%d * %d)",
                    G_svcinfo.size, sizeof(int), CONF_NDRX_NODEID_COUNT);
   
    
    M_init = EXTRUE;
    return EXSUCCEED;
}

/**
 * Open shared memory
 * WARNING ! Not thread safe.
 * MT protected by: called by ndrxd only (single thread)
 * @return
 */
expublic int ndrxd_shm_open_all(void)
{
    int ret=EXSUCCEED;
    
    /**
     * Library not initialized
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm library not initialized");
        EXFAIL_OUT(ret);
    }

    /* NOTE! shm might exist already, in that case attach
     * it might be created by ndrxd
     */
    if (EXSUCCEED!=ndrx_shm_open(&G_srvinfo, EXTRUE))
    {
        ret=EXFAIL;
        goto out;
    }

    if (EXSUCCEED!=ndrx_shm_open(&G_svcinfo, EXTRUE))
    {
        ret=EXFAIL;
        goto out;
    }

    if (EXSUCCEED!=ndrx_shm_open(&G_brinfo, EXTRUE))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Closes all shared memory resources, generally ignores errors.
 * WARNING ! Not thread safe.
 * MT protected by: 
 *  - called by ndrxd after fork (main thread only)
 *  - called by ndrxd on exit
 *  - called by atmi server un-init
 * @return FAIL if something failed.
 */
expublic int ndrxd_shm_close_all(void)
{
    int ret=EXSUCCEED;

    /**
     * Library not initialized
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm library not initialized");
        ret=EXFAIL;
        goto out;
    }
    
    ret=ndrx_shm_close(&G_srvinfo);

    if (EXFAIL==ndrx_shm_close(&G_svcinfo))
        ret=EXFAIL;

    if (EXFAIL==ndrx_shm_close(&G_brinfo))
        ret=EXFAIL;
out:
    return ret;
}

/**
 * Does delete all shared memory blocks.
 * WARNING ! Not thread safe.
 * MT protected by: called by ndrxd only (single thread)
 */
expublic int ndrxd_shm_delete(void)
{
    if (M_init)
    {
        ndrx_shm_remove(&G_srvinfo);
        ndrx_shm_remove(&G_svcinfo);
        ndrx_shm_remove(&G_brinfo);
    }
    else
    {
        NDRX_LOG(log_error, "SHM library not initialized!");
        return EXFAIL;
    }

    return EXSUCCEED;
}

/**
 * Attach to shared memory block.
 * WARNING ! Not thread safe.
 * MT protected by:
 * - Server does init first in single thread
 * - For clients tp_internal_init() does the thread safe call internally
 * @lev indicates the attach level (should it be service array only)?
 * @return 
 */
expublic int ndrx_shm_attach_all(int lev)
{
   int ret=EXSUCCEED;
   
   /**
     * Library not initialised
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm library not initialised!");
        EXFAIL_OUT(ret);
    }
   
   /* Attached to service shared mem */
   if (lev & NDRX_SHM_LEV_SVC &&
           EXSUCCEED!=ndrx_shm_open(&G_svcinfo, EXTRUE))
   {
       EXFAIL_OUT(ret);
   }
   
   /* Attach to srv shared mem */
   if (lev & NDRX_SHM_LEV_SRV &&
           EXSUCCEED!=ndrx_shm_open(&G_srvinfo, EXTRUE))
   {
       EXFAIL_OUT(ret);
   }
   
   
   /* Attach to srv shared mem */
   if (lev & NDRX_SHM_LEV_BR &&
           EXSUCCEED!=ndrx_shm_open(&G_brinfo, EXTRUE))
   {
       EXFAIL_OUT(ret);
   }
   
out:
   return ret;
}

/**
 * Returns true if service is available.
 * @param svc
 * @param have_shm set to EXTRUE, if shared memory is attached.
 * @return TRUE/FALSE/FAIL (on fail proceed because no SHM)
 */
expublic int ndrx_shm_get_svc(char *svc, char *send_q, int *is_bridge, int *have_shm)
{
    int ret=EXSUCCEED;
    int pos=EXFAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int use_cluster = EXFAIL;
    static int first = EXTRUE;
    shm_svcinfo_t *psvcinfo = NULL;
    int chosen_node = EXFAIL;
    ATMI_TLS_ENTRY;
    
    *is_bridge=EXFALSE;
    
    /* Initialy we stick to the local service */
    sprintf(send_q, NDRX_SVC_QFMT, G_atmi_tls->G_atmi_conf.q_prefix, svc);
    
    if (!ndrx_shm_is_attached(&G_svcinfo))
    {
#ifdef EX_USE_POLL
        /* lookup first service in cache: 
         * probably not relevant any more as SHM is already open
         */
        ret = ndrx_get_cached_svc_q(send_q);
#endif
        goto out; /* do not fail, try locally */
    }
    if (NULL!=have_shm)
    {
        *have_shm = EXTRUE;
    }
    
    /* Get the service entry */
    if (!_ndrx_shm_get_svc(svc, &pos, NDRX_SVCINSTALL_NOT, NULL))
    {
        NDRX_LOG(log_error, "Service %s not found in shm", svc);
        EXFAIL_OUT(ret);
    }
    
    psvcinfo = SHM_SVCINFO_INDEX(svcinfo, pos);
            
    if (psvcinfo->srvs<=0)
    {
        NDRX_LOG(log_error, "Service %s not available, count of servers: %d",
                                  svc, psvcinfo->srvs);
        EXFAIL_OUT(ret);
    }
    
    /* Now use the random to chose the service to send to */
    if (psvcinfo->srvs==psvcinfo->csrvs 
            && psvcinfo->srvs>0)
    {
        use_cluster=EXTRUE;
    }
    else if (0==psvcinfo->csrvs)
    {
        use_cluster=EXFALSE;
    }
    
    NDRX_LOG(log_debug, "use_cluster=%d srvs=%d csrvs=%d", 
            use_cluster, psvcinfo->srvs, 
            psvcinfo->csrvs);
    
    if (EXFAIL==use_cluster)
    {
        /* So we will randomize as we have services in local & in cluster */
        if (first)
        {
            first=EXFALSE;
            srandom(time(NULL));
        }
        
        if (0==G_atmi_env.ldbal)
        {
            /* Run locally */
            use_cluster=EXFALSE;
        }
        else if (100==G_atmi_env.ldbal)
        {
            /* Run in cluster */
            use_cluster=EXTRUE;
        }
        else
        {
            /* Use random */
            int n = rand()%100+1;
            if (n<=G_atmi_env.ldbal)
            {
                use_cluster = EXTRUE;
            }
            else
            {
                use_cluster = EXFALSE;
            }
        }
    }
 
    NDRX_LOG(log_debug, "use_cluster=%d srvs=%d csrvs=%d", 
        use_cluster, psvcinfo->srvs, 
            psvcinfo->csrvs);


    /* So we are using cluster, */
    if (EXTRUE==use_cluster)
    {
        int csrvs = psvcinfo->csrvs;
        int cluster_node = rand()%psvcinfo->csrvs+1;
        int i;
        int got_node = 0;
        int try = 0;
        
        /* TODO: Can be removed in case if we use read/write semaphore!!!
         * Just ensure that due to race condition csrv does not get corrupted.
         * if it will be 0, modulus will cause core dump. 
         */
        if (csrvs<0 || csrvs>CONF_NDRX_NODEID_COUNT)
        {
            NDRX_LOG(log_error, "Fixed csrvs to 0");
            csrvs=1;
        }
        
        cluster_node = rand()%csrvs+1;
        NDRX_LOG(log_debug, "rnd: cluster_node=%d, cnode_max_id=%d", 
                cluster_node, psvcinfo->cnodes_max_id);
        
        /* If cluster was modified (while we do not create read/write semaphores...!) */
        while (try<2)
        {
            /* First try, search the random server */
            for (i=0; i<psvcinfo->cnodes_max_id; i++)
            {
                if (psvcinfo->cnodes[i].srvs)
                {
                    got_node++;
                    if (1==try)
                    {
                        /*use first one we get at second try*/
                        chosen_node=i+1; /* cluster node numbers starts from 1 */
                        NDRX_LOG(log_debug, "try 1, use %d", chosen_node);       
                        break;
                    }
                }

                if (got_node==cluster_node)
                {
                    chosen_node=i+1;
                    NDRX_LOG(log_debug, "one shot: use %d", chosen_node);
                    break;
                }
            }
            
            /* Break out if node is chosen. */
            if (EXFAIL!=chosen_node)
            {
                break; 
            }
            
            try++;
        }
        
        if (EXFAIL!=chosen_node)
        {
/* 
 * only for epoll/fdpoll/kqueue(). For poll we do recursive call for service selection 
 * System V mode uses the same approach as for 
 */
#if defined(EX_USE_EPOLL) || defined(EX_USE_FDPOLL) || defined(EX_USE_KQUEUE) || defined(EX_USE_SVAPOLL)
            sprintf(send_q, NDRX_SVC_QBRDIGE, 
                    G_atmi_tls->G_atmi_conf.q_prefix, chosen_node);
#endif
            *is_bridge=EXTRUE;
        }
        else
        {
            NDRX_LOG(log_error, "Service [%s] not in cluster!",
                            svc);
            ret=EXFAIL;
        }
    }
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    else
    {
        int resid;
        int resrr;
        
        /* ###################### CRITICAL SECTION ############################### */
        /* lock for round-robin... */

        if (EXSUCCEED!=ndrx_lock_svc_nm(svc, __func__))
        {
            NDRX_LOG(log_error, "Failed to sem-lock service: %s", svc);
            EXFAIL_OUT(ret);
        }
        
        psvcinfo->resrr++;
        
        if (psvcinfo->resrr < 0 || /* just in case... */
                psvcinfo->resrr >= psvcinfo->resnr)
        {
            psvcinfo->resrr = 0;
        }
        
        resrr = psvcinfo->resrr;
        
        resid = psvcinfo->resids[resrr].resid;
        
        if (EXSUCCEED!=ndrx_unlock_svc_nm(svc, __func__))
        {
            NDRX_LOG(log_error, "Failed to sem-unlock service: %s", svc);
            EXFAIL_OUT(ret);
        }
        /* ###################### CRITICAL SECTION, END ########################## */
        
        /* OK we got an resource id, lets translate it to actual queue */
        
        /* lets have an actual callback to backend for providing the queue
         * for service
         */
        
        sprintf(send_q, NDRX_SVC_QFMT_SRVID, G_atmi_tls->G_atmi_conf.q_prefix, 
                svc, resid);
        
        if (EXSUCCEED!=ndrx_epoll_service_translate(send_q, 
                G_atmi_tls->G_atmi_conf.q_prefix, svc, resid))
        {
            NDRX_LOG(log_error, "Failed to translate svc [%s] resid=%d to "
                    "queue resrr=%d", 
                    svc, resid, resrr);
            userlog("Failed to translate svc [%s] resid=%d resrr=%d to queue", 
                    svc, resid, resrr);
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "Choosing local service by round-robin mode, "
                "rr: %d, srvid: %d, q: [%s]", resrr, resid, send_q);
    }
    
    if (*is_bridge && 0!=strncmp(svc, NDRX_SVC_BRIDGE, NDRX_SVC_BRIDGE_STATLEN))
    {
        char tmpsvc[MAXTIDENT+1];
        
        snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SVC_BRIDGE, chosen_node);
        
        NDRX_LOG(log_debug, "Recursive service lookup: [%s] ret %d", tmpsvc, ret);
        ret = ndrx_shm_get_svc(tmpsvc, send_q, is_bridge, NULL);
        *is_bridge = EXTRUE;
    }
    
#endif
    
out:
    NDRX_LOG(log_debug, "ndrx_shm_get_svc returns %d", ret);

    return ret;
}


/**
 * Returns list of servers providing the service (it does the malloc)
 * for poll() mode only.
 * @param svc
 * @param srvlist list of servers/resource id (mqd for system v)
 * @return 
 */
expublic int ndrx_shm_get_srvs(char *svc, ndrx_shm_resid_t **srvlist, int *len)
{
    int ret=EXSUCCEED;
    int pos=EXFAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    shm_svcinfo_t *psvcinfo = NULL;
    int local_count;
    
    *len = 0;
    
    if (!ndrx_shm_is_attached(&G_svcinfo))
    {
        ret=EXFAIL;
        goto out; /* do not fail, try locally */
    }
    
    if (EXSUCCEED!=ndrx_lock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-lock service: %s", svc);
        EXFAIL_OUT(ret);
    }
    
    /* Get the service entry */
    if (!_ndrx_shm_get_svc(svc, &pos, NDRX_SVCINSTALL_NOT, NULL))
    {
        NDRX_LOG(log_error, "Service %s not found in shm", svc);
        EXFAIL_OUT(ret);
    }
    
    psvcinfo = SHM_SVCINFO_INDEX(svcinfo, pos);
            
    
    local_count = psvcinfo->resnr;
    
    if (local_count<=0)
    {
        NDRX_LOG(log_error, "Service %s not available, count of servers: %d",
                                  svc, psvcinfo->srvs);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(*srvlist = NDRX_MALLOC(sizeof(ndrx_shm_resid_t) *local_count )))
    {
        NDRX_LOG(log_error, "malloc fail: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(*srvlist, &(psvcinfo->resids[0]), sizeof(ndrx_shm_resid_t) *local_count);

    *len = local_count;
    
out:

    if (EXSUCCEED!=ndrx_unlock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-unlock service: %s", svc);
    }

not_locked:
    
    NDRX_LOG(log_debug, "ndrx_shm_get_srvs: srvlist %p, ret %d, len %d",
        *srvlist, ret, *len);

    return ret;
}

/**
 * return number of service active in system
 * @return nr_services / EXFAIL
 */
expublic int ndrx_shm_get_svc_count(void)
{
    int ret = 0;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int i;
    
    if (!ndrx_shm_is_attached(&G_svcinfo))
    {
        /* no SHM infos */
        NDRX_LOG(log_debug, "SHM not attached -> no service count");
        ret=EXFAIL;
        goto out;
    }
        
    for (i=0; i< G_max_svcs; i++)
    {
        shm_svcinfo_t* ent = SHM_SVCINFO_INDEX(svcinfo, i);
        
        if (ent->flags & NDRXD_SVCINFO_INIT 
                && ent->srvs > 0)
        {
            ret++;
        }
    }
out:
    return ret;
}

/**
 * Search over the memory for service.
 * This is internal version and not meant be used outside of this file.
 * @param svc - service name
 * @param pos - store the last position
 * @param doing_install - we are doing service install, 
 *	thus we can return empty positions.
 *      In value: NDRX_SVCINSTALL_NOT
 *      In value: NDRX_SVCINSTALL_DO
 * @param p_install_cmd - Return command of installation process
 *	see values of _NDRX_SVCINSTALL_*
 *      Out value: NDRX_SVCINSTALL_OVERWRITE -> we got position for write (may be old or new)
 *      the caller is responsible for resetting to 0 this param value before call
 * @return TRUE/FALSE found existing value
 */
expublic int _ndrx_shm_get_svc(char *svc, int *pos, int doing_install, int *p_install_cmd)
{
    int ret=EXFALSE;
    int try = EXFAIL;
    int start = try;
    int overflow = EXFALSE;
    int iterations = 0;
    
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    
    /* Bug #349 20/10/2018
     * If doing write, we have to ensure that some stale service
     * does not block the our existing service at next position
     * thus firstly for writes we need to perform recursive read only lookup
     * and maybe we could re-use that existing index!
     */
    if (NDRX_SVCINSTALL_DO==doing_install)
    {
        int try_read = EXFAIL;
        
        if (_ndrx_shm_get_svc(svc, &try_read, NDRX_SVCINSTALL_NOT, NULL))
        {
            try = try_read;
        }
    }
    
    if (EXFAIL==try)
    {
        try = ndrx_hash_fn(svc) % G_max_svcs;
    }
    else
    {
        NDRX_LOG(log_debug, "Read only existing service [%s] found at [%d]", 
                svc, try);
    }
    
    /* Bug #475 Fix */
    start = try;
    *pos=EXFAIL;
    
    NDRX_LOG(log_debug, "Key for [%s] is %d, shm is: %p", 
                                        svc, try, svcinfo);
    
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((SHM_SVCINFO_INDEX(svcinfo, try)->flags & NDRXD_SVCINFO_INIT)
            && (!overflow || (overflow && try < start)))
    {
        if (0==strcmp(SHM_SVCINFO_INDEX(svcinfo, try)->service, svc))
        {
            ret=EXTRUE;
            *pos=try;
            break;  /* <<< Break! */
        }
        
        /* Feature #139 mvitolin, 09/05/2017 
         * Allow to reuse services... As we know we do not remove them from SHM
         * so that if we have some service with the same hash number, but different
         * name than existing. Thus if we remove existing and put empty space there
         * then our service search algorithm will identify service as not present.
         * 
         * But to save the space and we install new service and the cell was used
         * but is serving 0 services, then we write off new service here.
         */
        if (NDRX_SVCINSTALL_DO==doing_install)
        {
            if (SHM_SVCINFO_INDEX(svcinfo, try)->srvs == 0)
            {
                *p_install_cmd=NDRX_SVCINSTALL_OVERWRITE;
                break; /* <<< break! */
            }
        }

        try++;
        
        /* we loop over... 
         * Feature #139 mvitolin, 09/05/2017
         * Fix potential overflow issues at the border... of SHM...
         */
        if (try>=G_max_svcs)
        {
            try = 0;
            overflow=EXTRUE;
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", svc);
        }
        iterations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, svc);
    }
    
    /* 
     * In case if doing install 
     * and memory cell found is not initialized
     * then we can use it.
     */
    if (NDRX_SVCINSTALL_DO==doing_install &&
            *p_install_cmd!=NDRX_SVCINSTALL_OVERWRITE &&
            !(SHM_SVCINFO_INDEX(svcinfo, try)->flags & NDRXD_SVCINFO_INIT))
    {
        *p_install_cmd=NDRX_SVCINSTALL_OVERWRITE;
    }
    
    *pos=try;
    NDRX_LOG(log_debug, "ndrx_shm_get_svc [%s] - result: %d, "
                            "iterations: %d, pos: %d, install: %d",
                             svc, ret, iterations, *pos, 
                             (doing_install?*p_install_cmd:NDRX_SVCINSTALL_NOT));
    return ret;
}

/**
 * Install service data into shared memory.
 * If service already found, then just update the flags.
 * If service is not found the add details to shm.
 * !!! Must be run from semaphore locked area!
 * @param svc
 * @param flags
 * @return SUCCEED/FAIL
 */
expublic int ndrx_shm_install_svc_br(char *svc, int flags, 
                int is_bridge, int nodeid, int count, char mode, int resid)
{
    int ret=EXSUCCEED;
    int pos = EXFAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int i;
    int is_new;
    int shm_install_cmd = NDRX_SVCINSTALL_NOT;
    shm_svcinfo_t* el;
    
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    if (EXSUCCEED!=ndrx_lock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-lock service: %s", svc);
        ret=EXFAIL;
        goto lock_fail;
    }
#endif
    
    if (_ndrx_shm_get_svc(svc, &pos, NDRX_SVCINSTALL_DO, &shm_install_cmd))
    {
        NDRX_LOG(log_debug, "Updating flags for [%s] from %d to %d",
                svc, SHM_SVCINFO_INDEX(svcinfo, pos)->flags, flags);
        el = SHM_SVCINFO_INDEX(svcinfo, pos);
        /* service have been found at position, update flags */
        el->flags = flags | NDRXD_SVCINFO_INIT;
        
        /* If this is cluster & entry exits or count<=0, then do not increment. */
        if (!is_bridge || 
                (0==el->cnodes[nodeid-1].srvs && count>0))
        {

#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
                    
            if (!is_bridge && (el->resnr+1 > G_atmi_env.maxsvcsrvs))
            {
                NDRX_LOG(log_error, "Shared mem for svc [%s] is full - "
                        "max space for servers per service: %d! Currently: "
                        "srvs: %d csrvs:%d resnr:%hd",
                        svc, G_atmi_env.maxsvcsrvs, 
                        el->srvs,
                        el->csrvs,
                        el->resnr
                        );
                userlog("Shared mem for svc [%s] is full - "
                        "max space for servers per service: %d! Currently: "
                        "srvs: %d csrvs:%d resnr:%hd",
                        svc, G_atmi_env.maxsvcsrvs, 
                        el->srvs,
                        el->csrvs,
                        el->resnr);
                EXFAIL_OUT(ret);
            }
            else if (!is_bridge)
            {
                /* Add it to the array... */
                /* so we use the next number */
                int idx = EXFAIL;
                
                for (i=0; i < el->resnr; i++)
                {
                    if (el->resids[i].resid==resid)
                    {
                        idx = i;
                        el->resids[i].cnt++;
                        NDRX_LOG(log_debug, "installed resid/srvid %d at %d "
                                "increased to %hd", 
                                resid, idx, el->resids[i].cnt);
                    }
                }
                
                if (EXFAIL==idx)
                {
                    el->resids[el->resnr].resid = resid;
                    el->resids[el->resnr].cnt = 1;
                    NDRX_LOG(log_debug, "installed resid/srvid %d at %d", 
                            resid, el->resnr);
                    el->resnr++;
                }
            }
#endif
            el->srvs++;
            
            if (is_bridge)
            {
                el->csrvs++;
            }
            
        }
    }
    /* It is OK, if there is no entry, we just start from scratch! */
    else if (NDRX_SVCINSTALL_OVERWRITE==shm_install_cmd)
    {
        el = SHM_SVCINFO_INDEX(svcinfo, pos);
        is_new=EXTRUE;
        if (is_bridge && 0==count)
        {
            NDRX_LOG(log_debug, "Svc [%s] not found in shm, "
                    "and will not install bridged 0", svc);
            goto out;
        }
        else
        {
            NDRX_STRCPY_SAFE(el->service, svc);
            /* Basically just override the init flag */
            el->flags = flags | NDRXD_SVCINFO_INIT;
            NDRX_LOG(log_debug, "Svc [%s] not found in shm, "
                        "installed with flags %d",
                        el->service, 
                        el->flags);
            
            el->srvs++;
            
            if (is_bridge)
            {
                el->csrvs++;
            }
            else
            {
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
                int idx = 0;
                el->resids[idx].resid = resid;
                el->resids[idx].cnt = 1;
                NDRX_LOG(log_debug, "installed resid/srvid %d at idx %d", 
                        resid, idx);
                el->resnr++;
#endif
            }
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Cannot install [%s]!! There is no "
                "space in SHM! Try to increase %s",
                 svc, CONF_NDRX_SVCMAX);
        userlog("Cannot install [%s]!! There is no "
                "space in SHM! Try to increase %s",
                 svc, CONF_NDRX_SVCMAX);
        EXFAIL_OUT(ret);
    }
    
    /* we are ok & extra bridge processing */
    if (is_bridge)
    {
        int was_installed;
        el = SHM_SVCINFO_INDEX(svcinfo, pos);
        was_installed = (el->cnodes[nodeid-1].srvs > 0);
        /* our index starts with 0 */
        if (BRIDGE_REFRESH_MODE_FULL==mode)
        {
            /* Install fresh stuff */
            el->cnodes[nodeid-1].srvs = count;
            /* Add real count */
            NDRX_LOG(log_debug, "SHM Service refresh: [%s] Bridge: [%d]"
                    " Count: [%d]",
                    svc, nodeid, count);
        }
        else
        {
            el->cnodes[nodeid-1].srvs+=count;
            
            /* If there was dropped update..! */
            if (el->cnodes[nodeid-1].srvs<0)
            {
                el->cnodes[nodeid-1].srvs=0;
            }
            NDRX_LOG(log_debug, "SHM Service update: [%s] Bridge: "
                    "[%d] Diff: %d final count: [%d], cluster nodes: [%d]",
                    svc, nodeid, count, el->cnodes[nodeid-1].srvs, 
                    el->csrvs);
        }
        
        /* Note is being removed. */
        if (el->cnodes[nodeid-1].srvs<=0 && was_installed)
        {
            /* Reduce cluster nodes */
            el->csrvs--;
            el->srvs--;
        }
        
        /* Might want to install due to bridge updates */
        if (0==el->csrvs && 
                0==el->srvs)
        {
            NDRX_LOG(log_debug, 
                    "Bridge %d caused to remove svc [%s] from shm",
                    nodeid, svc);
            /* Clean up memory block. 
            memset(&svcinfo[pos], 0, sizeof(svcinfo[pos]));
             * but we cannot clean it all. Because during shutdown
             * removal of some svc can cause svc with same hash code to target
             * first cell not second one.
             * 
             * So we will just reset key fields, but flags & svc stays the same.  
             */
            
            memset(&el->cnodes, 0, 
                    sizeof(el->cnodes));
            el->totclustered = 0;
            
            goto out;
        }
        
        /*Get the max node id so that client can scan the stuff!
         * If service is removed, we might want to re-scan the list
         * and rebuild cnodes_max_id for better hint to clients.
         */
        if (nodeid > el->cnodes_max_id)
        {
            el->cnodes_max_id = nodeid;
        }
        
        /* Rebuild totals of cluster... */
        el->totclustered = 0;
        for (i=0; i<el->cnodes_max_id; i++)
        {
            el->totclustered+=
                        el->cnodes[i].srvs;
        }
        NDRX_LOG(log_debug, "Total clustered services: %d", 
                SHM_SVCINFO_INDEX(svcinfo, pos)->totclustered);
    }
out:

#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    if (EXSUCCEED!=ndrx_unlock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-unlock service: %s", svc);
    }
#endif
    
lock_fail:

    return ret;
}

/**
 * Wrapper for bridged version
 * !!! Must be run from semaphore locked area!
 * @param svc service name to install to shared memory
 * @param flags install flags
 * @param resid fo POLL mode it is server id, for SYSV mode it is QID
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_install_svc(char *svc, int flags, int resid)
{
    return ndrx_shm_install_svc_br(svc, flags, EXFALSE, 0, 0, 0, resid);
}

/**
 * Uninstall service from shared mem.
 * TODO: We should have similar function with counter for shutdown requested binaries
 * so that if shutdown was requested, service is no more called by other participans.
 * (if shutdown request was made to last available instance).
 * 
 * TOOD: Yep, counter is needed for shutdown requests. If requested count == srvs, then
 * We set up flag, that service is no more available, and this should be checked by tpcall!
 * 
 * TODO: We will want to compact the SHM when some service which have next one with same
 * hash is removed. so basically in reverse order we check, that service have some hash
 * but position is already taken by different service. The we chech the count of that different
 * position. If count == 0, then we can move service there, if not 0, then move to next pos.
 * But hmm.. this might require to have a readlock while doing write??? As service data is moved around :S
 * but if we have big svcmax, then probably this is not a problem (while we have fixed list of services).
 * 
 * @param svc
 * @param flags
 * @return SUCCEED/FAIL
 */
expublic void ndrxd_shm_uninstall_svc(char *svc, int *last, int resid)
{
    int pos = EXFAIL;
    int i;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int lpos;
    shm_svcinfo_t* el;
    
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    if (EXSUCCEED!=ndrx_lock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-lock service: %s", svc);
        return;
    }
#endif
    
    *last=EXFALSE;
    if (_ndrx_shm_get_svc(svc, &pos, NDRX_SVCINSTALL_NOT, NULL))
    {
        el = SHM_SVCINFO_INDEX(svcinfo, pos);
        if (el->srvs>1)
        {
            NDRX_LOG(log_debug, "Decreasing count of servers for "
                                "[%s] from %d to %d (resnr=%d)",
                                svc, el->srvs, 
                                el->srvs-1, el->resnr);

#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)

            /*
            tot_local_srvs = el->srvs - 
                    el->csrvs;
              */          
            lpos = EXFAIL;
            for (i=0; i<el->resnr; i++)
            {
                if (el->resids[i].resid==resid)
                {
                    lpos = i;
                    break;
                }
            }
            
            if (EXFAIL!=lpos)
            {
                NDRX_LOG(log_debug, "Local count: %hd", el->resids[lpos].cnt);
                
                if (el->resids[lpos].cnt>1)
                {
                    el->resids[lpos].cnt--;
                    NDRX_LOG(log_debug, "Resource %d decrement to %hd",
                            el->resids[lpos].resid, el->resids[lpos].cnt);
                }
                else if (lpos==el->resnr-1)
                {
                    el->resnr--;
                    NDRX_LOG(log_debug, "Server was at last position, "
                            "just shrink the numbers...");
                }
                else
                {
                    NDRX_LOG(log_debug, "Reducing the local server array...");
                    memmove(&(el->resids[lpos]),
                            &(el->resids[lpos+1]),
                            (el->resnr - lpos -1)*sizeof(el->resids[0]));
                    el->resnr--;
                }
            }
            
#endif
            el->srvs--;
        }
        else
        {
            el = SHM_SVCINFO_INDEX(svcinfo, pos);
        
            NDRX_LOG(log_debug, "Removing service from shared mem "
                                "[%s]",
                                svc);
            /* Clean up memory block. 
            memset(&svcinfo[pos], 0, sizeof(svcinfo[pos]));
            */
            
            /* Once service was added to system, we do not remove it
             * automatically, unless we resolve problems with linear hash. 
             */
            memset(&el->cnodes, 0, 
                    sizeof(el->cnodes));
            el->totclustered = 0;
            el->csrvs = 0;
            el->srvs = 0;
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
            el->resnr = 0;
            el->resrr = 0;
            el->resrr = 0;
#endif
            
            *last=EXTRUE;
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Service [%s] not present in shm", svc);
        *last=EXTRUE;
    }
    
#if defined(EX_USE_POLL) || defined(EX_USE_SYSVQ)
    if (EXSUCCEED!=ndrx_unlock_svc_nm(svc, __func__))
    {
        NDRX_LOG(log_error, "Failed to sem-unlock service: %s", svc);
        return;
    }
#endif
    
}

/**
 * Reset shared memory block
 * @param srvid
 */
expublic void ndrxd_shm_resetsrv(int srvid)
{
    shm_srvinfo_t *srv = ndrxd_shm_getsrv(srvid);
    if (NULL!=srv)
        memset(srv, 0, sizeof(shm_srvinfo_t));
}

/**
 * Get handler for server
 * Well event if we run with out ndrxd, we shall open the shared memory
 * blocks. This will make streamline testing in different modes, Posix and System V
 * @param srvid
 * @return
 */
expublic shm_srvinfo_t* ndrxd_shm_getsrv(int srvid)
{
    shm_srvinfo_t *ret=NULL;
    int pos=EXFAIL;
    shm_srvinfo_t *srvinfo = (shm_srvinfo_t *) G_srvinfo.mem;

    if (!ndrx_shm_is_attached(&G_srvinfo))
    {
        ret=NULL;
        goto out;
    }

    if (srvid >-1 && srvid < G_max_servers)
    {
        ret=&srvinfo[srvid];
    }
    else
    {
        NDRX_LOG(log_error, "Invalid srvid requested to "
                            "ndrxd_shm_getsrv => %d", srvid);
        ret=NULL;
    }

out:
    return ret;
}

/**
 * Return list of connected nodes, installed in array byte positions.
 * @return SUCCEED/FAIL
 */
expublic int ndrx_shm_birdge_getnodesconnected(char *outputbuf)
{
    int ret=EXSUCCEED;
    int *brinfo = (int *) G_brinfo.mem;
    int i;
    int pos=0;
    
    if (!ndrx_shm_is_attached(&G_brinfo))
    {
        EXFAIL_OUT(ret);
    }
    
    for (i=1; i<=CONF_NDRX_NODEID_COUNT; i++)
    {
        if (brinfo[i-1] & NDRX_SHM_BR_CONNECTED)
        {
            outputbuf[pos] = i;
            pos++;
        }
    }
    
out:
    return ret;
}


/**
 * Mark in the shm, that node is connected
 * @return 
 */
expublic int ndrx_shm_birdge_set_flags(int nodeid, int flags, int op_end)
{
    int ret=EXSUCCEED;
    int *brinfo = (int *) G_brinfo.mem;

    if (!ndrx_shm_is_attached(&G_brinfo))
    {
        ret=EXFAIL;
        goto out;
    }

    if (nodeid >= CONF_NDRX_NODEID_MIN && nodeid <= CONF_NDRX_NODEID_MAX)
    {
        if (op_end)
            brinfo[nodeid-1]&=flags;
        else
            brinfo[nodeid-1]|=flags;
    }
    else
    {
        NDRX_LOG(log_error, "Invalid nodeid requested to "
                            "shm_mark_br_connected => %d", nodeid);
        ret=EXFAIL;
    }

out:
    return ret;
}

/**
 * Unmark bridge
 * @param nodeid
 * @return 
 */
expublic int ndrx_shm_bridge_disco(int nodeid)
{
    return ndrx_shm_birdge_set_flags(nodeid, ~NDRX_SHM_BR_CONNECTED, EXTRUE);
}


/**
 * Mark bridge connected.
 * @param nodeid
 * @return TRUE/FALSE
 */
expublic int ndrx_shm_bridge_connected(int nodeid)
{
    return ndrx_shm_birdge_set_flags(nodeid, NDRX_SHM_BR_CONNECTED, EXFALSE);
}

/**
 * Check is bridge copnnected. TODO: What heppens if ndrxd restarts and bridges do some stuff?
 * @param nodeid
 * @return 
 */
expublic int ndrx_shm_bridge_is_connected(int nodeid)
{
    int *brinfo = (int *) G_brinfo.mem;
    int ret=EXFALSE;
    
    if (!ndrx_shm_is_attached(&G_brinfo))
    {
        goto out;
    }

    if (nodeid >= CONF_NDRX_NODEID_MIN && nodeid <= CONF_NDRX_NODEID_MAX)
    {
        if (brinfo[nodeid-1]&NDRX_SHM_BR_CONNECTED)
        {
            ret=EXTRUE;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Invalid nodeid requested to "
                            "shm_is_br_connected => %d", nodeid);
    }

out:
    return ret;
}



/* vim: set ts=4 sw=4 et smartindent: */
