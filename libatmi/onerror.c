/* 
** Standard library error handling
**
**  onerror.c
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
public char * ONstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Nstrerror() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
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
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Nstrerror() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for _Nget_Nerror_addr() - Auto generated.
 */
public int * O_Nget_Nerror_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;
    int * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Nget_Nerror_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
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
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Nget_Nerror_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


