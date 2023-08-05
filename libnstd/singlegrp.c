/**
 * @brief Singleton group support routines
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
exprivate ndrx_sg_shm_t M_dum = {0}; /**< Dummy record for group 0 */
/*---------------------------Prototypes---------------------------------*/

/** 
 * Return the ptr to single group in shared memory 
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

    /* in case if group 0 used, return dummy record */
    if (0 == singlegrp_no)
    {
        ret = &M_dum;
        goto out;
    }

    /* Check against group number limits */
    if (singlegrp_no < 0 || singlegrp_no > ndrx_G_libnstd_cfg.sgmax)
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
                    0, sizeof(ndrx_sg_shm_t)*ndrx_G_libnstd_cfg.sgmax);
    }
}

/**
 * If group was locked, but for some reason
 * we face a condition that we group shall be unlocked.
 * This resets key statistics of the group entry.
 * @param sg resolved group ptr
 * @param reason reason code, why group was unlocked
 */
expublic void ndrx_sg_unlock(ndrx_sg_shm_t * sg, int reason)
{
    atomic_store(&sg->is_srv_booted, EXFALSE);
    atomic_store(&sg->is_clt_booted, EXFALSE);
    atomic_store(&sg->is_locked, EXFALSE);
    atomic_store(&sg->reason, reason);
}

/**
 * Load all fields from shared memory to local copy
 * in atomic way.
 * @param sg local copy
 * @param sg_shm shared memory data
 */
exprivate void ndrx_sg_load(ndrx_sg_shm_t * sg, ndrx_sg_shm_t * sg_shm)
{
    /* atomic load multi-byte fields */
    sg->is_locked = atomic_load(&sg_shm->is_locked);
    sg->is_mmon = atomic_load(&sg_shm->is_mmon);
    sg->is_srv_booted = atomic_load(&sg_shm->is_srv_booted);
    sg->is_clt_booted = atomic_load(&sg_shm->is_clt_booted);
    sg->flags = atomic_load(&sg_shm->flags);
    sg->last_refresh = atomic_load(&sg_shm->last_refresh);
    sg->lockprov_pid = atomic_load(&sg_shm->lockprov_pid);
    sg->lockprov_srvid = atomic_load(&sg_shm->lockprov_srvid);
}

/**
 * Check is group locked (internal version)
 * @param singlegrp_no single group number
 * @param sg optional resolved group from shared memory (will be used if not NULL, instead of the lookup by no)
 * @param reference_file file for which to check future modification date
 * @param flags NDRX_SG_CHK_PID check for pid, default 0
 */
exprivate int ndrx_sg_is_locked_int(int singlegrp_no, ndrx_sg_shm_t * sg, char *reference_file, long flags)
{
    int ret = EXFALSE;
    ndrx_sg_shm_t sg_local;
    struct timespec ts;
    
    if (NULL==sg)
    {
        if (0==singlegrp_no)
        {
            /* group 0 is always locked */
            ret=EXTRUE;
            goto out;
        }

        sg=ndrx_sg_get(singlegrp_no);

        if (NULL==sg)
        {
            ret=EXFAIL;
            goto out;
        }
    }

    if (!sg->is_locked)
    {
        /* not locked */
        ret=EXFALSE;
        goto out;
    }

    /* load all fields from shared memory to local copy */
    ndrx_sg_load(&sg_local, sg);
    ndrx_realtime_get(&ts);

    /* validate the lock */
    if (llabs( (long long)ts.tv_sec-(long long)sg_local.last_refresh) > ndrx_G_libnstd_cfg.sgrefreshmax)
    {
        /* Mark system as not locked anymor! */
        ndrx_sg_unlock(sg, NDRX_SG_RSN_EXPIRED);

        NDRX_LOG(log_error, "ERROR: Lock for singleton group %d is inconsistent "
                "(did not refresh in %d sec)! "
                "Marking group as not locked!", singlegrp_no, ndrx_G_libnstd_cfg.sgrefreshmax);

        userlog("ERROR: Lock for singleton group %d is inconsistent "
                "(did not refresh in %d sec)! "
                "Marking group as not locked!", singlegrp_no, ndrx_G_libnstd_cfg.sgrefreshmax);

        ret=EXFALSE;
        goto out;
    }

    /* validate the PID if requested so */
    if (flags & NDRX_SG_CHK_PID)
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
                    "(reference file [%s] failed to open: [%s])! "
                    "Marking group as not locked!", singlegrp_no, strerror(err), reference_file);

            userlog("ERROR: Lock for singleton group %d is inconsistent "
                    "(reference file [%s] is missing: [%s])! "
                    "Marking group as not locked!", singlegrp_no, strerror(err), reference_file);

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
 * We count in group 0, which is virtual group and it is always locked
 */
expublic int ndrx_sg_nrgroups()
{
    return ndrx_G_libnstd_cfg.sgmax+1;
}

/**
 * Take a snapshoot of all groups, to be used by
 * batch startup processes (so that batch is started with the single status)
 * @param lock_status_out where to put the group statuses
 * @param lock_status_out_len length of the lock_status_out
 * @param flags 0 or (NDRX_SG_NOORDER_LCK -> if ordering not required, assume locked)
 *  additonally may pass NDRX_SG_CHK_PID to check for lock provider pid
 */
expublic void ndrx_sg_get_lock_snapshoot(int *lock_status_out, int *lock_status_out_len, long flags)
{
    int i=0;
    int locked;
    ndrx_sg_shm_t *p;

    /* fix up the output len */
    if (*lock_status_out_len>ndrx_G_libnstd_cfg.sgmax)
    {
        *lock_status_out_len = ndrx_G_libnstd_cfg.sgmax;
    }

    for (i=0; i<*lock_status_out_len; i++)
    {
        p=ndrx_sg_get(i);

        if ( (p->flags & NDRX_SG_NO_ORDER) && (flags & NDRX_SG_NOORDER_LCK))
        {
            lock_status_out[i]=EXTRUE;
        }
        else
        {
            lock_status_out[i]=ndrx_sg_is_locked_int(i, p, NULL, (flags & NDRX_SG_CHK_PID));
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
    if (singlegrp_no>0)
    {
        ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
        atomic_store(&sg->is_clt_booted, EXTRUE);
    }
}

/**
 * Set the bootflag on.
 * This assumes that shared memory is already attached.
 * If the LCF was not attached, then ndrxd would not get so far.
 * @param singlegrp_no single group number
 */
expublic void ndrx_sg_bootflag_srv_set(int singlegrp_no)
{
    if (singlegrp_no>0)
    {
        ndrx_sg_shm_t * sg = NDRX_SG_GET_PTR(singlegrp_no);
        atomic_store(&sg->is_srv_booted, EXTRUE);
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
