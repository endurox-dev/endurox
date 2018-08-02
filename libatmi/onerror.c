/* 
 * @brief Standard library error handling
 *   onerror.c
 *
 * @file onerror.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_tls.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Object-API wrapper for Nstrerror() - Auto generated.
 */
expublic char * ONstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Nstrerror() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Nstrerror() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Nstrerror() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Nstrerror(err);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Nstrerror() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Nstrerror() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for _Nget_Nerror_addr() - Auto generated.
 */
expublic int * O_Nget_Nerror_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;
    int * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: _Nget_Nerror_addr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Nget_Nerror_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! _Nget_Nerror_addr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = _Nget_Nerror_addr();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Nget_Nerror_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: _Nget_Nerror_addr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


