/**
 * @brief Daemon thread related routines
 *
 * @file dmn.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>

#include "ndebug.h"
#include "userlog.h"
#include <errno.h>
#include <sys/resource.h>
#include <ndrxdiag.h>
#include <thlock.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Init daemon shutdown flag checking struct
 * @param w shutdown wait object
 * @param argr arg to pass to the thread
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_dmnthread_init(ndrx_dmnthread_t *w, void *(*start_routine)(void *), void *arg)
{
    int ret = EXSUCCEED;
    pthread_attr_t pthread_custom_attr;

    MUTEX_VAR_INIT(w->shutdown_mutex);

    if (EXSUCCEED!=(ret=pthread_cond_init(&w->shutdown_flag_wait, NULL)))
    {
        userlog("ndrx_thlock_shutdown_init cond init fail: %s", strerror(ret));
        exit(-1);
    }

    w->shutdown_req = EXFALSE;

    /* prepare the thread */
    pthread_attr_init(&pthread_custom_attr);
    ndrx_platf_stack_set(&pthread_custom_attr);

    if (EXSUCCEED!=pthread_create(&w->thread, &pthread_custom_attr,
            start_routine, arg))
    {
        NDRX_PLATF_DIAG(NDRX_DIAG_PTHREAD_CREATE, errno, "ndrx_thlock_dmn_init");
        EXFAIL_OUT(ret);
    }

    /* so that we can do proper un-init */
    w->started = EXTRUE;

out:
    return ret;
}
/**
 * Destory the daemon thread shutdown control structure
 * @param w shutdown wait object
 * @return EXSUCCEED/EXFAIL
 */
exprivate void ndrx_dmnthread_destory(ndrx_dmnthread_t *w)
{
    int ret = EXSUCCEED;
    
    MUTEX_DESTROY_V(w->shutdown_mutex);
    pthread_cond_destroy(&w->shutdown_flag_wait);
}

/**
 * Check is shutdown requested
 * @param w shutdown wait object
 * @return EXTRUE/EXFALSE
 */
expublic int ndrx_dmnthread_is_shutdown(ndrx_dmnthread_t *w)
{
    /* no need to lock..., any race condition in worst case
     * might only end with some addition loop of the daemon process
     */
    return w->shutdown_req;
}

/**
 * daemon does sleep, but can be woken up by shutdown request
 * @param w shutdown wait object
 * @param ms milliseconds to sleep
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_dmnthread_sleep(ndrx_dmnthread_t *w, int ms)
{
    int ret = EXSUCCEED;
    struct timespec wait_time;
    struct timeval now;

    gettimeofday(&now,NULL);

    wait_time.tv_sec = now.tv_sec;
    wait_time.tv_nsec = now.tv_usec*1000;
    ndrx_timespec_plus(&wait_time, ms);

    MUTEX_LOCK_V(w->shutdown_mutex);

    if (!w->shutdown_req &&
        EXSUCCEED!=(ret=pthread_cond_timedwait(&w->shutdown_flag_wait, 
        &w->shutdown_mutex, &wait_time)))
    {
        if (ETIMEDOUT==ret)
        {
            ret = EXSUCCEED;
        }
        else
        {
            NDRX_LOG(log_error, "Failed to wait for shutdown flag (ms=%d): %s", ms, strerror(ret));
            EXFAIL_OUT(ret);
        }
    }

    /* if we are locked, get the status & unlock. */
    ret=w->shutdown_req;

out:

    /* unlock anyway.. */
    MUTEX_UNLOCK_V(w->shutdown_mutex);

    return ret;
}

/**
 * Request the shutdown of the other thread
 * @param w shutdown wait object
 * @return return from the thread
 */
expublic void* ndrx_dmnthread_shutdown(ndrx_dmnthread_t *w)
{
    void *ret=NULL;

    if (w->started)
    {
        /* signal for shutdown */
        MUTEX_LOCK_V(w->shutdown_mutex);
        w->shutdown_req = EXTRUE;
        pthread_cond_signal(&w->shutdown_flag_wait);
        MUTEX_UNLOCK_V(w->shutdown_mutex);

        /* join the thread */
        pthread_join(w->thread, &ret);

        /* destroy  */
        ndrx_dmnthread_destory(w);
        /* allow double shutdown... */
        w->started=EXFALSE;
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
