/**
 * @brief Shared memory for System V queues
 *   we need following ops here:
 *   - lookup for queue id, if specified create, then create on
 *   -
 *
 * @file sys_svqshm.c
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
#define INIT_ENTRY(X) \
if (!ndrx_G_svqshm_init) \
    {\
        MUTEX_LOCK_V(ndrx_G_svqshm_init_lock);\
        \
        if (!ndrx_G_svqshm_init)\
        {\
            X = ndrx_svqshm_init(EXFALSE);\
        }\
        \
        MUTEX_UNLOCK_V(ndrx_G_svqshm_init_lock);\
        \
        if (EXFAIL==X)\
        {\
            NDRX_LOG(log_error, "Failed to create/attach System V Queues "\
                    "mapping shared memory blocks!");\
            EXFAIL_OUT(X);\
        }\
    }

#define MAX_READERS_DFLT        50      /**< Max readers for RW lock... */
#define MAX_QUEUES_DLFT         20000   /**< Max number of queues, dflt */
/**
 * Default time for queue to live after removal.
 * After this time in seconds, ndrxd will zap the queue. This is needed
 * for reason so that if any process in background re-opens the service on
 * some request address and yet are not reported (in progress) the list
 * of the services, the ctime is changed of the ipc q, and thus ndrxd will
 * not attempt to remove it. After that ndrxd receives the message and
 * now queue will not be removed, because of its presence in service lists
 */
#define RM_TTL_DLFT             10

#define SHM_ENT_NONE            0       /**< Entry not used */
#define SHM_ENT_MATCH           1       /**< Entry matched  */
#define SHM_ENT_OLD             2       /**< Entry was used */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

expublic volatile int ndrx_G_svqshm_init = EXFALSE;
/* have some lock for this argument */
MUTEX_LOCKDECL(ndrx_G_svqshm_init_lock);

exprivate ndrx_shm_t M_map_p2s = {.fd=0, .path=""};   /**< Posix to System V mapping       */
exprivate ndrx_shm_t M_map_s2p = {.fd=0, .path=""};   /**< System V to Posix mapping       */
exprivate ndrx_sem_t M_map_sem = {.semid=0};/**< RW semaphore for SHM protection */

/* Also we need some array of semaphores for RW locking */

exprivate char *M_qprefix = NULL;       /**< Queue prefix used by mappings  */
exprivate long M_queuesmax = 0;         /**< Max number of queues           */
exprivate int  M_readersmax = 0;        /**< Max number of concurrent lckrds*/
exprivate key_t M_sem_key = 0;          /**< Semphoare key                  */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Remove shared mem resources
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqshm_down(int force)
{
    int ret = EXSUCCEED;
    int i;
    ndrx_svq_map_t *svq, *el;
    /* have some init first, so that we attach existing resources
     * before killing them
     */
    INIT_ENTRY(ret);
    
    /* Terminate polling threads... if any... */
    ndrx_atfork_prepare();
    
    if (force)
    {
        /* get write locks... */
        svq = (ndrx_svq_map_t *) M_map_p2s.mem;

        /* remove any queues left open...! */
        for (i=0; i<M_queuesmax; i++)
        {
            el = NDRX_SVQ_INDEX(svq, i);

            if (el->flags & NDRX_SVQ_MAP_ISUSED)
            {
                NDRX_LOG(log_error, "DOWN: Removing QID %d (%s) - should not be present!", 
                        el->qid, el->qstr);
                userlog("DOWN: Removing QID %d (%s) - should not be present!", 
                        el->qid, el->qstr);
                if (EXSUCCEED!=msgctl(el->qid, IPC_RMID, NULL))
                {
                    int err = errno;
                    NDRX_LOG(log_error, "got error when removing %d: %s - ignore", 
                         el->qid, strerror(err));
                    userlog("got error when removing %d: %s - ignore", 
                         el->qid, strerror(err));
                }
            }
        } /* for */
    } /* force */
     
    if (EXSUCCEED!=ndrx_shm_close(&M_map_p2s))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_shm_remove(&M_map_p2s))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_shm_close(&M_map_s2p))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_shm_remove(&M_map_s2p))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_sem_close(&M_map_sem))
    {
        ret = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_sem_remove(&M_map_sem, EXTRUE))
    {
        ret = EXFAIL;
    }
    
    ndrx_G_svqshm_init = EXFALSE;
    
out:
    return ret;
}

/**
 * Initialize the shared memory blocks.
 * This assumes that basic environment is loaded. Because we need access to
 * max queues setting.
 * @param[in] attach_only attach only, do not output resources and do not start
 *  AUX thread
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqshm_init(int attach_only)
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

    /* get number of concurrent threads */
    tmp = getenv(CONF_NDRX_SVQREADERSMAX);
    if (NULL==tmp)
    {
        M_readersmax = MAX_READERS_DFLT;
        NDRX_LOG(log_error, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_SVQREADERSMAX, M_readersmax);
    }
    else
    {
        M_readersmax = atol(tmp);
    }
    
    /* get queues max */
    tmp = getenv(CONF_NDRX_MSGQUEUESMAX);
    if (NULL==tmp)
    {
        M_queuesmax = MAX_QUEUES_DLFT;
        NDRX_LOG(log_error, "Missing config key %s - defaulting to %d", 
                CONF_NDRX_MSGQUEUESMAX, M_queuesmax);
    }
    else
    {
        M_queuesmax = atol(tmp);
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
    
    
    if (attach_only)
    {
        if (EXSUCCEED!=ndrx_shm_attach(&M_map_p2s))
        {
            NDRX_LOG(log_error, "Failed to oattach shm [%s] - System V Queues cannot work",
                        M_map_p2s.path);
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_shm_open(&M_map_p2s, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open shm [%s] - System V Queues cannot work",
                    M_map_p2s.path);
        EXFAIL_OUT(ret);
    }
    
    if (attach_only)
    {
        if (EXSUCCEED!=ndrx_shm_attach(&M_map_s2p))
        {
            NDRX_LOG(log_error, "Failed to attach shm [%s] - System V Queues cannot work",
                        M_map_s2p.path);
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_shm_open(&M_map_s2p, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open shm [%s] - System V Queues cannot work",
                    M_map_s2p.path);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "M_map_p2s.mem=%p M_map_s2p=%p", 
            M_map_p2s.mem, M_map_s2p.mem);
    
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
    if (attach_only)
    {
        if (EXSUCCEED!=ndrx_sem_attach(&M_map_sem))
        {
            NDRX_LOG(log_error, "Failed to attach semaphore for System V queue "
                    "map shared mem");
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_sem_open(&M_map_sem, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open semaphore for System V queue "
                "map shared mem");
        userlog("Failed to open semaphore for System V queue "
                "map shared mem");
        EXFAIL_OUT(ret);
    }
    
    /* init the support thread */
    if (!attach_only && EXSUCCEED!=ndrx_svq_event_init())
    {
        NDRX_LOG(log_error, "Failed to init System V queue monitoring thread!");
        userlog("Failed to init System V queue monitoring thread!");
        EXFAIL_OUT(ret);
    }
            
    ndrx_G_svqshm_init = EXTRUE;
out:
    return ret;
}

/**
 * This attempts to attach to System V resources, does not spawn AUX threads.
 * If you plan to run some other queue ops, then needs to close the shm
 * and let standard init to step in
 * @return EXFAIL - failed, EXSUCCEED - attached to already existing resources
 *  EXTRUE - attached only.
 */
expublic int ndrx_svqshm_attach(void)
{
    int ret = EXSUCCEED;
    
    if (ndrx_G_svqshm_init)
    {
        NDRX_LOG(log_debug, "Already System V resources open");
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_svqshm_init(EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to attach to System V resources");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Attached to shm/sem OK");
    
    ret = EXTRUE;
    
out:
    return ret;
}

/**
 * Detach (with attach only was done) from System V resources
 */
expublic void ndrx_svqshm_detach(void)
{
    ndrx_shm_close(&M_map_p2s);
    ndrx_shm_close(&M_map_s2p);
    ndrx_sem_close(&M_map_sem);
    
    ndrx_G_svqshm_init = EXFALSE;
}

/**
 * Get System V shared resources
 * @param map_p2s Posix to System V mapping table
 * @param map_s2p System V to Posix mapping table
 * @param map_sem Semaphore locks
 * @return EXFALSE - not attached to SHM, EXTRUE - attached to shm
 */
expublic int ndrx_svqshm_shmres_get(ndrx_shm_t **map_p2s, ndrx_shm_t **map_s2p, 
        ndrx_sem_t **map_sem, int *queuesmax)
{

    *map_p2s = &M_map_p2s;
    *map_s2p = &M_map_s2p;
    *map_sem = &M_map_sem;
    *queuesmax = M_queuesmax;
    
    return ndrx_G_svqshm_init;
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
    int ret=SHM_ENT_NONE;
    int try = EXFAIL;
    int start = try;
    int overflow = EXFALSE;
    int iterations = 0;
    ndrx_svq_map_t *svq = (ndrx_svq_map_t *) M_map_p2s.mem;
    
    /*
     * 20/10/2018 Got to loop twice!!!! 
     * Some definitions:
     * A - our Q
     * B - other Q
     * 
     * The problem:
     * 1) B gets installed in our cell / direct hash number
     * 2) A gets installed in cell+1 index
     * 3) B gets uninstalled / cell becomes as stale
     * 4) A tries installed Q again, and it hits the cell and not the cell+1
     * 
     * ----
     * Thus to solve this, we got to firstly perform "read only" lookup on the
     * queue tables to see, is queue already present there or not. And if not,
     * only then perform write to maps..
     * 
     * !!!! Needs to check with the _ndrx_shm_get_svc() wouldn't be there the
     * same problem !!!!
     * 
     */
    if (oflag & O_CREAT)
    {
        int try_read;
        int read_have_value;
        
        if (position_get_qstr(pathname, 0, &try_read, &read_have_value)
                && read_have_value)
        {
            try = try_read;
        }
    }
    
    if (EXFAIL==try)
    {
        try = ndrx_hash_fn(pathname) % M_queuesmax;
    }
    else
    {
        NDRX_LOG(log_debug, "Got existing record at %d", try);
    }
    
    *pos=EXFAIL;
    
    NDRX_LOG(log_debug, "Try key for [%s] is %d, shm is: %p oflag: %d", 
                                        pathname, try, svq, oflag);
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_WASUSED)
            && (!overflow || (overflow && try < start)))
    {
        if (0==strcmp(NDRX_SVQ_INDEX(svq, try)->qstr, pathname))
        {
            *pos=try;
            
            if (NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED)
            {
                ret=SHM_ENT_MATCH;
            }
            else
            {
                ret=SHM_ENT_OLD;
            }
            
            break;  /* <<< Break! */
        }
        
	if (oflag & O_CREAT)
	{
            if (!(NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED))
            {
                /* found used position */
                ret=SHM_ENT_OLD;
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
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", pathname);
        }
        iterations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, pathname);
    }
    
    *pos=try;
    
    switch (ret)
    {
        case SHM_ENT_OLD:
            *have_value = EXFALSE;
            ret = EXTRUE;   /* have position */
            break;
        case SHM_ENT_NONE:
            
            if (overflow)
            {
                *have_value = EXFALSE;
                ret = EXFALSE;   /* no position */
            }
            else
            {
                *have_value = EXFALSE;
                ret = EXTRUE;   /* have position */
            }
            
            break;
        case SHM_ENT_MATCH:
            *have_value = EXTRUE;
            ret = EXTRUE;   /* have position */
            break;
        default:
            
            NDRX_LOG(log_error, "!!! should not get here...");
            break;
    }
    
    NDRX_LOG(log_debug, "qstr_position_get [%s] - result: %d, "
                "iterations: %d, pos: %d, have_value: %d flags: %hd [%s]",
                pathname, ret, iterations, *pos, *have_value, 
                NDRX_SVQ_INDEX(svq, try)->flags, NDRX_SVQ_INDEX(svq, try)->qstr);
    
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
    int ret=SHM_ENT_NONE;
    int try = EXFAIL;
    int start = try;
    int overflow = EXFALSE;
    int iterations = 0;
    ndrx_svq_map_t *svq = (ndrx_svq_map_t *) M_map_s2p.mem;
    
    /*
     * 20/10/2018 Got to loop twice!!!! 
     * Some definitions:
     * A - our Q
     * B - other Q
     * 
     * The problem:
     * 1) B gets installed in our cell / direct hash number
     * 2) A gets installed in cell+1 index
     * 3) B gets uninstalled / cell becomes as stale
     * 4) A tries installed Q again, and it hits the cell and not the cell+1
     * 
     * ----
     * Thus to solve this, we got to firstly perform "read only" lookup on the
     * queue tables to see, is queue already present there or not. And if not,
     * only then perform write to maps..
     * 
     * !!!! Needs to check with the _ndrx_shm_get_svc() wouldn't be there the
     * same problem !!!!
     * 
     */
    if (oflag & O_CREAT)
    {
        int try_read;
        int read_have_value;
        
        if (position_get_qid(qid, 0, &try_read, &read_have_value)
                && read_have_value)
        {
            try = try_read;
        }
    }
    
    if (EXFAIL==try)
    {
        try = qid % M_queuesmax;
    }
    else
    {
        NDRX_LOG(log_debug, "Got existing record at %d", try);
    }

    *pos=EXFAIL;
    *have_value = EXFALSE;
    
    NDRX_LOG(log_debug, "Try key qid [%d] is %d, shm is: %p oflag: %d", 
                                        qid, try, svq, oflag);
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
            *pos=try;
            
            if (NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED)
            {
                ret=SHM_ENT_MATCH;
            }
            else
            {
                ret=SHM_ENT_OLD;
            }
            
            ret=EXTRUE;
            break;  /* <<< Break! */
        }
        
	if (oflag & O_CREAT)
	{
            if (!(NDRX_SVQ_INDEX(svq, try)->flags & NDRX_SVQ_MAP_ISUSED))
            {
                ret=SHM_ENT_OLD;
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
            NDRX_LOG(log_debug, "Overflow reached for search of [%d]", qid);
        }
        iterations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%d]", try, qid);
    }
    switch (ret)
    {
        case SHM_ENT_OLD:
            *have_value = EXFALSE;
            ret = EXTRUE;   /* have position */
            break;
        case SHM_ENT_NONE:
            
            if (overflow)
            {
                *have_value = EXFALSE;
                ret = EXFALSE;   /* no position */
            }
            else
            {
                *have_value = EXFALSE;
                ret = EXTRUE;   /* have position */
            }
            
            break;
        case SHM_ENT_MATCH:
            *have_value = EXTRUE;
            ret = EXTRUE;   /* have position */
            break;
        default:
            
            NDRX_LOG(log_error, "!!! should not get here...");
            break;
    }
    
    *pos=try;
    NDRX_LOG(log_debug, "[%d] - result: %d, "
                        "iterations: %d, pos: %d, have_value: %d",
                         qid, ret, iterations, *pos, *have_value);
    return ret;
}


/**
 * Get position queue id record
 * @param status status memory block
 * @param qid queue id
 * @param[out] pos position found suitable to succeed request
 * @param[out] have_value valid value is found? EXTRUE/EXFALSE.
 * @return EXTRUE -> found position/ EXFALSE - no position found
 */
expublic int ndrx_svqshm_get_status(ndrx_svq_status_t *status, 
        int qid, int *pos, int *have_value)
{
    int ret=SHM_ENT_NONE;
    int try = qid % M_queuesmax;
    int start = try;
    int overflow = EXFALSE;
    int iterations = 0;
    ndrx_svq_status_t *el;

    *pos=EXFAIL;
    *have_value = EXFALSE;
    
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while ((NDRX_SVQ_STATIDX(status, try)->flags & NDRX_SVQ_MAP_WASUSED)
            && (!overflow || (overflow && try < start)))
    {
        
        el = NDRX_SVQ_STATIDX(status, try);
                
        if (el->qid == qid)
        {
            *pos=try;
            
            if (el->flags & NDRX_SVQ_MAP_ISUSED)
            {
                ret=SHM_ENT_MATCH;
            }
            else
            {
                ret=SHM_ENT_OLD;
            }
            
            ret=EXTRUE;
            break;  /* <<< Break! */
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
            NDRX_LOG(log_debug, "Overflow reached for search of [%d]", qid);
        }
        iterations++;
        
        NDRX_LOG(log_dump, "Trying %d for [%s]", try, qid);
    }
    switch (ret)
    {
        case SHM_ENT_OLD:
            *have_value = EXFALSE;
            ret = EXTRUE;   /* have position */
            break;
        case SHM_ENT_NONE:
            
            if (overflow)
            {
                *have_value = EXFALSE;
                ret = EXFALSE;   /* no position */
            }
            else
            {
                *have_value = EXFALSE;
                ret = EXTRUE;   /* have position */
            }
            
            break;
        case SHM_ENT_MATCH:
            *have_value = EXTRUE;
            ret = EXTRUE;   /* have position */
            break;
        default:
            
            NDRX_LOG(log_error, "!!! should not get here...");
            break;
    }
    
    *pos=try;
    NDRX_LOG(log_debug, "[%d] - result: %d, "
                        "iterations: %d, pos: %d, have_value: %d",
                         qid, ret, iterations, *pos, *have_value);
    return ret;
}

/**
 * Lookup qid
 * @param in_qid System V queue id
 * @param out_qstr output string
 * @param out_qstr_len output string length 
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_svqshm_get_qid(int in_qid, char *out_qstr, int out_qstr_len)
{
    int ret = EXSUCCEED;
    int found_2;
    int have_value_2;
    int pos_2;
    ndrx_svq_map_t *svq2;
    ndrx_svq_map_t *sm;      /* System V map        */

    INIT_ENTRY(ret);
    
    svq2 = (ndrx_svq_map_t *) M_map_s2p.mem;

    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_READ))
    {
        goto out;
    }

    found_2 = position_get_qid(in_qid, 0, &pos_2, &have_value_2);
    
    if (have_value_2)
    {
        sm = NDRX_SVQ_INDEX(svq2, pos_2);
        NDRX_STRNCPY_SAFE(out_qstr, sm->qstr, out_qstr_len);
    }
    else
    {
        ret=EXFAIL;
    }
    
    ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_READ);
    /* ###################### CRITICAL SECTION, END ########################## */
    
out:
            
    return ret;
}

/**
 * Get queue from shared memory.
 * In case of O_CREAT + O_EXCL return EEXIST error!
 * @param qstr queue path
 * @param mode access mode/permissions
 * @param oflag are we creating a queue?
 * @param remove should we actually remove the queue?
 * @return resolve queue id
 */
expublic int ndrx_svqshm_get(char *qstr, mode_t mode, int oflag)
{
    int ret = EXSUCCEED;
    int qid;
    int found;
    int have_value;
    int pos;
    
    int found_2;
    int have_value_2;
    int pos_2;
    
    int err = 0;
    
    ndrx_svq_map_t *svq;
    ndrx_svq_map_t *svq2;
    
    ndrx_svq_map_t *pm;      /* Posix map           */
    ndrx_svq_map_t *sm;      /* System V map        */
    
    INIT_ENTRY(ret);
    
    svq = (ndrx_svq_map_t *) M_map_p2s.mem;
    svq2 = (ndrx_svq_map_t *) M_map_s2p.mem;
    
    /* Calculate first position of queue lookup */
    
    /* Read lock, only if attempting to open existing queue
     * For creating queues, we need to count the actual number
     * of creates called. So that we could sync with ndrxd the zapping of Queues.
     */
    
    if (!(oflag & O_CREAT))
    {
        /* ###################### CRITICAL SECTION ############################### */
        if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_READ))
        {
            goto out;
        }

        found = position_get_qstr(qstr, oflag, &pos, &have_value);
        
        if (have_value)
        {
            pm = NDRX_SVQ_INDEX(svq, pos);
            qid = pm->qid;
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
                        "it already exists at position with qid %d", qstr, ret);
                err = EEXIST;
                EXFAIL_OUT(ret);
            }
            else
            {
                NDRX_LOG(log_debug, "Queue [%s] mapped to qid %d", qstr, qid);
            }

            /* finish with it */
            goto out;
        }
        else
        {
            /* queue not found and we are in read mode... */
            NDRX_LOG(log_debug, "Queue not found for: [%s]", qstr);
            err = ENOENT;
            EXFAIL_OUT(ret);
        }
    }
    
    /* queue missing, release read lock, get write lock */
    NDRX_LOG(log_info, "[%s] queue not registered or write requested %d - Writ try...", 
            qstr, oflag);
    
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        goto out;
    }
    
    found = position_get_qstr(qstr, oflag, &pos, &have_value);
    
    /* check that we have found! */
    
    if (!found)
    {
        NDRX_LOG(log_error, "Location not found for [%s] - memory full?", qstr);
        userlog("Location not found for [%s] - memory full?", qstr);
        err = ENOMEM;
        EXFAIL_OUT(ret);
    }
    
    pm = NDRX_SVQ_INDEX(svq, pos);
    
    /* while we were locked, somebody may added such queue already...! */

    if (have_value)
    {
        qid = pm->qid;
        
        if ( (oflag & O_CREAT)  &&  (oflag & O_EXCL))
        {
            /* ###################### CRITICAL SECTION, END ########################## */
            ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
            NDRX_LOG(log_error, "Queue [%s] was requested with O_CREAT | O_EXCL, but "
                    "it already exists at position with qid %d", qstr, qid);
            err = EEXIST;
            EXFAIL_OUT(ret);
        }
        else
        {
            /* update the usage statistics */
            
            /* Lookup the second table... so that we can update
             * usage statistics
             */
            qid = pm->qid;
            
            found_2 = position_get_qid(qid, oflag, &pos_2, &have_value_2);
            sm = NDRX_SVQ_INDEX(svq2, pos_2);
            
            ndrx_stopwatch_reset(&(pm->ctime));
            sm->ctime = pm->ctime;
            
            ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
            /* ###################### CRITICAL SECTION, END ########################## */
            
            NDRX_LOG(log_info, "Queue [%s] mapped to qid %d", 
                    qstr, qid);
        }
        /* finish with it */
        goto out;
    }
    
    /* open queue, install mappings in both tables */
    
    /* extract only known flags.. */
    if (EXFAIL==(qid = msgget(IPC_PRIVATE, mode | IPC_CREAT | 
            ( (oflag & O_CREAT) | (oflag & O_EXCL)))))
    {
        int err = errno;
        ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
        /* ###################### CRITICAL SECTION, END ########################## */
        
        NDRX_LOG(log_error, "Failed msgget: %s for [%s]", strerror(err), qstr);
        userlog("Failed msgget: %s for [%s]", strerror(err), qstr);
        EXFAIL_OUT(ret);
    }
    
    /* write handlers off */
    pm->qid = qid;
    NDRX_STRCPY_SAFE(pm->qstr, qstr);
    pm->flags = (NDRX_SVQ_MAP_ISUSED | NDRX_SVQ_MAP_WASUSED);
    
    /* now locate the pid to string mapping... 
     * lookup the second table by new qid/ret
     */
    /* Lookup the second table... so that we can update
     * usage statistics
     */
    found_2 = position_get_qid(qid, oflag, &pos_2, &have_value_2);
    
    if (!found_2)
    {
        NDRX_LOG(log_error, "Location not found for qid [%d] - memory full?", qid);
        userlog("Location not found for qid [%d] - memory full?", qid);
        err = ENOMEM;
        EXFAIL_OUT(ret);
    }
    
    sm = NDRX_SVQ_INDEX(svq2, pos_2);
    
    sm->qid = qid;
    NDRX_STRCPY_SAFE( sm->qstr, qstr);
    sm->flags = (NDRX_SVQ_MAP_ISUSED | NDRX_SVQ_MAP_WASUSED);
    
    /* reset last create time... */
    ndrx_stopwatch_reset(&(pm->ctime));
    sm->ctime = pm->ctime;
    
    ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
    /* ###################### CRITICAL SECTION, END ########################## */
    
    NDRX_LOG(log_debug, "Open queue: [%s] to system v: [%d]", qstr, qid);
    
out:
    
    if (EXSUCCEED!=ret)
    {
        errno = err;
        return EXFAIL;
    }
    else
    {
        return qid;
    }
}

/**
 * Perform control operations over the shared memory / queue maps.
 * !!! THIS SHALL BE USED BY NDRXD ONLY FOR SERVICE QUEUES !!!
 * @param qstr conditional queue string, depending from which side we do
 *  lookup. Either \p qstr or \p qid must be present
 * @param qid queue ID, conditional, if not set then EXFAIL.
 * @param cmd currently only IPC_RMID operation is supported. It is expected
 *  that IPC_RMID will be called only by NDRXD when it expects that queue
 *  needs to be unlinked. But at which point? 
 *  Probably this way:
 *      1) ndrxd locks the svqshm, svc shm, br shm
 *      a)   Note that during this time some server is booting and might 
 *      b)   have grabbed the queued id, thus also increased the counter
 *      2) ndrxd scans the service lists. Find that particular qid is not used
 *      3) then it shall be removed from svqshm, but there is problem,
 *          as the a) have created the Q, and not yet reported to NDRXD,
 *          the operation shall be delayed. Thus we need a last creation
 *          timestamp. So that ndrxd removes un-used queues only after some
 *          time of not "created".
 *          for this scenario we will pass "arg1" which will give number of
 *          seconds to be exceeded for unlink.
 *          if unlink is needed immediately, then use -1.
 * @param[in] p_deletecb callback to delete event function. I.e. if not null
 *  it is called when attempt to delete the queue is performed, so that
 *  it would be possible to flush some messages.
 * @return EXFAIL, or Op related value. For example for IPC_RMID it will
 *  return number of instances left for the queue servers.
 */
expublic int ndrx_svqshm_ctl(char *qstr, int qid, int cmd, int arg1,
        int (*p_deletecb)(int qid, char *qstr))
{
    int ret = EXSUCCEED;
    
    int delta;
    
    int found;
    int have_value;
    int pos;
    
    int found_2;
    int have_value_2;
    int pos_2;
    
    int err = 0;
    int is_locked = EXFALSE;
    
    ndrx_svq_map_t *svq;
    ndrx_svq_map_t *svq2;
  
    
    ndrx_svq_map_t *pm;      /* Posix map           */
    ndrx_svq_map_t *sm;      /* System V map        */
    
    INIT_ENTRY(ret);
  
    svq = (ndrx_svq_map_t *) M_map_p2s.mem;
    svq2 = (ndrx_svq_map_t *) M_map_s2p.mem;
    
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        goto out;
    }
    
    is_locked = EXTRUE;
    
    if (qstr)
    {
        found = position_get_qstr(qstr, 0, &pos, &have_value);
        
        if (have_value)
        {
            pm = NDRX_SVQ_INDEX(svq, pos);
            
            found_2 = position_get_qid(pm->qid, 0, 
                    &pos_2, &have_value_2);
            
            if (!have_value_2)
            {
                NDRX_LOG(log_error, "qstr [%s] map not in sync with qid %d "
                        "map (have_value_2 is false)",
                        qstr, pm->qid);
                userlog("qstr [%s] map not in sync with qid %d "
                        "map (have_value_2 is false)",
                        qstr, pm->qid);
                err = EBADF;
                EXFAIL_OUT(ret);
            }
            
            sm = NDRX_SVQ_INDEX(svq2, pos_2);
        }
        
    }
    else if (EXFAIL!=qid)
    {
        found_2 = position_get_qid(qid, 0, 
                    &pos_2, &have_value_2);
        
        if (have_value_2)
        {
            sm = NDRX_SVQ_INDEX(svq2, pos_2);
            
            found = position_get_qstr(sm->qstr, 0, &pos, &have_value);
            
            if (!have_value)
            {
                NDRX_LOG(log_error, "qstr [%s] map not in sync with qid %d "
                        "map (have_value is false)",
                        sm->qstr, qstr, qid);
                
                userlog("qstr [%s] map not in sync with qid %d "
                        "map (have_value is false)",
                        sm->qstr, qstr, qid);
                err = EBADF;
                EXFAIL_OUT(ret);
            }
            
            pm = NDRX_SVQ_INDEX(svq, pos);
            
        }
    }
    else
    {
        NDRX_LOG(log_error, "qstr and qid are invalid => FAIL");
        err = EINVAL;
        EXFAIL_OUT(ret);
    }
        
    switch (cmd)
    {
        /* set the flags for the queue */
        case IPC_SET:
            
            NDRX_LOG(log_debug, "Adding flags %hd to qstr [%s]/qid %d",
                    (short)arg1, pm->qstr, pm->qid);
            pm->flags |= (short)arg1;
            sm->flags |= (short)arg1;
            
            break;
        case IPC_RMID:
            
            if (!have_value)
            {
                NDRX_LOG(log_error, "Queue not found [%s]/%d", 
                        qstr?qstr:"NULL", qid);
                err = ENOENT;
                EXFAIL_OUT(ret);
            }
            
            if (EXFAIL!=arg1)
            {
                delta = ndrx_stopwatch_get_delta_sec( &(pm->ctime));
            }
            
            if ( EXFAIL==arg1 || delta > arg1)
            {
                NDRX_LOG(log_info, "Unlinking queue: [%s]/%d (delta: %d, limit: %d)",
                        pm->qstr, pm->qid, delta, arg1);
                
                /* unlink the q and remove entries from shm */
                
                if (NULL!=p_deletecb && EXSUCCEED!=p_deletecb(pm->qid, pm->qstr))
                {
                    NDRX_LOG(log_error, "Delete callback failed for [%s]/%d",
                            pm->qstr, pm->qid);
                    EXFAIL_OUT(ret);
                }
                
                if (EXSUCCEED!=msgctl(pm->qid, IPC_RMID, NULL))
                {
                    err = errno;
                    
                    if (EIDRM!=err && EINVAL!=err)
                    {
                        NDRX_LOG(log_error, "got error when removing %d: %s", 
                            pm->qid, strerror(err));
                        userlog("got error when removing %d: %s", 
                            pm->qid, strerror(err));
                        EXFAIL_OUT(ret);
                    }
                    else
                    {
                        NDRX_LOG(log_warn, "got error when removing %d: %s - ignore", 
                            pm->qid, strerror(err));
                    }
                }
                    
                NDRX_LOG(log_debug, "Removing ISUSED flag for P2S/S2P mem");

                pm->flags &= ~(NDRX_SVQ_MAP_ISUSED);
                sm->flags &= ~(NDRX_SVQ_MAP_ISUSED);
            }
            
            break;
        
        default:
            
            NDRX_LOG(log_error, "Unsupported command: %d", cmd);
            err=EINVAL;
            EXFAIL_OUT(ret);
            break;
    }
    
    
out:
    
    if (is_locked)
    {
        ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_WRITE);
        /* ###################### CRITICAL SECTION, END ########################## */
    }
    
    errno = err;
    return ret;
}


/**
 * List queue open in the System V IPC.
 * So what we do here basically is get read lock SystemV SHM tables
 * and return the results.
 * @param qpath queue path, not used actually
 * @param[out] return_status EXSUCCEED/EXFAIL
 * @return NULL no queues found (or error) or ptr to queues
 */
expublic string_list_t* ndrx_sys_mqueue_list_make_svq(char *qpath, int *return_status)
{
    int retstat = EXSUCCEED;
    string_list_t* ret = NULL;
    int have_lock = EXFALSE;
    int i=0;
    ndrx_svq_map_t *svq;
    ndrx_svq_map_t *pm;      /* Posix map           */
    
    INIT_ENTRY(retstat);
    
    svq = (ndrx_svq_map_t *) M_map_p2s.mem;
    
    *return_status = EXSUCCEED;
            
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_READ))
    {
        goto out;
    }
    
    have_lock = EXTRUE;
    
    for (i=0;i<M_queuesmax;i++)
    {
        pm = NDRX_SVQ_INDEX(svq, i);
        
        if (pm->flags & NDRX_SVQ_MAP_ISUSED)
        {
            if (EXSUCCEED!=ndrx_string_list_add(&ret, pm->qstr))
            {
                NDRX_LOG(log_error, "failed to add string to list [%s]", 
                        pm->qstr);
                *return_status = EXFAIL;
                goto out;
            }
        }
    }
    
out:
    if (have_lock)
    {
        ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_READ);
        /* ###################### CRITICAL SECTION, END ########################## */
    }
    
    return ret;
}

/**
 * Remove queue status in newly allocated memory block
 * @param[out] len number elements in allocated block
 * @param[in] number of seconds to leave record in map until removal
 * @return ptr to alloc block or NULL in case of error
 */
expublic ndrx_svq_status_t* ndrx_svqshm_statusget(int *len, int ttl)
{
    int ret = EXSUCCEED;
    ndrx_svq_status_t* block = NULL;
    int sz = sizeof(ndrx_svq_status_t) * M_queuesmax;
    int err;
    int have_lock = EXFALSE;
    int i=0;
    ndrx_svq_map_t *svq = (ndrx_svq_map_t *) M_map_s2p.mem;
    ndrx_svq_map_t *pm;      /* Posix map           */
    
    block = NDRX_MALLOC(sz);
    
    if (NULL==block)
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", sz, strerror(err));
        userlog("Failed to malloc %d bytes: %s", sz, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    *len = M_queuesmax;
    
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_map_sem, 0, NDRX_SEM_TYP_READ))
    {
        EXFAIL_OUT(ret);
    }
    
    have_lock = EXTRUE;
    
    for (i=0;i<M_queuesmax;i++)
    {
        pm = NDRX_SVQ_INDEX(svq, i);
        block[i].flags = pm->flags;
        block[i].qid = pm->qid;
        
        if (block[i].flags & NDRX_SVQ_MAP_ISUSED &&
                ndrx_stopwatch_get_delta_sec( &(pm->ctime)) > ttl)
        {
            block[i].flags |= NDRX_SVQ_MAP_SCHEDRM;
        }
        
        NDRX_STRCPY_SAFE(block[i].qstr, pm->qstr);
    }
    
out:
            
    if (have_lock)
    {
        ndrx_sem_rwunlock(&M_map_sem, 0, NDRX_SEM_TYP_READ);
        /* ###################### CRITICAL SECTION, END ########################## */
    }

    return block;
}

/* vim: set ts=4 sw=4 et smartindent: */
