/* 
 * @brief Globals/TLS for libnstd
 *   All stuff here must work. thus if something is very bad, we ill print to stderr.
 *
 * @file nstd_tls.c
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
#include <nstdutil.h>
#include <nstd_tls.h>
#include <string.h>
#include "thlock.h"
#include "userlog.h"
#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
__thread nstd_tls_t *G_nstd_tls = NULL; /* single place for library TLS storage */
/*---------------------------Statics------------------------------------*/
exprivate pthread_key_t M_nstd_tls_key;
MUTEX_LOCKDECL(M_thdata_init);
exprivate int M_first = EXTRUE;
/*---------------------------Prototypes---------------------------------*/
exprivate void nstd_buffer_key_destruct( void *value );

/**
 * Thread destructor
 * @param value this is malloced thread TLS
 */
exprivate void nstd_buffer_key_destruct( void *value )
{
    /* TODO: Close any open log descriptors... */
    ndrx_nstd_tls_free((void *)value);
}

/**
 * Unlock, unset G_nstd_tls, return pointer to G_nstd_tls
 * @return 
 */
expublic void * ndrx_nstd_tls_get(void)
{
    nstd_tls_t *tmp = G_nstd_tls;
    
    G_nstd_tls = NULL;
    
    if (NULL!=tmp)
    {
        /*
         * Unset the destructor
         */
        if (tmp->is_auto)
        {
            pthread_setspecific( M_nstd_tls_key, NULL );
        }

        /* unlock object */
        MUTEX_UNLOCK_V(tmp->mutex);
    }
    
    return (void *)tmp;
}

/**
 * Get the lock & set the G_nstd_tls to this one
 * @param tls
 */
expublic int ndrx_nstd_tls_set(void *data)
{
    nstd_tls_t *tls = (nstd_tls_t *)data;
    
    if (NULL!=tls)
    {
        /* extra control... */
        if (NSTD_TLS_MAGIG!=tls->magic)
        {
            userlog("nstd_tls_set: invalid magic! expected: %x got %x", 
                    NSTD_TLS_MAGIG, tls->magic);
        }

        /* Lock the object */
        MUTEX_LOCK_V(tls->mutex);

        G_nstd_tls = tls;

        /*
         * Destruct automatically if it was auto-tls 
         */
        if (tls->is_auto)
        {
            pthread_setspecific( M_nstd_tls_key, (void *)tls );
        }
    }
    else
    {
        G_nstd_tls = NULL;
    }

    return EXSUCCEED;
}

/**
 * Free up the TLS data
 * @param tls
 * @return 
 */
expublic void ndrx_nstd_tls_free(void *data)
{
    if (NULL!=data)
    {
        if (data==G_nstd_tls)
        {
            if (G_nstd_tls->is_auto)
            {
                pthread_setspecific( M_nstd_tls_key, NULL );
            }
            G_nstd_tls=NULL;
        }
        
        /* Close debug loggers? Bug #274 */
        
        ndrx_nstd_tls_loggers_close((nstd_tls_t *)data);
        
        NDRX_FREE((char*)data);
    }
}

/**
 * Get the lock & init the data
 * @param auto_destroy if set to 1 then when tried exits, thread data will be made free
 * @return 
 */
expublic void * ndrx_nstd_tls_new(int auto_destroy, int auto_set)
{
    int ret = EXSUCCEED;
    nstd_tls_t *tls  = NULL;
    char fn[] = "nstd_context_new";
    
    /* init they key storage */
    
    if (M_first)
    {
        MUTEX_LOCK_V(M_thdata_init);
        if (M_first)
        {
            pthread_key_create( &M_nstd_tls_key, 
                    &nstd_buffer_key_destruct );
            M_first = EXFALSE;
        }
        MUTEX_UNLOCK_V(M_thdata_init);
    }
    
    if (NULL==(tls = (nstd_tls_t *)NDRX_MALLOC(sizeof(nstd_tls_t))))
    {
        userlog ("%s: failed to malloc", fn);
        EXFAIL_OUT(ret);
    }
    
    /* do the common init... */
    tls->magic = NSTD_TLS_MAGIG;
    tls->M_threadnr = 0;
    tls->M_nstd_error = 0;
    tls->M_last_err = 0;
    tls->M_last_err_msg[0] = EXEOS;
    
    /* disable log handlers: */
    memset(&tls->threadlog_tp, 0, sizeof(tls->threadlog_tp));
    memset(&tls->requestlog_tp, 0, sizeof(tls->requestlog_tp));
    
    memset(&tls->threadlog_ndrx, 0, sizeof(tls->threadlog_ndrx));
    memset(&tls->requestlog_ndrx, 0, sizeof(tls->requestlog_ndrx));
    
    memset(&tls->threadlog_ubf, 0, sizeof(tls->threadlog_ubf));
    memset(&tls->requestlog_ubf, 0, sizeof(tls->requestlog_ubf));
    
    tls->threadlog_tp.level = EXFAIL;
    tls->requestlog_tp.level = EXFAIL;
    tls->threadlog_ndrx.level = EXFAIL;
    tls->requestlog_ndrx.level = EXFAIL;
    tls->threadlog_ubf.level = EXFAIL;
    tls->requestlog_ubf.level = EXFAIL;
    
    tls->threadlog_tp.flags = LOG_FACILITY_TP_THREAD;
    tls->requestlog_tp.flags = LOG_FACILITY_TP_REQUEST;
    tls->threadlog_ndrx.flags = LOG_FACILITY_NDRX_THREAD;
    tls->requestlog_ndrx.flags = LOG_FACILITY_NDRX_REQUEST;
    tls->threadlog_ubf.flags = LOG_FACILITY_UBF_THREAD;
    tls->requestlog_ubf.flags = LOG_FACILITY_UBF_REQUEST;
    
    tls->threadlog_tp.code = LOG_CODE_TP_THREAD;
    tls->requestlog_tp.code = LOG_CODE_TP_REQUEST;
    tls->threadlog_ndrx.code = LOG_CODE_NDRX_THREAD;
    tls->requestlog_ndrx.code = LOG_CODE_NDRX_REQUEST;
    tls->threadlog_ndrx.code = LOG_CODE_UBF_THREAD;
    tls->requestlog_ndrx.code = LOG_CODE_UBF_REQUEST;
    
    NDRX_STRCPY_SAFE(tls->threadlog_ubf.module, "UBF ");
    NDRX_STRCPY_SAFE(tls->threadlog_ndrx.module, "NDRX");
    NDRX_STRCPY_SAFE(tls->threadlog_tp.module, "USER");
   
    NDRX_STRCPY_SAFE(tls->requestlog_ubf.module, "UBF ");
    NDRX_STRCPY_SAFE(tls->requestlog_ndrx.module, "NDRX");
    NDRX_STRCPY_SAFE(tls->requestlog_tp.module, "USER");
    
    tls->user_field_1 = 0;
    
    pthread_mutex_init(&tls->mutex, NULL);
    
    /* set callback, when thread dies, we need to get the destructor 
     * to be called
     */
    if (auto_destroy)
    {
        tls->is_auto = EXTRUE;
        pthread_setspecific( M_nstd_tls_key, (void *)tls );
    }
    else
    {
        tls->is_auto = EXFALSE;
    }
    
    if (auto_set)
    {
        ndrx_nstd_tls_set(tls);
    }
    
out:

    if (EXSUCCEED!=ret && NULL!=tls)
    {
        ndrx_nstd_tls_free((char *)tls);
    }

    return (void *)tls;
}

