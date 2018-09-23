/**
 * @brief Shared memory for System V queues
 *  we need following ops here:
 *  - lookup for queue id, if specified create, then create on
 *  - 
 *
 * @file sys_svqshm.c
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
#include <fcntl.h>           /* For O_* constants */
#include <ndrstandard.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <nstopwatch.h>
#include <nstd_tls.h>
#include <exhash.h>
#include <ndebug.h>
#include <sys_svq.h>
#include <atmi.h>   /**< little bit mess with dependencies as we are at nstd... */
#include "nstd_shm.h"
#include <sys_svq.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Perform init of the shared resources
 */
#define INIT_ENTRY \
if (!ndrx_G_svqshm_init) \
    {\
        MUTEX_LOCK_V(ndrx_G_svqshm_init_lock);\
        \
        if (!ndrx_G_svqshm_init)\
        {\
            ret = ndrx_svqshm_init();\
        }\
        \
        MUTEX_UNLOCK_V(ndrx_G_svqshm_init_lock);\
        \
        if (EXFAIL==ret)\
        {\
            NDRX_LOG(log_error, "Failed to create/attach System V Queues "\
                    "mapping shared memory blocks!");\
        }\
    }

#define MAX_READERS             50      /* Max readers for RW lock... */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

expublic volatile int ndrx_G_svqshm_init = EXFALSE;
/* have some lock for this argument */
MUTEX_LOCKDECL(ndrx_G_svqshm_init_lock);

exprivate ndrx_shm_t M_map_p2s;          /**< Posix to System V mapping       */
exprivate ndrx_shm_t M_map_s2p;          /**< System V to Posix mapping       */
exprivate ndrx_sem_t M_map_sem;          /**< RW semaphore for SHM protection */

/* Also we need some array of semaphores for RW locking */

exprivate char *M_qprefix = NULL;       /**< Queue prefix used by mappings  */
exprivate long M_queuesmax = 0;         /**< Max number of queues           */
exprivate int  M_readersmax = 0;        /**< Max number of concurrent lckrds*/
exprivate key_t M_sem_key = 0;          /**< Semphoare key                  */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize the shared memory blocks.
 * This assumes that basic environment is loaded. Because we need access to
 * max queues setting.
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_svqshm_init(void)
{
    int ret = EXSUCCEED;
    char *tmp;
    
    /* Load some minimum env for shared mem processing */
    M_qprefix = getenv(CONF_NDRX_QPREFIX);
    if (NULL==M_qprefix)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        userlog("Missing config key %s - FAIL", CONF_NDRX_QPREFIX);
        EXFAIL_OUT(ret);
    }

    /* get number of concurrent rheads */
    tmp = getenv(CONF_NDRX_SVQREADERSMAX);
    if (NULL==tmp)
    {
        M_readersmax = MAX_READERS;
        NDRX_LOG(log_error, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_MSGQUEUES_MAX, M_readersmax);
    }
    else
    {
        M_readersmax = atol(tmp);
    }
    
    /* Get SV5 IPC */
    tmp = getenv(CONF_NDRX_IPCKEY);
    if (NULL==tmp)
    {
        /* Write to ULOG? */
        NDRX_LOG(log_error, "Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        userlog("Missing config key %s - FAIL", CONF_NDRX_IPCKEY);
        EXFAIL_OUT(ret)
    }
    else
    {
	int tmpkey;
        
        sscanf(tmp, "%x", &tmpkey);
	M_sem_key = tmpkey;

        NDRX_LOG(log_debug, "(sysv queues): SystemV SEM IPC Key set to: [%x]",
                            M_sem_key);
    }
    
    
    memset(&M_map_p2s, 0, sizeof(M_map_p2s));
    memset(&M_map_s2p, 0, sizeof(M_map_s2p));
    
    /* fill in shared memory details, path + size */
    
    M_map_p2s.fd = EXFAIL;
    snprintf(M_map_p2s.path, sizeof(M_map_p2s.path), NDRX_SHM_P2S, M_qprefix);
    M_map_p2s.size = sizeof(ndrx_svq_map_t)*M_queuesmax;
    
    M_map_s2p.fd = EXFAIL;
    snprintf(M_map_s2p.path, sizeof(M_map_s2p.path), NDRX_SHM_S2P, M_qprefix);
    M_map_s2p.size = sizeof(ndrx_svq_map_t)*M_queuesmax;
    
    
    if (EXSUCCEED!=ndrx_shm_open(&M_map_p2s, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open shm [%s] - System V Queues cannot work",
                    M_map_p2s.path);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_shm_open(&M_map_s2p, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open shm [%s] - System V Queues cannot work",
                    M_map_s2p.path);
        EXFAIL_OUT(ret);
    }
    
    memset(&M_map_sem, 0, sizeof(M_map_sem));
    
    /* Service queue ops */
    M_map_sem.key = M_sem_key + NDRX_SEM_SV5LOCKS;
    
    /*
     * Currently using single semaphore.
     * But from balancing when searching entries, we could use multiple sems.. 
     * to protect both shared mems...
     * If we use different two sems one for p2s and another for s2p we could
     * easily run into deadlock. Semop allows atomically work with multiple
     * semaphores, so maybe this can be used to increase performance.
     */
    M_map_sem.nrsems = 1;
    M_map_sem.maxreaders = M_readersmax;
    
    NDRX_LOG(log_debug, "Using service semaphore key: %d max readers: %d", 
            M_map_sem.key, M_readersmax);
    
    /* OK, either create or attach... */
    if (EXSUCCEED!=ndrxd_sem_open(&M_map_sem))
    {
        NDRX_LOG(log_error, "Failed to open semaphore for System V queue "
                "map shared mem");
        userlog("Failed to open semaphore for System V queue "
                "map shared mem");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Get position, hash the pathname and find free space if creating
 * or just empty position, if not found..
 * @param pathname Posix queue string 
 * @param[out] pos position found suitable to succeed request
 * @param[out] have_value valid value is found? EXTRUE/EXFALSE.
 * @return EXTRUE -> found position/ EXFALSE - no position found
 */
exprivate int position_get_qstr(char *pathname, int oflag, int *pos, 
        int *have_value)
{
    int ret=EXFALSE;
    int try = ndrx_hash_fn(pathname) % M_queuesmax;
    int start = try;
    int overflow = EXFALSE;
    int interations = 0;
    
    ndrx_svq_map_t *svq = (ndrx_svq_map_t *) M_map_p2s.mem;

    *pos=EXFAIL;
    *have_value = EXFALSE;
    
    NDRX_LOG(log_debug, "Key for [%s] is %d, shm is: %p", 
                                        pathname, try, svq);
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_WASUSED)
            && (!overflow || (overflow && try < start)))
    {
        
        if (0==strcmp(NDRX_SVQ_INDEX(svq, try)->, svc))
        {
            ret=EXTRUE;
            *pos=try;
            
            if (NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED)
            {
                *have_value=EXTRUE;
            }
            
            break;  /* <<< Break! */
        }
        
	if (oflag & O_CREAT)
	{
            if (!(NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED))
            {
                /* found used position */
                break; /* <<< break! */
            }
	}

        try++;
        
        /* we loop over... 
         * Feature #139 mvitolin, 09/05/2017
         * Fix potential overflow issues at the border... of SHM...
         */
        if (try>=M_queuesmax)
        {
            try = 0;
            overflow=EXTRUE;
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", svc);
        }
        interations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, svc);
    }
    
    *pos=try;
    NDRX_LOG(log_debug, "qstr_position_get [%s] - result: %d, "
                            "interations: %d, pos: %d, have_value: %d",
                             pathname, ret, interations, *pos, *have_value);
    return ret;
}


/**
 * Get position queue id record
 * @param qid queue id
 * @param[out] pos position found suitable to succeed request
 * @param[out] have_value valid value is found? EXTRUE/EXFALSE.
 * @return EXTRUE -> found position/ EXFALSE - no position found
 */
exprivate int position_get_qid(int qid, int oflag, int *pos, 
        int *have_value)
{
    int ret=EXFALSE;
    int try = qid % M_queuesmax;
    int start = try;
    int overflow = EXFALSE;
    int interations = 0;
    
    ndrx_svq_map_t *svq = (ndrx_svq_map_t *) M_map_s2p.mem;

    *pos=EXFAIL;
    *have_value = EXFALSE;
    
    NDRX_LOG(log_debug, "Key for [%s] is %d, shm is: %p", 
                                        pathname, try, svq);
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_WASUSED)
            && (!overflow || (overflow && try < start)))
    {
        
        if (NDRX_SVQ_INDEX(svq, try)->qid == qid)
        {
            ret=EXTRUE;
            *pos=try;
            
            if (NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED)
            {
                *have_value=EXTRUE;
            }
            
            break;  /* <<< Break! */
        }
        
	if (oflag & O_CREAT)
	{
            if (!(NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED))
            {
                /* found used position */
                break; /* <<< break! */
            }
	}

        try++;
        
        /* we loop over... 
         * Feature #139 mvitolin, 09/05/2017
         * Fix potential overflow issues at the border... of SHM...
         */
        if (try>=M_queuesmax)
        {
            try = 0;
            overflow=EXTRUE;
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", svc);
        }
        interations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, svc);
    }
    
    *pos=try;
    NDRX_LOG(log_debug, "qstr_position_get [%s] - result: %d, "
                            "interations: %d, pos: %d, have_value: %d",
                             pathname, ret, interations, *pos, *have_value);
    return ret;
}

/**
 * Get queue from shared memory.
 * In case of O_CREAT + O_EXCL return EEXIST error!
 * @param pathname queue path
 * @param oflag are we creating a queue?
 * @return resolve queue id
 */
expublic int ndrx_svqshm_get(char *pathname, int oflag)
{
    int ret = EXFAIL;
    int found;
    int have_value;
    int pos;
    int err = 0;
    
    INIT_ENTRY;
    
    /* Calculate first position of queue lookup */
    
    /* Read lock */
    
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_READ))
    {
        goto out;
    }

    found = position_get_qstr(pathname, oflag, &pos, &have_value);
    if (found)
    {
        ret = NDRX_SVQ_INDEX(svq, try)->qid;
    }

    ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_READ);
    /* ###################### CRITICAL SECTION, END ########################## */

    if (have_value)
    {
        
        if (oflag & (O_CREAT | O_EXCL))
        {
            /* ###################### CRITICAL SECTION, END ########################## */
            ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
            NDRX_LOG(log_error, "Queue [%s] was requested with O_CREAT | O_EXCL, but "
                    "it already exists at position with qid %d", pathname, ret);
            err = EEXIST;
            ret=EXFAIL;
        }
        else
        {
            ret = NDRX_SVQ_INDEX(svq, try)->qid;
            NDRX_LOG(log_error, "Queue [%s] mapped to qid %d", pathname, ret);
        }
        
        /* finish with it */
        goto out;
    }

    /* queue missing, release read lock, get write lock */
    NDRX_LOG(log_info, "[%s] queue not registered or write requested %d - opening...", 
            pathname, oflag);
    
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        goto out;
    }
    
    found = position_get_qstr(pathname, oflag, &pos, &have_value);
    
    /* while we were locked, somebody may added such queue already...! */
    if (have_value)
    {
        ret = NDRX_SVQ_INDEX(M_map_p2s.mem, pos)->qid;
        
        if (oflag & (O_CREAT | O_EXCL))
        {
            /* ###################### CRITICAL SECTION, END ########################## */
            ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
            NDRX_LOG(log_error, "Queue [%s] was requested with O_CREAT | O_EXCL, but "
                    "it already exists at position with qid %d", pathname, ret);
            err = EEXIST;
            ret=EXFAIL;
        }
        else
        {
  
            ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
            /* ###################### CRITICAL SECTION, END ########################## */
            
            NDRX_LOG(log_error, "Queue [%s] mapped to qid %d, 
                    pathname, ret);
        }
        /* finish with it */
        goto out;
    }
    
    /* open queue, install mappings in both tables */
    
    /* extract only known flags.. */
    if (EXFAIL==(ret = msgget(IPC_PRIVATE, ( (oflag & O_CREAT) | (oflag & O_EXCL)))))
    {
        int err = errno;
        
        ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
        /* ###################### CRITICAL SECTION, END ########################## */
        
        NDRX_LOG(log_error, "Failed msgget: %s for [%s]", strerror(err), pathname);
        userlog("Failed msgget: %s for [%s]", strerror(err), pathname);
    }
    
    /* write handlers off */
    NDRX_SVQ_INDEX(M_map_p2s.mem, pos)->qid = ret;
    NDRX_STRCPY_SAFE( (NDRX_SVQ_INDEX(M_map_p2s.mem, pos)->qstr), pathname);
    NDRX_SVQ_INDEX(M_map_p2s.mem, pos)->status = NDRX_SVQ_MAP_ISUSED | NDRX_SVQ_MAP_WASUSED;
    
    /* now locate the pid to string mapping... */
    
    ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
    /* ###################### CRITICAL SECTION, END ########################## */
    
    NDRX_LOG(log_debug, "Open queue: [%s] to system v: [%d]", pathname, ret);
    
    
    
out:
    
    errno = err;
    return ret;
}

