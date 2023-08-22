/**
 * @brief Locking Finite State Machine
 *
 * @file locksm.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <ubfutil.h>
#include <cconfig.h>
#include <exsm.h>
#include <singlegrp.h>
#include <lcfint.h>
#include "exsinglesv.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NR_TRANS 10 /**< Max number of transitions for each state */
/*---------------------------Enums--------------------------------------*/

/**
 * Locking state machine events
 */
enum
{   
      ev_ok
    , ev_locked
    , ev_unlocked
    , ev_err
    , ev_busy
    , ev_abort
};

/**
 * Locking state machine states
 */
enum
{
    st_get_singlegrp
    , st_chk_l_lock
    , st_chk_l_unlock /**< Was local unlocked? */
    , st_do_lock
    , st_chk_mmon
    , st_ping_lock
    , st_shm_refresh
    , st_abort
    , st_abort_unlock
};

/*---------------------------Typedefs-----------------------------------*/
NDRX_SM_T(ndrx_locksm_t, NR_TRANS);

/**
 * Locking state machine context
 */
typedef struct
{
    ndrx_sg_shm_t *pshm; /**< Shared memory ptr of current group        */
    ndrx_sg_shm_t  local;   /**< Atomically copied shard memory entry   */
    /** 
     * New precalcualted refresh time, so that at of the system
     * freeze this would already be old
     */
    time_t new_refresh;
} ndrx_locksm_ctx_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate int get_singlegrp(void *ctx);
exprivate int chk_l_lock(void *ctx);
exprivate int chk_l_unlock(void *ctx);
exprivate int chk_mmon(void *ctx);
exprivate int do_lock(void *ctx);
exprivate int ping_lock(void *ctx);
exprivate int shm_refresh(void *ctx);
exprivate int do_abort(void *ctx);
exprivate int do_abort_unlock(void *ctx);

ndrx_locksm_t M_locksm[] = {

    NDRX_SM_STATE( st_get_singlegrp, get_singlegrp,
          NDRX_SM_TRAN      (ev_locked,     st_chk_l_lock)
        , NDRX_SM_TRAN      (ev_unlocked,   st_chk_l_unlock)
        , NDRX_SM_TRAN      (ev_err,        st_abort)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_chk_l_lock, chk_l_lock,
          NDRX_SM_TRAN      (ev_locked,     st_ping_lock)
        , NDRX_SM_TRAN      (ev_unlocked,   st_abort_unlock)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_chk_l_unlock, chk_l_lock,
          NDRX_SM_TRAN      (ev_locked,     st_abort)
        , NDRX_SM_TRAN      (ev_unlocked,   st_chk_mmon)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_ping_lock, ping_lock,
          NDRX_SM_TRAN      (ev_ok,         st_shm_refresh)
        , NDRX_SM_TRAN      (ev_err,        st_abort_unlock)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_shm_refresh, shm_refresh,
          NDRX_SM_TRAN      (ev_ok,         NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN      (ev_err,        st_abort_unlock)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_abort, do_abort,
          NDRX_SM_TRAN      (ev_abort,      NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_abort_unlock, do_abort_unlock,
          NDRX_SM_TRAN      (ev_abort,      NDRX_SM_ST_RETURN)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_chk_mmon, chk_mmon,
          NDRX_SM_TRAN      (ev_ok,         st_do_lock)
        , NDRX_SM_TRAN      (ev_busy,       NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN_END
        )
    , NDRX_SM_STATE(st_do_lock, do_lock,
          NDRX_SM_TRAN      (ev_ok,         NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN      (ev_busy,       NDRX_SM_ST_RETURN0)
        , NDRX_SM_TRAN      (ev_err,        st_abort_unlock)
        , NDRX_SM_TRAN_END
        )
};

/**
 * Read group entry, get ptr to SHM
 * and also get the local copy.
 * Calculate next refresh time.
 */
exprivate int get_singlegrp(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret;
    struct timespec ts;

    lock_ctx->pshm=ndrx_sg_get(ndrx_G_exsinglesv_conf.singlegrp);
    if (NULL==lock_ctx->pshm)
    {
        NDRX_LOG(log_error, "Failed to get single group: %s", 
                ndrx_G_exsinglesv_conf.singlegrp);
        ret = ev_err;
        goto out;
    }

    /* Load group locally... */
    ndrx_sg_load(&lock_ctx->local, lock_ctx->pshm);

    /* mark new refresh time */
    ndrx_realtime_get(&ts);
    lock_ctx->new_refresh = ts.tv_sec;

    /* determine the current state of the group */
    switch(ndrx_sg_is_locked(ndrx_G_exsinglesv_conf.singlegrp, NULL, 0))
    {
        case EXTRUE:
            ret = ev_locked;
            break;
        default:
            ret = ev_unlocked;
            break;
    }

out:
    return ret;
}

/**
 * Check if local process is locked
 */
exprivate int chk_l_lock(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret;

    if (ndrx_G_exsinglesv_conf.locked1)
    {
        /* local process is locked */
        ret = ev_locked;
    }
    else
    {
        /* local process is unlocked */
        ret = ev_unlocked;
    }

    return ret;
}

/**
 * Peform ping lock. In case if
 * lock2 is locked, unlock
 * in case if lock2 is unlocked, lock
 */
exprivate int ping_lock(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret=ev_ok;

    if (!ndrx_G_exsinglesv_conf.locked2)
    {
        if (EXSUCCEED!=ndrx_exsinglesv_file_lock(NDRX_LOCK_FILE_2, ndrx_G_exsinglesv_conf.lockfile_2))
        {
            ret=ev_err;
            goto out;
        }
        ndrx_G_exsinglesv_conf.locked2=EXTRUE;
    }
    else
    {
        if (EXSUCCEED!=ndrx_exsinglesv_file_unlock(NDRX_LOCK_FILE_2))
        {
            ret=ev_err;
            goto out;
        }
        ndrx_G_exsinglesv_conf.locked2=EXFALSE;
    }
out:
    return ret;
}

/**
 * Write cache timestamp back to shared memory...
 */
exprivate int shm_refresh(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret=ev_ok;

    if (EXSUCCEED!=ndrx_sg_do_refresh(EXFAIL, &lock_ctx->local,
        tpgetnodeid(), tpgetsrvid(), lock_ctx->new_refresh))
    {
        ret=ev_err;
        goto out;
    }
out:
    return ret;
}

/**
 * Remove the locks and return falure
 */
exprivate int do_abort(void *ctx)
{
    /* just close the locks  */
    ndrx_exsinglesv_uninit(EXFALSE, EXFALSE);

    return ev_abort;
}

/**
 * Abort the service and unlock shared memory
 */
exprivate int do_abort_unlock(void *ctx)
{
    /* just close the locks  */
    ndrx_exsinglesv_uninit(EXFALSE, EXTRUE);

    return ev_abort;
}

/**
 * Check maintenance mode.
 */
exprivate int chk_mmon(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret=ev_ok;

    if (lock_ctx->local.is_mmon)
    {
        /* maintenance mode is ON */
        NDRX_LOG(log_debug, "Singleton group %d is in maintenance mode", 
                ndrx_G_exsinglesv_conf.singlegrp);
        ret = ev_busy;
    }
    else if (ndrx_G_shmcfg->is_mmon)
    {
        /* maintenance mode is ON */
        NDRX_LOG(log_debug, "Application is in maintenance mode");
        ret = ev_busy;
    }
    else
    {
        /* maintenance mode is OFF */
        ret = ev_ok;
    }

    return ret;
}

/**
 * Perform lock file locking and
 * mark SHM as locked (if succeed)
 */
exprivate int do_lock(void *ctx)
{
    ndrx_locksm_ctx_t *lock_ctx = (ndrx_locksm_ctx_t *)ctx;
    int ret=ev_ok;
    char *boot_script = NULL;

    switch (ndrx_exsinglesv_file_lock(NDRX_LOCK_FILE_1, 
        ndrx_G_exsinglesv_conf.lockfile_1))
    {
        case NDRX_LOCKE_BUSY:
            /* file is locked */
            NDRX_LOG(log_info, "Singleton group %d is already locked (by other node)", 
                    ndrx_G_exsinglesv_conf.singlegrp);
            ret = ev_busy;
            goto out;
        case EXSUCCEED:

            ndrx_G_exsinglesv_conf.locked1=EXTRUE;

            break;
        default:
            ret = ev_err;
            goto out;  
    }

    if (ndrx_G_exsinglesv_conf.first_boot
        && EXEOS!=ndrx_G_exsinglesv_conf.exec_on_bootlocked[0])
    {
        boot_script=ndrx_G_exsinglesv_conf.exec_on_bootlocked;
    }
    else if (ndrx_G_exsinglesv_conf.first_boot
        && EXEOS!=ndrx_G_exsinglesv_conf.exec_on_locked[0])
    {
        boot_script=ndrx_G_exsinglesv_conf.exec_on_locked;
    }

    if (NULL!=boot_script)
    {
        /* execute boot script */
        NDRX_LOG(log_info, "Executing boot script: %s", boot_script);

        ret=system(boot_script);

        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to execute boot script [%s], "
                "finished with %d", boot_script, ret);
            userlog("Failed to execute boot script [%s], "
                "finished with %d", boot_script, ret);
            ret=ev_err;
            goto out;
        }
    }

    /* mark shm as locked by us too */
    NDRX_LOG(log_debug, "Lock shared memory...");
    if (EXSUCCEED!=ndrx_sg_do_lock(ndrx_G_exsinglesv_conf.singlegrp, 
            tpgetnodeid(), tpgetsrvid(), (char *)(EX_PROGNAME), lock_ctx->new_refresh))
    {
        ret=ev_err;
        goto out;
    }

out:
    return ret;
}

/**
 * Run lock/check statemachine
 * @return 0 (on success), otherwise fail
 */
expublic int ndrx_exsinglesv_sm_run(void)
{
    ndrx_locksm_ctx_t ctx;
    return ndrx_sm_run((void *)M_locksm, NR_TRANS, st_get_singlegrp, (void *)&ctx);
}

/**
 * Runtime validation of the state machine
 * @return 0 (on success), otherwise fail
 */
expublic int ndrx_exsinglesv_sm_validate(void)
{
    return ndrx_sm_validate((void *)M_locksm, NR_TRANS, st_get_singlegrp, st_abort_unlock);
}
/* vim: set ts=4 sw=4 et smartindent: */
