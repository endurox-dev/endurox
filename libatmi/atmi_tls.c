/* 
** Globals/TLS for libatmi
**
** @file atmi_tls.c
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
#include <stdio.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <atmi_tls.h>
#include <string.h>
#include "thlock.h"
#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
__thread atmi_tls_t *G_atmi_tls = NULL; /* single place for library TLS storage */
/*---------------------------Statics------------------------------------*/
private pthread_key_t M_atmi_tls_key;
private pthread_key_t M_atmi_switch_key; /* switch the structure */

MUTEX_LOCKDECL(M_thdata_init);
private int M_first = TRUE;
/*---------------------------Prototypes---------------------------------*/
private void atmi_buffer_key_destruct( void *value );

/**
 * Thread destructor
 * @param value this is malloced thread TLS
 */
private void atmi_buffer_key_destruct( void *value )
{
    ndrx_atmi_tls_free((void *)value);
}

/**
 * Unlock, unset G_atmi_tls, return pointer to G_atmi_tls
 * @return 
 */
public void * ndrx_atmi_tls_get(long priv_flags)
{
    atmi_tls_t *tmp = G_atmi_tls;
    char *fn = "ndrx_atmi_tls_get";
    if (NULL!=tmp)
    {
        /*
         * Unset the destructor
         */
        if (tmp->is_auto)
        {
            pthread_setspecific( M_atmi_tls_key, NULL );
        }

        /* suspend the transaction if any in progress: similar to tpsrvgetctxdata() */
#ifdef NDRX_OAPI_DEBUG
        
        NDRX_LOG(log_debug, "%s: G_atmi_xa_curtx.txinfo: %p", 
                    fn, tmp->G_atmi_xa_curtx.txinfo);
#endif
        
        if (priv_flags & CTXT_PRIV_TRAN)
        {
            tmp->global_tx_suspended = FALSE;
            
            if (tmp->G_atmi_xa_curtx.txinfo)
            {
                /* WARNING !!! WARNING !!!
                 * GLOBAL Transaction is suspended in thread
                 * and not in the context data. Thus if we want o-api to work,
                 * needs some kind of flag that global transaction was suspended
                 * in thread.
                 * Probably we want to save the tranid in thread..
                 */

                if (SUCCEED!=tpsuspend(&tmp->tranid, 0))
                {
                    userlog("ndrx_atmi_tls_get: Failed to suspend transaction: [%s]", 
                            tpstrerror(tperrno));

#if 0
                    Nothing to do here! it will fail next time when user
                    will try to do some DB operation...
                    MUTEX_UNLOCK_V(tmp->mutex);

                    ndrx_atmi_tls_free(tmp);
                    /* fail it. */
                    tmp = NULL;
                    goto out;
#endif
                }
                else
                {
                    tmp->global_tx_suspended = TRUE;
                }

                /* Disable curren thread TLS... */
                G_atmi_tls = NULL;
            }
        }

        /* unlock object */
        MUTEX_UNLOCK_V(tmp->mutex);
    }
out:
    return (void *)tmp;
}

/**
 * Get the lock & set the G_atmi_tls to this one
 * @param tls
 */
public int ndrx_atmi_tls_set(void *data, int flags, long priv_flags)
{
    int ret = SUCCEED;
    atmi_tls_t *tls = (atmi_tls_t *)data;
    char *fn = "ndrx_atmi_tls_set";
   
    if (NULL!=tls)
    {
        /* extra control... */
        if (ATMI_TLS_MAGIG!=tls->magic)
        {
            userlog("atmi_tls_set: invalid magic! expected: %x got %x", 
                    ATMI_TLS_MAGIG, tls->magic);
        }

        /* Lock the object 
         * TODO: We need PTHREAD_MUTEX_RECURSIVE
         * so that we can acquire multiple locks (for example server process calls 
         * back tpsrvinit or done which internally may acquire some more locks!
         */
        MUTEX_LOCK_V(tls->mutex);


        /* Add the additional flags to the user. */
        tls->G_last_call.sysflags |= flags;
        
#ifdef NDRX_OAPI_DEBUG
        NDRX_LOG(log_debug, "%s: G_atmi_xa_curtx.txinfo: %p", 
                    fn, tls->G_atmi_xa_curtx.txinfo);
#endif
        /* Resume the transaction only if flag is set
         * For Object API some of the operations do not request transaction to
         * be open.
         */
        if (priv_flags & CTXT_PRIV_TRAN)
        {
            if(tls->global_tx_suspended)
            {   
                if (SUCCEED!=tpresume(&tls->tranid, 0))
                {
                    userlog("Failed to resume transaction: [%s]", tpstrerror(tperrno));
                }
                else
                {
                    tls->global_tx_suspended = FALSE;
                }
            }
        }

        G_atmi_tls = tls;

        /*
         * Destruct automatically if it was auto-tls 
         */
        if (tls->is_auto)
        {
            pthread_setspecific( M_atmi_tls_key, (void *)tls );
        }
    }
    else
    {
        G_atmi_tls = NULL;
    }
    
out:
    return ret;
}

/**
 * Free up the TLS data
 * @param tls
 * @return 
 */
public void ndrx_atmi_tls_free(void *data)
{
    if (NULL!=data)
    {
        if (data == G_atmi_tls)
        {
            if (G_atmi_tls->is_auto)
            {
                pthread_setspecific( M_atmi_tls_key, NULL );
            }
            G_atmi_tls = NULL;
        }

        NDRX_FREE((char*)data);
    }
}

/**
 * Get the lock & init the data
 * @param auto_destroy if set to 1 then when tried exits, thread data will be made free
 * @return 
 */
public void * ndrx_atmi_tls_new(int auto_destroy, int auto_set)
{
    int ret = SUCCEED;
    atmi_tls_t *tls  = NULL;
    char fn[] = "atmi_context_new";
    
    /* init they key storage */
    if (M_first)
    {
        MUTEX_LOCK_V(M_thdata_init);
        if (M_first)
        {
            pthread_key_create( &M_atmi_tls_key, 
                    &atmi_buffer_key_destruct );
            M_first = FALSE;
        }
        MUTEX_UNLOCK_V(M_thdata_init);
    }
    
    if (NULL==(tls = (atmi_tls_t *)NDRX_MALLOC(sizeof(atmi_tls_t))))
    {
        userlog ("%s: failed to malloc", fn);
        FAIL_OUT(ret);
    }
    
    /* do the common init... */
    tls->magic = ATMI_TLS_MAGIG;
    
    
    /* init.c */    
    tls->conv_cd=1;/*  first available */
    tls->callseq = 0;
    tls->G_atmi_is_init= 0;/*  Is environment initialised */
    
    /* tpcall.c */
    tls->M_svc_return_code = 0;
    tls->tpcall_first = TRUE;
    tls->tpcall_cd = 0;
    
    /* tperror.c */
    tls->M_atmi_error_msg_buf[0] = EOS;
    tls->M_atmi_error = TPMINVAL;
    tls->M_atmi_reason = NDRX_XA_ERSN_NONE;
    tls->errbuf[0] = EOS;
    tls->is_associated_with_thread = FALSE;
    /* xa.c */
    tls->M_is_curtx_init = FALSE;
    memset(&tls->G_atmi_xa_curtx, 0, sizeof(tls->G_atmi_xa_curtx));
    
    pthread_mutex_init(&tls->mutex, NULL);
    
    /* set callback, when thread dies, we need to get the destructor 
     * to be called
     */
    if (auto_destroy)
    {
        tls->is_auto = TRUE;
        pthread_setspecific( M_atmi_tls_key, (void *)tls );
    }
    else
    {
        tls->is_auto = FALSE;
    }
    
    if (auto_set)
    {
        ndrx_atmi_tls_set(tls, 0, 0);
    }
    
out:

    if (SUCCEED!=ret && NULL!=tls)
    {
        ndrx_atmi_tls_free((char *)tls);
    }

    return (void *)tls;
}

/**
 * Kill the given context,
 * Do we need tpterm here?
 * @param context
 * @param flags
 * @return 
 */
public void _tpfreectxt(TPCONTEXT_T context)
{
    atmi_tls_t * ctx = (atmi_tls_t *)context;
    
    if (NULL!=ctx)
    {
        if (NULL!=ctx->p_nstd_tls)
        {
            ndrx_nstd_tls_free(ctx->p_nstd_tls);
        }
        
        if (NULL!=ctx->p_ubf_tls)
        {
            ndrx_ubf_tls_free(ctx->p_ubf_tls);
        }
        
        ndrx_atmi_tls_free(ctx);
    }
}

/**
 * Internal version of get context
 * @param context
 * @param flags
 * @param priv_flags private flags (for sharing the functionality)
 * @return 
 */
public int _tpsetctxt(TPCONTEXT_T context, long flags, long priv_flags)
{
    int ret = SUCCEED;
    atmi_tls_t * ctx;
    char *fn="_tpsetctxt";
    
#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: %s enter, context: %p, current: %p", fn, 
            context, G_atmi_tls);
    
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)context)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (priv_flags) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (priv_flags) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (priv_flags) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (priv_flags) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (priv_flags) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (priv_flags) & CTXT_PRIV_IGN );
#endif

    
    if (context == TPNULLCONTEXT)
    {
        if (NULL!=G_atmi_tls && G_atmi_tls->is_auto)
        {
            /* free the current thread context data (only in case if it was auto) */
            _tpfreectxt((TPCONTEXT_T)G_atmi_tls);
        }
        /* In case if we are already in NULL context, then go out... */
        goto out; /* we are done. */
    }
    
    ctx = (atmi_tls_t *)context;
    
    if (!(priv_flags & CTXT_PRIV_NOCHK))
    {
        /* have a deep checks */
        if (priv_flags & CTXT_PRIV_ATMI && ATMI_TLS_MAGIG!=ctx->magic)
        {
            _TPset_error_fmt(TPENOENT, "_tpsetctxt: invalid atmi magic: "
                    "expected: %x got %x!", ATMI_TLS_MAGIG, ctx->magic);
            FAIL_OUT(ret);
        }

        if (priv_flags & CTXT_PRIV_NSTD && NULL!=ctx->p_nstd_tls 
                && NSTD_TLS_MAGIG!=ctx->p_nstd_tls->magic)
        {
            _TPset_error_fmt(TPENOENT, "_tpsetctxt: invalid nstd magic: "
                    "expected: %x got %x!", NSTD_TLS_MAGIG, ctx->p_nstd_tls->magic);
            FAIL_OUT(ret);
        }

        if (priv_flags & CTXT_PRIV_UBF && NULL!=ctx->p_ubf_tls 
                && UBF_TLS_MAGIG!=ctx->p_ubf_tls->magic)
        {
            _TPset_error_fmt(TPENOENT, "_tpsetctxt: invalid ubf magic: "
                    "expected: %x got %x!", UBF_TLS_MAGIG, ctx->p_ubf_tls->magic);
            FAIL_OUT(ret);
        }
    }
    
    /* free the current context (with tpterm?) 
     * if one in progress 
     */
    if (!(priv_flags & CTXT_PRIV_IGN) && G_atmi_tls!=ctx && NULL!=G_atmi_tls)
    {
        NDRX_LOG(log_warn, "Free up context %p", G_atmi_tls);
        tpterm();
        tpfreectxt((TPCONTEXT_T)G_atmi_tls);
    }
    
    
    if (priv_flags & CTXT_PRIV_NSTD && NULL!=ctx->p_nstd_tls &&
            SUCCEED!=ndrx_nstd_tls_set((void *)ctx->p_nstd_tls))
    {
        _TPset_error_fmt(TPESYSTEM, "_tpsetctxt: failed to restore libnstd context");
        FAIL_OUT(ret);
    }
    
    if (priv_flags & CTXT_PRIV_UBF &&  NULL!=ctx->p_ubf_tls &&
            SUCCEED!=ndrx_ubf_tls_set((void *)ctx->p_ubf_tls))
    {
        _TPset_error_fmt(TPESYSTEM, "_tpsetctxt: failed to restore libubf context");
        FAIL_OUT(ret);
    }
    
    if (priv_flags & CTXT_PRIV_ATMI)
    {
        if (SUCCEED!=ndrx_atmi_tls_set((void *)ctx, flags, priv_flags))
        {
            _TPset_error_fmt(TPESYSTEM, "_tpsetctxt: failed to restore libatmi context");
            FAIL_OUT(ret);
        }
        
        ctx->is_associated_with_thread = TRUE;
    }
    
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: %s returns %d, context: %p, current: %p", fn, 
            ret, context, G_atmi_tls);
#endif
    return ret;
}

/**
 * Internal version of get full context
 * This disconnects current thread from TLS.
 * @param flags
 * @return 
 */
public int _tpgetctxt(TPCONTEXT_T *context, long flags, long priv_flags)
{
    int ret = TPMULTICONTEXTS; /* default */
    atmi_tls_t * ctx;
    char *fn="_tpgetctxt";
    
#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: %s enter, context: %p, current: %p", 
                fn, *context, G_atmi_tls);
    
    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (priv_flags) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (priv_flags) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (priv_flags) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (priv_flags) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (priv_flags) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (priv_flags) & CTXT_PRIV_IGN );
#endif
    if (NULL==context)
    {
        _TPset_error_msg(TPEINVAL, "_tpgetctxt: context must not be NULL!");
        FAIL_OUT(ret);
    }
    
    if (0!=flags)
    {
        _TPset_error_msg(TPEINVAL, "_tpgetctxt: flags must be 0!");
        FAIL_OUT(ret);
    }
    
    /* if not using ATMI, then use existing context object */
    if (priv_flags & CTXT_PRIV_ATMI)
    {
        ctx = (atmi_tls_t *)ndrx_atmi_tls_get(priv_flags);
        
        /* Dis associate */
        ctx->is_associated_with_thread = FALSE;
    }
    else   
    {
        ctx = (TPCONTEXT_T)*context;
    }
    
    if (NULL!=ctx)
    {
        if (priv_flags & CTXT_PRIV_NSTD)
        {
            ctx->p_nstd_tls = ndrx_nstd_tls_get();
        }
        
        if (priv_flags & CTXT_PRIV_UBF)
        {
            ctx->p_ubf_tls = ndrx_ubf_tls_get();
        }
    }
    
    if (priv_flags & CTXT_PRIV_ATMI)
    {
        *context = (TPCONTEXT_T)ctx;
    }
    
    if (NULL==ctx)
    {
        ret = TPNULLCONTEXT;
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: %s returns %d, context: %p, current: %p", fn, 
            ret, *context, G_atmi_tls);
#endif

    return ret;
}
