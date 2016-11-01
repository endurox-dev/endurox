/* 
** Standard library debugging object API code (auto-generated)
**
**  ondebug.c
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
 * Object-API wrapper for tplogdumpdiff() - Auto generated.
 */
public void Otplogdumpdiff(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr1, void *ptr2, int len) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdumpdiff() failed to set context");
    }
    
    tplogdumpdiff(lev, comment, ptr1, ptr2, len);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdumpdiff() failed to get context");
    }
out:    
    return;
}

/**
 * Object-API wrapper for tplogdump() - Auto generated.
 */
public void Otplogdump(TPCONTEXT_T *p_ctxt, int lev, char *comment, void *ptr, int len) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdump() failed to set context");
    }
    
    tplogdump(lev, comment, ptr, len);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogdump() failed to get context");
    }
out:    
    return;
}

/**
 * Object-API wrapper for tplog() - Auto generated.
 */
public void Otplog(TPCONTEXT_T *p_ctxt, int lev, char *message) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplog() failed to set context");
    }
    
    tplog(lev, message);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplog() failed to get context");
    }
out:    
    return;
}


/**
 * Object-API wrapper for tploggetreqfile() - Auto generated.
 */
public int Otploggetreqfile(TPCONTEXT_T *p_ctxt, char *filename, int bufsize) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tploggetreqfile() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
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
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}


/**
 * Object-API wrapper for tplogconfig() - Auto generated.
 */
public int Otplogconfig(TPCONTEXT_T *p_ctxt, int logger, int lev, char *debug_string, char *module, char *new_file) 
{
    int ret = SUCCEED;
    int did_set = FALSE;

    /* set the context */
    if (!((atmi_tls_t *)*p_ctxt)->is_associated_with_thread)
    {
        if (SUCCEED!=_tpsetctxt(*p_ctxt, 0, 
            CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
        {
            userlog("ERROR! tplogconfig() failed to set context");
            FAIL_OUT(ret);
        }
        did_set = TRUE;
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
            FAIL_OUT(ret);
        }
    }
out:    
    return ret; 
}

/**
 * Object-API wrapper for tplogclosereqfile() - Auto generated.
 */
public void Otplogclosereqfile(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogclosereqfile() failed to set context");
    }
    
    tplogclosereqfile();

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogclosereqfile() failed to get context");
    }
out:    
    return;
}

/**
 * Object-API wrapper for tplogclosethread() - Auto generated.
 */
public void Otplogclosethread(TPCONTEXT_T *p_ctxt) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogclosethread() failed to set context");
    }
    
    tplogclosethread();

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogclosethread() failed to get context");
    }
out:    
    return;
}

/**
 * Object-API wrapper for tplogsetreqfile_direct() - Auto generated.
 */
public void Otplogsetreqfile_direct(TPCONTEXT_T *p_ctxt, char *filename) 
{
    int did_set = FALSE;

    /* set the context */
    if (SUCCEED!=_tpsetctxt(*p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogsetreqfile_direct() failed to set context");
    }
    
    tplogsetreqfile_direct(filename);

    if (TPMULTICONTEXTS!=_tpgetctxt(p_ctxt, 0,
        CTXT_PRIV_NSTD | CTXT_PRIV_IGN))
    {
        userlog("ERROR! tplogsetreqfile_direct() failed to get context");
    }
out:    
    return;
}


