/* 
** Standard library debugging object API code (auto-generated)
**
**  ondebug.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
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
 * Object-API wrapper for tplogdumpdiff() - Auto generated.
 */
expublic void Otplogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogdumpdiff() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdumpdiff() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogdumpdiff() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogdumpdiff(lev, comment, ptr1, ptr2, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdumpdiff() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogdumpdiff() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for tplogdump() - Auto generated.
 */
expublic void Otplogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogdump() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdump() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogdump() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogdump(lev, comment, ptr, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogdump() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogdump() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for tplog() - Auto generated.
 */
expublic void Otplog(TPCONTEXT_T *p_ctxt, int lev, char *message) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplog() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplog() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplog() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplog(lev, message);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplog() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplog() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for tplogex() - Auto generated.
 */
expublic void Otplogex(TPCONTEXT_T *p_ctxt, int lev, char *file, long line, char *message) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogex() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogex() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogex() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogex(lev, file, line, message);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogex() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogex() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for tploggetiflags() - Auto generated.
 */
expublic char * Otploggetiflags(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;
    char * ret = NULL;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tploggetiflags() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetiflags() failed to set context");
            ret = NULL;
            goto out;
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tploggetiflags() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tploggetiflags();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
                CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetiflags() failed to get context");
            ret = NULL;
            goto out;
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tploggetiflags() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return ret; 
}

/**
 * Object-API wrapper for ndrxlogdumpdiff() - Auto generated.
 */
expublic void Ondrxlogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrxlogdumpdiff() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlogdumpdiff() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrxlogdumpdiff() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ndrxlogdumpdiff(lev, comment, ptr1, ptr2, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlogdumpdiff() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrxlogdumpdiff() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for ndrxlogdump() - Auto generated.
 */
expublic void Ondrxlogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrxlogdump() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlogdump() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrxlogdump() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ndrxlogdump(lev, comment, ptr, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlogdump() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrxlogdump() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for ndrxlog() - Auto generated.
 */
expublic void Ondrxlog(TPCONTEXT_T *p_ctxt, int lev, char *message) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ndrxlog() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlog() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ndrxlog() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ndrxlog(lev, message);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ndrxlog() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ndrxlog() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for ubflogdumpdiff() - Auto generated.
 */
expublic void Oubflogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ubflogdumpdiff() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflogdumpdiff() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ubflogdumpdiff() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ubflogdumpdiff(lev, comment, ptr1, ptr2, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflogdumpdiff() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ubflogdumpdiff() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for ubflogdump() - Auto generated.
 */
expublic void Oubflogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ubflogdump() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflogdump() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ubflogdump() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ubflogdump(lev, comment, ptr, len);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflogdump() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ubflogdump() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for ubflog() - Auto generated.
 */
expublic void Oubflog(TPCONTEXT_T *p_ctxt, int lev, char *message) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: ubflog() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflog() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! ubflog() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    ubflog(lev, message);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! ubflog() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: ubflog() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


/**
 * Object-API wrapper for tploggetreqfile() - Auto generated.
 */
expublic int Otploggetreqfile(TPCONTEXT_T *p_ctxt, char *filename, int bufsize) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tploggetreqfile() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetreqfile() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tploggetreqfile() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tploggetreqfile(filename, bufsize);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetreqfile() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tploggetreqfile() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}


/**
 * Object-API wrapper for tplogconfig() - Auto generated.
 */
expublic int Otplogconfig(TPCONTEXT_T *p_ctxt, int logger, int lev, char *debug_string, char *module, char *new_file) 
{
    int ret = EXSUCCEED;
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogconfig() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogconfig() failed to set context");
            EXFAIL_OUT(ret);
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogconfig() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }
    
    ret = tplogconfig(logger, lev, debug_string, module, new_file);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogconfig() failed to get context");
            EXFAIL_OUT(ret);
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogconfig() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif

 
    return ret; 
}

/**
 * Object-API wrapper for tplogclosereqfile() - Auto generated.
 */
expublic void Otplogclosereqfile(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogclosereqfile() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogclosereqfile() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogclosereqfile() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogclosereqfile();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogclosereqfile() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogclosereqfile() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for tplogclosethread() - Auto generated.
 */
expublic void Otplogclosethread(TPCONTEXT_T *p_ctxt) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogclosethread() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogclosethread() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogclosethread() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogclosethread();

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogclosethread() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogclosethread() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}

/**
 * Object-API wrapper for tplogsetreqfile_direct() - Auto generated.
 */
expublic void Otplogsetreqfile_direct(TPCONTEXT_T *p_ctxt, char *filename) 
{
    int did_set = EXFALSE;


#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "ENTRY: tplogsetreqfile_direct() enter, context: %p, current: %p", *p_ctxt, G_atmi_tls);
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

 

 /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
         /* set the context */
        if (EXSUCCEED!=_tpsetctxt(*p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogsetreqfile_direct() failed to set context");
        }
        did_set = EXTRUE;
    }
    else if ((atmi_tls_t *)*p_ctxt != G_atmi_tls)
    {
        userlog("WARNING! tplogsetreqfile_direct() context %p thinks that it is assocated "
                "with current thread, but thread is associated with %p context!",
                p_ctxt, G_atmi_tls);
    }

    tplogsetreqfile_direct(filename);

    if (did_set)
    {
        if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogsetreqfile_direct() failed to get context");
        }
    }
out:

#ifdef NDRX_OAPI_DEBUG
    NDRX_LOG(log_debug, "RETURN: tplogsetreqfile_direct() returns, context: %p, current: %p",
        *p_ctxt, G_atmi_tls);
#endif


    return;
}


