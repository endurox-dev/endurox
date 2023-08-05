/**
 * @brief Latent Command Framework - real time settings frameworks for all Enduro/X
 *  processes (clients & servers)
 *
 * @file lcf.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <nstd_int.h>
#include <nstd_shm.h>
#include <ndebugcmn.h>
#include <sys_unix.h>
#include <nerror.h>
#include <exhash.h>
#include <exregex.h>
#include <lcfint.h>
#include "exsha1.h"
#include <exatomic.h>
#include <singlegrp.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_LCFMAX_DFLT         20      /**< Default max commands       */
#define MAX_READERS_DFLT        50      /**< Max readers for RW lock... */
#define MAX_LCFREADERS_DFLT     1000    /**< Lcf readers max this is priority */
#define MAX_LCFSTARTMAX_DFLT    60      /**< Apply 60 seconds old commands */
#define SGMREFRESHMAX_DFLT      30      /**< Default maximum lock refresh t */
#define SGMAX_DFLT              64      /**< Maximum number of singleton groups */
#define MAX_QUEUES_DLFT         20000   /**< Max number of queues, dflt */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_nstd_libconfig_t ndrx_G_libnstd_cfg;
expublic ndrx_lcf_shmcfg_t *ndrx_G_shmcfg=NULL;        /**< Shared mem config, full          */
expublic ndrx_lcf_shmcfg_ver_t M_ver_start = {.shmcfgver_lcf=0};

/** 
 * During the init we will change the ptrs ... 
 * value 1 will initiate the pull-in
 */
expublic volatile ndrx_lcf_shmcfg_ver_t *ndrx_G_shmcfg_ver=&M_ver_start;/**< Only version handling            */

expublic volatile unsigned ndrx_G_shmcfgver_chk = 0;            /**< Last checked shared mem cfg vers */
/*---------------------------Statics------------------------------------*/
exprivate ndrx_shm_t M_lcf_shm = {.fd=0, .path="", .mem=NULL};  /**< Shared memory for settings       */
exprivate ndrx_sem_t M_lcf_sem = {.semid=0};          /**< RW semaphore for SHM protection  */

exprivate ndrx_lcf_command_seen_t *M_locl_lcf=NULL;   /**< Local LCFs seen, set when mmap   */

exprivate ndrx_lcf_reg_funch_t *M_funcs=NULL;         /**< functions registered for LCF     */
exprivate ndrx_lcf_reg_xadminh_t *M_xadmin_cmds=NULL; /**< xadmin commands registered       */

exprivate int M_startup_run = EXTRUE;                 /**< First startup run                */
exprivate MUTEX_LOCKDECL(M_lcf_run);

/*---------------------------Prototypes---------------------------------*/

exprivate int ndrx_lcf_logrotate(ndrx_lcf_command_t *cmd, long *p_flags);
exprivate int ndrx_lcf_logchg(ndrx_lcf_command_t *cmd, long *p_flags);

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
    
    MUTEX_LOCK_V(M_lcf_run);
    
    EXHASH_FIND_STR( M_xadmin_cmds, cmdstr, ret);
    
    MUTEX_UNLOCK_V(M_lcf_run);
    
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
        NDRX_LOG_EARLY(log_debug, "xadmin [%s] lcf command %d", xcmd->cmdstr, xcmd->command);
        _Nset_error_fmt(NEEXISTS, "xadmin [%s] lcf command %d", xcmd->cmdstr, xcmd->command);
        EXFAIL_OUT(ret);
    }
    else
    {
        NDRX_LOG_EARLY(log_debug, "Adding [%s] xadmin lcf command %d", 
                xcmd->cmdstr, xcmd->command);
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
    
    MUTEX_LOCK_V(M_lcf_run);
    EXHASH_ADD_STR(M_xadmin_cmds, cmdstr, h);
    MUTEX_UNLOCK_V(M_lcf_run);
    
out:
    return ret;
}

/**
 * Find the command in hash (internal version)
 * @param command command code
 * @return hash object or NULL
 */
expublic ndrx_lcf_reg_funch_t* ndrx_lcf_func_find_int(int command, int do_lock)
{
    ndrx_lcf_reg_funch_t *ret = NULL;
    
    if (do_lock)
    {
        MUTEX_LOCK_V(M_lcf_run);
    }
    
    EXHASH_FIND_INT( M_funcs, &command, ret);
    
    if (do_lock)
    {
        MUTEX_UNLOCK_V(M_lcf_run);
    }
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
    
    h = ndrx_lcf_func_find_int(cfunc->command, EXTRUE);
    
    if (NULL!=h)
    {
        /* duplicate func not allowd */
        _Nset_error_fmt(NEEXISTS, "Command [%d] already registered for [%s]",
                h->command, h->cfunc.cmdstr);
        EXFAIL_OUT(ret);
        
    }
    else
    {
        NDRX_LOG_EARLY(log_debug, "Adding [%d] func lcf command [%s]", 
                cfunc->command, cfunc->cmdstr);
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
    
    MUTEX_LOCK_V(M_lcf_run);
    EXHASH_ADD_INT(M_funcs, command, h);
    MUTEX_UNLOCK_V(M_lcf_run);
    
out:
    return ret;
}

/**
 * Load the LCF settings. Attach semaphores and shared memory.
 * - So we will re-use the same system-v sempahore array used for System-V queues
 *  with slot 2 used for LCF
 * - Add new shared memory block for LCF commands and general real time settings
 *  like current page of DDR routing data
 * We need early logging here, because once debug flag is open, the LCF is ready
 * to start to consume the commands.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_init(void)
{
    int ret = EXSUCCEED;
    char *tmp;
    size_t sz;
    ndrx_lcf_reg_func_t creg;
    
    memset(&ndrx_G_libnstd_cfg, 0, sizeof(ndrx_G_libnstd_cfg));
    
    tmp = getenv(CONF_NDRX_LCFNORUN);
    
    if (NULL!=tmp && (tmp[0]=='y' || tmp[0]=='Y'))
    {
        ndrx_G_libnstd_cfg.lcf_norun=EXTRUE;
    }
    else
    {
        ndrx_G_libnstd_cfg.lcf_norun=EXFALSE;
    }
    
    
   /* Load some minimum env for shared mem processing */
    ndrx_G_libnstd_cfg.qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==ndrx_G_libnstd_cfg.qprefix)
    {
        /* Write to ULOG? */
        NDRX_LOG_EARLY(log_info, "Missing config key %s", CONF_NDRX_QPREFIX);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG_EARLY(log_info, "%s set to %s", CONF_NDRX_QPREFIX, 
            ndrx_G_libnstd_cfg.qprefix);

    /* Read maximum number of singleton groups */
    tmp = getenv(CONF_NDRX_SGMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.sgmax = SGMAX_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.sgmax = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_SGMAX,
            ndrx_G_libnstd_cfg.sgmax);

    /* Read the SG refresh time */
    tmp = getenv(CONF_NDRX_SGREFRESH);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.sgrefreshmax = SGMREFRESHMAX_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.sgrefreshmax = atol(tmp);
    }

    NDRX_LOG_EARLY(log_debug, "%s set to %d", CONF_NDRX_SGREFRESH, 
            ndrx_G_libnstd_cfg.sgrefreshmax);

    /* get number of concurrent threads */
    tmp = getenv(CONF_NDRX_SVQREADERSMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.svqreadersmax = MAX_READERS_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.svqreadersmax = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_SVQREADERSMAX, 
            ndrx_G_libnstd_cfg.svqreadersmax);
    
    /* get number of concurrent threads */
    tmp = getenv(CONF_NDRX_LCFREADERSMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.lcfreadersmax = MAX_LCFREADERS_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.lcfreadersmax = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_LCFREADERSMAX, 
            ndrx_G_libnstd_cfg.lcfreadersmax);
    
    /* Max number of LCF commands */
    tmp = getenv(CONF_NDRX_LCFMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.lcfmax = MAX_LCFMAX_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.lcfmax = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_LCFMAX, 
            ndrx_G_libnstd_cfg.lcfmax);
    
    /* get queues max */
    tmp = getenv(CONF_NDRX_MSGQUEUESMAX);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.queuesmax = MAX_QUEUES_DLFT;
        NDRX_LOG_EARLY(log_info, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_MSGQUEUESMAX, ndrx_G_libnstd_cfg.queuesmax);
    }
    else
    {
        ndrx_G_libnstd_cfg.queuesmax = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_MSGQUEUESMAX, 
            ndrx_G_libnstd_cfg.queuesmax);
    
    /* Get SV5 IPC */
    tmp = getenv(CONF_NDRX_IPCKEY);
    if (NULL==tmp)
    {
        /* Write to ULOG? */
        NDRX_LOG_EARLY(log_info, "Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        EXFAIL_OUT(ret);
    }
    else
    {
	    int tmpkey;
        
        sscanf(tmp, "%x", &tmpkey);
	    ndrx_G_libnstd_cfg.ipckey = tmpkey;

        NDRX_LOG_EARLY(log_info, "(sysv queues): SystemV IPC Key set to: [%x]",
                            ndrx_G_libnstd_cfg.ipckey);
    }
    
    /* LCF Startup expiry for published commands (seconds) */
    tmp = getenv(CONF_NDRX_LCFCMDEXP);
    if (NULL==tmp)
    {
        ndrx_G_libnstd_cfg.startcmdexp = MAX_LCFSTARTMAX_DFLT;
    }
    else
    {
        ndrx_G_libnstd_cfg.startcmdexp = atol(tmp);
    }
    NDRX_LOG_EARLY(log_info, "%s set to %d", CONF_NDRX_LCFCMDEXP, 
            ndrx_G_libnstd_cfg.startcmdexp);
    
    
    /* Open up settings shared memory... */
    NDRX_LOG_EARLY(log_info, "Opening LCF shared memory...");
    
    /* We always create, thus if nstd library is loaded, we open semaphore
     * attach to settings memory
     */
    M_lcf_shm.fd = EXFAIL;
    M_lcf_shm.key = ndrx_G_libnstd_cfg.ipckey + NDRX_SHM_LCF_KEYOFSZ;
    
    /* calculate the size of the LCF */
    M_lcf_shm.size = sizeof(ndrx_lcf_shmcfg_t) 
        /* LCF command segment */
        + sizeof(ndrx_lcf_command_t) * ndrx_G_libnstd_cfg.lcfmax 
        /*  Singleton group segment */
        + sizeof(ndrx_sg_shm_t) * ndrx_G_libnstd_cfg.sgmax;
    
    snprintf(M_lcf_shm.path,  sizeof(M_lcf_shm.path), NDRX_SHM_LCF,  ndrx_G_libnstd_cfg.qprefix);
    
    if (EXSUCCEED!=ndrx_shm_open(&M_lcf_shm, EXTRUE))
    {
        /* cannot attach if settings are different */
        NDRX_LOG_EARLY(log_error, "Failed to attach to LCF memory");
        userlog("Failed to open LCF shared memory - possibly changed "
                "NDRX_LCFMAX, thus use xadmin down -y to restart the app");
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
    M_lcf_sem.maxreaders = ndrx_G_libnstd_cfg.lcfreadersmax;
    
    if (EXSUCCEED!=ndrx_sem_open(&M_lcf_sem, EXTRUE))
    {
        NDRX_LOG_EARLY(log_error, "Failed to open LCF semaphore");
        EXFAIL_OUT(ret);
    }
    
    sz = sizeof(ndrx_lcf_command_seen_t)*ndrx_G_libnstd_cfg.lcfmax;
     
    M_locl_lcf = NDRX_FPMALLOC(sz, 0);
    
    if (NULL==M_locl_lcf)
    {
        NDRX_LOG_EARLY(log_error, "Failed to malloc local LCF storage (%d bytes): %s",
                sz, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* default init... */
    memset(M_locl_lcf, 0, sz);
    
    /* assign the mem -> do as last step, as in case of LCF failure, we
     * ignore the condition
     */
    ndrx_G_shmcfg = (ndrx_lcf_shmcfg_t*)M_lcf_shm.mem;
    
    /* swap the pointers --> This aspect makes as to have G_ndrx_debug_first in
     * debug macros
     * as otherwise we could just relay on shm config version */
    ndrx_G_shmcfg_ver = (ndrx_lcf_shmcfg_ver_t*)M_lcf_shm.mem;
    
    /* register standard callbacks */
    
    memset(&creg, 0, sizeof(creg));
    
    creg.version=NDRX_LCF_CCMD_VERSION;
    creg.pf_callback=ndrx_lcf_logrotate;
    creg.command=NDRX_LCF_CMD_LOGROTATE;
    NDRX_STRCPY_SAFE(creg.cmdstr, NDRX_LCF_CMDSTR_LOGROTATE);
    
    ndrx_lcf_func_add_int(&creg);
    
    memset(&creg, 0, sizeof(creg));
    
    
    creg.version=NDRX_LCF_CCMD_VERSION;
    creg.pf_callback=ndrx_lcf_logchg;
    creg.command=NDRX_LCF_CMD_LOGCHG;
    NDRX_STRCPY_SAFE(creg.cmdstr, NDRX_LCF_CMDSTR_LOGCHG);
    
    ndrx_lcf_func_add_int(&creg);
    
out:
    
    if (EXSUCCEED!=ret)
    {
        /* We do not run also... */
        ndrx_G_libnstd_cfg.lcf_norun=EXTRUE;
    }
    
    return ret;    
}

/**
 * Process all relevant CLF commands
 * and save the results
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_run(void)
{
    int ret = EXSUCCEED;
    long flags;
    int i;
    long apply;
    ndrx_lcf_command_t *cur;
    char tmpbuf[32];
    long cmdage;
    ndrx_lcf_reg_funch_t* cbfunc;
    ndrx_lcf_command_t cmd_tmp;
    
    /* avoid run by other threads..., thus lock them, but as already
     * as possible we update the shared memory version so that threads
     * does not stuck here, but do the work instead
     */
    MUTEX_LOCK_V(M_lcf_run);
    
    /* If we get here. */
    if (ndrx_G_libnstd_cfg.lcf_norun)
    {
        /* If we get inconsistent readings (i.e not atomic), at the next run
         * they will be set to latest value
         */
        ndrx_G_shmcfgver_chk=ndrx_G_shmcfg_ver->shmcfgver_lcf;
        goto out;
    }

    if (ndrx_G_shmcfgver_chk==ndrx_G_shmcfg_ver->shmcfgver_lcf)
    {
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_lcf_sem, 0, NDRX_SEM_TYP_READ))
    {
        EXFAIL_OUT(ret);
    }
    
    /* mark the current version, not other threads will not pass to this func */
    ndrx_G_shmcfgver_chk=ndrx_G_shmcfg_ver->shmcfgver_lcf;
    
    for (i=0; i<ndrx_G_libnstd_cfg.lcfmax; i++)
    {
        if (ndrx_G_shmcfg->commands[i].cmdversion!=M_locl_lcf[i].cmdversion ||
              ndrx_G_shmcfg->commands[i].command !=M_locl_lcf[i].command ||
                0!=ndrx_stopwatch_diff(& ndrx_G_shmcfg->commands[i].publtim, &M_locl_lcf[i].publtim)
                )
        {
            cur = &ndrx_G_shmcfg->commands[i];

            apply = 0;
            /* Does affect us ? */
            if (cur->flags & NDRX_LCF_FLAG_ALL)
            {
                apply=EXTRUE;
            }
            else if (cur->flags & NDRX_LCF_FLAG_PID)
            {
                
                /* test the pid regex */
                if (cur->flags & NDRX_LCF_FLAG_DOREX)
                {
                    snprintf(tmpbuf, sizeof(tmpbuf), "%d", (int)getpid());
                    if (EXSUCCEED==ndrx_regqexec(cur->procid, tmpbuf))
                    {
                        apply++;
                    }
                }
                else
                {
                    pid_t pp = (pid_t)atoi (cur->procid);
                    
                    if (pp==getpid())
                    {
                        apply++;
                    }
                }
            }
            else if (cur->flags & NDRX_LCF_FLAG_BIN)
            {
                /* test the binary regex */
                
                if (cur->flags & NDRX_LCF_FLAG_DOREX)
                {                    
                    if (EXSUCCEED==ndrx_regqexec(cur->procid, EX_PROGNAME))
                    {
                        apply++;
                    }
                }
                else if (0==strcmp(cur->procid, EX_PROGNAME))
                {
                    apply++;
                }
            }
            
            /* stage 2 filter: */
            cmdage = ndrx_stopwatch_get_delta_sec(&cur->publtim);
            
            if (M_startup_run)
            {
                if ( (cur->flags & NDRX_LCF_FLAG_DOSTARTUPEXP) && 
                    cmdage <= ndrx_G_libnstd_cfg.startcmdexp)
                {
                    apply++;
                }
                else if (cur->flags & NDRX_LCF_FLAG_DOSTARTUP)
                {
                    apply++;
                }
            }
            else
            {
                apply++;
            }
            
            if (2==apply && NULL!=(cbfunc =ndrx_lcf_func_find_int(cur->command, EXFALSE)))
            {
                apply++;
            }
            
            /* write the log only if startup & apply */
            
            if (apply==3)
            {
                NDRX_LOG(log_debug, "LCF: Slot %d changed command code %d (%s) version %u "
                        "apply: %d flags: 0x%lx age: %ld apply: %d (%s)", 
                        i, cur->command, cur->cmdstr, cur->version, apply, 
                        cur->flags, cmdage, apply, apply==3?"apply":"ignore");
            }
            
            if (apply==3)
            {
                /* lookup for command code... & exec */    
                
                memcpy(&cmd_tmp, cur, sizeof(cmd_tmp));
                
                flags=0;
                if (EXSUCCEED!=cbfunc->cfunc.pf_callback(&cmd_tmp, &flags))
                {
                    NDRX_ATOMIC_ADD(&cur->failed, 1);
                }
                else
                {
                    NDRX_ATOMIC_ADD(&cur->applied, 1);
                }
                
                /* load the responses, if any: */
                if (flags & NDRX_LCF_FLAG_FBACK_CODE)
                {
                    cur->fbackcode = cmd_tmp.fbackcode;
                }
                
                if (flags & NDRX_LCF_FLAG_FBACK_MSG)
                {
                    cmd_tmp.fbackmsg[NDRX_LCF_FEEDBACK_BUF-1]=EXEOS;
                    NDRX_STRCPY_SAFE(cur->fbackmsg, cmd_tmp.fbackmsg);
                }
            }
            else
            {
                NDRX_ATOMIC_ADD(&cur->seen, 1);
            }
            
            /* mark command as processed */
            M_locl_lcf[i].cmdversion = cur->cmdversion;
            M_locl_lcf[i].command = cur->command;
            M_locl_lcf[i].publtim = cur->publtim;
        }
    }
    
    if (EXSUCCEED!=ndrx_sem_rwunlock(&M_lcf_sem, 0, NDRX_SEM_TYP_READ))
    {
        EXFAIL_OUT(ret);
    }
    
out:
                            
                            
    M_startup_run=EXFALSE;
    
    MUTEX_UNLOCK_V(M_lcf_run);
    
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

/**
 * Reopen the log handlers
 * @param cmd shared mem command
 * @param p_flags feedback flags
 * @return EXSUCCEED
 */
exprivate int ndrx_lcf_logrotate(ndrx_lcf_command_t *cmd, long *p_flags)
{
    return ndrx_debug_reopen_all();
}

/**
 * Re-configure logging on the fly
 * @param cmd shared mem command
 * @param p_flags feedback flags
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_lcf_logchg(ndrx_lcf_command_t *cmd, long *p_flags)
{
    /* Update loggers to have dynamic config version
     * WARNING! After this all loggers will switch to process level settings
     */
    return tplogconfig_int(LOG_FACILITY_NDRX|LOG_FACILITY_UBF|LOG_FACILITY_TP, 
            EXFAIL, (char *)cmd->arg_a, NULL, NULL, NDRX_TPLOGCONFIG_VERSION_INC);
}

/**
 * Publish the LCF command
 * @param slot slot number to which command shall be applied
 * @param cmd fully initialized LCF command structure
 *  NOTE! counters must be set to zero here by caller.
 * @return EXSUCCEED/EXFAIL (possible issue with slot numbers)
 */
expublic int ndrx_lcf_publish_int(int slot, ndrx_lcf_command_t *cmd)
{
    int ret = EXSUCCEED;
    unsigned  cmdversion;
    /* check the shared mem config is LCF used at all 
     * If shm is not initialized, we cannot post
     */
    if (ndrx_G_shmcfg_ver==&M_ver_start)
    {
        _Nset_error_fmt(NESUPPORT, "LCF framework disabled - cannot publish command %d [%s]", 
                cmd->command, cmd->cmdstr);
        
        NDRX_LOG(log_error, "LCF framework disabled - cannot publish command %d [%s]", 
                cmd->command, cmd->cmdstr);
        EXFAIL_OUT(ret);
    }
    
    if (slot >=ndrx_G_libnstd_cfg.lcfmax)
    {
        _Nset_error_fmt(NELIMIT, "Invalid command slot number, max slot: %d got: %d", 
                ndrx_G_libnstd_cfg.lcfmax-1, slot);
        EXFAIL_OUT(ret);
    }
    
    if (slot < 0 )
    {
        _Nset_error_fmt(NEINVAL, "Invalid command slot number, min slot: %d got: %d", 
                0, slot);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_lcf_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        _Nset_error_msg(NESYSTEM, "Failed to lock lcf sem");
        EXFAIL_OUT(ret);
    }
    /* first have a clean memory, if publisher dies before copy data to dest
     * thus have full of terminator there.. 
     */
    memset(&ndrx_G_shmcfg->commands[slot], 0, sizeof(ndrx_G_shmcfg->commands[slot]));
    
    cmdversion = ndrx_G_shmcfg->commands[slot].cmdversion;
    memcpy(&ndrx_G_shmcfg->commands[slot], cmd, sizeof(*cmd));
    
    cmdversion++;
    ndrx_G_shmcfg->commands[slot].cmdversion=cmdversion;
    
    /* set the age of command, we have write lock, so no worry */
    ndrx_stopwatch_reset(&ndrx_G_shmcfg->commands[slot].publtim);
    
    /* finally let processes to see what we have done: */
    ndrx_G_shmcfg->shmcfgver_lcf++;
    
    if (EXSUCCEED!=ndrx_sem_rwunlock(&M_lcf_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        EXFAIL_OUT(ret);
    }
    
out:
    
    return ret;
}

/**
 * List the registered commands via callback
 * @param pf_callback callback function to inspect the commands
 */
expublic void ndrx_lcf_xadmin_list(void (*pf_callback)(ndrx_lcf_reg_xadminh_t *xcmd))
{
    ndrx_lcf_reg_xadminh_t *el, *elt;
    
    MUTEX_LOCK_V(M_lcf_run);
    
    /* loop over and do the callbacks */
    EXHASH_ITER(hh, M_xadmin_cmds, el, elt)
    {
        pf_callback(el);
    }
    
    MUTEX_UNLOCK_V(M_lcf_run);
    
}

/**
 * Perform read lock
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_read_lock(void)
{
    return ndrx_sem_rwlock(&M_lcf_sem, 0, NDRX_SEM_TYP_READ);
}

/**
 * Perform read unlock
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_lcf_read_unlock(void)
{
    return ndrx_sem_rwunlock(&M_lcf_sem, 0, NDRX_SEM_TYP_READ);
}

/**
 * Check is LCF supported
 * @return EXTRUE/EXFALSE
 */
expublic int ndrx_lcf_supported_int(void)
{
    int ret = EXTRUE;
    
    if (ndrx_G_shmcfg_ver==&M_ver_start)
    {
        ret=EXFALSE;
    }
    
out:                            
    return ret;
}

/**
 * Would disable LCF processing
 * by switching back to local memory instead of SHM and restoring the version
 * numbers of LCF checks...
 * Note this assumes that LCF config is loaded. (i.e. some previous debug
 * did the pull-in)
 * @param ipckeybase resolve ipc key base of Enduro/X
 * @param q_prefix queue prefix used by app
 */
expublic void ndrx_lcf_remove(key_t ipckeybase, char *q_prefix)
{
    int do_reply = EXFALSE;
    NDRX_LOG(log_debug, "Removing LCF memory");
    /* Let other threads to lave the lcf runn */
    MUTEX_LOCK_V(M_lcf_run);
    
    ndrx_dbg_intlock_set();
    
    /* lock the debug for ptr swap */
    ndrx_dbg_lock();
    
    /* do not allow anybody else to get in the region, but really should
     * this must be last operations in xadmin down
     * all threads even system-v queues are terminated
     */
    G_ndrx_debug_first=EXTRUE;
    
    /* normally no other threads are expected at this point, but let it be
     */
    sched_yield();
    
    /* mask the LCF is any stuff new stuff is done */
    ndrx_G_shmcfg_ver=&M_ver_start;
    ndrx_G_shmcfgver_chk = ndrx_G_shmcfg_ver->shmcfgver_lcf;
    
    /* close the shared memory.. */
    ndrx_lcf_detach();
    
    /* remove lcf memory.. - re configure if the init was bad*/
    M_lcf_shm.key = ipckeybase + NDRX_SHM_LCF_KEYOFSZ;
    snprintf(M_lcf_shm.path,  sizeof(M_lcf_shm.path), NDRX_SHM_LCF,  q_prefix);
    
    ndrx_shm_remove(&M_lcf_shm);
    ndrx_G_shmcfg=NULL;
    /* remove semaphores.. */
    ndrx_sem_remove(&M_lcf_sem, EXTRUE);
     
    G_ndrx_debug_first=EXFALSE;
    
    ndrx_dbg_unlock();
    
    /* reply the logs finally... */
    ndrx_dbg_intlock_unset(&do_reply);
    
    /* to avoid deadlocks, do this ouside any load locking */
    if (do_reply)
    {
        ndrx_dbg_reply_memlog_all();
    }
    
out:
    MUTEX_UNLOCK_V(M_lcf_run);
    
}

/**
 * Clear the LCF command blocks
 */
expublic void ndrx_lcf_reset(void)
{
    MUTEX_LOCK_V(M_lcf_run);
    
    /* acquire the write lock to shm... */
    
    /* if not attached, nothing todo */
    if (ndrx_G_shmcfg_ver==&M_ver_start)
    {
        goto out;
    }
    
    /* full write protect against all others...  */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_lcf_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        goto out;
    }
    
    /* clean the thing... */
    memset(ndrx_G_shmcfg->commands, 0, sizeof(ndrx_G_shmcfg->commands[0])*ndrx_G_libnstd_cfg.lcfmax);
    
    ndrx_sem_rwunlock(&M_lcf_sem, 0, NDRX_SEM_TYP_WRITE);
    
    ndrx_G_shmcfg->use_ddr = EXFALSE;
    ndrx_G_shmcfg->ddr_page=0;
    ndrx_G_shmcfg->ddr_ver1=0;
    
out:
    MUTEX_UNLOCK_V(M_lcf_run);
}

/* vim: set ts=4 sw=4 et smartindent: */
