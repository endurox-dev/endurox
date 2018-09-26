/**
 * @brief Enduro/X Standard Libary Shared memory commons
 *  For some history. Some parts are moved here from libatmi, for the System V
 *  support. System V queues uses some SHM/SEM parts for the qid mappings to
 *  Posix queues.
 *
 * @file nstd_shm.c
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

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
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
    
    if (shm->fd <=0 || shm->fd <=0)
    {
        ret=EXFALSE;
    }

    return ret;
}

/**
 * Open shared memory segment. The path and size must be filled in shm handler
 * @param shm shm handler. path and size must be set.
 * @param attach_on_exists if shm exists, perform attach instead of failure
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_shm_open(ndrx_shm_t *shm, int attach_on_exists)
{
    int ret=EXSUCCEED;
    
    /* creating the shared memory object --  shm_open() */
    shm->fd = shm_open(shm->path, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);

    if (shm->fd < 0)
    {
        int err = errno;
        
        /* if failed to create with EEXISTS error, try to attach */
        
        if (EEXIST==err && attach_on_exists)
        {
            NDRX_LOG(log_error, "Shared memory created [%s] - attaching",
                    shm->path);
            return ndrx_shm_attach(shm);
        }
        
        NDRX_LOG(log_error, "Failed to create shm [%s]: %s",
                            shm->path, strerror(err));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ftruncate(shm->fd, shm->size))
    {
        NDRX_LOG(log_error, "Failed to set [%s] fd: %d to size %d bytes: %s",
                            shm->path, shm->fd, shm->size, strerror(errno));
        EXFAIL_OUT(ret);
    }

    shm->mem = (char *)mmap(NULL, shm->size, 
                        PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (MAP_FAILED==shm->mem)
    {
        NDRX_LOG(log_error, "Failed to map memory for [%s] fd %d bytes %d: %s",
                            shm->path, shm->fd, shm->size, strerror(errno));
        EXFAIL_OUT(ret);
    }
    /* Reset SHM */
    memset(shm->mem, 0, shm->size);
    NDRX_LOG(log_debug, "Shm: [%s] created size: %d mem: %p", 
            shm->path, shm->size, shm->mem);
    
out:
    /*
     * Should close the SHM if failed to open.
     */
    if (EXSUCCEED!=ret && EXFAIL!=shm->fd)
    {
        if (shm_unlink(shm->path) != 0) {
            NDRX_LOG(log_error, "Failed to unlink [%s]: %s",
                            shm->path, strerror(errno));
        }
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
        NDRX_LOG(log_debug, "shm [%s] size: %d already attached", shm->path,
                shm->size);
        goto out;
    }
    
    /* Attach to shared memory block */
    shm->fd = shm_open(shm->path, O_RDWR, S_IRWXU | S_IRWXG);

    if (shm->fd < 0) 
    {
        NDRX_LOG(log_error, "Failed to attach shm [%s]: %s",
                            shm->path, strerror(errno));
        EXFAIL_OUT(ret);
    }

    shm->mem = (char *)mmap(NULL, shm->size,
                        PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (MAP_FAILED==shm->mem)
    {
        NDRX_LOG(log_error, "Failed to map memory for [%s] fd %d bytes %d: %s",
                            shm->path, shm->fd, shm->size, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Shm: [%s] attach size: %d mem: %p", 
            shm->path, shm->size, shm->mem);

out:
    /*
     * Should close the SHM if failed to open.
     */
    if (EXSUCCEED!=ret)
    {
        shm->fd=EXFAIL;
    }

    NDRX_LOG(log_debug, "return %d", ret);
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

