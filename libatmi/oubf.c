/**
 * @brief UBF Object API code (auto-generated)
 *
 * @file oubf.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
 * -----------------------------------------------------------------------------
 * AGPL license:
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
 * Object-API wrapper for ndrx_Bget_Ferror_addr() - Auto generated.
 */
expublic int * Ondrx_Bget_Ferror_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;
    int * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrx_Bget_Ferror_addr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Bget_Ferror_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_Bget_Ferror_addr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = ndrx_Bget_Ferror_addr();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Bget_Ferror_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrx_Bget_Ferror_addr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for _Bget_Ferror_addr() - Auto generated.
 */
expublic int * O_Bget_Ferror_addr(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;
    int * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: _Bget_Ferror_addr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Bget_Ferror_addr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! _Bget_Ferror_addr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: _Bget_Ferror_addr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Blen() - Auto generated.
 */
expublic int OBlen(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Blen() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Blen() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Blen() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Blen() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBadd() - Auto generated.
 */
expublic int OCBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBadd() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBadd() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBadd() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBadd() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBchg() - Auto generated.
 */
expublic int OCBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBchg() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBchg() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBchg() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBchg() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBget() - Auto generated.
 */
expublic int OCBget(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBget() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBget() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBget() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBget() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bdel() - Auto generated.
 */
expublic int OBdel(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bdel() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdel() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdel() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bdel() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bpres() - Auto generated.
 */
expublic int OBpres(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bpres() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpres() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpres() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bpres() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bproj() - Auto generated.
 */
expublic int OBproj(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID * fldlist) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bproj() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bproj() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bproj() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bproj() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bprojcpy() - Auto generated.
 */
expublic int OBprojcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bprojcpy() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprojcpy() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprojcpy() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bprojcpy() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfldid() - Auto generated.
 */
expublic BFLDID OBfldid(TPCONTEXT_T *p_ctxt, char *fldnm) 
{
    BFLDID ret = BBADFLDID;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfldid() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
 
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldid() failed to set context");
            ret = BBADFLDID;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldid() failed to get context");
            ret = BBADFLDID;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfldid() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bfname() - Auto generated.
 */
expublic char * OBfname(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfname() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfname() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfname() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfname() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bcpy() - Auto generated.
 */
expublic int OBcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bcpy() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcpy() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcpy() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bcpy() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bchg() - Auto generated.
 */
expublic int OBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bchg() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchg() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchg() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bchg() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Binit() - Auto generated.
 */
expublic int OBinit(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDLEN len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Binit() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Binit() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Binit() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Binit() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bnext() - Auto generated.
 */
expublic int OBnext(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bnext() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bnext() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bnext2() - Auto generated.
 */
expublic int OBnext2(TPCONTEXT_T *p_ctxt, Bnext_state_t *bnext_state, UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len, char **d_ptr) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bnext2() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext2() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bnext2() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bnext2(bnext_state, p_ub, bfldid, occ, buf, len, d_ptr);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnext2() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bnext2() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bget() - Auto generated.
 */
expublic int OBget(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bget() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bget() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bget() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bget() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bboolco() - Auto generated.
 */
expublic char * OBboolco(TPCONTEXT_T *p_ctxt, char * expr) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolco() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolco() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolco() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolco() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bfind() - Auto generated.
 */
expublic char * OBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfind() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfind() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfind() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfind() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bboolev() - Auto generated.
 */
expublic int OBboolev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolev() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolev() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolev() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolev() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfloatev() - Auto generated.
 */
expublic double OBfloatev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree) 
{
    double ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfloatev() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfloatev() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfloatev() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfloatev() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Badd() - Auto generated.
 */
expublic int OBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Badd() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badd() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badd() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Badd() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Baddfast() - Auto generated.
 */
expublic int OBaddfast(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len, Bfld_loc_info_t *next_fld) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Baddfast() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Baddfast() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Baddfast() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Baddfast(p_ub, bfldid, buf, len, next_fld);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Baddfast() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Baddfast() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Becodestr() - Auto generated.
 */
expublic char * OBecodestr(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Becodestr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Becodestr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Becodestr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Becodestr(err);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Becodestr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Becodestr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}

/**
 * Object-API wrapper for B_error() - Auto generated.
 */
expublic void OB_error(TPCONTEXT_T *p_ctxt, char *str) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: B_error() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B_error() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! B_error() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    B_error(str);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B_error() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: B_error() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bstrerror() - Auto generated.
 */
expublic char * OBstrerror(TPCONTEXT_T *p_ctxt, int err) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bstrerror() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bstrerror() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bstrerror() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bstrerror() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bmkfldid() - Auto generated.
 */
expublic BFLDID OBmkfldid(TPCONTEXT_T *p_ctxt, int fldtype, BFLDID bfldid) 
{
    BFLDID ret = BBADFLDID;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bmkfldid() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
 
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bmkfldid() failed to set context");
            ret = BBADFLDID;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bmkfldid() failed to get context");
            ret = BBADFLDID;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bmkfldid() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Boccur() - Auto generated.
 */
expublic BFLDOCC OBoccur(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Boccur() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Boccur() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Boccur() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Boccur() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bused() - Auto generated.
 */
expublic long OBused(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bused() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bused() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bused() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bused() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfldtype() - Auto generated.
 */
expublic int OBfldtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfldtype() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldtype() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldtype() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfldtype() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bdelall() - Auto generated.
 */
expublic int OBdelall(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bdelall() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelall() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelall() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bdelall() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bdelete() - Auto generated.
 */
expublic int OBdelete(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldlist) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bdelete() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelete() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bdelete() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bdelete() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfldno() - Auto generated.
 */
expublic BFLDOCC OBfldno(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfldno() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldno() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfldno() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfldno() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bunused() - Auto generated.
 */
expublic long OBunused(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bunused() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunused() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunused() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bunused() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bsizeof() - Auto generated.
 */
expublic long OBsizeof(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bsizeof() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsizeof() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsizeof() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bsizeof() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Btype() - Auto generated.
 */
expublic char * OBtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Btype() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btype() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btype() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Btype() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bfree() - Auto generated.
 */
expublic int OBfree(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfree() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfree() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfree() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfree() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Balloc() - Auto generated.
 */
expublic UBFH * OBalloc(TPCONTEXT_T *p_ctxt, BFLDOCC f, BFLDLEN v) 
{
    int did_set = EXFALSE;
    UBFH * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Balloc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Balloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Balloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Balloc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Brealloc() - Auto generated.
 */
expublic UBFH * OBrealloc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDOCC f, BFLDLEN v) 
{
    int did_set = EXFALSE;
    UBFH * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Brealloc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brealloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brealloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Brealloc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bupdate() - Auto generated.
 */
expublic int OBupdate(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bupdate() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bupdate() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bupdate() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bupdate() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bconcat() - Auto generated.
 */
expublic int OBconcat(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bconcat() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bconcat() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bconcat() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bconcat() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBfind() - Auto generated.
 */
expublic char * OCBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBfind() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfind() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfind() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBfind() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBgetalloc() - Auto generated.
 */
expublic char * OCBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBgetalloc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBgetalloc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBfindocc() - Auto generated.
 */
expublic BFLDOCC OCBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len, int usrtype) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBfindocc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindocc() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindocc() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBfindocc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfindocc() - Auto generated.
 */
expublic BFLDOCC OBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfindocc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindocc() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindocc() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfindocc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bgetalloc() - Auto generated.
 */
expublic char * OBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgetalloc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgetalloc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bfindlast() - Auto generated.
 */
expublic char * OBfindlast(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid,BFLDOCC *occ, BFLDLEN *len) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfindlast() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindlast() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfindlast() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfindlast() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bgetlast() - Auto generated.
 */
expublic int OBgetlast(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,BFLDOCC *occ, char *buf, BFLDLEN *len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgetlast() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetlast() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetlast() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgetlast() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bprint() - Auto generated.
 */
expublic int OBprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bprint() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprint() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bprint() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bprint() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfprint() - Auto generated.
 */
expublic int OBfprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfprint() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprint() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprint() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfprint() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bfprintcb() - Auto generated.
 */
expublic int OBfprintcb(TPCONTEXT_T *p_ctxt, UBFH *p_ub, ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfprintcb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprintcb() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bfprintcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bfprintcb(p_ub, p_writef, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfprintcb() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfprintcb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Btypcvt() - Auto generated.
 */
expublic char * OBtypcvt(TPCONTEXT_T *p_ctxt, BFLDLEN * to_len, int to_type,char *from_buf, int from_type, BFLDLEN from_len) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Btypcvt() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btypcvt() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btypcvt() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Btypcvt() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bextread() - Auto generated.
 */
expublic int OBextread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE *inf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bextread() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextread() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextread() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bextread() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bextreadcb() - Auto generated.
 */
expublic int OBextreadcb(TPCONTEXT_T *p_ctxt, UBFH * p_ub, long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bextreadcb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextreadcb() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bextreadcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bextreadcb(p_ub, p_readf, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bextreadcb() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bextreadcb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for Bboolpr() - Auto generated.
 */
expublic void OBboolpr(TPCONTEXT_T *p_ctxt, char * tree, FILE *outf) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolpr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolpr() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolpr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    Bboolpr(tree, outf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolpr() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolpr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for Bboolprcb() - Auto generated.
 */
expublic void OBboolprcb(TPCONTEXT_T *p_ctxt, char * tree, int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolprcb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolprcb() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolprcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    Bboolprcb(tree, p_writef, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolprcb() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolprcb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bread() - Auto generated.
 */
expublic int OBread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE * inf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bread() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bread() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bread() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bread() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bwrite() - Auto generated.
 */
expublic int OBwrite(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bwrite() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwrite() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwrite() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bwrite() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bwritecb() - Auto generated.
 */
expublic int OBwritecb(TPCONTEXT_T *p_ctxt, UBFH *p_ub, long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bwritecb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwritecb() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bwritecb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bwritecb(p_ub, p_writef, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bwritecb() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bwritecb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for Btreefree() - Auto generated.
 */
expublic void OBtreefree(TPCONTEXT_T *p_ctxt, char *tree) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Btreefree() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btreefree() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Btreefree() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    Btreefree(tree);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Btreefree() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Btreefree() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bboolsetcbf() - Auto generated.
 */
expublic int OBboolsetcbf(TPCONTEXT_T *p_ctxt, char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname)) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolsetcbf() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolsetcbf() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bboolsetcbf2() - Auto generated.
 */
expublic int OBboolsetcbf2(TPCONTEXT_T *p_ctxt, char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname, char *arg1)) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bboolsetcbf2() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf2() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bboolsetcbf2() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bboolsetcbf2(funcname, functionPtr);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bboolsetcbf2() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bboolsetcbf2() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Badds() - Auto generated.
 */
expublic int OBadds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Badds() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badds() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Badds() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Badds() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bchgs() - Auto generated.
 */
expublic int OBchgs(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bchgs() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchgs() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bchgs() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bchgs() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bgets() - Auto generated.
 */
expublic int OBgets(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgets() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgets() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgets() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgets() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bgetsa() - Auto generated.
 */
expublic char * OBgetsa(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgetsa() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetsa() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetsa() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgetsa() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bfinds() - Auto generated.
 */
expublic char * OBfinds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bfinds() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfinds() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bfinds() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bfinds() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bisubf() - Auto generated.
 */
expublic int OBisubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bisubf() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bisubf() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bisubf() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bisubf() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bindex() - Auto generated.
 */
expublic int OBindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bindex() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bindex() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bindex() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bindex() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bunindex() - Auto generated.
 */
expublic BFLDOCC OBunindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bunindex() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunindex() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bunindex() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bunindex() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bidxused() - Auto generated.
 */
expublic long OBidxused(TPCONTEXT_T *p_ctxt, UBFH * p_ub) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bidxused() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bidxused() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bidxused() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bidxused() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Brstrindex() - Auto generated.
 */
expublic int OBrstrindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Brstrindex() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brstrindex() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Brstrindex() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Brstrindex() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bjoin() - Auto generated.
 */
expublic int OBjoin(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bjoin() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bjoin() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bjoin() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bjoin(dest, src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bjoin() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bjoin() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bojoin() - Auto generated.
 */
expublic int OBojoin(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bojoin() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bojoin() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bojoin() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bojoin(dest, src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bojoin() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bojoin() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bcmp() - Auto generated.
 */
expublic int OBcmp(TPCONTEXT_T *p_ctxt, UBFH *p_ubf1, UBFH *p_ubf2) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bcmp() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcmp() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bcmp() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bcmp(p_ubf1, p_ubf2);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bcmp() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bcmp() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bsubset() - Auto generated.
 */
expublic int OBsubset(TPCONTEXT_T *p_ctxt, UBFH *p_ubf1, UBFH *p_ubf2) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bsubset() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsubset() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bsubset() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bsubset(p_ubf1, p_ubf2);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bsubset() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bsubset() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bnum() - Auto generated.
 */
expublic BFLDOCC OBnum(TPCONTEXT_T *p_ctxt, UBFH * p_ub) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bnum() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnum() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bnum() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bnum(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bnum() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bnum() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bneeded() - Auto generated.
 */
expublic long OBneeded(TPCONTEXT_T *p_ctxt, BFLDOCC nrfields, BFLDLEN totsize) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bneeded() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bneeded() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bneeded() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bneeded(nrfields, totsize);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bneeded() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bneeded() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvnull() - Auto generated.
 */
expublic int OBvnull(TPCONTEXT_T *p_ctxt, char *cstruct, char *cname, BFLDOCC occ, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvnull() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnull() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvnull() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvnull(cstruct, cname, occ, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnull() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvnull() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvselinit() - Auto generated.
 */
expublic int OBvselinit(TPCONTEXT_T *p_ctxt, char *cstruct, char *cname, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvselinit() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvselinit() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvselinit() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvselinit(cstruct, cname, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvselinit() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvselinit() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvsinit() - Auto generated.
 */
expublic int OBvsinit(TPCONTEXT_T *p_ctxt, char *cstruct, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvsinit() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsinit() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvsinit() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvsinit(cstruct, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsinit() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvsinit() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for Bvrefresh() - Auto generated.
 */
expublic void OBvrefresh(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvrefresh() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvrefresh() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvrefresh() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    Bvrefresh();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvrefresh() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvrefresh() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bvopt() - Auto generated.
 */
expublic int OBvopt(TPCONTEXT_T *p_ctxt, char *cname, int option, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvopt() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvopt() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvopt() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvopt(cname, option, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvopt() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvopt() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvftos() - Auto generated.
 */
expublic int OBvftos(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cstruct, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvftos() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvftos() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvftos() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvftos(p_ub, cstruct, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvftos() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvftos() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvstof() - Auto generated.
 */
expublic int OBvstof(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cstruct, int mode, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvstof() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvstof() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvstof() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvstof(p_ub, cstruct, mode, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvstof() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvstof() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBvget() - Auto generated.
 */
expublic int OCBvget(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype, long flags) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvget() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvget() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvget() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvget(cstruct, view, cname, occ, buf, len, usrtype, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvget() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvget() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBvgetalloc() - Auto generated.
 */
expublic char * OCBvgetalloc(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ, int usrtype, long flags, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvgetalloc() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetalloc() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvgetalloc() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvgetalloc(cstruct, view, cname, occ, usrtype, flags, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetalloc() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvgetalloc() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBvchg() - Auto generated.
 */
expublic int OCBvchg(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ, char *buf, BFLDLEN len, int usrtype) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvchg() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvchg() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvchg() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvchg(cstruct, view, cname, occ, buf, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvchg() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvchg() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvsizeof() - Auto generated.
 */
expublic long OBvsizeof(TPCONTEXT_T *p_ctxt, char *view) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvsizeof() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsizeof() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvsizeof() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvsizeof(view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsizeof() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvsizeof() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvcpy() - Auto generated.
 */
expublic long OBvcpy(TPCONTEXT_T *p_ctxt, char *cstruct_dst, char *cstruct_src, char *view) 
{
    long ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvcpy() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvcpy() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvcpy() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvcpy(cstruct_dst, cstruct_src, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvcpy() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvcpy() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvoccur() - Auto generated.
 */
expublic BFLDOCC OBvoccur(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size, int* fldtype) 
{
    BFLDOCC ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvoccur() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvoccur() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvoccur() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvoccur(cstruct, view, cname, maxocc, realocc, dim_size, fldtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvoccur() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvoccur() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvsetoccur() - Auto generated.
 */
expublic int OBvsetoccur(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvsetoccur() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsetoccur() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvsetoccur() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvsetoccur(cstruct, view, cname, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvsetoccur() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvsetoccur() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvnext() - Auto generated.
 */
expublic int OBvnext(TPCONTEXT_T *p_ctxt, Bvnext_state_t *state, char *view, char *cname, int *fldtype, BFLDOCC *maxocc, long *dim_size) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvnext() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnext() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvnext() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvnext(state, view, cname, fldtype, maxocc, dim_size);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnext() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvnext() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvcmp() - Auto generated.
 */
expublic int OBvcmp(TPCONTEXT_T *p_ctxt, char *cstruct1, char *view1, char *cstruct2, char *view2) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvcmp() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvcmp() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvcmp() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvcmp(cstruct1, view1, cstruct2, view2);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvcmp() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvcmp() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvprint() - Auto generated.
 */
expublic int OBvprint(TPCONTEXT_T *p_ctxt, char *cstruct, char *view) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvprint() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvprint() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvprint() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvprint(cstruct, view);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvprint() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvprint() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvfprint() - Auto generated.
 */
expublic int OBvfprint(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, FILE * outf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvfprint() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvfprint() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvfprint() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvfprint(cstruct, view, outf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvfprint() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvfprint() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvfprintcb() - Auto generated.
 */
expublic int OBvfprintcb(TPCONTEXT_T *p_ctxt, char *cstruct, char *view,ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvfprintcb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvfprintcb() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvfprintcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvfprintcb(cstruct, view, p_writef, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvfprintcb() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvfprintcb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvextread() - Auto generated.
 */
expublic int OBvextread(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, FILE *inf) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvextread() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvextread() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvextread() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvextread(cstruct, view, inf);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvextread() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvextread() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvextreadcb() - Auto generated.
 */
expublic int OBvextreadcb(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvextreadcb() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvextreadcb() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvextreadcb() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvextreadcb(cstruct, view, p_readf, dataptr1);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvextreadcb() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvextreadcb() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for ndrx_ubf_tls_set() - Auto generated.
 */
expublic int Ondrx_ubf_tls_set(TPCONTEXT_T *p_ctxt, void *data) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrx_ubf_tls_set() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_set() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
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
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_set() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrx_ubf_tls_set() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for ndrx_ubf_tls_free() - Auto generated.
 */
expublic void Ondrx_ubf_tls_free(TPCONTEXT_T *p_ctxt, void *data) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrx_ubf_tls_free() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_free() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_ubf_tls_free() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ndrx_ubf_tls_free(data);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_ubf_tls_free() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrx_ubf_tls_free() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bflddbload() - Auto generated.
 */
expublic int OBflddbload(TPCONTEXT_T *p_ctxt) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbload() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbload() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbload() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbload();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbload() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbload() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bflddbid() - Auto generated.
 */
expublic BFLDID OBflddbid(TPCONTEXT_T *p_ctxt, char *fldname) 
{
    BFLDID ret = BBADFLDID;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbid() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
 
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbid() failed to set context");
            ret = BBADFLDID;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbid() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ret = Bflddbid(fldname);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbid() failed to get context");
            ret = BBADFLDID;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbid() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bflddbname() - Auto generated.
 */
expublic char * OBflddbname(TPCONTEXT_T *p_ctxt, BFLDID bfldid) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbname() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbname() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbname() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbname(bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbname() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbname() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bflddbget() - Auto generated.
 */
expublic int OBflddbget(TPCONTEXT_T *p_ctxt, EDB_val *data, short *p_fldtype,BFLDID *p_bfldno, BFLDID *p_bfldid, char *fldname, int fldname_bufsz) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbget() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbget() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbget() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbget(data, p_fldtype, p_bfldno, p_bfldid, fldname, fldname_bufsz);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbget() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbget() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bflddbunlink() - Auto generated.
 */
expublic int OBflddbunlink(TPCONTEXT_T *p_ctxt) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbunlink() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbunlink() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbunlink() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbunlink();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbunlink() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbunlink() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for Bflddbunload() - Auto generated.
 */
expublic void OBflddbunload(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbunload() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbunload() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbunload() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    Bflddbunload();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbunload() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbunload() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for Bflddbdrop() - Auto generated.
 */
expublic int OBflddbdrop(TPCONTEXT_T *p_ctxt, EDB_txn *txn) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbdrop() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbdrop() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbdrop() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbdrop(txn);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbdrop() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbdrop() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bflddbdel() - Auto generated.
 */
expublic int OBflddbdel(TPCONTEXT_T *p_ctxt, EDB_txn *txn, BFLDID bfldid) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbdel() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbdel() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbdel() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbdel(txn, bfldid);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbdel() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbdel() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bflddbadd() - Auto generated.
 */
expublic int OBflddbadd(TPCONTEXT_T *p_ctxt, EDB_txn *txn, short fldtype, BFLDID bfldno, char *fldname) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bflddbadd() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbadd() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bflddbadd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bflddbadd(txn, fldtype, bfldno, fldname);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bflddbadd() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bflddbadd() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for B32to16() - Auto generated.
 */
expublic int OB32to16(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: B32to16() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B32to16() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! B32to16() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = B32to16(dest, src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B32to16() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: B32to16() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for B16to32() - Auto generated.
 */
expublic int OB16to32(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: B16to32() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B16to32() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! B16to32() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = B16to32(dest, src);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! B16to32() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: B16to32() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bgetr() - Auto generated.
 */
expublic int OBgetr(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID *fldidocc, char * buf, BFLDLEN * buflen) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgetr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetr() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgetr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgetr(p_ub, fldidocc, buf, buflen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetr() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgetr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bgetrv() - Auto generated.
 */
expublic int OBgetrv(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char * buf, BFLDLEN * buflen, ...) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bgetrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetrv() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bgetrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bgetrv(p_ub, buf, buflen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bgetrv() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bgetrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBgetr() - Auto generated.
 */
expublic int OCBgetr(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID *fldidocc,char * buf, BFLDLEN * buflen, int usrtype) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBgetr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetr() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBgetr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBgetr(p_ub, fldidocc, buf, buflen, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetr() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBgetr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBgetrv() - Auto generated.
 */
expublic int OCBgetrv(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char * buf, BFLDLEN * buflen, int usrtype, ...) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBgetrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetrv() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBgetrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBgetrv(p_ub, buf, buflen, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetrv() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBgetrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBgetallocr() - Auto generated.
 */
expublic char * OCBgetallocr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc, int usrtype, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBgetallocr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetallocr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBgetallocr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBgetallocr(p_ub, fldidocc, usrtype, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetallocr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBgetallocr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBgetallocrv() - Auto generated.
 */
expublic char * OCBgetallocrv(TPCONTEXT_T *p_ctxt, UBFH *p_ub, int usrtype, BFLDLEN *extralen, ...) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBgetallocrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetallocrv() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBgetallocrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBgetallocrv(p_ub, usrtype, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBgetallocrv() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBgetallocrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBfindr() - Auto generated.
 */
expublic char * OCBfindr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc, BFLDLEN *len, int usrtype) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBfindr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBfindr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBfindr(p_ub, fldidocc, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBfindr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for CBfindrv() - Auto generated.
 */
expublic char * OCBfindrv(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDLEN *len, int usrtype, ...) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBfindrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindrv() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBfindrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBfindrv(p_ub, len, usrtype);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBfindrv() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBfindrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for Bpresr() - Auto generated.
 */
expublic int OBpresr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bpresr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpresr() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bpresr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bpresr(p_ub, fldidocc);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpresr() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bpresr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bpresrv() - Auto generated.
 */
expublic int OBpresrv(TPCONTEXT_T *p_ctxt, UBFH *p_ub, ...) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bpresrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpresrv() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bpresrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bpresrv(p_ub);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bpresrv() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bpresrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBvgetr() - Auto generated.
 */
expublic int OCBvgetr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc, char *cname, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype, long flags) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvgetr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetr() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvgetr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvgetr(p_ub, fldidocc, cname, occ, buf, len, usrtype, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetr() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvgetr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBvgetrv() - Auto generated.
 */
expublic int OCBvgetrv(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cname, BFLDOCC occ,char *buf, BFLDLEN *len, int usrtype, long flags, ...) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvgetrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetrv() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvgetrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvgetrv(p_ub, cname, occ, buf, len, usrtype, flags);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetrv() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvgetrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvnullr() - Auto generated.
 */
expublic int OBvnullr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc, char *cname, BFLDOCC occ) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvnullr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnullr() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvnullr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvnullr(p_ub, fldidocc, cname, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnullr() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvnullr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for Bvnullrv() - Auto generated.
 */
expublic int OBvnullrv(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cname, BFLDOCC occ, ...) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: Bvnullrv() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnullrv() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! Bvnullrv() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = Bvnullrv(p_ub, cname, occ);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! Bvnullrv() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: Bvnullrv() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for CBvgetallocr() - Auto generated.
 */
expublic char * OCBvgetallocr(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldidocc, char *cname, BFLDOCC occ, int usrtype, long flags, BFLDLEN *extralen) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: CBvgetallocr() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 
    
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        /* set the context */
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetallocr() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! CBvgetallocr() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = CBvgetallocr(p_ub, fldidocc, cname, occ, usrtype, flags, extralen);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! CBvgetallocr() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: CBvgetallocr() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}


/**
 * Object-API wrapper for ndrx_Fchg() - Auto generated.
 */
expublic int Ondrx_Fchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrx_Fchg() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Fchg() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_Fchg() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = ndrx_Fchg(p_ub, bfldid, occ, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Fchg() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrx_Fchg() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for ndrx_Fadd() - Auto generated.
 */
expublic int Ondrx_Fadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrx_Fadd() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
    NDRX_LOG(log_debug, "ENTRY: is_associated_with_thread = %d", 
        ((atmi_tls_t *)*p_ctxt)->is_associated_with_thread);

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NSTD = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NSTD );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_UBF = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_UBF );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_ATMI = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_ATMI );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_TRAN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_TRAN );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_NOCHK = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_NOCHK );

    NDRX_LOG(log_debug, "ENTRY: CTXT_PRIV_IGN = %d", 
        (CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN) & CTXT_PRIV_IGN );
#endif

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=ndrx_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Fadd() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrx_Fadd() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = ndrx_Fadd(p_ub, bfldid, buf, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=ndrx_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD|CTXT_PRIV_UBF | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrx_Fadd() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrx_Fadd() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


