/**
 * @brief Semaphore handling, ATMI level
 *
 * @file sem.c
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

#include <nstd_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_sem_t G_sem_svcop;              /* Service operations       */
exprivate int M_init = EXFALSE;                 /* no init yet done         */

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int ndrx_sem_lock(ndrx_sem_t *sem, const char *msg, int sem_num);
extern int ndrx_sem_unlock(ndrx_sem_t *sem, const char *msg, int sem_num);

/**
 * Initialise prefix part, that is needed for shm...
 * @param ndrx_prefix
 * @return 
 */
expublic int ndrxd_sem_init(char *q_prefix)
{
    memset(&G_sem_svcop, 0, sizeof(G_sem_svcop));
    
    /* Service queue ops */
    G_sem_svcop.key = G_atmi_env.ipckey + NDRX_SEM_SVC_OPS;
    G_sem_svcop.nrsems = G_atmi_env.nrsems;
    NDRX_LOG(log_debug, "Using service semaphore key: [%d]", 
            G_sem_svcop.key);
    
    M_init = EXTRUE;
    return EXSUCCEED;
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
 * Open semaphore
 * @return
 */
expublic int ndrx_sem_open_all(void)
{
    int ret=EXSUCCEED;

    if (EXSUCCEED!=ndrxd_sem_open(&G_sem_svcop, EXTRUE))
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
 * Does delete all semaphore blocks.
 */
expublic int ndrxd_sem_delete(void)
{
    if (M_init)
    {
        ndrx_sem_remove(&G_sem_svcop, EXFALSE);
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
    
    if (EXSUCCEED==ndrxd_sem_open(&G_sem_svcop, EXTRUE))
    {
        ndrx_sem_remove(&G_sem_svcop, EXTRUE);
    }
}

/**
 * Attach to semaphore block.
 * @lev indicates the attach level (should it be service array only)?
 * @return 
 */
expublic int ndrx_sem_attach_all(void)
{
    int ret=EXSUCCEED;
   
    /**
     * Library not initialised
     */
    if (!M_init)
    {
        NDRX_LOG(log_error, "ndrx shm/sem library not initialised!");
        ret=EXFAIL;
        goto out;
    }
    
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
    return ndrx_sem_lock(&G_sem_svcop, msg, NDRX_SEM_SVC_GLOBAL_NUM);
}

/**
 * Unlock service operation
 * @return 
 */
expublic int ndrx_unlock_svc_op(const char *msg)
{
    return ndrx_sem_unlock(&G_sem_svcop, msg, NDRX_SEM_SVC_GLOBAL_NUM);
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
    
    return ndrx_sem_lock(&G_sem_svcop, svcnm, semnum);
    
#else 
    
    return ndrx_sem_lock(&G_sem_svcop, svcnm, semnum);
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
    return ndrx_sem_unlock(&G_sem_svcop, svcnm, semnum);
#else
    return ndrx_sem_unlock(&G_sem_svcop, svcnm, semnum);
#endif
}


/* vim: set ts=4 sw=4 et smartindent: */
