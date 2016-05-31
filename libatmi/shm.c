/* 
** Common shared memory routines for EnduroX.
** Generally we do not use any global vars because this stuff should
** generic one.
** SHM Example see here:
** http://mij.oltrelinux.com/devel/unixprg/
** General rule regarding the SVCINFO shared memory area is, that we do not
** Delete any services out from that area. That ensures that we can alwasy search
** The data up!
**
** @file shm.c
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
#include <errno.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <atmi_shm.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public ndrx_shm_t G_srvinfo;
public ndrx_shm_t G_svcinfo;
public ndrx_shm_t G_brinfo;     /* Info about bridges */

public int G_max_servers   = FAIL;         /* max servers         */
public int G_max_svcs      = FAIL;         /* max svcs per server */

int M_init = FALSE;                 /* no init yet done */
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialise prefix part, that is needed for shm...
 * @param ndrx_prefix
 * @return 
 */
public int shm_init(char *q_prefix, int max_servers, int max_svcs)
{
    memset(&G_srvinfo, 0, sizeof(G_srvinfo));
    memset(&G_svcinfo, 0, sizeof(G_svcinfo));
    memset(&G_brinfo, 0, sizeof(G_brinfo));

    G_svcinfo.fd = FAIL;
    G_srvinfo.fd = FAIL;
    G_brinfo.fd = FAIL;
    
    sprintf(G_srvinfo.path, NDRX_SHM_SRVINFO, q_prefix);
    sprintf(G_svcinfo.path, NDRX_SHM_SVCINFO, q_prefix);
    sprintf(G_brinfo.path,  NDRX_SHM_BRINFO,  q_prefix);
    
    G_max_servers = max_servers;
    G_max_svcs = max_svcs;

    /* Initialise sizes */

    G_srvinfo.size = sizeof(shm_srvinfo_t)*max_servers;
    NDRX_LOG(log_debug, "G_srvinfo.size = %d (%d * %d)",
                    G_srvinfo.size, sizeof(shm_srvinfo_t), max_servers);
    
    G_svcinfo.size = sizeof(shm_svcinfo_t)*max_svcs;
    NDRX_LOG(log_debug, "G_svcinfo.size = %d (%d * %d)",
                    G_svcinfo.size, sizeof(shm_svcinfo_t), max_svcs);
   
    G_brinfo.size = sizeof(int)*CONF_NDRX_NODEID_COUNT;
    NDRX_LOG(log_debug, "G_brinfo.size = %d (%d * %d)",
                    G_svcinfo.size, sizeof(int), CONF_NDRX_NODEID_COUNT);
   
    
    M_init = TRUE;
    return SUCCEED;
}

/**
 * Close opened shared memory segment.
 * @return
 */
private int ndrxd_shm_close(ndrx_shm_t *shm)
{
    int ret=SUCCEED;

    /**
     * Library not initialized
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm library not initialized");
        ret=FAIL;
        goto out;
    }

    if (shm->fd > 2)
    {
        ret = close(shm->fd);
        if (SUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to close shm [%s]: %d - %s",
                        errno, strerror(errno));
        }
    }
    else
    {
        NDRX_LOG(log_error, "cannot close shm [%s] as fd is %d",
                    shm->path, shm->fd);
        ret=FAIL;
        goto out;
    }

out:
    return ret;
}

/**
 * Open service info shared memory segment
 * @return
 */
private int ndrxd_shm_open(ndrx_shm_t *shm)
{
    int ret=SUCCEED;
    char *fn = "ndrxd_shm_open";

    NDRX_LOG(log_debug, "%s enter", fn);
    /**
     * Library not initialized
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm library not initialized");
        ret=FAIL;
        goto out;
    }

    /* creating the shared memory object --  shm_open() */
    shm->fd = shm_open(shm->path, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);

    if (shm->fd < 0) {
        NDRX_LOG(log_error, "%s: Failed to create shm [%s]: %s",
                            fn, shm->path, strerror(errno));
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=ftruncate(shm->fd, shm->size))
    {
        NDRX_LOG(log_error, "%s: Failed to set [%s] fd: %d to size %d bytes: %s",
                            fn, shm->path, shm->fd, shm->size, strerror(errno));
        ret=FAIL;
        goto out;        
    }

    shm->mem = (char *)mmap(NULL, shm->size, 
                        PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (MAP_FAILED==shm->mem)
    {
        NDRX_LOG(log_error, "%s: Failed to map memory for [%s] fd %d bytes %d: %s",
                            fn, shm->path, shm->fd, shm->size, strerror(errno));
        ret=FAIL;
        goto out;
    }
    /* Reset SHM */
    memset(shm->mem, 0, shm->size);
    NDRX_LOG(log_debug, "Shm: [%s] created", shm->path);
    
out:
    /*
     * Should close the SHM if failed to open.
     */
    if (SUCCEED!=ret && FAIL!=shm->fd)
    {
        if (shm_unlink(shm->path) != 0) {
            NDRX_LOG(log_error, "%s: Failed to unlink [%s]: %s",
                            fn, shm->path, strerror(errno));
        }
    }

    NDRX_LOG(log_debug, "%s return %d", fn, ret);

    return ret;
}

/**
 * Open shared memory
 * @return
 */
public int ndrxd_shm_open_all(void)
{
    int ret=SUCCEED;

    if (SUCCEED!=ndrxd_shm_open(&G_srvinfo))
    {
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=ndrxd_shm_open(&G_svcinfo))
    {
        ret=FAIL;
        goto out;
    }

    if (SUCCEED!=ndrxd_shm_open(&G_brinfo))
    {
        ret=FAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Closes all shared memory resources, generally ignores errors.
 * @return FAIL if something failed.
 */
public int ndrxd_shm_close_all(void)
{
    int ret=SUCCEED;

    ret=ndrxd_shm_close(&G_srvinfo);

    if (FAIL==ndrxd_shm_close(&G_svcinfo))
        ret=FAIL;

    if (FAIL==ndrxd_shm_close(&G_brinfo))
        ret=FAIL;
    
    return ret;
}

/**
 * Does delete all shared memory blocks.
 */
public int ndrxd_shm_delete(void)
{
    if (M_init)
    {
        if (SUCCEED!=shm_unlink(G_srvinfo.path))
        {
            NDRX_LOG(log_error, "Failed to remove: [%s]: %s",
                                G_srvinfo.path, strerror(errno));
        }
        if (SUCCEED!=shm_unlink(G_svcinfo.path))
        {
            NDRX_LOG(log_error, "Failed to remove: [%s]: %s",
                                G_svcinfo.path, strerror(errno));
        }
        if (SUCCEED!=shm_unlink(G_brinfo.path))
        {
            NDRX_LOG(log_error, "Failed to remove: [%s]: %s",
                                G_brinfo.path, strerror(errno));
        }
    }
    else
    {
            NDRX_LOG(log_error, "SHM library not initialized!");
            return FAIL;
    }

    return SUCCEED;
}

/**
 * Returns true if currently attached to shm
 * WARNING: This assumes that fd 0 could never be used by shm!
 * @return TRUE/FALSE
 */
public int ndrxd_shm_is_attached(ndrx_shm_t *shm)
{
    int ret=TRUE;
    
    if (shm->fd <=0 || shm->fd <=0)
    {
        ret=FALSE;
    }

    return ret;
}

/**
 * Attach to shared memory block
 * @return
 */
public int ndrx_shm_attach(ndrx_shm_t *shm)
{
    int ret=SUCCEED;
    char *fn = "ndrx_shm_attach";

    NDRX_LOG(log_debug, "%s enter", fn);
    /**
     * Library not initialised
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "%s: ndrx shm library not initialised!", fn);
        ret=FAIL;
        goto out;
    }
    
    if (ndrxd_shm_is_attached(shm))
    {
        NDRX_LOG(log_debug, "%s: shm %s already attached", shm->path);
        goto out;
    }
    
    /* Attach to shared memory block */
    shm->fd = shm_open(shm->path, O_RDWR, S_IRWXU | S_IRWXG);

    if (shm->fd < 0) {
        NDRX_LOG(log_error, "%s: Failed to attach shm [%s]: %s",
                            fn, shm->path, strerror(errno));
        ret=FAIL;
        goto out;
    }

    shm->mem = (char *)mmap(NULL, shm->size,
                        PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (MAP_FAILED==shm->mem)
    {
        NDRX_LOG(log_error, "%s: Failed to map memory for [%s] fd %d bytes %d: %s",
                            fn, shm->path, shm->fd, shm->size, strerror(errno));
        ret=FAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Shm: [%s] attach", shm->path);

out:
    /*
     * Should close the SHM if failed to open.
     */
    if (SUCCEED!=ret)
    {
        shm->fd=FAIL;
    }

    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
}

/**
 * Attach to shared memory block.
 * @lev indicates the attach level (should it be service array only)?
 * @return 
 */
public int ndrx_shm_attach_all(int lev)
{
   int ret=SUCCEED;
   
   /* Attached to service shared mem */
   if (lev & NDRX_SHM_LEV_SVC)
   {
       if (SUCCEED!=ndrx_shm_attach(&G_svcinfo))
       {
            ret=FAIL;
            goto out;
       }
   }
   
   /* Attach to srv shared mem */
   if (lev & NDRX_SHM_LEV_SRV &&
           SUCCEED!=ndrx_shm_attach(&G_srvinfo))
   {
       ret=FAIL;
       goto out;
   }
   
   
   /* Attach to srv shared mem */
   if (lev & NDRX_SHM_LEV_BR &&
           SUCCEED!=ndrx_shm_attach(&G_brinfo))
   {
       ret=FAIL;
       goto out;
   }
   
out:
   return ret;
}

/**
 * Make key out of string. Gen
 */
static unsigned int ndrx_hash_fn( void *k )
{
    unsigned int hash = 5381;
    int c;
    char *str = (char *)k;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Returns true if service is available.
 * @param svc
 * @return 
 */
public int ndrx_shm_get_svc(char *svc, char *send_q, int *is_bridge)
{
    int ret=SUCCEED;
    int pos=FAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int use_cluster = FAIL;
    static int first = TRUE;
    
    *is_bridge=FALSE;
    
    /* Initially we stick to the local service */
    sprintf(send_q, NDRX_SVC_QFMT, G_atmi_conf.q_prefix, svc);
    
    if (!ndrxd_shm_is_attached(&G_svcinfo))
    {
        ret=FAIL;
        goto out;
    }
    
    /* Get the service entry */
    ret = _ndrx_shm_get_svc(svc, &pos);
    
    if (ret && svcinfo[pos].srvs<=0)
    {
        NDRX_LOG(log_error, "Service %s not available, count of servers: %d",
                                  svc, svcinfo[pos].srvs);
        ret=FAIL;
    }
    
    /* Now use the random to chose the service to send to */
    if (svcinfo[pos].srvs==svcinfo[pos].csrvs && svcinfo[pos].srvs>0)
    {
        use_cluster=TRUE;
    }
    else if (0==svcinfo[pos].csrvs)
    {
        use_cluster=FALSE;
    }
    
    NDRX_LOG(log_debug, "use_cluster=%d srvs=%d csrvs=%d", 
            use_cluster, svcinfo[pos].srvs, svcinfo[pos].csrvs);
    
    if (FAIL==use_cluster)
    {
        /* So we will randomize as we have services in local & in cluster */
        if (first)
        {
            first=FALSE;
            srandom(time(NULL));
        }
        
        if (0==G_atmi_env.ldbal)
        {
            /* Run locally */
            use_cluster=FALSE;
        }
        else if (100==G_atmi_env.ldbal)
        {
            /* Run in cluster */
            use_cluster=TRUE;
        }
        else
        {
            /* Use random */
            int n = rand()%100+1;
            if (n<=G_atmi_env.ldbal)
            {
                use_cluster = TRUE;
            }
            else
            {
                use_cluster = FALSE;
            }
        }
    }
 
    NDRX_LOG(log_debug, "use_cluster=%d srvs=%d csrvs=%d", 
        use_cluster, svcinfo[pos].srvs, svcinfo[pos].csrvs);


    /* So we are using cluster, */
    if (TRUE==use_cluster)
    {
        
        int csrvs = svcinfo[pos].csrvs;
        int cluster_node = rand()%svcinfo[pos].csrvs+1;
        int i;
        int chosen_node = FAIL;
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
                cluster_node, svcinfo[pos].cnodes_max_id);
        
        /* If cluster was modified (while we do not create read/write semaphores...!) */
        while (try<2)
        {
            /* First try, search the random server */
            for (i=0; i<svcinfo[pos].cnodes_max_id; i++)
            {
                if (svcinfo[pos].cnodes[i].srvs)
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
            if (FAIL!=chosen_node)
            {
                break; 
            }
            
            try++;
        }
        
        if (FAIL!=chosen_node)
        {
            sprintf(send_q, NDRX_SVC_QBRDIGE, G_atmi_conf.q_prefix, chosen_node);
            *is_bridge=TRUE;
        }
        else
        {
            NDRX_LOG(log_error, "Service [%s] not in cluster!",
                            svc);
            ret=FAIL;
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
 * @return TRUE/FALSE
 */
public int _ndrx_shm_get_svc(char *svc, int *pos)
{
    int ret=FALSE;
    int try = ndrx_hash_fn(svc) % G_max_svcs;
    int start = try;
    int overflow = FALSE;
    int interations = 0;
    
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;

    *pos=FAIL;
    
    NDRX_LOG(log_debug, "Key for [%s] is %d, shm is: %p", 
                                        svc, try, svcinfo);
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((svcinfo[try].flags & NDRXD_SVCINFO_INIT)
            && (!overflow || (overflow && try < start)))
    {
        if (0==strcmp(svcinfo[try].service, svc))
        {
            ret=TRUE;
            *pos=try;
            break;  /* <<< Break! */

        }

        try++;

        if (try>G_svcinfo.size-1)
        {
            try = 0;
            overflow=TRUE;
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", svc);
        }
        interations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, svc);
    }

    *pos=try;
    NDRX_LOG(log_debug, "ndrx_shm_get_svc [%s] - result: %d, "
                                    "interations: %d, pos: %d",
                                     svc, ret, interations, *pos);
    return ret;
}

/**
 * Install service data into shared memory.
 * If service already found, then just update the flags.
 * If service is not found the add details to shm.
 * @param svc
 * @param flags
 * @return SUCCEED/FAIL
 */
public int ndrx_shm_install_svc_br(char *svc, int flags, 
                int is_bridge, int nodeid, int count, char mode)
{
    int ret=SUCCEED;
    int pos = FAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;
    int i;
    int is_new;
    if (_ndrx_shm_get_svc(svc, &pos))
    {
        NDRX_LOG(log_debug, "Updating flags for [%s] from %d to %d",
                svc, svcinfo[pos].flags, flags);
        /* service have been found at position, update flags */
        svcinfo[pos].flags = flags | NDRXD_SVCINFO_INIT;
        
        /* If this is cluster & entry exits or count<=0, then do not increment. */
        if (!is_bridge || (0==svcinfo[pos].cnodes[nodeid-1].srvs && count>0))
        {
            svcinfo[pos].srvs++;
            
            if (is_bridge)
            {
                svcinfo[pos].csrvs++;
            }
            
        }
    }
    /* It is OK, if there is no entry, we just start from scratch! */
    else if (!(svcinfo[pos].flags & NDRXD_SVCINFO_INIT))
    {
        is_new=TRUE;
        if (is_bridge && 0==count)
        {
            NDRX_LOG(log_debug, "Svc [%s] not found in shm, "
                    "and will not install bridged 0", svc);
            goto out;
        }
        else
        {
            strcpy(svcinfo[pos].service, svc);
            /* Basically just override the init flag */
            svcinfo[pos].flags = flags | NDRXD_SVCINFO_INIT;
            NDRX_LOG(log_debug, "Svc [%s] not found in shm, "
                        "installed with flags %d",
                        svcinfo[pos].service, svcinfo[pos].flags);
            svcinfo[pos].srvs++;
            
            if (is_bridge)
            {
                svcinfo[pos].csrvs++;
            }
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Cannot install [%s]!! There is no "
                "space in SHM! Try to increase %s",
                 svc, CONF_NDRX_SVCMAX);
        ret=FAIL;
        goto out;
    }
    
    /* we are ok & extra bridge processing */
    if (is_bridge)
    {
        int was_installed = (svcinfo[pos].cnodes[nodeid-1].srvs > 0);
        /* our index starts with 0 */
        if (BRIDGE_REFRESH_MODE_FULL==mode)
        {
            /* Install fresh stuff */
            svcinfo[pos].cnodes[nodeid-1].srvs = count;
            /* Add real count */
            NDRX_LOG(log_debug, "SHM Service refresh: [%s] Bridge: [%d]"
                    " Count: [%d]",
                    svc, nodeid, count);
        }
        else
        {
            svcinfo[pos].cnodes[nodeid-1].srvs+=count;
            
            /* If there was dropped update..! */
            if (svcinfo[pos].cnodes[nodeid-1].srvs<0)
            {
                svcinfo[pos].cnodes[nodeid-1].srvs=0;
            }
            NDRX_LOG(log_debug, "SHM Service update: [%s] Bridge: "
                    "[%d] Diff: %d final count: [%d], cluster nodes: [%d]",
                    svc, nodeid, count, svcinfo[pos].cnodes[nodeid-1].srvs, 
                    svcinfo[pos].csrvs);
        }
        
        /* Note is being removed. */
        if (svcinfo[pos].cnodes[nodeid-1].srvs<=0 && was_installed)
        {
            /* Reduce cluster nodes */
            svcinfo[pos].csrvs--;
            svcinfo[pos].srvs--;
        }
        
        /* Might want to install due to bridge updates */
        if (0==svcinfo[pos].csrvs && 0==svcinfo[pos].srvs)
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
            
            memset(&svcinfo[pos].cnodes, 0, sizeof(svcinfo[pos].cnodes));
            svcinfo[pos].totclustered = 0;
            
            goto out;
        }
        
        /*Get the max node id so that client can scan the stuff!
         * If service is removed, we might want to re-scan the list
         * and rebuild cnodes_max_id for better hint to clients.
         */
        if (nodeid > svcinfo[pos].cnodes_max_id)
        {
            svcinfo[pos].cnodes_max_id = nodeid;
        }
        
        /* Rebuild totals of cluster... */
        svcinfo[pos].totclustered = 0;
        for (i=0; i<svcinfo[pos].cnodes_max_id; i++)
        {
            svcinfo[pos].totclustered+=svcinfo[pos].cnodes[i].srvs;
        }
        NDRX_LOG(log_debug, "Total clustered services: %d", 
                svcinfo[pos].totclustered);
    }
    
out:
    return ret;
}

/**
 * Wrapper for bridged version
 * @param svc
 * @param flags
 * @return 
 */
public int ndrx_shm_install_svc(char *svc, int flags)
{
    return ndrx_shm_install_svc_br(svc, flags, FALSE, 0, 0, 0);
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
public void ndrxd_shm_uninstall_svc(char *svc, int *last)
{
    int pos = FAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;

    *last=FALSE;
    if (_ndrx_shm_get_svc(svc, &pos))
    {
        if (svcinfo[pos].srvs>1)
        {
            NDRX_LOG(log_debug, "Decreasing count of servers for "
                                "[%s] from %d to %d",
                                svc, svcinfo[pos].srvs, svcinfo[pos].srvs-1);
            svcinfo[pos].srvs--;
        }
        else
        {
            NDRX_LOG(log_debug, "Removing service from shared mem "
                                "[%s]",
                                svc);
            /* Clean up memory block. 
            memset(&svcinfo[pos], 0, sizeof(svcinfo[pos]));
            */
            
            /* Once service was added to system, we do not remove it
             * automatically, unless we resolve problems with linear hash. 
             */
            memset(&svcinfo[pos].cnodes, 0, sizeof(svcinfo[pos].cnodes));
            svcinfo[pos].totclustered = 0;
            svcinfo[pos].csrvs = 0;
            svcinfo[pos].srvs = 0;
            
            *last=TRUE;
        }
    }
    else
    {
            NDRX_LOG(log_debug, "Service [%s] not present in shm", svc);
            *last=TRUE;
    }
    
}


public void ndrxd_shm_shutdown_svc(char *svc, int *last)
{
    int pos = FAIL;
    shm_svcinfo_t *svcinfo = (shm_svcinfo_t *) G_svcinfo.mem;

    *last=FALSE;
    if (_ndrx_shm_get_svc(svc, &pos))
    {
        if (svcinfo[pos].srvs>1)
        {
            NDRX_LOG(log_debug, "Decreasing count of servers for "
                                "[%s] from %d to %d",
                                svc, svcinfo[pos].srvs, svcinfo[pos].srvs-1);
            svcinfo[pos].srvs--;
        }
        else
        {
            NDRX_LOG(log_debug, "Removing service from shared mem "
                                "[%s]",
                                svc);
            /* Clean up memory block. */
            memset(&svcinfo[pos], 0, sizeof(svcinfo[pos]));
            *last=TRUE;
        }
    }
    else
    {
            NDRX_LOG(log_debug, "Service [%s] not present in shm");
            *last=TRUE;
    }
    
}

/**
 * Reset shared memory block
 * @param srvid
 */
public void ndrxd_shm_resetsrv(int srvid)
{
    shm_srvinfo_t *srv = ndrxd_shm_getsrv(srvid);
    if (NULL!=srv)
        memset(srv, 0, sizeof(shm_srvinfo_t));
}

/**
 * Get handler for server
 * @param srvid
 * @return
 */
public shm_srvinfo_t* ndrxd_shm_getsrv(int srvid)
{
    shm_srvinfo_t *ret=NULL;
    int pos=FAIL;
    shm_srvinfo_t *srvinfo = (shm_srvinfo_t *) G_srvinfo.mem;

    if (!ndrxd_shm_is_attached(&G_srvinfo))
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
 * Mark in the shm, that node is connected
 * @return 
 */
public int ndrx_shm_birdge_getnodesconnected(char *outputbuf)
{
    int ret=SUCCEED;
    int *brinfo = (int *) G_brinfo.mem;
    int i;
    int pos=0;
    
    if (!ndrxd_shm_is_attached(&G_brinfo))
    {
        ret=FAIL;
        goto out;
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
public int ndrx_shm_birdge_set_flags(int nodeid, int flags, int op_end)
{
    int ret=SUCCEED;
    int *brinfo = (int *) G_brinfo.mem;

    if (!ndrxd_shm_is_attached(&G_brinfo))
    {
        ret=FAIL;
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
        ret=FAIL;
    }

out:
    return ret;
}

/**
 * Unmark bridge
 * @param nodeid
 * @return 
 */
public int ndrx_shm_bridge_disco(int nodeid)
{
    return ndrx_shm_birdge_set_flags(nodeid, ~NDRX_SHM_BR_CONNECTED, TRUE);
}


/**
 * Mark bridge connected.
 * @param nodeid
 * @return TRUE/FALSE
 */
public int ndrx_shm_bridge_connected(int nodeid)
{
    return ndrx_shm_birdge_set_flags(nodeid, NDRX_SHM_BR_CONNECTED, FALSE);
}

/**
 * Check is bridge copnnected. TODO: What heppens if ndrxd restarts and bridges do some stuff?
 * @param nodeid
 * @return 
 */
public int ndrx_shm_bridge_is_connected(int nodeid)
{
    int *brinfo = (int *) G_brinfo.mem;
    int ret=FALSE;
    
    if (!ndrxd_shm_is_attached(&G_brinfo))
    {
        goto out;
    }

    if (nodeid >= CONF_NDRX_NODEID_MIN && nodeid <= CONF_NDRX_NODEID_MAX)
    {
        if (brinfo[nodeid-1]&NDRX_SHM_BR_CONNECTED)
        {
            ret=TRUE;
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



