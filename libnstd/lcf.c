/**
 * @brief Latent Command Framework - real time settings frameworks for all Enduro/X
 *  processes (clients & servers)
 *
 * @file lcf.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <lcf.h>
#include <ndebug.h>
#include <nstd_shm.h>
#include <ndebugcmn.h>
#include <sys_unix.h>

#include "exsha1.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_LCFMAX_DFLT         20      /**< Default max commands       */
#define MAX_READERS_DFLT        50      /**< Max readers for RW lock... */
#define MAX_QUEUES_DLFT         20000   /**< Max number of queues, dflt */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_nstd_libconfig_t ndrx_G_libnstd_cfg;
/*---------------------------Statics------------------------------------*/
exprivate ndrx_shm_t M_lcf_shm = {.fd=0, .path=""};   /**< Shared memory for settings       */
exprivate ndrx_sem_t M_lcf_sem = {.semid=0};          /**< RW semaphore for SHM protection  */

exprivate ndrx_lcf_command_seen_t *M_locl_lcf=NULL;   /**< Local LCFs seen */
/*---------------------------Prototypes---------------------------------*/
exprivate void lcf_fork_resume_child(void);
/*
 * - installcb
 * - install cli infos (command_str, id, args A|B, descr) -> 
 */

/**
 * Load the LCF settings. Attach semaphores and shared memory.
 * - So we will re-use the same system-v sempahore array used for System-V queues
 *  with slot 2 used for LCF
 * - Add new shared memory block for LCF commands and general real time settings
 *  like current page of DDR routing data
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_init(void)
{
    int ret = EXSUCCEED;
    char *tmp;
    
    memset(&ndrx_G_libnstd_cfg, 0, sizeof(ndrx_G_libnstd_cfg));
    
   /* Load some minimum env for shared mem processing */
    ndrx_G_libnstd_cfg.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==ndrx_G_libnstd_cfg.qprefix)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        EXFAIL_OUT(ret);
    }

    /* get number of concurrent threads */
    tmp = getenv(CONF_NDRX_SVQREADERSMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.readersmax = MAX_READERS_DFLT;
        NDRX_LOG(log_info, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_SVQREADERSMAX, ndrx_G_libnstd_cfg.readersmax);
    }
    else
    {
        ndrx_G_libnstd_cfg.readersmax = atol(tmp);
    }
    
    /* get number of concurrent threads */
    tmp = getenv(CONF_NDRX_LCFMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.readersmax = MAX_LCFMAX_DFLT;
        NDRX_LOG(log_info, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_LCFMAX, ndrx_G_libnstd_cfg.lcfmax);
    }
    else
    {
        ndrx_G_libnstd_cfg.lcfmax = atol(tmp);
    }
    
    /* get queues max */
    tmp = getenv(CONF_NDRX_MSGQUEUESMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.queuesmax = MAX_QUEUES_DLFT;
        NDRX_LOG(log_info, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_MSGQUEUESMAX, ndrx_G_libnstd_cfg.queuesmax);
    }
    else
    {
        ndrx_G_libnstd_cfg.queuesmax = atol(tmp);
    }
    
    /* Get SV5 IPC */
    tmp = getenv(CONF_NDRX_IPCKEY);
    if (NULL==tmp)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        userlog("Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        EXFAIL_OUT(ret);
    }
    else
    {
	int tmpkey;
        
        sscanf(tmp, "%x", &tmpkey);
	ndrx_G_libnstd_cfg.ipckey = tmpkey;

        NDRX_LOG(log_debug, "(sysv queues): SystemV IPC Key set to: [%x]",
                            ndrx_G_libnstd_cfg.ipckey);
    }
    
    /* Open up settings shared memory... */
    NDRX_LOG(log_debug, "Opening LCF shared memory...");
    
    /* We always create, thus if nstd library is loaded, we open semaphore
     * attach to settings memory
     */
    
    M_lcf_shm.fd = EXFAIL;
    M_lcf_shm.key = ndrx_G_libnstd_cfg.ipckey + NDRX_SHM_LCF_KEYOFSZ;
    
    /* calculate the size of the LCF */
    M_lcf_shm.size = sizeof(ndrx_lcf_shmcfg_t) + sizeof(ndrx_lcf_command_t) * 
            ndrx_G_libnstd_cfg.lcfmax;
    
    if (EXSUCCEED!=ndrx_shm_open(&M_lcf_shm, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open LCF shared memory");
        userlog("Failed to open LCF shared memory");
        EXFAIL_OUT(ret);
    }
    
    M_lcf_sem.key = ndrx_G_libnstd_cfg.ipckey + NDRX_SEM_LCFLOCKS;
    
    /*
     * Currently using single semaphore.
     * But from balancing when searching entries, we could use multiple sems.. 
     * to protect both shared mems...
     * If we use different two sems one for p2s and another for s2p we could
     * easily run into deadlock. Semop allows atomically work with multiple
     * semaphores, so maybe this can be used to increase performance.
     */
    M_lcf_sem.nrsems = 1;
    M_lcf_sem.maxreaders = ndrx_G_libnstd_cfg.readersmax;
    
    if (EXSUCCEED!=ndrx_sem_open(&M_lcf_sem, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open LCF semaphore");
        userlog("Failed to open LCF semaphore");
        EXFAIL_OUT(ret);
    }
#if 0
    /* when we fork....
     * detach from LCF...
     * and close all loggers
     */
    if (EXSUCCEED!=(ret=ndrx_atfork(NULL, NULL, lcf_fork_resume_child)))
    {
        userlog("Failed to register fork handlers: %s", strerror(ret));
        EXFAIL_OUT(ret);
    }
    
#endif
    
    /* Sync the LCFs? -> Copy actual state as executed.. */
    
out:
    
    /* cannot continue if basic config is missing */
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_always, "ERROR: Enduro/X configuration is missing - terminating with failure\n");
        userlog("ERROR: Enduro/X configuration is missing - terminating with failure");
        exit(1);
    }
    
    return ret;    
}

/**
 * Detach and remove resources
 */
expublic int ndrx_lcf_down(void)
{
    int ret = EXSUCCEED;
    
    ndrx_lcf_detach();
    
    
    if (EXSUCCEED!=ndrx_shm_remove(&M_lcf_shm))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_sem_remove(&M_lcf_sem, EXTRUE))
    {
        ret = EXFAIL;
    }
    
    return ret;
    
}

/**
 * Once we detach, we shall close all loggers...
 */
expublic void ndrx_lcf_detach(void)
{
    /* process disconnects from shared settings memory
     * This somehow needs to be done when the debug is closed too.
     * Not?
     * So probably we need to add fork handler to close shared mem here after the
     * fork. Mark the debug-as non init.
     * Close the log file descriptors.
     * 
     * So if forked process continues to do some works, then debug-init will
     * start life again and attach to shm
     */
    ndrx_shm_close(&M_lcf_shm);
    ndrx_sem_close(&M_lcf_sem);
    
    /* de init the debug finally 
    ndrx_debug_force_closeall();
     * ? after exec shms and sems are released.
     * */

}

/* vim: set ts=4 sw=4 et smartindent: */
