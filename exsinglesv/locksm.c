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
    , ev_wait
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
    , st_ping_lock
    , st_shm_refresh
    , st_abort
    , st_abort_unlock
    , st_chk_mmon
    , st_do_lock
    , st_count
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

ndrx_locksm_t M_locksm[st_count] = {

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
        , NDRX_SM_TRAN      (ev_wait,       NDRX_SM_ST_RETURN0)
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
    int ret, lock_status;
    struct timespec ts;

    /* get new time-stamp. Shall be done as first step, so that if
     * system freezes happens during the work of the binary
     * we would write expired time-stamp to shared memory
     * and that would cause to group to unlock
     */
    ndrx_realtime_get(&ts);

    lock_ctx->pshm=ndrx_sg_get(ndrx_G_exsinglesv_conf.procgrp_lp_no);
    if (NULL==lock_ctx->pshm)
    {
        NDRX_LOG(log_error, "Failed to get singleton process group: %s", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
        ret = ev_err;
        goto out;
    }

    /* Load group locally... */
    ndrx_sg_load(&lock_ctx->local, lock_ctx->pshm);

    lock_ctx->new_refresh = ts.tv_sec;

    lock_status = ndrx_sg_is_locked(ndrx_G_exsinglesv_conf.procgrp_lp_no, NULL, 0);

    NDRX_LOG(log_debug, "Current group %d lock status: %d", 
        ndrx_G_exsinglesv_conf.procgrp_lp_no, lock_status);

    /* determine the current state of the group */
    switch(lock_status)
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

    if (ndrx_G_exsinglesv_conf.is_locked)
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

    /* 
     * ==== PHASE1 (essential transactionality) ====
     * This ensure that commit activity is proceeded, that we have guarantee
     * that node has still lock.
     * 
     * However the phase1 does not deal fact, that node can be suspended and resumed
     * and tmsrv and tmqueue is not removed immediately. Event in state of
     * GPFS "preMount", processes might try to do some activity with FS, but
     * as FS it available, the script shall killall binaries from the
     * process group.
     * =============================================
     * 
     * Validate that lock is still functional:
     * however this is not very useful as GPFS keeps reporting us as a lock
     * owners after the suspend / resume, however other node has already taken
     * over.
     * The refresh time after vm suspend/resume does not help too, as hupervisors
     * seem to gradually increase the system time, instead of direct leap which
     * would allow transactions at commit decision to detect that lock is lost.
     * 
     * To fix this issue, there could be several ways to do that:
     * 1. use third time-based arbitration node. If us as lock holder node has
     * non matching system time against all of the remote nodes, then unlock the group
     * as we might have been suspended and resumed. Downside for this approach is that
     * additional server for the cluster is required to do the arbitration.
     * 
     * 2. Second option would be that each node at certain ping-file offset writes
     * node_id, lock_status (0|1), and UTC timestamp, checksum.
     * Each exsinglesv periodically updates the status in shm (acquires write lock firstly)
     * and then checks the status of the other nodes. if our node is lagging behind other
     * nodes clock, then unlock the group. Check all nodes, as some of them can be turned
     * off.
     *  2.1 we create new XATMI call tpsgislocked(<sg_id => 0 default>, 0 flags); Function
     * perofrms following: from the list of nodes in NDRX_SGNODES=2,4,5, performs call
     * to remote nodes (not our node), to check their lock_status and timestamp. If time
     * difference for called nodes is less than NDRX_SGREFRESH, then we are good. and we
     * keep lock status from the shared memory / locked or not.
     * Network service is advertised by EXISNGLESV, Service name: @EXSINGLESV-<nodeid>-<sg_id>
     * 2.2 if for some node network is down, then we read other node/SG status from the ping lock
     * file at given node offset. If checksum is valid, we use that time. If checksums are
     * invalid, we assume that node is not locked and time is valid our against their.
     * It is crutial for admins to keep time accurrate.
     * 
     * In case if STONITH is provided by the underlaying cluster, these checks may be disabled,
     * as STONITH shall guarantee that new node gets the lock, if STONITH has switched off
     * the originally locked node.
     * 
     * tpsgislocked() shall be called by:
     *  - exsinglesv() for getting extneded shared memory lock checks.
     *  - tmsrv() at point when commit or abort is logged. As at this point it is crutial
     *  to proceed only we know that we are still locked.
     * 
     * ==== tmqueue and tmsrv shall be updated to following (PHASE2), if preMount 
     * process removal is not supported ====
     * 
     * A) Periodically (activated by tmrecoversv/cl):
     * 
     * B) tmqueue, collects records with xa_recover. Chek that all transactions are in tmqueue
     *  transaction registry. If missing, check again file existance (maybe completed) transaction.
     * If file still exist, process shall restart as concurrent activity has happened.
     * Tmqueue shall validate that all msgid files exist too. Again check registry/if missing
     * check file again, if missing restart is required.
     * 
     * For TMQ if there is requested for commit, but transaction is not found in the registry, then
     * check prepared folder, if txn is found, restart is required.
     * 
     * C) tmsrv, activated by extra tmrecover call, shall collect all log files, check registry,
     * if missing check file again (exists). If exists, then restart is required (to reload)
     * and repocess all transactions. Additionaly check that all memory transactions have
     * files on disk, if missing, then restart is required (some concurrent activity has
     * happened).
     */
    if ( EXSUCCEED!=ndrx_exsinglesv_file_chkpid(NDRX_LOCK_FILE_1, ndrx_G_exsinglesv_conf.lockfile_1) )
    {
        ret=ev_err;
        goto out;
    }

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

    if (EXSUCCEED!=ndrx_sg_do_refresh(ndrx_G_exsinglesv_conf.procgrp_lp_no, NULL,
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
        NDRX_LOG(log_debug, "Singleton process group %d is in maintenance mode", 
                ndrx_G_exsinglesv_conf.procgrp_lp_no);
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

    if (!ndrx_G_exsinglesv_conf.locked1)
    {
        switch (ndrx_exsinglesv_file_lock(NDRX_LOCK_FILE_1, 
            ndrx_G_exsinglesv_conf.lockfile_1))
        {
            case NDRX_LOCKE_BUSY:
                /* file is locked */
                NDRX_LOG(log_info, "Singleton process group %d "
                        "is already locked (by other node)", 
                        ndrx_G_exsinglesv_conf.procgrp_lp_no);
                ret = ev_busy;
                goto out;
            case EXSUCCEED:

                ndrx_G_exsinglesv_conf.locked1=EXTRUE;

                break;
            default:
                ret = ev_err;
                goto out;  
        }
    }

    /* If this is first boot, lock immeditally */
    if (ndrx_G_exsinglesv_conf.first_boot)
    {
        ndrx_G_exsinglesv_conf.is_locked=EXTRUE;
    }
    else
    {
        ndrx_G_exsinglesv_conf.wait_counter++;
    
        if (ndrx_G_exsinglesv_conf.wait_counter>ndrx_G_exsinglesv_conf.locked_wait)
        {
            /* we have waited enough, lock it */
            ndrx_G_exsinglesv_conf.is_locked=EXTRUE;
        }
        else
        {
            /* we have to wait more */
            NDRX_LOG(log_info, "Waiting after files locked %d/%d",
                ndrx_G_exsinglesv_conf.wait_counter,
                ndrx_G_exsinglesv_conf.locked_wait);

            ret=ev_wait;
            goto out;
        }
    }

    /* we are locked down here... */
    if (ndrx_G_exsinglesv_conf.first_boot
        && EXEOS!=ndrx_G_exsinglesv_conf.exec_on_bootlocked[0])
    {
        boot_script=ndrx_G_exsinglesv_conf.exec_on_bootlocked;
    }
    else if (!ndrx_G_exsinglesv_conf.first_boot
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
            NDRX_LOG(log_error, "ERROR: Lock script [%s], "
                "exited with %d", boot_script, ret);
            userlog("ERROR: Lock script [%s], "
                "exited with %d", boot_script, ret);
            ret=ev_err;
            goto out;
        }
    }

    /* mark shm as locked by us too */
    NDRX_LOG(log_debug, "Lock shared memory...");
    if (EXSUCCEED!=ndrx_sg_do_lock(ndrx_G_exsinglesv_conf.procgrp_lp_no, 
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
 * Runtime compilation of the state machine
 * @return 0 (on success), otherwise fail
 */
expublic int ndrx_exsinglesv_sm_comp(void)
{
    return ndrx_sm_comp((void *)M_locksm, st_count, NR_TRANS, st_do_lock);
}
/* vim: set ts=4 sw=4 et smartindent: */
