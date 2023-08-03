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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
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
 * Is given group locked?
 * Get the ptr to ndrx_sg_shm_t and check following items:
 * - field is_locked is EXTRUE
 * - last_refresh (substracted current boot time / gettimeofday()) is less ndrx_G_libnstd_cfg.sgrefresh
 * @param singlegrp_no single group number
 * @param flags biwtise flags, NDRX_SG_CHK_PID used for additional check of exsinglesv running.
 * @return EXFAIL/EXFALSE (not locked)/EXTRUE (locked)
 */
expublic int ndrx_sg_is_locked(int singlegrp_no, long flags)
{
    int ret = EXFALSE;
    ndrx_sg_shm_t * sg = ndrx_sg_get(singlegrp_no);

    if (NULL==sg)
    {
        return EXFAIL;
    }

    if (0==singlegrp_no)
    {
        /* group 0 is not locked */
        return EXFALSE;
    }

    /* TODO:
     * Use atomic load to get the values needed for checking
     */
    return ret;
}

/** Return snapshoot of current locking */
expublic void ndrx_sg_get_lock_snapshoot(int *lock_status_out, int lock_status_out_len)
{

}


/* vim: set ts=4 sw=4 et smartindent: */
