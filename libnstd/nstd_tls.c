/* 
** Globals/TLS for libnstd
** All stuff here must work. thus if something is very bad, we ill print to stderr.
**
** @file nstd_tls.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <ndrstandard.h>
#include <nstd_tls.h>
#include <string.h>
#include "thlock.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
__thread nstd_tls_t *G_nstd_tls = NULL; /* single place for library TLS storage */
/*---------------------------Statics------------------------------------*/
private pthread_key_t nstd_buffer_key;
/*---------------------------Prototypes---------------------------------*/
private void nstd_buffer_key_destruct( void *value );

/**
 * Thread destructor
 * @param value
 */
private void nstd_buffer_key_destruct( void *value )
{
    if (NULL!=G_nstd_tls)
    {
        nstd_tls_free(G_nstd_tls);
        G_nstd_tls = NULL;
    }
}

/**
 * Unlock, unset G_nstd_tls, return pointer to G_nstd_tls
 * @return 
 */
public nstd_tls_t * nstd_tls_get(void)
{
    nstd_tls_t *tmp = G_nstd_tls;
    
    G_nstd_tls = NULL;
    
    /* unlock tmp */
    MUTEX_UNLOCK_V(tmp->mutex);
    
    return tmp;
}

/**
 * Get the lock & set the G_nstd_tls to this one
 * @param tls
 */
public void nstd_tls_set(nstd_tls_t *tls)
{
    /* unlock tmp */
    MUTEX_LOCK_V(tls->mutex);
    
    G_nstd_tls = tls;
}

/**
 * Free up the TLS data
 * @param tls
 * @return 
 */
public int nstd_tls_free(nstd_tls_t *tls)
{
    free((char*)tls);
}

/**
 * Get the lock & init the data
 * @param auto_destroy if set to 1 then when tried exits, thread data will be made free
 * @return 
 */
public nstd_tls_t * nstd_tls_new(int auto_destroy, int auto_set)
{
    int ret = SUCCEED;
    nstd_tls_t *tls  = NULL;
    char fn[] = "nstd_context_new";
    
    if (NULL==(tls = (nstd_tls_t *)malloc(sizeof(nstd_tls_t))))
    {
        userlog ("%s: failed to malloc", fn);
        FAIL_OUT(ret);
    }
    
    /* do the common init... */
    tls->M_threadnr = 0;
    tls->M_nstd_error = 0;
    tls->M_last_err = 0;
    tls->M_last_err_msg[0] = EOS;
    
    pthread_mutex_init(&tls->mutex, NULL);
    
    /* set callback, when thread dies, we need to get the destructor 
     * to be called
     */
    if (auto_destroy)
    {
         pthread_key_create( &nstd_buffer_key, 
                 &nstd_buffer_key_destruct );
    }
    
    if (auto_set)
    {
        nstd_tls_set(tls);
    }
    
out:

    if (SUCCEED!=ret && NULL!=tls)
    {
        nstd_tls_free(tls);
    }

    return tls;
}

