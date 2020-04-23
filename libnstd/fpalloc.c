/**
 * @brief Fast Pool Allocator
 *
 * @file fpalloc.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys_primitives.h> /**< spin locks for MacOS */
#include <ndrstandard.h>
#include <fpalloc.h>
#include <thlock.h>
#include <errno.h>
#include <nstdutil.h>
#include <userlog.h>

#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/* states: */            
#define NDRX_FP_GETSIZE         1
#define NDRX_FP_GETMODE         2
#define NDRX_FP_USEMALLOC       3
#define NDRX_FP_USENUMBL        4


/* debug of FPA */
#define NDRX_FPDEBUG(fmt, ...)
/*#define NDRX_FPDEBUG(fmt, ...) NDRX_LOG(log_debug, fmt, ##__VA_ARGS__)*/


/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/**
 * List of malloc pools with different sizes and min/max settings
 */
exprivate volatile ndrx_fpapool_t M_fpa_pools[NDRX_FPA_MAX] =
{  
    /* size                 flags   */
     {NDRX_FPA_0_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_0_DNUM}      /* 0 */
    ,{NDRX_FPA_1_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_1_DNUM}      /* 1 */
    ,{NDRX_FPA_2_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_2_DNUM}      /* 2 */
    ,{NDRX_FPA_3_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_3_DNUM}      /* 3 */
    ,{NDRX_FPA_4_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_4_DNUM}      /* 4 */
    ,{NDRX_FPA_5_SIZE,       NDRX_FPNOFLAG, NDRX_FPA_5_DNUM}      /* 5 */
    ,{NDRX_FPA_SIZE_SYSBUF,  NDRX_FPSYSBUF, NDRX_FPA_SYSBUF_DNUM} /* 6 */
};

/*---------------------------Statics------------------------------------*/
MUTEX_LOCKDECL(M_initlock);
exprivate volatile int M_init_first = EXTRUE;   /**< had the init ?     */
exprivate int M_malloc_all = EXFALSE;           /**< do sys malloc only */
exprivate char *M_opts = NULL;                  /**< env options        */
/*---------------------------Prototypes---------------------------------*/


/**
 * return pool stats (not for prod use)
 * @param poolno pool number
 * @param stats p_stats stnapshoot of the pool
 */
expublic void ndrx_fpstats(int poolno, ndrx_fpapool_t *p_stats)
{
    /* lock the pool */
    pthread_spin_lock(&M_fpa_pools[poolno].spinlock);
    memcpy((char *)p_stats, (char *)&M_fpa_pools[poolno], sizeof(ndrx_fpapool_t));
    pthread_spin_unlock(&M_fpa_pools[poolno].spinlock);
}

/**
 * This is thread safe way...
 */
expublic void ndrx_fpuninit(void)
{
    int i;
    ndrx_fpablock_t *freebl;
    /* free up all allocated pages... */
    
    if (M_init_first)
    {
        /* nothing todo */
        return;
    }
    
    /* remove all blocks */
    for (i=0; i<N_DIM(M_fpa_pools); i++)
    {
        do
        {
            freebl = NULL;
            
            pthread_spin_lock(&M_fpa_pools[i].spinlock);

            freebl = M_fpa_pools[i].stack;
            
            if (NULL!=freebl)
            {
                M_fpa_pools[i].stack = freebl->next;
            }

            pthread_spin_unlock(&M_fpa_pools[i].spinlock);
            
            if (NULL!=freebl)
            {
                NDRX_FREE(freebl);
            }
            
        } 
        while (NULL!=freebl);
    }
    
    M_init_first=EXTRUE;
}

/**
 * Init the feedback allocator. This will parse CONF_NDRX_FPAOPTS env
 * variable, if provided.
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_fpinit(void)
{
    int ret = EXSUCCEED;
    int i, state, blocksz, bnum, found, len, multiplier;
    char settings[1024];
    char *bconf, *bopt, *saveptr1, *saveptr2;
    /* load defaults */
    for (i=0; i<N_DIM(M_fpa_pools); i++)
    {
        M_fpa_pools[i].cur_blocks = 0;
        M_fpa_pools[i].stack = NULL;
        M_fpa_pools[i].allocs = 0;
        pthread_spin_init(&(M_fpa_pools[i].spinlock), PTHREAD_PROCESS_PRIVATE);
    }
    
    /* setup the options if any... */
    M_opts=getenv(CONF_NDRX_FPAOPTS);
    
    if (NULL!=M_opts)
    {
        NDRX_FPDEBUG("Loading config: [%s]", M_opts);
        
        NDRX_STRCPY_SAFE(settings, M_opts);
        
        /* start parse. 
         * format:
         * [<D|S|KB>:{M|<MIN>{[:<MAX>]|[:<MAX>,<HITS>]}}][,..,[<D|S|KB>:{M|<MIN>{[:<MAX>]|[:<MAX>:<HITS>]}}]]
         * where:
         * 'D' - Defaults, change all allocation stacks
         * 'S' - System buffer 
         * 'KB' - Block size, currently const 1,2,4,5,16,32,64
         * 'M' - use system malloc
         * 'MIN' - min blocks in pool
         * 'MAX' - max blocks in pool
         * 'HITS' - number of gotten.
         * This will not remove any spaces, etc.... In case of parse failure,
         * ulog will be written and defaults will be used.
         */
        for (bconf=strtok_r(settings, ",", &saveptr1); NULL!=bconf; bconf=strtok_r(NULL, ",", &saveptr1))
        {   
            /* got block config, parse each */
            
            NDRX_FPDEBUG("Parsing block: [%s]", bconf);
            
            state = NDRX_FP_GETSIZE;
            bnum=EXFAIL;
            for (bopt=strtok_r(bconf, ":", &saveptr2); NULL!=bopt; bopt=strtok_r(NULL, ":", &saveptr2))
            {
                NDRX_FPDEBUG("Parsing option: [%s]", bopt);
                
                switch (state)
                {
                    case NDRX_FP_GETSIZE:
                        
                        if (0==strcmp(bopt, "D"))
                        {
                            blocksz = NDRX_FPA_SIZE_DEFAULT;
                        }
                        else if (0==strcmp(bopt, "S"))
                        {
                            blocksz = NDRX_FPA_SIZE_SYSBUF;
                        }
                        else
                        {
                            len = strlen(bopt);
                            multiplier=1;
                            
                            if (len > 0)
                            {
                                if (bopt[len-1]=='K')
                                {
                                    multiplier=1024;
                                    bopt[len-1]=EXEOS;
                                }
                                else if (bopt[len-1]<'0' || bopt[len-1]>'9')
                                {
                                    userlog("%s: Invalid 'size' size multiplier "
                                            "[%c] for token [%s] in [%s] ", 
                                        CONF_NDRX_FPAOPTS, bopt[len-1], bopt, M_opts);
                                    EXFAIL_OUT(ret);
                                }
                            }
                            
                            if (!ndrx_is_numberic(bopt))
                            {
                                userlog("%s: Invalid 'size' token [%s] in [%s]", 
                                        CONF_NDRX_FPAOPTS, bopt, M_opts);
                                EXFAIL_OUT(ret);
                            }
                            
                            blocksz = atoi(bopt)*multiplier;
                        }
                        
                        state = NDRX_FP_GETMODE;
                        break;
                    case NDRX_FP_GETMODE:
                        /* mode is either "M" or number (as min) */
                        if (0==strcmp(bopt, "M"))
                        {
                            NDRX_FPDEBUG("Use Malloc for size %d", blocksz);
                            
                            state = NDRX_FP_USEMALLOC;
                        }
                        else
                        {
                            if (!ndrx_is_numberic(bopt))
                            {
                                userlog("%s: Invalid 'bnum' token [%s] in [%s]", 
                                        CONF_NDRX_FPAOPTS, bopt, M_opts);
                                EXFAIL_OUT(ret);
                            }
                            
                            bnum = atoi(bopt);
                            
                            if (bnum < 1)
                            {
                                userlog("%s: Invalid 'bnum' token (<1) [%s] in [%s]", 
                                        CONF_NDRX_FPAOPTS, bopt, M_opts);
                                EXFAIL_OUT(ret);
                            }
                            
                            state=NDRX_FP_USENUMBL;
                            NDRX_FPDEBUG("bnum=%d", bnum);
                        }
                        break;
                    case NDRX_FP_USEMALLOC:
                        /* err ! not more settings! */
                        userlog("%s: Invalid argument for 'M' token [%s] in [%s]", 
                                        CONF_NDRX_FPAOPTS, bopt, M_opts);
                        EXFAIL_OUT(ret);
                        break;
                    case NDRX_FP_USENUMBL:
                        userlog("%s: Token not expected [%s] in [%s]", 
                                        CONF_NDRX_FPAOPTS, bopt, M_opts);
                        EXFAIL_OUT(ret);
                        break;
                } /* case state */
            } /* for bopt */
            
            if (NDRX_FP_USEMALLOC > state)
            {
                userlog("%s: Missing mode/min for block size %d in [%s]", 
                        CONF_NDRX_FPAOPTS, (int)blocksz, M_opts);
                EXFAIL_OUT(ret);
            }
            
            /* not the best search, but only once for startup */
            found=EXFALSE;
            for (i=0; i<N_DIM(M_fpa_pools); i++)
            {
                if (NDRX_FPA_SIZE_DEFAULT==blocksz || 
                        blocksz==M_fpa_pools[i].bsize)
                {
                    /* Setup the block */
                    if (NDRX_FP_USEMALLOC==state)
                    {
                        M_fpa_pools[i].flags|=NDRX_FPNOPOOL;
                        NDRX_FPDEBUG("Pool %d flags set to: [%d]", 
                                i, M_fpa_pools[i].flags);
                        found=EXTRUE;
                        
                        if (blocksz==M_fpa_pools[i].bsize)
                        {
                            break;
                        }
                    }
                    else 
                    {
                        if (EXFAIL!=bnum)
                        {
                            M_fpa_pools[i].num_blocks=bnum;
                        }
                        
                        found=EXTRUE;
                        
                        if (blocksz==M_fpa_pools[i].bsize)
                        {
                            break;
                        }
                    }
                } /* check block size */
            } /* for stack */
            
            if (!found)
            {
                userlog("%s: Block size %d not supported in [%s]", 
                        CONF_NDRX_FPAOPTS, blocksz, M_opts);
            }
        } /* for block config */
        
        /* check if all default -> set global flag to default malloc. */
        M_malloc_all = EXTRUE;
        for (i=0; i<N_DIM(M_fpa_pools); i++)
        {
            if (!(M_fpa_pools[i].flags & NDRX_FPNOPOOL))
            {
                NDRX_FPDEBUG("Not malloc all due to [%d]", i);
                M_malloc_all = EXFALSE;
                break;
            }
        }
    } /* if has NDRX_FPAOPTS */
    
    M_init_first=EXFALSE;
    
out:
                        
    if (EXSUCCEED!=ret)
    {
        errno=EINVAL;
    }

    return ret;
}

/**
 * Malloc the memory block
 * @param size bytes to alloc
 * @param flags any special flag
 * @return for error = NULL  (errno set), or ptr to alloc'd block
 */
expublic NDRX_API void *ndrx_fpmalloc(size_t size, int flags)
{
    ndrx_fpablock_t *ret = NULL;
    int poolno=EXFAIL;
    
    /* do the init. */
    if (NDRX_UNLIKELY(M_init_first))
    {
        MUTEX_LOCK_V(M_initlock);
        
        if (M_init_first)
        {
            if (EXSUCCEED!=ndrx_fpinit())
            {
                MUTEX_UNLOCK_V(M_initlock);
                return NULL;    /**< return here! */
            }
        }
        MUTEX_UNLOCK_V(M_initlock);
    }
    
    /* run all malloc */
    if (NDRX_UNLIKELY(M_malloc_all))
    {
        ret=malloc(size);
        NDRX_FPDEBUG("all malloc %p", ret);
        return ret;
    }
    
    /* bin search... for the descriptor */
    if (flags & NDRX_FPSYSBUF)
    {
        poolno = NDRX_FPA_MAX-1;
    }
    else
    {
        /* get the pool size */
        int low=0, mid, high=NDRX_FPA_DYN_MAX-1;
        
        while(low <= high)
        {
            mid = (low + high) / 2;
             
            if (M_fpa_pools[mid].bsize > size)
            {
                high = mid - 1;
            }
            else if (M_fpa_pools[mid].bsize < size)
            {
                low = mid + 1;
            }
            else
            {
                poolno = mid;
                break;
            }
        }
        
        if (EXFAIL==poolno && high < NDRX_FPA_DYN_MAX-2)
        {
            /* select next size */
            poolno = high+1;
        }
    }
    
    if (NDRX_UNLIKELY(EXFAIL==poolno))
    {
        /* do malloc.., arb size */
        ret = (ndrx_fpablock_t *)NDRX_MALLOC(size+sizeof(ndrx_fpablock_t));
        
        NDRX_FPDEBUG("Arb size alloc %p", ret);
        
        if (NULL==ret)
        {
            goto out;
        }
        ret->flags=NDRX_FPABRSIZE;
        ret->magic = NDRX_FPA_MAGIC;
        ret->next = NULL;
        ret->poolno = EXFAIL;
    }
    else
    {
        
        if (M_fpa_pools[poolno].flags & NDRX_FPNOPOOL)
        {
            /* return new block as no pool used for particular size */
            ret = (ndrx_fpablock_t *)NDRX_MALLOC(M_fpa_pools[poolno].bsize+sizeof(ndrx_fpablock_t));
            NDRX_FPDEBUG("no-pool block for %d alloc %p", poolno, ret);
            
            if (NULL==ret)
            {
                goto out;
            }
            
            ret->poolno = poolno;
            ret->flags=NDRX_FPNOPOOL;
            ret->magic = NDRX_FPA_MAGIC;
            ret->next = NULL;
        }
        
        /* get from stack alloc if needed */
        pthread_spin_lock(&M_fpa_pools[poolno].spinlock);
        
        if (NULL!=M_fpa_pools[poolno].stack)
        {
            ret = M_fpa_pools[poolno].stack;
            M_fpa_pools[poolno].stack=M_fpa_pools[poolno].stack->next;
            /* reduce the block count */
            M_fpa_pools[poolno].cur_blocks--;
        }
#ifdef NDRX_FPA_STATS
        else
        {
            M_fpa_pools[poolno].allocs++;
        }
#endif
        
        pthread_spin_unlock(&M_fpa_pools[poolno].spinlock);
        
        if (NULL==ret)
        {
            /* do malloc.. */
            if (NDRX_FPA_SIZE_SYSBUF==M_fpa_pools[poolno].bsize)
            {
                /* size must be correctly passed in */
                ret = (ndrx_fpablock_t *)NDRX_MALLOC(size+sizeof(ndrx_fpablock_t));
                NDRX_FPDEBUG("Pool %d sysbuf alloc %p", poolno, ret);
            }
            else
            {
                ret = (ndrx_fpablock_t *)NDRX_MALLOC(M_fpa_pools[poolno].bsize+sizeof(ndrx_fpablock_t));
                NDRX_FPDEBUG("Pool %d block alloc %p", poolno, ret);
            }
            if (NULL==ret)
            {
                goto out;
            }
            ret->flags=0;
            ret->magic = NDRX_FPA_MAGIC;
            ret->next = NULL;
            ret->poolno = poolno;
        }
        else
        {
            NDRX_FPDEBUG("Got from pool %d addr %p", poolno, ret);
        }
    }
    
out:
                        
    return (void *) (((char *)ret) + sizeof(ndrx_fpablock_t));
}

/**
 * Free up the Feedback Pool Allocator memory block
 * @param ptr ptr alloc'd by ndrx_fmalloc
 */
expublic NDRX_API void ndrx_fpfree(void *ptr)
{
    ndrx_fpablock_t *ret = (ndrx_fpablock_t *)(((char *)ptr)-sizeof(ndrx_fpablock_t));
    int poolno;
    int action_free = EXFALSE;
#ifdef NDRX_FPA_STATS
    static int callnum=0;
    MUTEX_LOCKDECL(callnum_lock);
#endif
    
    if (NDRX_UNLIKELY(M_malloc_all))
    {
        /* free only use of direct memory.. */
        NDRX_FPDEBUG("M_alloc_all free %p", ptr);
        NDRX_FREE(ptr); 
        goto out;
    }
    
    if (NDRX_UNLIKELY(ret->magic!=NDRX_FPA_MAGIC))
    {
        ssize_t wret;
        /* signal safe error msg */
#define ERR_MSG1    "***************************************************\n"
#define ERR_MSG2    "* INVALID FPA FREE: Invalid magic                 *\n"
#define ERR_MSG3    "***************************************************\n"
        
        wret=write(STDERR_FILENO, ERR_MSG1, strlen(ERR_MSG1));
        wret=write(STDERR_FILENO, ERR_MSG2, strlen(ERR_MSG2));
        wret=write(STDERR_FILENO, ERR_MSG3, strlen(ERR_MSG3));
        abort();
    }
    
    /* remove arb size */
    if (NDRX_UNLIKELY(ret->flags & NDRX_FPABRSIZE))
    {
        NDRX_FPDEBUG("Arb/nopool free %p", ret);
        NDRX_FREE(ret); 
        goto out;
    }
    
    /* decide the feedback, at hits level we free up the buffer */
    poolno = ret->poolno;
    
    if (M_fpa_pools[poolno].flags & NDRX_FPNOPOOL)
    {
        NDRX_FPDEBUG("fpnopool %d free %p", poolno, ret);
        NDRX_FREE(ret); 
        goto out;
    }
    
    pthread_spin_lock(&M_fpa_pools[poolno].spinlock);
    
    /* Add back to the pool
     */
    if (M_fpa_pools[poolno].cur_blocks >= M_fpa_pools[poolno].num_blocks)
    {
        action_free = EXTRUE;
        NDRX_FPDEBUG("No space or hits in pool free up %p cur_hist=0", ret);
    }
    else
    {
        /* add block to stack */
        ret->next = M_fpa_pools[poolno].stack;
        M_fpa_pools[poolno].stack = ret;
        M_fpa_pools[poolno].cur_blocks++;
        NDRX_FPDEBUG("Add to pool %d: %p", poolno, ret);
    }
    
    pthread_spin_unlock(&M_fpa_pools[poolno].spinlock);
    
#ifdef NDRX_FPA_STATS
    MUTEX_LOCK_V(callnum_lock);
    callnum++;

    if ( callnum > 100)
    {
        int i;
        for (i=0; i<N_DIM(M_fpa_pools); i++)
        {   
            userlog("bsize %d allocs: %d", M_fpa_pools[i].bsize, M_fpa_pools[i].allocs);
        }
        callnum=0;
    }
    MUTEX_UNLOCK_V(callnum_lock);
#endif
        
    if (action_free)
    {
        NDRX_FPDEBUG("Action free %p", ret);
        NDRX_FREE(ret);
    }
out:    
    return;
}

/* vim: set ts=4 sw=4 et smartindent: */
