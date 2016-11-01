/* 
** ATMI Object API code (auto-generated)
**
**  oatmi.c
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
 * Object-API wrapper for tpacall() - Auto generated.
 */
public int Otpacall(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpacall() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpacall() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpacall(svc, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpacall() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpalloc() - Auto generated.
 */
public char * Otpalloc(TPCONTEXT_T *p_ctxt, char *type, char *subtype, long size) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpalloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpalloc(type, subtype, size);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpcall() - Auto generated.
 */
public int Otpcall(TPCONTEXT_T *p_ctxt, char *svc, char *idata, long ilen, char **odata, long *olen, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcall() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpcall() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpcall(svc, idata, ilen, odata, olen, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcall() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpcancel() - Auto generated.
 */
public int Otpcancel(TPCONTEXT_T *p_ctxt, int cd) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcancel() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpcancel() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpcancel(cd);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcancel() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpconnect() - Auto generated.
 */
public int Otpconnect(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpconnect() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpconnect() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpconnect(svc, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpconnect() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpdiscon() - Auto generated.
 */
public int Otpdiscon(TPCONTEXT_T *p_ctxt, int cd) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdiscon() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpdiscon() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpdiscon(cd);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdiscon() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpfree() - Auto generated.
 */
public void Otpfree(TPCONTEXT_T *p_ctxt, char *ptr) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpfree() failed to set context");
    }
    
    tpfree(ptr);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpfree() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tpgetrply() - Auto generated.
 */
public int Otpgetrply(TPCONTEXT_T *p_ctxt, int *cd, char **data, long *len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetrply() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpgetrply() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpgetrply(cd, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetrply() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tprealloc() - Auto generated.
 */
public char * Otprealloc(TPCONTEXT_T *p_ctxt, char *ptr, long size) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tprealloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tprealloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tprealloc(ptr, size);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tprealloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tprecv() - Auto generated.
 */
public int Otprecv(TPCONTEXT_T *p_ctxt, int cd, char **data, long *len, long flags, long *revent) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tprecv() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tprecv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tprecv(cd, data, len, flags, revent);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tprecv() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpsend() - Auto generated.
 */
public int Otpsend(TPCONTEXT_T *p_ctxt, int cd, char *data, long len, long flags, long *revent) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpsend() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsend() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsend(cd, data, len, flags, revent);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpsend() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tptypes() - Auto generated.
 */
public long Otptypes(TPCONTEXT_T *p_ctxt, char *ptr, char *type, char *subtype) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tptypes() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tptypes() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tptypes(ptr, type, subtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tptypes() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpabort() - Auto generated.
 */
public int Otpabort(TPCONTEXT_T *p_ctxt, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpabort() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpabort() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpabort(flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpabort() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpbegin() - Auto generated.
 */
public int Otpbegin(TPCONTEXT_T *p_ctxt, unsigned long timeout, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpbegin() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpbegin() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpbegin(timeout, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpbegin() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpcommit() - Auto generated.
 */
public int Otpcommit(TPCONTEXT_T *p_ctxt, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcommit() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpcommit() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpcommit(flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpcommit() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpconvert() - Auto generated.
 */
public int Otpconvert(TPCONTEXT_T *p_ctxt, char *strrep, char *binrep, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpconvert() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpconvert() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpconvert(strrep, binrep, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpconvert() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpsuspend() - Auto generated.
 */
public int Otpsuspend(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsuspend() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsuspend() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsuspend(tranid, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsuspend() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpresume() - Auto generated.
 */
public int Otpresume(TPCONTEXT_T *p_ctxt, TPTRANID *tranid, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpresume() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpresume() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpresume(tranid, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpresume() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpopen() - Auto generated.
 */
public int Otpopen(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpopen() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpopen() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpopen();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpopen() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpclose() - Auto generated.
 */
public int Otpclose(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpclose() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpclose() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpclose();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpclose() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpgetlev() - Auto generated.
 */
public int Otpgetlev(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetlev() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpgetlev() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpgetlev();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetlev() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpstrerror() - Auto generated.
 */
public char * Otpstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpstrerror() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpstrerror() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpstrerror(err);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpstrerror() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpgetnodeid() - Auto generated.
 */
public long Otpgetnodeid(TPCONTEXT_T *p_ctxt) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpgetnodeid() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpgetnodeid() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpgetnodeid();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpgetnodeid() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpsubscribe() - Auto generated.
 */
public long Otpsubscribe(TPCONTEXT_T *p_ctxt, char *eventexpr, char *filter, TPEVCTL *ctl, long flags) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsubscribe() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsubscribe() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsubscribe(eventexpr, filter, ctl, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsubscribe() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpunsubscribe() - Auto generated.
 */
public int Otpunsubscribe(TPCONTEXT_T *p_ctxt, long subscription, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpunsubscribe() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpunsubscribe() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpunsubscribe(subscription, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpunsubscribe() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tppost() - Auto generated.
 */
public int Otppost(TPCONTEXT_T *p_ctxt, char *eventname, char *data, long len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tppost() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tppost() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tppost(eventname, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tppost() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for _exget_tperrno_addr() - Auto generated.
 */
public int * O_exget_tperrno_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;
    int * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _exget_tperrno_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! _exget_tperrno_addr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = _exget_tperrno_addr();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _exget_tperrno_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for _exget_tpurcode_addr() - Auto generated.
 */
public long * O_exget_tpurcode_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;
    long * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _exget_tpurcode_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! _exget_tpurcode_addr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = _exget_tpurcode_addr();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _exget_tpurcode_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpinit() - Auto generated.
 */
public int Otpinit(TPCONTEXT_T *p_ctxt, TPINIT *tpinfo) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpinit() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpinit() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpinit(tpinfo);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpinit() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpterm() - Auto generated.
 */
public int Otpterm(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpterm() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpterm() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpterm();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpterm() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpjsontoubf() - Auto generated.
 */
public int Otpjsontoubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpjsontoubf() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpjsontoubf() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpjsontoubf(p_ub, buffer);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpjsontoubf() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpubftojson() - Auto generated.
 */
public int Otpubftojson(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *buffer, int bufsize) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpubftojson() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpubftojson() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpubftojson(p_ub, buffer, bufsize);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpubftojson() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpenqueue() - Auto generated.
 */
public int Otpenqueue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char *data, long len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpenqueue() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpenqueue() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpenqueue(qspace, qname, ctl, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpenqueue() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpdequeue() - Auto generated.
 */
public int Otpdequeue(TPCONTEXT_T *p_ctxt, char *qspace, char *qname, TPQCTL *ctl, char **data, long *len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdequeue() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpdequeue() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpdequeue(qspace, qname, ctl, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdequeue() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpenqueueex() - Auto generated.
 */
public int Otpenqueueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char *data, long len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpenqueueex() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpenqueueex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpenqueueex(nodeid, srvid, qname, ctl, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpenqueueex() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpdequeueex() - Auto generated.
 */
public int Otpdequeueex(TPCONTEXT_T *p_ctxt, short nodeid, short srvid, char *qname, TPQCTL *ctl, char **data, long *len, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdequeueex() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpdequeueex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpdequeueex(nodeid, srvid, qname, ctl, data, len, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpdequeueex() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpgetctxt() - Auto generated.
 */
public int Otpgetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T *context, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetctxt() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpgetctxt() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpgetctxt(context, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpgetctxt() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpsetctxt() - Auto generated.
 */
public int Otpsetctxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpsetctxt() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsetctxt() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsetctxt(context, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
        {
            userlog("ERROR! tpsetctxt() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpfreectxt() - Auto generated.
 */
public void Otpfreectxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpfreectxt() failed to set context");
    }
    
    tpfreectxt(context);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpfreectxt() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tplogsetreqfile() - Auto generated.
 */
public int Otplogsetreqfile(TPCONTEXT_T *p_ctxt, char **data, char *filename, char *filesvc) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogsetreqfile() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogsetreqfile() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tplogsetreqfile(data, filename, filesvc);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogsetreqfile() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tploggetbufreqfile() - Auto generated.
 */
public int Otploggetbufreqfile(TPCONTEXT_T *p_ctxt, char *data, char *filename, int bufsize) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetbufreqfile() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tploggetbufreqfile() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tploggetbufreqfile(data, filename, bufsize);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetbufreqfile() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tplogdelbufreqfile() - Auto generated.
 */
public int Otplogdelbufreqfile(TPCONTEXT_T *p_ctxt, char *data) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdelbufreqfile() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogdelbufreqfile() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tplogdelbufreqfile(data);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdelbufreqfile() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tplogprintubf() - Auto generated.
 */
public void Otplogprintubf(TPCONTEXT_T *p_ctxt, int lev, char *title, UBFH *p_ub) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogprintubf() failed to set context");
    }
    
    tplogprintubf(lev, title, p_ub);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogprintubf() failed to get context");
    }
out:    
    return;
}


