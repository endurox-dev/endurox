/* 
 * @brief Globals/TLS for libubf
 *
 * @file ubf_tls.c
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
#include <ubf_tls.h>
#include <string.h>
#include "thlock.h"
#include "userlog.h"
#include <ndebug.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
__thread ubf_tls_t *G_ubf_tls = NULL; /* single place for library TLS storage */
/*---------------------------Statics------------------------------------*/
exprivate pthread_key_t M_ubf_tls_key;
MUTEX_LOCKDECL(M_thdata_init);
exprivate int M_first = EXTRUE;
/*---------------------------Prototypes---------------------------------*/
exprivate void ubf_buffer_key_destruct( void *value );

/**
 * Thread destructor
 * @param value this is malloced thread TLS
 */
exprivate void ubf_buffer_key_destruct( void *value )
{
    ndrx_ubf_tls_free((void *)value);
}

/**
 * Unlock, unset G_ubf_tls, return pointer to G_ubf_tls
 * @return 
 */
expublic void * ndrx_ubf_tls_get(void)
{
    ubf_tls_t *tmp = G_ubf_tls;
    
    G_ubf_tls = NULL;
    
    if (NULL!=tmp)
    {
        /*
         * Unset the destructor
         */
        if (tmp->is_auto)
        {
            pthread_setspecific( M_ubf_tls_key, NULL );
        }

        /* unlock object */
        MUTEX_UNLOCK_V(tmp->mutex);
    }
    
    return (void *)tmp;
}

/**
 * Get the lock & set the G_ubf_tls to this one
 * @param tls
 */
expublic int ndrx_ubf_tls_set(void *data)
{
    ubf_tls_t *tls = (ubf_tls_t *)data;
    
    if (NULL!=tls)
    {
        /* extra control... */
        if (UBF_TLS_MAGIG!=tls->magic)
        {
            userlog("ubf_tls_set: invalid magic! expected: %x got %x", 
                    UBF_TLS_MAGIG, tls->magic);
        }

        /* Lock the object */
        MUTEX_LOCK_V(tls->mutex);

        G_ubf_tls = tls;

        /*
         * Destruct automatically if it was auto-tls 
         */
        if (tls->is_auto)
        {
            pthread_setspecific( M_ubf_tls_key, (void *)tls );
        }
    }
    else
    {
        G_ubf_tls = NULL;
    }
    
    return EXSUCCEED;
    
}

/**
 * Free up the TLS data
 * @param tls
 * @return 
 */
expublic void ndrx_ubf_tls_free(void *data)
{
    if (NULL!=data)
    {
        if (data==G_ubf_tls)
        {
            if (G_ubf_tls->is_auto)
            {
                pthread_setspecific( M_ubf_tls_key, NULL );
            }
            G_ubf_tls=NULL;
        }
        NDRX_FREE((char*)data);
    }
}

/**
 * Get the lock & init the data
 * @param auto_destroy if set to 1 then when tried exits, thread data will be made free
 * @return 
 */
expublic void * ndrx_ubf_tls_new(int auto_destroy, int auto_set)
{
    int ret = EXSUCCEED;
    ubf_tls_t *tls  = NULL;
    char fn[] = "ubf_context_new";
    
    /* init they key storage */
    if (M_first)
    {
        MUTEX_LOCK_V(M_thdata_init);
        if (M_first)
        {
            pthread_key_create( &M_ubf_tls_key, 
                    &ubf_buffer_key_destruct );
            M_first = EXFALSE;
        }
        MUTEX_UNLOCK_V(M_thdata_init);
    }
    
    if (NULL==(tls = (ubf_tls_t *)NDRX_MALLOC(sizeof(ubf_tls_t))))
    {
        userlog ("%s: failed to malloc", fn);
        EXFAIL_OUT(ret);
    }
    
    /* do the common init... */
    tls->magic = UBF_TLS_MAGIG;
    
    tls->tbuf_s =0;
    tls->tbuf_l =0;
    tls->tbuf_c =0;
    tls->tbuf_f =0.0f;
    tls->tbuf_d =0.0f;
    
    tls->str_buf_ptr = NULL;
    tls->str_dat_len = 0;
     
    tls->carray_buf_ptr= NULL;
    tls->carray_dat_len = 0;
    
    tls->M_ubf_error_msg_buf[0]= EXEOS;
    tls->M_ubf_error = BMINVAL;
    
    
    pthread_mutex_init(&tls->mutex, NULL);
    
    /* set callback, when thread dies, we need to get the destructor 
     * to be called
     */
    if (auto_destroy)
    {
        tls->is_auto = EXTRUE;
        pthread_setspecific( M_ubf_tls_key, (void *)tls );
    }
    else
    {
        tls->is_auto = EXFALSE;
    }
    
    if (auto_set)
    {
        ndrx_ubf_tls_set(tls);
    }
    
out:

    if (EXSUCCEED!=ret && NULL!=tls)
    {
        ndrx_ubf_tls_free((char *)tls);
    }

    return (void *)tls;
}

