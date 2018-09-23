/**
 * @brief System V Queue polling
 *  Basically we will operate with special shared memory registry
 *  where will be mapping between string name -> queue id
 *  and vice versa queue id -> string
 *  the registry is protected by read/write System V semaphore. 
 *  The shared mem by it self is based on Posix shared mem.
 *  For System V, there will be new parameter used NDRX_QUEUES_MAX
 *  that will device the limits of the shared memory for queues.
 *
 * @file sys_svq.c
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

/*---------------------------Includes-----------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <ndrstandard.h>

#include <nstopwatch.h>
#include <nstd_tls.h>
#include <exhash.h>
#include <ndebug.h>
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

#if 0
struct qd_hash
{
    void *qd;
    EX_hash_handle hh; /* makes this structure hashable        */
};
typedef struct qd_hash qd_hash_t;

#endif

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#if 0
MUTEX_LOCKDECL(M_lock);
qd_hash_t *M_qd_hash = NULL;

exprivate int M_lock_secs = 0; /* Lock timeout... */
/**
 * Add queue descriptor to hash
 * @param q queue descriptor
 * @return EXSUCCEED/EXFAIL
 */
exprivate int qd_exhash_add(mqd_t q)
{
    int ret = EXSUCCEED;
    qd_hash_t * el = NDRX_CALLOC(1, sizeof(qd_hash_t));
    
    NDRX_LOG(log_dump, "Registering %p as mqd_t", q);
    if (NULL==el)
    {
        NDRX_LOG(log_error, "Failed to alloc: %s", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    el->qd  = (void *)q;
    
    MUTEX_LOCK_V(M_lock);
    
    EXHASH_ADD_PTR(M_qd_hash, qd, el);
    NDRX_LOG(log_dump, "added...");
    
    MUTEX_UNLOCK_V(M_lock);
    
out:

    return ret;
}

/**
 * Check is queue descriptor logged
 * @param q queue descriptor
 * @return EXSUCCEED/EXFAIL
 */
exprivate int qd_hash_chk(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_dump, "checking qd %p", qd);
    
    MUTEX_LOCK_V(M_lock);
    
    EXHASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    MUTEX_UNLOCK_V(M_lock);
    
    if (NULL!=ret)
    {
        return EXTRUE;
    }
    else
    {
        return EXFALSE;
    }
}

/**
 * Delete queue descriptor
 * @param q queue descriptor
 */
exprivate void qd_hash_del(mqd_t qd)
{
    qd_hash_t *ret = NULL;
    
    NDRX_LOG(log_dump, "Unregistering %p as mqd_t", qd);
    
    MUTEX_LOCK_V(M_lock);
    EXHASH_FIND_PTR( M_qd_hash, ((void **)&qd), ret);
    
    if (NULL!=ret)
    {
        EXHASH_DEL(M_qd_hash, ret);
        NDRX_FREE(ret);
    }
    
    MUTEX_UNLOCK_V(M_lock);
}

#endif

/**
 * Close queue. Basically we remove the dynamic data associated with queue
 * the queue id by it self continues to live on... until it is unlinked
 * @param mqd queue descriptor
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_close(mqd_t mqd)
{
    NDRX_LOG(log_debug, "close %p mqd", mqd);
    
    if (NULL!=mqd && (mqd_t)EXFAIL!=mqd)
    {
        NDRX_FREE(mqd);
        return EXSUCCEED;
    }
    else
    {
        NDRX_LOG(log_error, "Invalid mqd %p!", mqd);
        errno = EBADF;
        return EXFAIL;
    }
}

/**
 * Get attributes of the queue
 * @param mqd queue descriptor / ptr to descr block
 * @param emqstat queue stats
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svq_getattr(mqd_t mqd, struct mq_attr *emqstat)
{
    return EXFAIL;
}

/**
 * Perform notification - not supported
 * @param mqd queue descriptor 
 * @param notification signal event descr
 * @return EXFAIL
 */
expublic int ndrx_svq_notify(mqd_t mqd, const struct sigevent *notification)
{
    NDRX_LOG(log_error, "mq_notify() not supported by System V queue emulation!");
    userlog("mq_notify() not supported by System V queue emulation!");
    return EXFAIL;
}

/**
 * Open queue this, will include following steps:
 * - build queue object
 * - register into hash list
 * - Generate next queue id, according shm
 * - if fails, something then queue must be removed from shm
 * @param pathname queue path
 * @param oflag queue flags
 * @param mode unix permissions mode
 * @param mq_attr attributes
 * @return queue descriptor or EXFAIL
 */
expublic mqd_t ndrx_svq_open(const char *pathname, int oflag, mode_t mode, 
        struct mq_attr *attr)
{
    /* Allocate queue object */
    mqd_t mq = (mqd_t)EXFAIL;
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "enter");
    mq = NDRX_CALLOC(1, sizeof(struct ndrx_svq_info));
    
    if (NULL==mq)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes queue descriptor", 
                (int)sizeof(struct ndrx_svq_info));
        userlog("%s: Failed to malloc %d bytes queue descriptor", 
                __func__, (int)sizeof(struct ndrx_svq_info));
        mq = (mqd_t)EXFAIL;
        EXFAIL_OUT(ret);
    }
    
    /* so we have an options:
     * - if we create a Q, then alloc new ID
     * - if queue already exists SHM, then we can use that ID directly
     */
    mq->qid = ndrx_svqshm_get((char *)pathname, oflag);
    mq->thread = pthread_self();
    NDRX_STRCPY_SAFE(mq->qstr, pathname);
    mq->mode = mode;
    memcpy(&(mq->attr), attr, sizeof (*attr));
    
out:
    
    if (EXSUCCEED!=ret)
    {
        if (NULL!=(mqd_t)EXFAIL)
        {
            NDRX_FREE((char *)mq);
            mq = (mqd_t)EXFAIL;
        }
    }

    NDRX_LOG(log_debug, "return %p/%d", mq, (int)mq);
    return mq;
}

expublic ssize_t ndrx_svq_timedreceive(mqd_t mqd, char *ptr, size_t maxlen, 
        unsigned int *priop, const struct timespec *__abs_timeout)
{
    return EXFAIL;
}

expublic int ndrx_svq_timedsend(mqd_t mqd, const char *ptr, size_t len, 
        unsigned int prio, const struct timespec *__abs_timeout)
{
    return EXFAIL;
}

expublic int ndrx_svq_send(mqd_t mqd, const char *ptr, size_t len, 
        unsigned int prio)
{
    return EXFAIL;
}

expublic ssize_t ndrx_svq_receive(mqd_t mqd, char *ptr, size_t maxlen, 
        unsigned int *priop)
{
    return EXFAIL;
}

expublic int ndrx_svq_setattr(mqd_t mqd, const struct mq_attr *emqstat, 
        struct mq_attr *oemqstat)
{
    return EXFAIL;
}

expublic int ndrx_svq_unlink(const char *pathname)
{
    return EXFAIL;
}

