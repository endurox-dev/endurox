/**
 * @brief Log file file handles hashing
 *   
 * @file ndebugfd.c
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>                   /* purely for dbg_timer()       */
#include <sys/stat.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <sys_unix.h>
#include <cconfig.h>

#include "nstd_tls.h"
#include "userlog.h"
#include "utlist.h"
#include <expluginbase.h>
#include <sys_test.h>
#include <lcf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate MUTEX_LOCKDECL(M_sink_lock);
exprivate ndrx_debug_file_sink_t *M_sink_hash = NULL; /** list of sinks */

/*---------------------------Prototypes---------------------------------*/

/* Open logger
 * - get global lock
 * - Create new object or increment references
 */

/**
 * Special file names are:
 * /dev/stderr -> these are also fallbacks if file cannot be open
 * /dev/stdout 
 * 
 * @param fname full path to file
 * @param do_lock shall we perform locking?
 * @param dbg_ptr set the logger name atomically
 * @return file sink
 */
expublic ndrx_debug_file_sink_t* ndrx_debug_get_sink(char *fname, 
        int do_lock, ndrx_debug_t *dbg_ptr)
{
    ndrx_debug_file_sink_t *ret = NULL;
    long flags=0;
    
    if (do_lock)
    {
        MUTEX_LOCK_V(M_sink_lock);
    }
    
    /* if logger is process level, then no h */
    EXHASH_FIND_STR( M_sink_hash, fname, ret);
    
    if (NULL==ret)
    {
        /* OK, add handler to hash, set refcount to 1*/
        ret = NDRX_FPMALLOC(sizeof(ndrx_debug_file_sink_t), 0);
        
        if (NULL==ret)
        {
            userlog("Failed to malloc file sink handler (%s) bytes: %s",
                    sizeof(ndrx_debug_file_sink_t), strerror(errno));
            goto out;
        }
        
        /* debug does not exist, thus create one... */
        if (0==strcmp(fname, NDRX_LOG_OSSTDOUT))
        {
            ret->fp=stdout;
            flags |=NDRX_LOG_FOSHSTDOUT;
        }
        else if (0==strcmp(fname, NDRX_LOG_OSSTDERR))
        {
            ret->fp=stderr;
            flags |=NDRX_LOG_FOSHSTDERR;
        }
        else
        {
            ret->fp = ndrx_dbg_fopen_mkdir(fname, "a", dbg_ptr);
        
            if (NULL==ret->fp)
            {
                 userlog("Failed to to open [%s]: %d/%s", fname,
                                    errno, strerror(errno));
                /* Fallback to stderr */
                MUTEX_UNLOCK_V(M_sink_lock);
                NDRX_FPFREE(ret);
                
                return ndrx_debug_get_sink(NDRX_LOG_OSSTDERR, EXTRUE, dbg_ptr); /*<<< RETURN! */
            }
        }
        
        /* init the attribs */
        
        MUTEX_VAR_INIT(ret->change_lock);
        MUTEX_VAR_INIT(ret->busy_lock);
        NDRX_SPIN_INIT_V(ret->writters_lock);
        pthread_cond_init(&ret->change_wait, NULL);
        
        NDRX_STRCPY_SAFE(ret->fname, fname);
        ret->writters=0;
        ret->chwait=0;
        ret->refcount=1;
        ret->flags=flags;
        
        EXHASH_ADD_STR( M_sink_hash, fname, ret );

    }
    else
    {
        /* Increment the users + add any flags */
        ret->refcount++;
        
        /* set process level if any */
        if (dbg_ptr->flags & LOG_FACILITY_PROCESS)
        {
            ret->flags |=NDRX_LOG_FPROC;
        }
        
    }
    
out:
    
    if (NULL!=dbg_ptr && NULL!=ret)
    {
        dbg_ptr->dbg_f_ptr = ret;
        NDRX_STRCPY_SAFE(dbg_ptr->filename, ret->fname);
    }

    if (do_lock)
    {
        MUTEX_UNLOCK_V(M_sink_lock);
    }

    if (NULL==ret)
    {
        userlog("Cannot work with out logger (OOM?)!");
        exit(EXFAIL);
    }

    return ret;
}

/**
 * Thread removes from sink
 * it will be closed, if there are 0 refcount
 * @param mysink this is threads sink ptr
 * @param force do not check the reference count...
 * @return EXTRUE if was removed, EXFALSE if not
 */
expublic int ndrx_debug_unset_sink(ndrx_debug_file_sink_t* mysink, int do_lock, int force)
{
    int ret = EXFALSE;
    
    if (do_lock)
    {
        MUTEX_LOCK_V(M_sink_lock);
    }
    
    mysink->refcount--;

    assert(mysink->refcount >=0);
    
    /* remove only if it is not process level sink... 
     * why not remove proclevel ? if it is fully
     */
    if (mysink->refcount == 0 && ! (mysink->flags & NDRX_LOG_FPROC) || force)
    {
        NDRX_FCLOSE(mysink->fp);
        
        /* un-init the resources */
        pthread_cond_destroy(&mysink->change_wait);
        MUTEX_DESTROY_V(mysink->change_lock);
        MUTEX_DESTROY_V(mysink->busy_lock);
        NDRX_SPIN_DESTROY_V(mysink->writters_lock);
        
        /* remove it from hash */
        EXHASH_DEL(M_sink_hash, mysink);
        NDRX_FPFREE(mysink);
        
        ret=EXTRUE;
        
    }
    
    if (do_lock)
    {
        MUTEX_UNLOCK_V(M_sink_lock);
    }
    
    return ret;
}

/**
 * Increase reference count for copied object..
 * @param mysink sink to 
 */
expublic void ndrx_debug_addref(ndrx_debug_file_sink_t* mysink)
{
    MUTEX_LOCK_V(M_sink_lock);
    mysink->refcount++;
    MUTEX_UNLOCK_V(M_sink_lock);
}

/**
 * Get total count of the sinks present and number of references to sinks
 * @param sinks return number of sinks
 * @param refs return number of refs
 */
expublic void ndrx_debug_refcount(int *sinks, int *refs)
{
    ndrx_debug_file_sink_t* el, *elt;
    
    *sinks=0;
    *refs=0;
    
    
    MUTEX_LOCK_V(M_sink_lock);
    
    EXHASH_ITER(hh, M_sink_hash, el, elt)
    {
        (*sinks)++;
        *refs = *refs + el->refcount;
    }
    
    MUTEX_UNLOCK_V(M_sink_lock);
    
    
}
/**
 * Perform synchronization, if somebody is doing logfile change
 * If busy then wait on busy lock
 * then we get exclusive access to log logger, and thus we finally increment
 * @param mysink file obj used
 */
expublic void ndrx_debug_lock(ndrx_debug_file_sink_t* mysink)
{
    int is_busy=EXFALSE;
    
    /* step 1 -> check if busy lock is set? */
    NDRX_SPIN_LOCK_V(mysink->writters_lock);
    
    if (mysink->chwait)
    {
        is_busy=EXTRUE;
    }
    else
    {
        mysink->writters++;
    }

    NDRX_SPIN_UNLOCK_V(mysink->writters_lock);
    
    if (is_busy)
    {
        /* wait for busy mutex */
        MUTEX_LOCK_V(mysink->busy_lock);
        
        /* now we can set that we want to write */
        NDRX_SPIN_LOCK_V(mysink->writters_lock);
        mysink->writters++;
        NDRX_SPIN_UNLOCK_V(mysink->writters_lock);
        
        MUTEX_UNLOCK_V(mysink->busy_lock);
        
    }
    
}

/**
 * Remove logger lock
 * @param mysink file sink object
 */
expublic void ndrx_debug_unlock(ndrx_debug_file_sink_t* mysink)
{
    int do_signal = EXFALSE;
    
    /* step 1 -> check if busy lock is set? */
    NDRX_SPIN_LOCK_V(mysink->writters_lock);
    
    mysink->writters--;
    
    /* Debug file is locked by busy_lock mutex, thus we are last
     * ones which finishes off, thus in next step we can wake up the
     * thread which wants to do the changes
     */
    if (mysink->chwait && 0==mysink->writters)
    {
        do_signal=EXTRUE;
    }

    NDRX_SPIN_UNLOCK_V(mysink->writters_lock);
    
    if (do_signal)
    {
        MUTEX_LOCK_V(mysink->change_lock);
        /* wake up the log changer...*/
        pthread_cond_signal(&mysink->change_wait);
        MUTEX_UNLOCK_V(mysink->change_lock);
    }
}

/**
 * Change name of the logger.
 * if file name is the same, it just closes & open (thus may be used for logrotate)
 * Warning ! files descrs cannot be changed from logging area
 * 
 * If changing process level 
 * 
 * @param toname new filename
 * @param do_lock shall locking be performed, e.g. not needed if wrapped
 * @param logger_flags logger code for which changes are made
 * @param dbg_ptr -> here the final file name is written
 * @return EXSUCCEED/EXFAIL
 */
expublic void ndrx_debug_changename(char *toname, int do_lock, ndrx_debug_t *dbg_ptr)
{
    int writters;
    ndrx_debug_file_sink_t** mysink = &dbg_ptr->dbg_f_ptr;

          
    if (do_lock)
    {
        /* only one at the time please! */
        MUTEX_LOCK_V(M_sink_lock);
    }
    
    /* In case if changing the name, and we have more references
     * then 1 (thus others uses the name too)
     * open then the new handle
     * and leave this as is.
     * 
     * If it is system logger, then we do not close it but just change the name
     * If this process level logger, then we have an issue:
     * - we might want to separate from other loggers,
     *   if we just change the 
     * 
     */
    if ( !(LOG_FACILITY_PROCESS & dbg_ptr->flags) &&
            (*mysink)->refcount > 1 && 0!=strcmp((*mysink)->fname, toname))
    {
        /* remove process level indication  */
        ndrx_debug_unset_sink(*mysink, EXFALSE, EXFALSE);
        
        /* at this moment we could stream in the debug buffer? */
        *mysink = ndrx_debug_get_sink(toname, EXFALSE, dbg_ptr);
        
        /* we are done, ptrs are set & file names */
        goto out_final;
    }
    
    /* If this is process level logger and ref count is > 1 
     * and file name is different than current,
     * - we shall lock the sink
     * - alloc new sink
     * - reassing the sink
     * - unlock this sink
     */
    
    
    /* so firstly sync to none of writters */
    MUTEX_LOCK_V((*mysink)->busy_lock);
    MUTEX_LOCK_V((*mysink)->change_lock);
    
    /* We will start wait (if some is in write area */
    /* get spin lock of number of writters */
    NDRX_SPIN_LOCK_V((*mysink)->writters_lock);
    (*mysink)->chwait=EXTRUE;
    writters = (*mysink)->writters;
    NDRX_SPIN_UNLOCK_V((*mysink)->writters_lock);
    
    assert(writters>=0);
    
    if (writters)
    {
        /* somebody is writing, so when it finishes, let us to wake up! */
        pthread_cond_wait(&(*mysink)->change_wait, &(*mysink)->change_lock);
    }
    

    /* close handler, if it is not OS */
    if (! ((*mysink)->flags & NDRX_LOG_FOSHSTDERR ||
            (*mysink)->flags & NDRX_LOG_FOSHSTDOUT)
            )
    {
        NDRX_FCLOSE((*mysink)->fp);
    }

    /* remove markings */
    (*mysink)->flags&=(~NDRX_LOG_FOSHSTDERR);
    (*mysink)->flags&=(~NDRX_LOG_FOSHSTDOUT);


    /* open if new is not OS, remove or add markings... */
    if (0==strcmp(toname, NDRX_LOG_OSSTDERR))
    {
        (*mysink)->fp=stderr;
        (*mysink)->flags|=NDRX_LOG_FOSHSTDERR;
    }
    else if (0==strcmp(toname, NDRX_LOG_OSSTDOUT))
    {
        (*mysink)->fp=stdout;
        (*mysink)->flags|=NDRX_LOG_FOSHSTDOUT;
    }
    else
    {
        (*mysink)->fp=ndrx_dbg_fopen_mkdir(toname, "a", dbg_ptr);

        if (NULL==(*mysink)->fp)
        {
            userlog("Failed to set log file to [%s]: %s -> fallback to stderr", 
                    toname, strerror(errno));

            (*mysink)->fp=stderr;
            (*mysink)->flags|=NDRX_LOG_FOSHSTDERR;
            NDRX_STRCPY_SAFE((*mysink)->fname, NDRX_LOG_OSSTDERR);
        }
        else
        {
            /* OK New name is set */
            NDRX_STRCPY_SAFE((*mysink)->fname, toname);
        }
    }

    /* unlock originals */
    (*mysink)->chwait=EXFALSE;
    
    MUTEX_UNLOCK_V((*mysink)->change_lock);
    MUTEX_UNLOCK_V((*mysink)->busy_lock);
    
out:
    
    /* no errors, just update to actual logger */
    NDRX_STRCPY_SAFE(dbg_ptr->filename, (*mysink)->fname);

out_final:
    if (do_lock)
    {
        /* only one at the time please! */
        MUTEX_UNLOCK_V(M_sink_lock);
    }
    
}

/**
 * This is force close of all open loggers
 * And this performs de-init of the logging system.
 * Assuming know what your are doing. All threads must be stopped.
 * This can be use when living after the child fork
 */
expublic void ndrx_debug_force_closeall(void)
{
    ndrx_debug_file_sink_t* el, *elt;
    
    MUTEX_LOCK_V(M_sink_lock);
    
    EXHASH_ITER(hh, M_sink_hash, el, elt)
    {
        ndrx_debug_unset_sink(el, EXFALSE, EXTRUE);
    }
    
    /* mark debug as non-init */
    
    G_ndrx_debug_first=EXFALSE;
    
    MUTEX_UNLOCK_V(M_sink_lock);
    
}



/* vim: set ts=4 sw=4 et smartindent: */
