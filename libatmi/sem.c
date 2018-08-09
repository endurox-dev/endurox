/**
 * @brief Semaphore handling
 *
 * @file sem.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <sys/sem.h>

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
#include <sys/ipc.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define SEM_SVC_OPS             0   /* Semaphore for shared memory management */
#define SEM_SVC_GLOBAL_NUM      0   /* Semaphore array for global svc managmenet */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_sem_t G_sem_svcop;              /* Service operations       */
exprivate int M_init = EXFALSE;                 /* no init yet done         */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int ndrx_lock(ndrx_sem_t *sem, const char *msg, int sem_num);
extern int ndrx_unlock(ndrx_sem_t *sem, const char *msg, int sem_num);

/**
 * Initialise prefix part, that is needed for shm...
 * @param ndrx_prefix
 * @return 
 */
expublic int ndrxd_sem_init(char *q_prefix)
{
    memset(&G_sem_svcop, 0, sizeof(G_sem_svcop));
    
    /* Service queue ops */
    G_sem_svcop.key = G_atmi_env.ipckey + SEM_SVC_OPS;
    NDRX_LOG(log_debug, "Using service semaphore key: [%d]", 
            G_sem_svcop.key);
    
    M_init = EXTRUE;
    return EXSUCCEED;
}

/**
 * Attach to semaphore, semaphore must exist!
 * @return
 */
expublic int ndrx_sem_attach(ndrx_sem_t *sem)
{
    int ret=EXSUCCEED;
    char *fn = "ndrx_sem_attach";

    NDRX_LOG(log_debug, "%s enter", fn);
    
    /**
     * Library not initialised
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "%s: ndrx shm/sem library not initialised!", fn);
        ret=EXFAIL;
        goto out;
    }
    
    if (sem->attached)
    {
        NDRX_LOG(log_debug, "%s: sem, key %x, id: %d already attached", 
                sem->key, sem->semid);
        goto out;
    }
    
    /* Attach to semaphore block */
    sem->semid = semget(sem->key, G_atmi_env.nrsems, IPC_EXCL);

    if (EXFAIL==sem->semid) 
    {
        NDRX_LOG(log_error, "%s: Failed to attach sem, key [%d]: %s",
                            fn, sem->key, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
#if 0
    sem->sem[0].sem_num = 0;
    sem->sem[1].sem_num = 0;
    sem->sem[0].sem_flg = SEM_UNDO; /* Release semaphore on exit */
    sem->sem[1].sem_flg = SEM_UNDO; /* Release semaphore on exit */
#endif
    NDRX_LOG(log_debug, "sem: [%d] attached", sem->semid);

out:

    NDRX_LOG(log_debug, "%s return %d", fn, ret);
    return ret;
}

/**
 * Close opened semaphore segment.
 * ??? Not sure ca we close it ?
 * @return
 */
exprivate int ndrxd_sem_close(ndrx_sem_t *sem)
{
    int ret=EXSUCCEED;

out:
    return ret;
}

/**
 * Open service info semaphore segment
 * @return
 */
exprivate int ndrxd_sem_open(ndrx_sem_t *sem)
{
    int ret=EXSUCCEED;
    char *fn = "ndrxd_sem_open";
    union semun 
    {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;

    NDRX_LOG(log_debug, "%s enter", fn);
    /**
     * Library not initialized
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx sem library not initialized");
        ret=EXFAIL;
        goto out;
    }

    /* Unlink semaphore, if any... ??
    sem_unlink(sem->path);
    */

    /* creating the semaphore object --  sem_open() */
    sem->semid = semget(sem->key, G_atmi_env.nrsems, 0660|IPC_CREAT);

    if (EXFAIL==sem->semid) 
    {
        NDRX_LOG(log_error, "%s: Failed to create sem, key[%x]: %s",
                            fn, sem->key, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    /* Reset semaphore... */
    /*arg = 0;*/
    memset(&arg, 0, sizeof(arg));
   
    if (semctl(sem->semid, 0, SETVAL, arg) == -1) 
    {
        NDRX_LOG(log_error, "%s: Failed to reset to 0, key[%x], semid: %d: %s",
                            fn, sem->key, sem->semid, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    sem->attached = EXTRUE;
    
#if 0
    /* These never change so leave them outside the loop */
    sem->sem[0].sem_num = 0;
    sem->sem[1].sem_num = 0;
    sem->sem[0].sem_flg = SEM_UNDO; /* Release semaphore on exit */
    sem->sem[1].sem_flg = SEM_UNDO; /* Release semaphore on exit */
#endif
    NDRX_LOG(log_warn, "Semaphore for key %x open, id: %d", 
            sem->key, sem->semid);
out:

    NDRX_LOG(log_debug, "%s return %d", fn, ret);

    return ret;
}

/**
 * Open semaphore
 * @return
 */
expublic int ndrxd_sem_open_all(void)
{
    int ret=EXSUCCEED;

    if (EXSUCCEED!=ndrxd_sem_open(&G_sem_svcop))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Closes all semaphore resources, generally ignores errors.
 * @return FAIL if something failed.
 */
expublic int ndrxd_sem_close_all(void)
{
    int ret=EXSUCCEED;

    ndrxd_sem_close(&G_sem_svcop);
    
    return ret;
}

/**
 * Remove semaphores...
 * @param sem
 */
exprivate void remove_sem(ndrx_sem_t *sem, int force)
{
    /* Close that one... */
    if (sem->attached || force)
    {
        NDRX_LOG(log_error, "Removing semid: %d", sem->semid);
        if (EXSUCCEED!= semctl(sem->semid, 0, IPC_RMID))
        {
                NDRX_LOG(log_warn, "semctl DEL failed err: %s", 
                        strerror(errno));
        }
    }
    sem->attached=EXFALSE;
}

/**
 * Does delete all semaphore blocks.
 */
expublic int ndrxd_sem_delete(void)
{
    if (M_init)
    {
        remove_sem(&G_sem_svcop, EXFALSE);
    }
    else
    {
        NDRX_LOG(log_error, "SHM library not initialized!");
        return EXFAIL;
    }

    return EXSUCCEED;
}

/**
 * Does delete all semaphore blocks.
 */
expublic void ndrxd_sem_delete_with_init(char *q_prefix)
{
    if (!M_init)
    {
        ndrxd_sem_init(q_prefix);
    }
    
    if (EXSUCCEED==ndrxd_sem_open(&G_sem_svcop))
    {
        remove_sem(&G_sem_svcop, EXTRUE);
    }
}


/**
 * Returns true if currently attached to sem
 * WARNING: This assumes that fd 0 could never be used by sem!
 * @return TRUE/FALSE
 */
expublic int ndrxd_sem_is_attached(ndrx_sem_t *sem)
{
    int ret=EXTRUE;
    
    if (!sem->attached)
    {
        ret=EXFALSE;
    }

    return ret;
}
/**
 * Attach to semaphore block.
 * @lev indicates the attach level (should it be service array only)?
 * @return 
 */
expublic int ndrx_sem_attach_all(void)
{
    int ret=EXSUCCEED;
   
    if (EXSUCCEED!=ndrx_sem_attach(&G_sem_svcop))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
   return ret;
}

/**
 * Lock service operation
 * @return 
 */
expublic int ndrx_lock_svc_op(const char *msg)
{
    return ndrx_lock(&G_sem_svcop, msg, SEM_SVC_GLOBAL_NUM);
}

/**
 * Unlock service operation
 * @return 
 */
expublic int ndrx_unlock_svc_op(const char *msg)
{
    return ndrx_unlock(&G_sem_svcop, msg, SEM_SVC_GLOBAL_NUM);
}

/**
 * Lock the access to specific service in shared mem
 * Only for poll() mode
 * @param svcnm service name
 * @return 
 */
expublic int ndrx_lock_svc_nm(char *svcnm, const char *msg)
{
    int semnum = 1 + ndrx_hash_fn(svcnm) % (G_atmi_env.nrsems-1);
#ifdef NDRX_SEM_DEBUG
    char tmp_buf[1024];
    
    snprintf(tmp_buf, sizeof(tmp_buf), "ndrx_unlock_svc_nm-> semnum:%d, %s - %s", 
            semnum, svcnm, msg);
    
    return ndrx_lock(&G_sem_svcop, svcnm, semnum);
    
#else 
    
    return ndrx_lock(&G_sem_svcop, svcnm, semnum);
#endif
}

/**
 * Unlock the access to service
 * @param svcnm
 * @return 
 */
expublic int ndrx_unlock_svc_nm(char *svcnm, const char *msg)
{
    int semnum = 1 + ndrx_hash_fn(svcnm) % (G_atmi_env.nrsems-1);
#ifdef NDRX_SEM_DEBUG
    char tmp_buf[1024];

    snprintf(tmp_buf, sizeof(tmp_buf), "ndrx_unlock_svc_nm-> semnum: %d, %s - %s", 
            semnum, svcnm, msg);
    return ndrx_unlock(&G_sem_svcop, svcnm, semnum);
#else
    return ndrx_unlock(&G_sem_svcop, svcnm, semnum);
#endif
}

/**
 * Generic sem lock
 * @param sem
 * @param msg
 * @return 
 */
expublic int ndrx_lock(ndrx_sem_t *sem, const char *msg, int sem_num)
{
    int ret=EXSUCCEED;
    int errno_int;
    struct sembuf semOp[2];
       
    semOp[0].sem_num = sem_num;
    semOp[1].sem_num = sem_num;
    semOp[0].sem_flg = SEM_UNDO; /* Release semaphore on exit */
    semOp[1].sem_flg = SEM_UNDO; /* Release semaphore on exit */
    
    semOp[0].sem_op = 0; /* Wait for zero */
    semOp[1].sem_op = 1; /* Add 1 to lock it*/
    
#ifdef NDRX_SEM_DEBUG
    userlog("ENTER: ndrx_lock: %s", msg);
#endif
    
    while(EXFAIL==(ret=semop(sem->semid, semOp, 2)) && (EINTR==errno || EAGAIN==errno))
    {
        NDRX_LOG(log_warn, "%s: Interrupted while waiting for semaphore!!", msg);
    };
    errno_int = errno;
    
    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_warn, "%s/%d/%d: semaphore locked... ", msg, sem->semid, sem_num);
    }
    else
    {
        NDRX_LOG(log_warn, "%s/%d/%d: failed to lock (%d): %s", msg, sem->semid, 
                sem_num, errno_int,
                strerror(errno_int));
    }
    
#ifdef NDRX_SEM_DEBUG
    userlog("EXIT: ndrx_lock %d: %s", ret, msg);
#endif
    
    return ret;
}

/**
 * Generic sem unlock
 * @param sem
 * @param msg
 * @return 
 */
expublic int ndrx_unlock(ndrx_sem_t *sem, const   char *msg, int sem_num)
{
    struct sembuf semOp[1];
       
    semOp[0].sem_num = sem_num;
    semOp[0].sem_flg = SEM_UNDO; /* Release semaphore on exit */
    semOp[0].sem_op = -1; /* Decrement to unlock */
    

#ifdef NDRX_SEM_DEBUG
    userlog("ENTER: ndrx_unlock: %s", msg);
#endif
    
    if (EXSUCCEED!=semop(sem->semid, semOp, 1))
    {
        NDRX_LOG(log_debug, "%s/%d%/d: failed: %s", msg, 
                sem->semid, sem_num, strerror(errno));
        return EXFAIL;
    }
    
    NDRX_LOG(log_warn, "%s/%d/%d semaphore un-locked", 
            msg, sem->semid, sem_num);
    
#ifdef NDRX_SEM_DEBUG
    userlog("EXIT: ndrx_unlock: %s", msg);
#endif
    
    return EXSUCCEED;
}

/* vim: set ts=4 sw=4 et smartindent: */
