/**
 * @brief Enduro/X Standard Library Shared memory commons
 *   For some history. Some parts are moved here from libatmi, for the System V
 *   support. System V queues uses some SHM/SEM parts for the qid mappings to
 *   Posix queues.
 *   This implementation uses System V shared memory interfaces, due to some
 *   nasty issues with Oracle Solaris... See Support #355
 *
 * @file nstd_shmsv.c
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
#include <sys/ipc.h>
#include <sys/shm.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstd_shm.h>
#include <xatmi.h>

#include "sys_unix.h"


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
typedef struct
{
    char *suffix;
    int idx;
} segmap_t;
    
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
segmap_t M_map[] = {
        {NDRX_SHM_SRVINFO_SFX, NDRX_SHM_SRVINFO_KEYOFSZ}
        ,{NDRX_SHM_SVCINFO_SFX, NDRX_SHM_SVCINFO_KEYOFSZ}
        ,{NDRX_SHM_BRINFO_SFX, NDRX_SHM_BRINFO_KEYOFSZ}
        ,{NDRX_SHM_P2S_SFX, NDRX_SHM_P2S_KEYOFSZ}
        ,{NDRX_SHM_S2P_SFX, NDRX_SHM_S2P_KEYOFSZ}
        ,{NULL}
    };
/*---------------------------Prototypes---------------------------------*/    
    
/**
 * Make key out of string. Gen
 * @param k string to hash
 * @return hash value
 */
expublic unsigned int ndrx_hash_fn( void *k )
{
    unsigned int hash = 5381;
    int c;
    char *str = (char *)k;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Returns true if currently attached to shm
 * WARNING: This assumes that fd 0 could never be used by shm!
 * @return TRUE/FALSE
 */
expublic int ndrx_shm_is_attached(ndrx_shm_t *shm)
{
    int ret=EXTRUE;
    
    if (NULL==shm->mem || (void *)EXFAIL==shm->mem)
    {
        ret=EXFALSE;
    }

    return ret;
}

/**
 * Open shared memory segment. The path and size must be filled in shm handler
 * !!!! WARNING FOR NON NDRXD MODE !!!! 
 * If there could be some race condition, if several binaries are started
 * in the same time and they attempt to create a shared memory blocks.
 * of course it is not normal mode of operations, we use that only for testing
 * thus be aware, maybe some test cases needs to be tuned with some sleep periods.
 * 
 * @param shm shm handler. path and size must be set.
 * @param attach_on_exists if shm exists, perform attach instead of failure
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_open(ndrx_shm_t *shm, int attach_on_exists)
{
    int ret=EXSUCCEED;
    
    /* creating the shared memory object --  shm_open() */
    shm->fd = shmget(shm->key, shm->size, IPC_CREAT | IPC_EXCL| S_IRWXU | S_IRWXG);

    if (shm->fd < 0)
    {
        int err = errno;
        
        /* if failed to create with EEXISTS error, try to attach */
        
        if (EEXIST==err && attach_on_exists)
        {
            NDRX_LOG(log_error, "Shared memory exists [%s]/%x - attaching",
                    shm->path, shm->key);
            return ndrx_shm_attach(shm);
        }
        
        NDRX_LOG(log_error, "Failed to create shm [%s]: %s",
                            shm->path, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    shm->mem = (char *)shmat(shm->fd, 0, 0);
    
    if ((void *)EXFAIL==shm->mem)
    {
        NDRX_LOG(log_error, "Failed to shmat memory for [%s] fd %d/%x bytes %d: %s",
                            shm->path, shm->fd, shm->key, shm->size, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* Reset SHM */
    memset(shm->mem, 0, shm->size);
    
    NDRX_LOG(log_debug, "Shm: [%s] %d/%x created size: %d mem: %p", 
            shm->path, shm->fd, shm->key, shm->size, shm->mem);
    
out:
    /*
     * Should close the SHM if failed to open.
     */
    if (EXSUCCEED!=ret && EXFAIL!=shm->fd)
    {
        ndrx_shm_remove(shm);
    }

    NDRX_LOG(log_debug, "return %d", ret);

    return ret;
}

/**
 * Attach to shared memory block
 * @param shm shared memory block
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_attach(ndrx_shm_t *shm)
{
    int ret=EXSUCCEED;
    
    if (ndrx_shm_is_attached(shm))
    {
        NDRX_LOG(log_debug, "shm [%s] %d/%x size: %d already attached", shm->path,
                shm->fd, shm->key, shm->size);
        goto out;
    }
    
    /* Attach to shared memory block */
    shm->fd = shmget(shm->key, shm->size, S_IRWXU | S_IRWXG);

    if (shm->fd < 0) 
    {
        NDRX_LOG(log_error, "Failed to shmget/attach shm key=%x [%s]: %s",
                            shm->key, shm->path, strerror(errno));
        EXFAIL_OUT(ret);
    }

    shm->mem = (char *)shmat(shm->fd, 0, 0);
    
    if (MAP_FAILED==shm->mem)
    {
        NDRX_LOG(log_error, "Failed to shmat memory for [%s] fd %d/%x bytes %d: %s",
                            shm->path, shm->fd, shm->key, shm->size, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Shm: [%s] %d/%x attach size: %d mem: %p", 
            shm->path, shm->fd, shm->key, shm->size, shm->mem);

out:
    /*
     * Should close the SHM if failed to open.
     
    if (EXSUCCEED!=ret)
    {
        shm->fd=EXFAIL;
    }
    */
            
    NDRX_LOG(log_debug, "return %d", ret);

    return ret;
}

/**
 * Close opened shared memory segment.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_close(ndrx_shm_t *shm)
{
    int ret=EXSUCCEED;

    if ((void *)EXFAIL==shm->mem || NULL==shm->mem)
    {
        NDRX_LOG(log_debug, "[%s] %x already closed", shm->path, shm->key);
    }
    else 
    {
        ret = shmdt(shm->mem);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to detach shm [%s] %d/%x addr [%p]: %d - %s",
                        shm->path, shm->fd, shm->key, shm->mem, errno, strerror(errno));
        }
        else
        {
            shm->mem = NULL;
        }
    }

out:
    return ret;
}

/**
 * Remove/unlink shared memory resource
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_remove(ndrx_shm_t *shm)
{
    int ret=EXSUCCEED;
    int fd;
    
    if (EXFAIL!=(fd = shmget(shm->key, shm->size, S_IRWXU | S_IRWXG)))
    {
        if (EXSUCCEED!=shmctl(fd, IPC_RMID, NULL))
        {
            NDRX_LOG(log_error, "Failed to IPC_RMID %d/%x: [%s]: %s",
                            fd, shm->key, shm->path, strerror(errno));
            ret = EXFAIL;
        }
    }
    else
    {
        NDRX_LOG(log_warn, "Failed to remove: [%s] %x", shm->path, shm->key);
    }
    
    return ret;
}

/**
 * Get shared memory key from the path
 * @param path
 * @return 
 */
exprivate key_t keygetshm(char *path, key_t ipckey)
{
    char *p;
    key_t ret;
    segmap_t *pp;
    
    if (NULL==(p = strchr(path, NDRX_FMT_SEP)))
    {
        NDRX_LOG(log_error, "Failed to get suffix for memory segment [%s]", 
                path);
        EXFAIL_OUT(ret);
    }
    
    /* next after comma */
    p++;
    
    pp = M_map;
    
    while (NULL!=pp->suffix)
    {
        if (0==strcmp(pp->suffix, p))
        {
            break;
        }
        pp++;
    }
    
    if (NULL==pp->suffix)
    {
        NDRX_LOG(log_error, "Failed to map shared memory segment [%s] to system v key",
                p);
        EXFAIL_OUT(ret);
    }
    
    ret = ipckey + pp->idx;
    
    NDRX_LOG(log_info, "[%s] segment mapped to ipc key %x", path, ret);
    
out:
    
    return ret;
}

/**
 * Remove shared memory segment by name
 * @param path shm path
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_remove_name(char *path, key_t ipckey)
{
    int ret = EXSUCCEED;
    key_t key = keygetshm(path, ipckey);
    int fd;
    
    if (EXFAIL!=key)
    {
        /* try to remove the mem.. */
        if (EXFAIL!=(fd = shmget(key, 0, S_IRWXU | S_IRWXG)))
        {
            if (EXSUCCEED!=shmctl(fd, IPC_RMID, NULL))
            {
                NDRX_LOG(log_error, "Failed to IPC_RMID %d/%x: [%s]: %s",
                                fd, key, path, strerror(errno));
                ret = EXFAIL;
            }
        }
    }
    
out:
    return ret;
}

/**
 * List shred mem blocks
 * @param ipckey IPC key configured for the system
 * @return list of segments or NULL if no segments open (or error)
 */
expublic string_list_t * ndrx_shm_shms_list(key_t ipckey)
{
    string_list_t *ret = NULL;
    char segment[NDRX_MAX_Q_SIZE*2];
    int i;
    key_t key;
    int fd;
    
    for (i=0; NULL!=M_map[i].suffix; i++)
    {
        key = ipckey + M_map[i].idx;
        
        if (EXFAIL!=(fd = shmget(key, 0, S_IRWXU | S_IRWXG)))
        {
            snprintf(segment, sizeof(segment), "%x: %s", key, M_map[i].suffix);
            if (EXSUCCEED!=ndrx_string_list_add(&ret, segment))
            {
                NDRX_LOG(log_error, "Failed to add shm segment to list");
                if (NULL!=ret)
                {
                    ndrx_string_list_free(ret);
                    ret = NULL;
                }
                goto out;
            }
        }
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
