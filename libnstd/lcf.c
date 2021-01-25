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
#include <ndebug.h>
#include <lcf.h>
#include <lcfint.h>
#include <nstd_shm.h>
#include <ndebugcmn.h>
#include <sys_unix.h>
#include <nerror.h>
#include <exhash.h>

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
expublic ndrx_lcf_shmcfg_t *ndrx_G_shmcfg=NULL;        /**< Shared mem config, full          */
expublic ndrx_lcf_shmcfg_ver_t *ndrx_G_shmcfg_ver=NULL;/**< Only version handling            */

expublic unsigned ndrx_G_shmcfgver_chk = 0;            /**< Last checked shared mem cfg vers */
/*---------------------------Statics------------------------------------*/
exprivate ndrx_shm_t M_lcf_shm = {.fd=0, .path=""};   /**< Shared memory for settings       */
exprivate ndrx_sem_t M_lcf_sem = {.semid=0};          /**< RW semaphore for SHM protection  */

exprivate ndrx_lcf_command_seen_t *M_locl_lcf=NULL;   /**< Local LCFs seen, set when mmap   */

exprivate ndrx_lcf_reg_funch_t *M_funcs=NULL;         /**< functions registered for LCF     */
exprivate ndrx_lcf_reg_xadminh_t *M_xadmin_cmds=NULL; /**< xadmin commands registered       */

/*---------------------------Prototypes---------------------------------*/
exprivate void lcf_fork_resume_child(void);
/*
 * - installcb
 * - install cli infos (command_str, id, args A|B, descr) -> 
 */

/**
 * Find the command in hash (internal version)
 * @param cmdstr command name
 * @return hash object or NULL
 */
expublic ndrx_lcf_reg_xadminh_t* ndrx_lcf_xadmin_find_int(char *cmdstr)
{
    ndrx_lcf_reg_xadminh_t *ret = NULL;
    
    EXHASH_FIND_STR( M_xadmin_cmds, cmdstr, ret);
    
    return ret;
}

/**
 * Delete xadmin command (internal version)
 * @param h hash handle of the command
 */
expublic void ndrx_lcf_xadmin_del_int(ndrx_lcf_reg_xadminh_t *h)
{
    EXHASH_DEL( M_xadmin_cmds, h);
}

/**
 * Delete command by string code.
 * This does not allow to delete internal Enduro/X commands.
 * @param cmdstr command text code
 * @param EXSUCCEED (delete ok, found), EXFAIL(not found)
 */
expublic int ndrx_lcf_xadmin_delstr_int(char *cmdstr)
{
    int ret=EXSUCCEED;
    
    ndrx_lcf_reg_xadminh_t *h = ndrx_lcf_xadmin_find_int(cmdstr);
    
    if (NULL==h)
    {
        _Nset_error_msg(NENOENT, "Command not registered.");
        EXFAIL_OUT(ret);
    }
    
    if (h->xcmd.command < NDRX_LCF_CMD_MIN_CUST)
    {
        _Nset_error_msg(NESUPPORT, "Cannot delete internal commands");
        EXFAIL_OUT(ret);
    }
    
    ndrx_lcf_xadmin_del_int(h);
    
out:    
    return ret;
}

/**
 * If command not found -> add
 * if command is found -> Replace
 * NOTE! we cannot use log functions. Only NDRX_LOG_EARLY allowed
 * as plugins may register the functions 
 * @param xcmd Xadmin function definition
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_xadmin_add_int(ndrx_lcf_reg_xadmin_t *xcmd)
{
    int ret = EXSUCCEED;
    ndrx_lcf_reg_xadminh_t *h;
    
    h = ndrx_lcf_xadmin_find_int(xcmd->cmdstr);
    
    if (NULL!=h)
    {
        NDRX_LOG_EARLY(log_debug, "Replacing [%s] xadmin lcf command", xcmd->cmdstr);
        ndrx_lcf_xadmin_del_int(h);
    }
    else
    {
        NDRX_LOG_EARLY(log_debug, "Adding [%s] xadmin lcf command", xcmd->cmdstr);
    }
    
    /* OK now add */
    h = NDRX_FPMALLOC(sizeof(ndrx_lcf_reg_xadminh_t), 0);
    
    if (NULL==h)
    {
        NDRX_LOG_EARLY(log_error, "Failed to malloc %d bytes (xadmin lcf cmd hash): %s",
                sizeof(ndrx_lcf_reg_xadminh_t), strerror(errno));
        _Nset_error_fmt(NEMALLOC, "Failed to malloc %d bytes (xadmin lcf cmd hash): %s",
                sizeof(ndrx_lcf_reg_xadminh_t), strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(&h->xcmd, xcmd, sizeof(ndrx_lcf_reg_xadmin_t));
    NDRX_STRCPY_SAFE(h->cmdstr, xcmd->cmdstr);
    EXHASH_ADD_STR(M_xadmin_cmds, cmdstr, h);
    
out:
    return ret;
}

/**
 * Find the command in hash (internal version)
 * @param command command code
 * @return hash object or NULL
 */
expublic ndrx_lcf_reg_funch_t* ndrx_lcf_func_find_int(int command)
{
    ndrx_lcf_reg_funch_t *ret = NULL;
    
    EXHASH_FIND_INT( M_funcs, &command, ret);
    
    return ret;
}

/**
 * Delete func command (internal version)
 * @param h hash handle of the command
 */
expublic void ndrx_lcf_func_del_int(ndrx_lcf_reg_funch_t *h)
{
    EXHASH_DEL( M_funcs, h);
}

/**
 * Delete command by string code.
 * This does not allow to delete internal Enduro/X commands.
 * @param cmdstr command text code
 * @param EXSUCCEED (delete ok, found), EXFAIL(not found)
 */
expublic int ndrx_lcf_func_delcmd_int(int command)
{
    int ret=EXSUCCEED;
    
    ndrx_lcf_reg_funch_t *h = ndrx_lcf_func_find_int(command);
    
    if (NULL==h)
    {
        _Nset_error_msg(NENOENT, "Command not registered.");
        EXFAIL_OUT(ret);
    }
    
    if (h->cfunc.command < NDRX_LCF_CMD_MIN_CUST)
    {
        _Nset_error_fmt(NESUPPORT, "Cannot delete internal commands (%d)", 
                h->cfunc.command);
        EXFAIL_OUT(ret);
    }
    
    ndrx_lcf_func_del_int(h);
    
out:    
    return ret;
}

/**
 * Register callback commands
 * @param xcmd
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_func_add_int(ndrx_lcf_reg_func_t *cfunc)
{
    int ret = EXSUCCEED;
    ndrx_lcf_reg_funch_t *h;
    
    h = ndrx_lcf_func_find_int(cfunc->command);
    
    if (NULL!=h)
    {
        NDRX_LOG_EARLY(log_debug, "Replacing [%d] func lcf command", cfunc->command);
        ndrx_lcf_func_del_int(h);
    }
    else
    {
        NDRX_LOG_EARLY(log_debug, "Adding [%d] func lcf command", cfunc->command);
    }
    
    /* OK now add */
    h = NDRX_FPMALLOC(sizeof(ndrx_lcf_reg_funch_t), 0);
    
    if (NULL==h)
    {
        NDRX_LOG_EARLY(log_error, "Failed to malloc %d bytes (func lcf cmd hash): %s",
                sizeof(ndrx_lcf_reg_funch_t), strerror(errno));
        _Nset_error_fmt(NEMALLOC, "Failed to malloc %d bytes (func lcf cmd hash): %s",
                sizeof(ndrx_lcf_reg_funch_t), strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    memcpy(&h->cfunc, cfunc, sizeof(ndrx_lcf_reg_func_t));
    h->command = cfunc->command;
    EXHASH_ADD_INT(M_funcs, command, h);
    
out:
    return ret;
}

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
    
    /* assign the mem */
    ndrx_G_shmcfg = (ndrx_lcf_shmcfg_t*)M_lcf_shm.mem;
    ndrx_G_shmcfg_ver = (ndrx_lcf_shmcfg_ver_t*)M_lcf_shm.mem;
    
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
    
    /* TODO: Alloc M_locl_lcf -> to keep the local results */
    
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
 * Process all relevant CLF commands
 * and save the results
 * @param is_startup is this startup run? or run detected by NDRX_LOG?
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_run(int is_startup)
{
    /* TODO: hard work! */
    
    /* TOOD: Read lock */
    
    /* TOOD: Read unlock (when executing) */
    
    
    /* TODO: Write lock -> update results */
    
    /* TODO: Write lock -> update results, think about this..., as at 
     * heavy loads writes may cause stall? */
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
