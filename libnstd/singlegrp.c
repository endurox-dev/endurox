/**
 * @brief Singleton group support routines
 *  Internal library, assumes that group number is pre-valdiated (i.e. >0 && <=pgmax)
 *
 * @file singlegrp.c
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
#include <ndrx_config.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <singlegrp.h>
#include <lcfint.h>
#include <sys/stat.h>
#include <sys_unix.h>
#include <sys/time.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* have macro to get the pointer to the group in shared memory */
#define NDRX_SG_GET_PTR(sg_no) \
    (ndrx_sg_shm_t *)((char *)ndrx_G_shmcfg->commands \
            + sizeof(ndrx_G_shmcfg->commands[0])*ndrx_G_libnstd_cfg.lcfmax \
            + sizeof(ndrx_sg_shm_t)*(sg_no-1))

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int  ndrx_sg_chk_timestamp(int singlegrp_no, ndrx_sg_shm_t *sg);

/**
 * Perform init of the singleton group support
*/
expublic int ndrx_sg_init(void)
{
    return EXSUCCEED;
}

/** 
 * Return the ptr to singleton group in shared memory 
 * Check against group number limits
 * Check is the LCF shared memory actually attached (as it is optional)
 * The group numbers start from 1. Group 0 is reserved for internal use,
 *  for cases when no group is specified.
 * Finally calculate the offset in shared memory
 * @param singlegrp_no single group number
 * @return ptr to single group in shared memory or NULL if not found
 */
expublic ndrx_sg_shm_t *ndrx_sg_get(int singlegrp_no)
{
    ndrx_sg_shm_t *ret = NULL;

    /* Check against group number limits */
    if (singlegrp_no < 0 || singlegrp_no > ndrx_G_libnstd_cfg.pgmax)
    {
        NDRX_LOG(log_error, "Invalid single group number: %d", singlegrp_no);
        goto out;
    }

    /* Check is the LCF shared memory actually attached (as it is optional) */
    if (NULL==ndrx_G_shmcfg)
    {
        NDRX_LOG(log_error, "LCF shared memory is not attached!");
        goto out;
    }

    /* Finally calculate the offset in shared memory 
     * as group numbers go from 1, we need to subtract 1, to keep the offset 0 utilised
     */
    ret = (ndrx_sg_shm_t *)((char *)ndrx_G_shmcfg->commands 
            + sizeof(ndrx_G_shmcfg->commands[0])*ndrx_G_libnstd_cfg.lcfmax
            + sizeof(ndrx_sg_shm_t)*(singlegrp_no-1));
out:
    return ret;
}

/**
 * Reset singleton group infos
 */
expublic void ndrx_sg_reset(void)
{
    if (NULL!=ndrx_G_shmcfg)
    {
        memset( (char *)ndrx_G_shmcfg->commands 
                    + sizeof(ndrx_G_shmcfg->commands[0])*ndrx_G_libnstd_cfg.lcfmax,
                    0, sizeof(ndrx_sg_shm_t)*ndrx_G_libnstd_cfg.pgmax);
        /* remove from maintenance mode too */
        ndrx_G_shmcfg->is_mmon=EXFALSE;
    }
}

/**
 * If group was locked, but for some reason
 * we face a condition that we group shall be unlocked.
 * This resets key statistics of the group entry.
 * if already unlocked, leave the original data there in shm.
 * @param sg resolved group ptr
 * @param reason reason code, why group was unlocked
 */
expublic void ndrx_sg_unlock(ndrx_sg_shm_t * sg, int reason)
{
    unsigned char is_locked = atomic_load(&sg->is_locked);
    
    if (is_locked)
    {
        atomic_store(&sg->is_srv_booted, EXFALSE);
        atomic_store(&sg->is_clt_booted, EXFALSE);
        atomic_store(&sg->is_locked, EXFALSE);
        atomic_store(&sg->reason, reason);
    }
}

/**
 * Refresh lock for given group
 * @param singlegrp_no single group number
 * @param sg resolved group ptr
 * @param nodeid cluster node id (current one, but now known in standard libary)
 * @param srvid server id (also current one, but now known in standard libary)
 * @param chk_lock check lock status (match current lock provider)
 * @param new_last_refresh new refresh time
 * @return EXSUCCEED if successfull, EXFAIL if failed
*/
exprivate int ndrx_sg_do_refresh_int(int singlegrp_no, ndrx_sg_shm_t * sg, 
    short nodeid, int srvid, int chk_lock, time_t new_last_refresh)
{
    int ret = EXSUCCEED;
    ndrx_sg_shm_t sg_local;
    pid_t pid;
    ndrx_sg_shm_t * sg_shm = NDRX_SG_GET_PTR(singlegrp_no);

    if (NULL==sg)
    {
        /* copy of the shm.. */
        ndrx_sg_load(&sg_local, sg_shm);
        sg=&sg_local;
    }

    if (chk_lock)
    {
        if (!sg->is_locked)
        {
            NDRX_LOG(log_error, "ERROR Group %d is not locked, but "
                "must be - terminating!", singlegrp_no);
            userlog("ERROR Group %d is not locked, but "
                "must be - terminating!", singlegrp_no);
            EXFAIL_OUT(ret);
        }

        /* Check lockserver pid, nodeid, srvid -> must be ours... */
        if (sg->lockprov_pid!=(pid=getpid()))
        {
            NDRX_LOG(log_error, "ERROR Group %d is locked by PID %d, "
                "but must be locked by PID %d - terminating!", 
                singlegrp_no, sg->lockprov_pid, (int)pid);
            userlog("ERROR Group %d is locked by PID %d, "
                "but must be locked by PID %d - terminating!", 
                singlegrp_no, sg->lockprov_pid, (int)pid);
            EXFAIL_OUT(ret);
        }

        if (sg->lockprov_nodeid!=nodeid)
        {
            NDRX_LOG(log_error, "ERROR Group %d is locked by Node %hd, "
                "but must be locked by Node %hd - terminating!", 
                singlegrp_no, sg->lockprov_nodeid, nodeid);
            userlog("ERROR Group %d is locked by Node %hd, "
                "but must be locked by Node %hd - terminating!", 
                singlegrp_no, sg->lockprov_nodeid, nodeid);
            EXFAIL_OUT(ret);
        }

        if (sg->lockprov_srvid!=srvid)
        {
            NDRX_LOG(log_error, "ERROR Group %d is locked by Server id %d, "
                "but must be locked by Server %d - terminating!", 
                singlegrp_no, sg->lockprov_srvid, srvid);
            userlog("ERROR Group %d is locked by Server is %d, "
                "but must be locked by Server %d - terminating!", 
                singlegrp_no, sg->lockprov_srvid, srvid);
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=ndrx_sg_chk_timestamp(singlegrp_no, sg))
        {
            EXFAIL_OUT(ret);
        }
    }

    atomic_store(&sg_shm->last_refresh, new_last_refresh);

out:
    return ret;
}

/**
 * Refresh lock for given group
 * @param singlegrp_no single group number
 * @param nodeid cluster node id (current one, but now known in standard libary)
 * @param srvid server id (also current one, but now known in standard libary)
 * @param new_last_refresh new refresh time
 * @return EXSUCCEED if successfull, EXFAIL if failed
 */
expublic int ndrx_sg_do_refresh(int singlegrp_no, ndrx_sg_shm_t * sg, 
    short nodeid, int srvid, time_t new_last_refresh)
{
    return ndrx_sg_do_refresh_int(singlegrp_no, sg, nodeid, srvid, 
        EXTRUE, new_last_refresh);
}

/**
 * Perform group locking (by the lock provider)
 * @param singlegrp_no single group number
 * @param nodeid cluster node id (current one, but now known in standard libary)
 * @param srvid server id
 * @param procname process name
 * @param new_last_refresh lock time (when checks were started...)
 * @param new_sequence new sequence number
 * @return EXSUCCEED if successfull, EXFAIL if failed
 */
expublic int ndrx_sg_do_lock(int singlegrp_no, short nodeid, int srvid, char *procname,
        time_t new_last_refresh, long new_sequence)
{
    int ret = EXSUCCEED;
    ndrx_sg_shm_t * sg_shm, sg;

    sg_shm = NDRX_SG_GET_PTR(singlegrp_no);
    ndrx_sg_load(&sg, sg_shm);

    if (sg.is_locked)
    {
        NDRX_LOG(log_error, "ERROR: Group %d already locked by Node %d, PID %d, Procname [%s] - terminating",
                singlegrp_no, sg.lockprov_nodeid, sg.lockprov_pid, sg.lockprov_procname);
        userlog("ERROR: Group %d already locked by Node %d, PID %d, Procname [%s] - terminating",
                singlegrp_no, sg.lockprov_nodeid, sg.lockprov_pid, sg.lockprov_procname);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED != ndrx_sg_do_refresh_int(singlegrp_no, 
        &sg, nodeid, srvid, EXFALSE, new_last_refresh))
    {
        EXFAIL_OUT(ret);   
    }

    /* store current proc info */
    ndrx_volatile_strcpy(sg_shm->lockprov_procname, procname, sizeof(sg.lockprov_procname));
    __sync_synchronize();

    atomic_store(&sg_shm->lockprov_nodeid, nodeid);
    atomic_store(&sg_shm->lockprov_pid, getpid());
    atomic_store(&sg_shm->lockprov_srvid, srvid);
    atomic_store(&sg_shm->is_locked, EXTRUE);
    atomic_store(&sg_shm->reason, 0);
    atomic_store(&sg_shm->sequence, new_sequence);

    NDRX_LOG(log_debug, "Group %d locked", singlegrp_no);
    userlog("Group %d locked", singlegrp_no);

out:
    return ret;
}

/**
 * Load all fields from shared memory to local copy
 * in atomic way.
 * @param sg local copy
 * @param sg_shm shared memory data
 */
expublic void ndrx_sg_load(ndrx_sg_shm_t * sg, ndrx_sg_shm_t * sg_shm)
{
    /* atomic load multi-byte fields */
    sg->is_locked = atomic_load(&sg_shm->is_locked);
    sg->is_mmon = atomic_load(&sg_shm->is_mmon);
    sg->is_srv_booted = atomic_load(&sg_shm->is_srv_booted);
    sg->is_clt_booted = atomic_load(&sg_shm->is_clt_booted);
    sg->flags = atomic_load(&sg_shm->flags);
    sg->last_refresh = atomic_load(&sg_shm->last_refresh);
    sg->sequence = atomic_load(&sg_shm->sequence);
    sg->lockprov_nodeid = atomic_load(&sg_shm->lockprov_nodeid);
    sg->lockprov_pid = atomic_load(&sg_shm->lockprov_pid);
    sg->lockprov_srvid = atomic_load(&sg_shm->lockprov_srvid);
    ndrx_volatile_strcpy(sg->lockprov_procname, sg_shm->lockprov_procname,
        sizeof(sg->lockprov_procname));

    ndrx_volatile_memcy(sg->sg_nodes, sg_shm->sg_nodes, sizeof(sg->sg_nodes));

    sg->reason = atomic_load(&sg_shm->reason);
}

/**
 * Check is shared memory timestmap still valid.
 * @param singlegrp_no single group number
 * @param sg local copy of the shared memory entry
 * @return EXSUCCEED if successfull (still valid), EXFAIL if failed
 */
exprivate int ndrx_sg_chk_timestamp(int singlegrp_no, ndrx_sg_shm_t *sg)
{
    int ret=EXSUCCEED;
    struct timespec ts;
    long long time_diff;

    ndrx_realtime_get(&ts);

    time_diff=(long long)ts.tv_sec-(long long)sg->last_refresh;

    /* validate the lock */
    if (llabs(time_diff) > ndrx_G_libnstd_cfg.sgrefreshmax)
    {
        /* Mark system as not locked anymore! */
        ndrx_sg_unlock(NDRX_SG_GET_PTR(singlegrp_no), NDRX_SG_RSN_EXPIRED);

        NDRX_LOG(log_error, "ERROR: Lock for singleton group %d is inconsistent "
                "(did not refresh in %d sec, diff %lld sec)! "
                "Marking group as not locked!", singlegrp_no, ndrx_G_libnstd_cfg.sgrefreshmax,
                time_diff);

        userlog("ERROR: Lock for singleton group %d is inconsistent "
                "(did not refresh in %d sec, diff %lld sec)! "
                "Marking group as not locked!", singlegrp_no, ndrx_G_libnstd_cfg.sgrefreshmax,
                time_diff);

        EXFAIL_OUT(ret);
    }
out:
    return ret;
}

/**
 * Check is group locked (internal version)
 * @param singlegrp_no single group number
 * @param sg optional resolved group from shared memory 
 *  (will be used if not NULL, instead of the lookup by no)
 * @param reference_file file for which to check future modification date
 * @param flags NDRX_SG_CHK_PID check for pid, default 0
 * @return EXTRUE/EXFALSE/EXFAIL
 */
expublic int ndrx_sg_is_locked_int(int singlegrp_no, ndrx_sg_shm_t * sg,
    char *reference_file, long flags)
{
    int ret = EXFALSE;
    ndrx_sg_shm_t sg_local;
    
    if (NULL==sg)
    {
        sg=ndrx_sg_get(singlegrp_no);

        if (NULL==sg)
        {
            NDRX_LOG(log_error, "singleton group %d not found", singlegrp_no);
            ret=EXFAIL;
            goto out;
        }
    }

    if (!sg->is_locked)
    {
        ret=EXFALSE;
        goto out;
    }

    /* load all fields from shared memory to local copy */
    ndrx_sg_load(&sg_local, sg);

    if (EXSUCCEED!=ndrx_sg_chk_timestamp(singlegrp_no, &sg_local))
    {
        ret=EXFALSE;
        goto out;
    }

    if (flags & NDRX_SG_SRVBOOTCHK 
        && !(sg_local.flags & NDRX_SG_NO_ORDER) 
        && !sg_local.is_srv_booted)
    {
        /* if servers are not booted, but order required, 
         * report group as not locked
         */
        NDRX_LOG(log_debug, "Singleton group %d servers not booted",
            singlegrp_no);
        ret=EXFALSE;
        goto out;
    }
    /* validate the PID if requested so */
    else if (flags & NDRX_SG_CHK_PID)
    {
        if (!ndrx_sys_is_process_running_by_pid(sg_local.lockprov_pid))
        {
            /* Mark system as not locked anymor! */
            ndrx_sg_unlock(sg, NDRX_SG_RSN_NOPID);

            NDRX_LOG(log_error, "ERROR: Lock for singleton group %d is inconsistent "
                    "(lock provider PID %d is not running)! "
                    "Marking group as not locked!", singlegrp_no, sg_local.lockprov_pid);

            userlog("ERROR: Lock for singleton group %d is inconsistent "
                    "(lock provider PID %d is not running)! "
                    "Marking group as not locked!", singlegrp_no, sg_local.lockprov_pid);

            ret=EXFALSE;
            goto out;
        }
    }

    /* stat() the file, in case if modification date is future or file is
     * unlock the group!
     */
    if (NULL!=reference_file)
    {
        struct stat st;
        int rc;
        struct timeval now;

        rc = stat(reference_file, &st);

        if (0!=rc)
        {
            int err=errno;
            /* Mark system as not locked anymore! */
            ndrx_sg_unlock(sg, NDRX_SG_RSN_REFNOFILE);

            NDRX_LOG(log_error, "ERROR: Lock for singleton group %d is inconsistent "
                    "(reference file [%s] failed to open: %s)! "
                    "Marking group as not locked!", singlegrp_no, reference_file, strerror(err));

            userlog("ERROR: Lock for singleton group %d is inconsistent "
                    "(reference file [%s] is missing: %s)! "
                    "Marking group as not locked!", singlegrp_no, reference_file, strerror(err));

            ret=EXFALSE;
            goto out;
        }

        /* get current time of day... 
         * due to NTP gettimeofday() might actually be in the past.
         * thus we can detect that other node has taken over.
         */
        gettimeofday(&now,NULL);

        if (st.st_mtime > now.tv_sec)
        {
            /* Mark system as not locked anymor! */
            ndrx_sg_unlock(sg, NDRX_SG_RSN_REFFFUT);

            NDRX_LOG(log_error, "ERROR: Lock for singleton group %d is inconsistent "
                    "(reference file %s is in future)! "
                    "Marking group as not locked!", singlegrp_no, reference_file);

            userlog("ERROR: Lock for singleton group %d is inconsistent "
                    "(reference file %s is in future)! "
                    "Marking group as not locked!", singlegrp_no, reference_file);

            ret=EXFALSE;
            goto out;
        }
    }

    /* finally we are done, and we are locked */
    ret=EXTRUE;

out:
    return ret;
}

/**
 * Is given group locked?
 * Get the ptr to ndrx_sg_shm_t and check following items:
 * - field is_locked is EXTRUE
 * - last_refresh (substracted current boot time / gettimeofday()) is less ndrx_G_libnstd_cfg.sgrefresh
 * - if maintenance mode is enabled, then we do not perform any further checks, other than lock status.
 * @param singlegrp_no single group number
 * @param reference_file this is refrence file name, used to check arn't there any future modifications
 *  applied, or file missing?
 * @param flags biwtise flags, NDRX_SG_CHK_PID used for additional check of exsinglesv running.
 * @return EXFAIL/EXFALSE (not locked)/EXTRUE (locked)
 */

expublic int ndrx_sg_is_locked(int singlegrp_no, char *reference_file, long flags)
{
    return ndrx_sg_is_locked_int(singlegrp_no, NULL, reference_file, flags);
}

/**
 * Number of singleton groups.
 */
expublic int ndrx_sg_nrgroups()
{
    return ndrx_G_libnstd_cfg.pgmax;
}

/**
 * Take a snapshoot of all groups, to be used by
 * batch startup processes (so that batch is started with the single status)
 * @param lock_status_out where to put the group statuses
 * @param lock_status_out_len length of the lock_status_out
 * @param flags passed to lock status check
 *  additonally may pass NDRX_SG_CHK_PID to check for lock provider pid
 * @return returns array of single group statuses. May contain 0, if not locked,
 *   NDRX_SG_NO_ORDER if locked, but no order required, NDRX_SG_IN_USE if locked.
 */
expublic void ndrx_sg_get_lock_snapshoot(int *lock_status_out, int *lock_status_out_len, long flags)
{
    int i=0;
    int locked;
    ndrx_sg_shm_t *p;

    /* fix up the output len */
    if (*lock_status_out_len>ndrx_G_libnstd_cfg.pgmax)
    {
        *lock_status_out_len = ndrx_G_libnstd_cfg.pgmax;
    }

    memset(lock_status_out, 0, sizeof(int)*(*lock_status_out_len));

    for (i=0; i<*lock_status_out_len; i++)
    {
        p=ndrx_sg_get(i+1);

        if (EXTRUE==ndrx_sg_is_locked_int(i+1, p, NULL, flags))
        {
            if (p->flags & NDRX_SG_NO_ORDER)
            {
                lock_status_out[i]=NDRX_SG_NO_ORDER;
            }
            else
            {
                lock_status_out[i]=NDRX_SG_IN_USE;
            }
            NDRX_LOG(log_debug, "Group %d lock flag: 0x%x", i+1, lock_status_out[i]);
        }
    }

    return;
}

/**
 * Set the bootflag on.
 * This assumes that shared memory is already attached.
 * If the LCF was not attached, then ndrxd would not get so far.
 * @param singlegrp_no single group number
 */
expublic void ndrx_sg_bootflag_clt_set(int singlegrp_no)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    atomic_store(&sg->is_clt_booted, EXTRUE);
}

/**
 * Get the boot flag
 * @param singlegrp_no single group number
 */
expublic unsigned char ndrx_sg_bootflag_clt_get(int singlegrp_no)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    return atomic_load(&sg->is_clt_booted);
}

/**
 * Set the bootflag on.
 * This assumes that shared memory is already attached.
 * If the LCF was not attached, then ndrxd would not get so far.
 * @param singlegrp_no single group number
 */
expublic void ndrx_sg_bootflag_srv_set(int singlegrp_no)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    atomic_store(&sg->is_srv_booted, EXTRUE);
}

/**
 * Get server boot flag
 * @param singlegrp_no single group number
 */
expublic unsigned char ndrx_sg_bootflag_srv_get(int singlegrp_no)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    return atomic_load(&sg->is_srv_booted);
}

/**
 * Check the singleton group validity
 * @param singlegrp_no number to check
 * @return EXTRUE/EXFALSE
 */
expublic int ndrx_sg_is_valid(int singlegrp_no)
{
    int ret = EXTRUE;

    if (singlegrp_no <= 0 || singlegrp_no > ndrx_G_libnstd_cfg.pgmax)
    {
        NDRX_LOG(log_error, "Invalid single group number: %d", singlegrp_no);
        ret=EXFALSE;
        goto out;
    }

out:
    return ret;
}

/**
 * Store flags for the group
 * @param singlegrp_no group number
 * @param flags new flags value
 */
expublic void ndrx_sg_flags_set(int singlegrp_no, unsigned short flags)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    atomic_store(&sg->flags, flags);
}

/**
 * Return the singleton group flags
 * @return group flags
 */
expublic unsigned short ndrx_sg_flags_get(int singlegrp_no)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    return atomic_load(&sg->flags);
}

/**
 * Set the used cluster nodes
 */
expublic void ndrx_sg_nodes_set(int singlegrp_no, char *sg_nodes)
{
    ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
    ndrx_volatile_memcy(sg->sg_nodes, sg_nodes, sizeof(sg->sg_nodes));
}

/* vim: set ts=4 sw=4 et smartindent: */
