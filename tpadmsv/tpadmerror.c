/**
 * @brief Admin API error handling
 *
 * @file tpadmerror.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>

#include "tpadmsv.h"
#include <tpadm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Read TA_ERROR from UBF buffer.
 * If no error is set, then OK is returned.
 * @param p_ub UBF buffer
 * @return 
 */
expublic long ndrx_adm_error_get(UBFH *p_ub)
{
    long err = TAOK;
    
    if (Bpres(p_ub, TA_ERROR, 0))
    {
        Bget(p_ub, TA_ERROR, 0, (char *)&err, 0L);
    }
    
    return err;
}

/**
 * Set error for the current request
 * @param[out] p_ub UBF buffer with request data
 * @param[in] error_code error code (or OK status) to be set
 * @param[in] fldid Field id with has the error (if not BFLDID)
 * @param[in] fmt format string for the message
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_adm_error_set(UBFH *p_ub, long error_code, 
        long fldid, const char *fmt, ...)
{
    long curerr;
    int ret = EXSUCCEED;
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    va_list ap;

    if (TAOK!=(curerr=ndrx_adm_error_get(p_ub)))
    {
        NDRX_LOG(log_info, "Error code %d already set -> no override to %d",
                curerr, error_code);
        goto out;
    }
    
    va_start(ap, fmt);
    (void) vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    if (error_code >= TAOK)
    {
        NDRX_LOG(log_info, "Setting MIB error to: %d:[%s] (%d)",
                error_code, msg, fldid);
    }
    else
    {
        NDRX_LOG(log_error, "Setting MIB error to: %d:[%s] (%d)",
                error_code, msg, fldid);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TA_ERROR, 0, (char *)&error_code, 0L))
    {
        NDRX_LOG(log_error, "Failed to set TA_ERROR: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS!=msg[0])
    {
        if (EXSUCCEED!=Bchg(p_ub, TA_STATUS, 0, (char *)msg, 0L))
        {
            NDRX_LOG(log_error, "Failed to set TA_STATUS: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    if (BBADFLDID!=fldid)
    {
        if (EXSUCCEED!=Bchg(p_ub, TA_BADFLD, 0, (char *)&fldid, 0L))
        {
            NDRX_LOG(log_error, "Failed to set TA_BADFLD: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;

}

/* vim: set ts=4 sw=4 et smartindent: */
