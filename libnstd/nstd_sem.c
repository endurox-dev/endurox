/**
 * @brief Enduro/X Standard Libary Semaphore commons
 *
 * @file nstd_sem.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#include <stdlib.h>
#include <stdio.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstd_shm.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform RW lock
 * @param sem semaphore def
 * @param sem_num semaphore number / primitive
 * @param typ see NDRX_SEM_OP_WRITE or NDRX_SEM_OP_READ
 * @return EXFAIL in case of failure
 */
expublic int ndrx_sem_rwlock(ndrx_sem_t *sem, int sem_num, int typ)
{
    int ret;
    struct sembuf semops;
    semops.sem_num = sem_num;
    semops.sem_flg = SEM_UNDO;

    if( typ == NDRX_SEM_TYP_WRITE ) 
    {
        semops.sem_op = -sem->maxreaders;  
        do 
        {
            ret = semop( sem->semid, &semops, 1 );
            
        } while ( ret == -1 && errno == EINTR );
    }
    else 
    {
    
        semops.sem_op = -1;
        
        do 
        {
            
            ret = semop( sem->semid, &semops, 1 );
          
        } while ( ret == -1 && errno == EINTR );
        
    }
    
    if (EXFAIL==ret)
    {
        int err = errno;
        
        NDRX_LOG(log_error, "semop() failed for type %d lock: %s", 
                typ, strerror(err));
        userlog("semop() failed for type %d lock: %s", 
                typ, strerror(err));
    }

    return ret;
}

/**
 * Read / Write locks - unlock
 * @param sem semaphore def
 * @param sem_num semaphore number / primitive
 * @param typ see NDRX_SEM_OP_WRITE or NDRX_SEM_OP_READ
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_sem_rwunlock(ndrx_sem_t *sem, int sem_num, int typ)
{
    int ret;
    struct sembuf semops;
    semops.sem_num = sem_num;
    semops.sem_op = 1;
    semops.sem_flg = SEM_UNDO;

    if ( typ == NDRX_SEM_TYP_WRITE )
    {
        semops.sem_op = sem->maxreaders;
    }

    do 
    {
        ret = semop(sem->semid,&semops,1);

    } while ( ret == -1 && errno == EINTR );
    
    if (EXFAIL==ret)
    {
        int err = errno;
        
        NDRX_LOG(log_error, "semop() failed for type %d lock: %s", 
                typ, strerror(err));
        userlog("semop() failed for %d type lock: %s", 
                typ, strerror(err));
    }
    
    return ret;
}

/**
 * Generic sem lock
 * @param sem
 * @param msg
 * @return 
 */
expublic int ndrx_sem_lock(ndrx_sem_t *sem, const char *msg, int sem_num)
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
expublic int ndrx_sem_unlock(ndrx_sem_t *sem, const   char *msg, int sem_num)
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

/**
 * Open service info semaphore segment
 * @param sem initialized semaphore block
 * @param attach_on_exists if not EXFALSE, attach to sem if exists
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_sem_open(ndrx_sem_t *sem, int attach_on_exists)
{
    int ret=EXSUCCEED;
    int err;
    union semun 
    {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    
    /* creating the semaphore object --  sem_open() 
     * this will attach anyway?
     */
    sem->semid = semget(sem->key, sem->nrsems, 0660|IPC_CREAT|IPC_EXCL);

    if (EXFAIL==sem->semid) 
    {
        err = errno;
        
        if (EEXIST==err && attach_on_exists)
        {
            NDRX_LOG(log_error, "Semaphore exists [%x] - attaching",
                    sem->key);
            return ndrx_sem_attach(sem);
        }
        
        NDRX_LOG(log_error, "Failed to create sem, key[%x]: %s",
                            sem->key, strerror(err));
        ret=EXFAIL;
        goto out;
    }
    
    /* Reset semaphore... */
    memset(&arg, 0, sizeof(arg));
    arg.val = sem->maxreaders;
   
    if (semctl(sem->semid, 0, SETVAL, arg) == -1) 
    {
        NDRX_LOG(log_error, "Failed to reset to 0, key[%x], semid: %d: %s",
                            sem->key, sem->semid, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
    
    sem->attached = EXTRUE;
    
    NDRX_LOG(log_warn, "Semaphore for key %x open, id: %d", 
            sem->key, sem->semid);
out:

    NDRX_LOG(log_debug, "return %d", ret);

    return ret;
}


/**
 * Remove semaphores...
 * @param sem
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_sem_remove(ndrx_sem_t *sem, int force)
{
    int ret = EXSUCCEED;
    /* Close that one... */
    if ((sem->attached || force) && sem->semid)
    {
        NDRX_LOG(log_error, "Removing semid: %d", sem->semid);
        if (EXSUCCEED!= semctl(sem->semid, 0, IPC_RMID))
        {
            NDRX_LOG(log_warn, "semctl DEL failed err: %s", 
                    strerror(errno));
            ret=EXFAIL;
        }
        else
        {
            sem->semid=0;
        }
    }
    sem->attached=EXFALSE;
    
    return ret;
}

/**
 * Returns true if currently attached to sem
 * WARNING: This assumes that fd 0 could never be used by sem!
 * @return TRUE/FALSE
 */
expublic int ndrx_sem_is_attached(ndrx_sem_t *sem)
{
    int ret=EXTRUE;
    
    if (!sem->attached)
    {
        ret=EXFALSE;
    }

    return ret;
}

/**
 * Attach to semaphore, semaphore must exist!
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_sem_attach(ndrx_sem_t *sem)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_debug, "enter");
    
    if (sem->attached)
    {
        NDRX_LOG(log_debug, "sem, key %x, id: %d already attached", 
                sem->key, sem->semid);
        goto out;
    }
    
    /* Attach to semaphore block */
    sem->semid = semget(sem->key, sem->nrsems, IPC_EXCL);

    if (EXFAIL==sem->semid) 
    {
        NDRX_LOG(log_error, "Failed to attach sem, key [%d]: %s",
                            sem->key, strerror(errno));
        ret=EXFAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "sem: [%d] attached", sem->semid);

out:

    NDRX_LOG(log_debug, "return %d", ret);
    return ret;
}

/**
 * Close opened semaphore segment.
 * ??? Not sure shall we close it ?
 * For System V not needed.
 * @return EXSUCCEED
 */
expublic int ndrx_sem_close(ndrx_sem_t *sem)
{
    int ret=EXSUCCEED;

out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
