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
#include <atmi_shm.h>
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpacall() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpacall(svc, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpacall() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpalloc() - Auto generated.
 */
public char * Otpalloc(TPCONTEXT_T *p_ctxt, char *type, char *subtype, long size) 
{
    char * ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpalloc() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = tpalloc(type, subtype, size);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpalloc() failed to get context");
        ret = NULL;
        goto out;
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcall() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpcall(svc, idata, ilen, odata, olen, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcall() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcancel() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpcancel(cd);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcancel() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpconnect() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpconnect(svc, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpconnect() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdiscon() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpdiscon(cd);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdiscon() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpfree() - Auto generated.
 */
public void Otpfree(TPCONTEXT_T *p_ctxt, char *ptr) 
{
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpfree() failed to set context");
    }
    
    tpfree(ptr);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetrply() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpgetrply(cd, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetrply() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tprealloc() - Auto generated.
 */
public char * Otprealloc(TPCONTEXT_T *p_ctxt, char *ptr, long size) 
{
    char * ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tprealloc() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = tprealloc(ptr, size);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tprealloc() failed to get context");
        ret = NULL;
        goto out;
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tprecv() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tprecv(cd, data, len, flags, revent);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tprecv() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpsend() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpsend(cd, data, len, flags, revent);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpsend() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tptypes() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tptypes(ptr, type, subtype);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tptypes() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpabort() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpabort(flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpabort() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpbegin() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpbegin(timeout, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpbegin() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcommit() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpcommit(flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpcommit() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpconvert() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpconvert(strrep, binrep, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpconvert() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpsuspend() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpsuspend(tranid, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpsuspend() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpresume() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpresume(tranid, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpresume() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpopen() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpopen();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpopen() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpclose() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpclose();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpclose() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetlev() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpgetlev();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetlev() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tpstrerror() - Auto generated.
 */
public char * Otpstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    char * ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpstrerror() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = tpstrerror(err);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpstrerror() failed to get context");
        ret = NULL;
        goto out;
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpgetnodeid() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpgetnodeid();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpgetnodeid() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpsubscribe() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpsubscribe(eventexpr, filter, ctl, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpsubscribe() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpunsubscribe() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpunsubscribe(subscription, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpunsubscribe() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tppost() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tppost(eventname, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tppost() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for _exget_tperrno_addr() - Auto generated.
 */
public int * O_exget_tperrno_addr(TPCONTEXT_T *p_ctxt) 
{
    int * ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! _exget_tperrno_addr() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = _exget_tperrno_addr();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! _exget_tperrno_addr() failed to get context");
        ret = NULL;
        goto out;
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for _exget_tpurcode_addr() - Auto generated.
 */
public long * O_exget_tpurcode_addr(TPCONTEXT_T *p_ctxt) 
{
    long * ret = NULL;
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! _exget_tpurcode_addr() failed to set context");
        ret = NULL;
        goto out;
    }
    
    ret = _exget_tpurcode_addr();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! _exget_tpurcode_addr() failed to get context");
        ret = NULL;
        goto out;
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpinit() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpinit(tpinfo);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpinit() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpterm() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpterm();

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpterm() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpjsontoubf() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpjsontoubf(p_ub, buffer);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpjsontoubf() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpubftojson() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpubftojson(p_ub, buffer, bufsize);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tpubftojson() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpenqueue() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpenqueue(qspace, qname, ctl, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpenqueue() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdequeue() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpdequeue(qspace, qname, ctl, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdequeue() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpenqueueex() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpenqueueex(nodeid, srvid, qname, ctl, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpenqueueex() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdequeueex() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpdequeueex(nodeid, srvid, qname, ctl, data, len, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpdequeueex() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetctxt() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpgetctxt(context, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpgetctxt() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpsetctxt() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tpsetctxt(context, flags);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpsetctxt() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tpfreectxt() - Auto generated.
 */
public void Otpfreectxt(TPCONTEXT_T *p_ctxt, TPCONTEXT_T context) 
{
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN| CTXT_PRIV_TRAN))
    {
        userlog("ERROR! tpfreectxt() failed to set context");
    }
    
    tpfreectxt(context);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogsetreqfile() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tplogsetreqfile(data, filename, filesvc);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogsetreqfile() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tploggetbufreqfile() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tploggetbufreqfile(data, filename, bufsize);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tploggetbufreqfile() failed to get context");
        FAIL_OUT(ret);
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
    
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdelbufreqfile() failed to set context");
        FAIL_OUT(ret);
    }
    
    ret = tplogdelbufreqfile(data);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0, 
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdelbufreqfile() failed to get context");
        FAIL_OUT(ret);
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tplogprintubf() - Auto generated.
 */
public void Otplogprintubf(TPCONTEXT_T *p_ctxt, int lev, char *title, UBFH *p_ub) 
{
    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogprintubf() failed to set context");
    }
    
    tplogprintubf(lev, title, p_ub);

    if (SUCCEED!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD|CTXT_PRIV_UBF| CTXT_PRIV_ATMI | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogprintubf() failed to get context");
    }
out:    
    return;
}


