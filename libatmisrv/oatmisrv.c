/* 
** ATMI Server Level Object API code (auto-generated)
**
**  oatmisrv.c
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
 * Object-API wrapper for tpadvertise_full() - Auto generated.
 */
public int Otpadvertise_full(TPCONTEXT_T *p_ctxt, char *svc_nm, void (*p_func)(TPSVCINFO *), char *fn_nm) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpadvertise_full() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpadvertise_full() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpadvertise_full(svc_nm, p_func, fn_nm);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpadvertise_full() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpreturn() - Auto generated.
 */
public void Otpreturn(TPCONTEXT_T *p_ctxt, int rval, long rcode, char *data, long len, long flags) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpreturn() failed to set context");
    }
    
    tpreturn(rval, rcode, data, len, flags);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpreturn() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tpunadvertise() - Auto generated.
 */
public int Otpunadvertise(TPCONTEXT_T *p_ctxt, char *svcname) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpunadvertise() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpunadvertise() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpunadvertise(svcname);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpunadvertise() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpforward() - Auto generated.
 */
public void Otpforward(TPCONTEXT_T *p_ctxt, char *svc, char *data, long len, long flags) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpforward() failed to set context");
    }
    
    tpforward(svc, data, len, flags);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpforward() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tpsrvgetctxdata() - Auto generated.
 */
public char * Otpsrvgetctxdata(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsrvgetctxdata() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsrvgetctxdata() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsrvgetctxdata();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsrvgetctxdata() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpsrvsetctxdata() - Auto generated.
 */
public int Otpsrvsetctxdata(TPCONTEXT_T *p_ctxt, char *data, long flags) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsrvsetctxdata() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpsrvsetctxdata() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpsrvsetctxdata(data, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpsrvsetctxdata() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpcontinue() - Auto generated.
 */
public void Otpcontinue(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpcontinue() failed to set context");
    }
    
    tpcontinue();

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpcontinue() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tpext_addpollerfd() - Auto generated.
 */
public int Otpext_addpollerfd(TPCONTEXT_T *p_ctxt, int fd, uint32_t events, void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1)) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addpollerfd() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_addpollerfd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_addpollerfd(fd, events, ptr1, p_pollevent);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addpollerfd() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpext_delpollerfd() - Auto generated.
 */
public int Otpext_delpollerfd(TPCONTEXT_T *p_ctxt, int fd) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delpollerfd() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_delpollerfd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_delpollerfd(fd);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delpollerfd() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpext_addperiodcb() - Auto generated.
 */
public int Otpext_addperiodcb(TPCONTEXT_T *p_ctxt, int secs, int (*p_periodcb)(void)) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addperiodcb() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_addperiodcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_addperiodcb(secs, p_periodcb);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addperiodcb() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpext_delperiodcb() - Auto generated.
 */
public int Otpext_delperiodcb(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delperiodcb() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_delperiodcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_delperiodcb();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delperiodcb() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpext_addb4pollcb() - Auto generated.
 */
public int Otpext_addb4pollcb(TPCONTEXT_T *p_ctxt, int (*p_b4pollcb)(void)) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addb4pollcb() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_addb4pollcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_addb4pollcb(p_b4pollcb);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_addb4pollcb() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpext_delb4pollcb() - Auto generated.
 */
public int Otpext_delb4pollcb(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delb4pollcb() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpext_delb4pollcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpext_delb4pollcb();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpext_delb4pollcb() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpgetsrvid() - Auto generated.
 */
public int Otpgetsrvid(TPCONTEXT_T *p_ctxt) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpgetsrvid() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tpgetsrvid() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tpgetsrvid();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tpgetsrvid() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for ndrx_main() - Auto generated.
 */
public int Ondrx_main(TPCONTEXT_T *p_ctxt, int argc, char **argv) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_main() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_main() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = ndrx_main(argc, argv);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_main() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


