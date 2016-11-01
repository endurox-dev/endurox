/* 
** UBF Object API code (auto-generated)
**
**  oubf.c
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
 * Object-API wrapper for _Bget_Ferror_addr() - Auto generated.
 */
public int * O_Bget_Ferror_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;
    int * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Bget_Ferror_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! _Bget_Ferror_addr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = _Bget_Ferror_addr();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Bget_Ferror_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Blen() - Auto generated.
 */
public int OBlen(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Blen() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Blen() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Blen(p_ub, bfldid, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Blen() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBadd() - Auto generated.
 */
public int OCBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBadd() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBadd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBadd(p_ub, bfldid, buf, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBadd() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBchg() - Auto generated.
 */
public int OCBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBchg() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBchg() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBchg(p_ub, bfldid, occ, buf, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBchg() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBget() - Auto generated.
 */
public int OCBget(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBget() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBget() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBget(p_ub, bfldid, occ, buf, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBget() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bdel() - Auto generated.
 */
public int OBdel(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdel() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bdel() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bdel(p_ub, bfldid, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdel() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bpres() - Auto generated.
 */
public int OBpres(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpres() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bpres() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bpres(p_ub, bfldid, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpres() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bproj() - Auto generated.
 */
public int OBproj(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID * fldlist) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bproj() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bproj() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bproj(p_ub, fldlist);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bproj() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bprojcpy() - Auto generated.
 */
public int OBprojcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprojcpy() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bprojcpy() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bprojcpy(p_ub_dst, p_ub_src, fldlist);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprojcpy() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfldid() - Auto generated.
 */
public BFLDID OBfldid(TPCONTEXT_T *p_ctxt, char *fldnm) 
{
    BFLDID ret = BBADFLDID;
    int did_set = FALSE;
 
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldid() failed to set context");
            ret = BBADFLDID;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfldid() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ret = Bfldid(fldnm);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldid() failed to get context");
            ret = BBADFLDID;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfname() - Auto generated.
 */
public char * OBfname(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfname() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfname() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfname(bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfname() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bcpy() - Auto generated.
 */
public int OBcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcpy() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bcpy() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bcpy(p_ub_dst, p_ub_src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcpy() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bchg() - Auto generated.
 */
public int OBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchg() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bchg() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bchg(p_ub, bfldid, occ, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchg() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Binit() - Auto generated.
 */
public int OBinit(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDLEN len) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Binit() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Binit() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Binit(p_ub, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Binit() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bnext() - Auto generated.
 */
public int OBnext(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bnext() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bnext(p_ub, bfldid, occ, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bget() - Auto generated.
 */
public int OBget(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bget() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bget() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bget(p_ub, bfldid, occ, buf, buflen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bget() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bboolco() - Auto generated.
 */
public char * OBboolco(TPCONTEXT_T *p_ctxt, char * expr) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolco() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolco() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bboolco(expr);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolco() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfind() - Auto generated.
 */
public char * OBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfind() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfind() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfind(p_ub, bfldid, occ, p_len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfind() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bboolev() - Auto generated.
 */
public int OBboolev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolev() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolev() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bboolev(p_ub, tree);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolev() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfloatev() - Auto generated.
 */
public double OBfloatev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree) 
{
    double ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfloatev() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfloatev() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfloatev(p_ub, tree);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfloatev() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Badd() - Auto generated.
 */
public int OBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badd() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Badd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Badd(p_ub, bfldid, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badd() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for B_error() - Auto generated.
 */
public void OB_error(TPCONTEXT_T *p_ctxt, char *str) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! B_error() failed to set context");
    }
    
    B_error(str);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! B_error() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for Bstrerror() - Auto generated.
 */
public char * OBstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bstrerror() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bstrerror() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bstrerror(err);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bstrerror() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bmkfldid() - Auto generated.
 */
public BFLDID OBmkfldid(TPCONTEXT_T *p_ctxt, int fldtype, BFLDID bfldid) 
{
    BFLDID ret = BBADFLDID;
    int did_set = FALSE;
 
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bmkfldid() failed to set context");
            ret = BBADFLDID;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bmkfldid() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ret = Bmkfldid(fldtype, bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bmkfldid() failed to get context");
            ret = BBADFLDID;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Boccur() - Auto generated.
 */
public BFLDOCC OBoccur(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid) 
{
    BFLDOCC ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Boccur() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Boccur() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Boccur(p_ub, bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Boccur() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bused() - Auto generated.
 */
public long OBused(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bused() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bused() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bused(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bused() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfldtype() - Auto generated.
 */
public int OBfldtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldtype() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfldtype() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfldtype(bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldtype() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bdelall() - Auto generated.
 */
public int OBdelall(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelall() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bdelall() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bdelall(p_ub, bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelall() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bdelete() - Auto generated.
 */
public int OBdelete(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldlist) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelete() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bdelete() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bdelete(p_ub, fldlist);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelete() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfldno() - Auto generated.
 */
public BFLDOCC OBfldno(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    BFLDOCC ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldno() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfldno() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfldno(bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldno() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bunused() - Auto generated.
 */
public long OBunused(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunused() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bunused() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bunused(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunused() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bsizeof() - Auto generated.
 */
public long OBsizeof(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsizeof() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bsizeof() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bsizeof(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsizeof() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Btype() - Auto generated.
 */
public char * OBtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btype() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Btype() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Btype(bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btype() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfree() - Auto generated.
 */
public int OBfree(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfree() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfree() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfree(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfree() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Balloc() - Auto generated.
 */
public UBFH * OBalloc(TPCONTEXT_T *p_ctxt, BFLDOCC f, BFLDLEN v) 
{
    int did_set = FALSE;
    UBFH * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Balloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Balloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Balloc(f, v);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Balloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Brealloc() - Auto generated.
 */
public UBFH * OBrealloc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDOCC f, BFLDLEN v) 
{
    int did_set = FALSE;
    UBFH * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brealloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Brealloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Brealloc(p_ub, f, v);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brealloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bupdate() - Auto generated.
 */
public int OBupdate(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bupdate() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bupdate() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bupdate(p_ub_dst, p_ub_src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bupdate() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bconcat() - Auto generated.
 */
public int OBconcat(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bconcat() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bconcat() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bconcat(p_ub_dst, p_ub_src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bconcat() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBfind() - Auto generated.
 */
public char * OCBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfind() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBfind() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBfind(p_ub, bfldid, occ, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfind() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBgetalloc() - Auto generated.
 */
public char * OCBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBgetalloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBgetalloc(p_ub, bfldid, occ, usrtype, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for CBfindocc() - Auto generated.
 */
public BFLDOCC OCBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len, int usrtype) 
{
    BFLDOCC ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindocc() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBfindocc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBfindocc(p_ub, bfldid, buf, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindocc() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfindocc() - Auto generated.
 */
public BFLDOCC OBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len) 
{
    BFLDOCC ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindocc() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfindocc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfindocc(p_ub, bfldid, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindocc() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bgetalloc() - Auto generated.
 */
public char * OBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgetalloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgetalloc(p_ub, bfldid, occ, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfindlast() - Auto generated.
 */
public char * OBfindlast(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid,BFLDOCC *occ, BFLDLEN *len) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindlast() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfindlast() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfindlast(p_ub, bfldid, occ, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindlast() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bgetlast() - Auto generated.
 */
public int OBgetlast(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,BFLDOCC *occ, char *buf, BFLDLEN *len) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetlast() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgetlast() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgetlast(p_ub, bfldid, occ, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetlast() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bprint() - Auto generated.
 */
public int OBprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprint() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bprint() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bprint(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprint() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfprint() - Auto generated.
 */
public int OBfprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprint() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfprint() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfprint(p_ub, outf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprint() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Btypcvt() - Auto generated.
 */
public char * OBtypcvt(TPCONTEXT_T *p_ctxt, BFLDLEN * to_len, int to_type,char *from_buf, int from_type, BFLDLEN from_len) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btypcvt() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Btypcvt() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Btypcvt(to_len, to_type, from_buf, from_type, from_len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btypcvt() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bextread() - Auto generated.
 */
public int OBextread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE *inf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextread() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bextread() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bextread(p_ub, inf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextread() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for Bboolpr() - Auto generated.
 */
public void OBboolpr(TPCONTEXT_T *p_ctxt, char * tree, FILE *outf) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! Bboolpr() failed to set context");
    }
    
    Bboolpr(tree, outf);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! Bboolpr() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for Bread() - Auto generated.
 */
public int OBread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE * inf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bread() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bread() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bread(p_ub, inf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bread() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bwrite() - Auto generated.
 */
public int OBwrite(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwrite() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bwrite() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bwrite(p_ub, outf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwrite() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for Btreefree() - Auto generated.
 */
public void OBtreefree(TPCONTEXT_T *p_ctxt, char *tree) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! Btreefree() failed to set context");
    }
    
    Btreefree(tree);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! Btreefree() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for Bboolsetcbf() - Auto generated.
 */
public int OBboolsetcbf(TPCONTEXT_T *p_ctxt, char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname)) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolsetcbf() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bboolsetcbf(funcname, functionPtr);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Badds() - Auto generated.
 */
public int OBadds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badds() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Badds() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Badds(p_ub, bfldid, buf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badds() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bchgs() - Auto generated.
 */
public int OBchgs(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchgs() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bchgs() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bchgs(p_ub, bfldid, occ, buf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchgs() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bgets() - Auto generated.
 */
public int OBgets(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgets() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgets() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgets(p_ub, bfldid, occ, buf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgets() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bgetsa() - Auto generated.
 */
public char * OBgetsa(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetsa() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgetsa() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgetsa(p_ub, bfldid, occ, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetsa() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bfinds() - Auto generated.
 */
public char * OBfinds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int did_set = FALSE;
    char * ret = NULL;
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfinds() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfinds() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfinds(p_ub, bfldid, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfinds() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bisubf() - Auto generated.
 */
public int OBisubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bisubf() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bisubf() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bisubf(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bisubf() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bindex() - Auto generated.
 */
public int OBindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bindex() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bindex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bindex(p_ub, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bindex() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bunindex() - Auto generated.
 */
public BFLDOCC OBunindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub) 
{
    BFLDOCC ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunindex() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bunindex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bunindex(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunindex() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Bidxused() - Auto generated.
 */
public long OBidxused(TPCONTEXT_T *p_ctxt, UBFH * p_ub) 
{
    long ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bidxused() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bidxused() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bidxused(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bidxused() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for Brstrindex() - Auto generated.
 */
public int OBrstrindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brstrindex() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Brstrindex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Brstrindex(p_ub, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brstrindex() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for ndrx_ubf_tls_set() - Auto generated.
 */
public int Ondrx_ubf_tls_set(TPCONTEXT_T *p_ctxt, void *data) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_set() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_ubf_tls_set() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = ndrx_ubf_tls_set(data);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_set() failed to get context");
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for ndrx_ubf_tls_free() - Auto generated.
 */
public void Ondrx_ubf_tls_free(TPCONTEXT_T *p_ctxt, void *data) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! ndrx_ubf_tls_free() failed to set context");
    }
    
    ndrx_ubf_tls_free(data);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
    {
        userlog("ERROR! ndrx_ubf_tls_free() failed to get context");
    }
out:    
    return;
}


