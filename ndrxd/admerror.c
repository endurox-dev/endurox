/**
 * @brief Error processing for admins (i.e. keep last error message) to provide back to admin.
 *
 * @file admerror.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ndrstandard.h>
#include <ndrxdcmn.h>
#include <ndebug.h>
#include <stdarg.h>
#include <ndrxd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRXD_ERROR_DESCRIPTION(X) (M_ndrxd_error_defs[X<NDRXD_EMINVAL?TPMINVAL:(X>NDRXD_EMAXVAL?NDRXD_EMAXVAL:X)].msg)
#define NDRXD_ERROR(X) X, #X
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

char M_ndrxd_error_msg_buf[MAX_NDRXD_ERROR_LEN+1] = {EXEOS};
int M_ndrxd_error = NDRXD_EMINVAL;

/* Do we need this to be in message catalogue?
 * table bellow is indexed by id (i.e. direct array erorr lookup should work)
 */
struct err_msg
{
    int id;
    char *msg;
} M_ndrxd_error_defs [] =
{
    {NDRXD_ERROR(NDRXD_EMINVAL)},	/* minimum error message */

    {NDRXD_ERROR(NDRXD_ESRVCIDDUP)},
    {NDRXD_ERROR(NDRXD_ESRVCIDINV)},
    {NDRXD_ERROR(NDRXD_EOS)},
    {NDRXD_ERROR(NDRXD_ECFGLDED)},
    {NDRXD_ERROR(NDRXD_ECFGINVLD)},
    {NDRXD_ERROR(NDRXD_EPMOD)},
    {NDRXD_ERROR(NDRXD_ESHMINIT)},
    {NDRXD_ERROR(NDRXD_NOTSTARTED)},
    {NDRXD_ERROR(NDRXD_ECMDNOTFOUND)},
    {NDRXD_ERROR(NDRXD_ENONICONTEXT)},
    {NDRXD_ERROR(NDRXD_EREBBINARYRUN)},
    {NDRXD_ERROR(NDRXD_EBINARYRUN)},
    {NDRXD_ERROR(NDRXD_ECONTEXT)},
    {NDRXD_ERROR(NDRXD_EINVPARAM)},
    {NDRXD_ERROR(NDRXD_EABORT)},
    {NDRXD_ERROR(NDRXD_EENVFAIL)},
    {NDRXD_ERROR(NDRXD_EINVAL)},
    
    {NDRXD_ERROR(NDRXD_EMAXVAL)}	/* maximum error message */
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Get error handling library resources, for direct access from other libraries
 * @param error_code_ptr double ptr to error code
 * @param errbufsz buffer size
 * @return error string buffer
 */
expublic char * NDRXD_error_res_get(int **error_code_ptr, int *errbufsz)
{
    *error_code_ptr = &M_ndrxd_error;
    *errbufsz = sizeof(M_ndrxd_error_msg_buf);
    return M_ndrxd_error_msg_buf;
}
/**
 * Standard.
 * Printer error to stderr
 * @param str
 */
expublic void NDRXD_error (char *str)
{
    if (EXEOS!=M_ndrxd_error_msg_buf[0])
        fprintf(stderr, "%s:%d:%s (%s)\n", str, M_ndrxd_error, 
                NDRXD_ERROR_DESCRIPTION(M_ndrxd_error), M_ndrxd_error_msg_buf);
    else
        fprintf(stderr, "%s:%d:%s\n", str, M_ndrxd_error, 
                NDRXD_ERROR_DESCRIPTION(M_ndrxd_error));
}

/**
 * Extended version. This will print internal error message what happened.
 * This is not thread safe (as all other functions).
 * @param error_code
 */
expublic char * ndrxd_strerror (int err)
{
    static char errbuf[MAX_NDRXD_ERROR_LEN+1];

    if (EXEOS!=M_ndrxd_error_msg_buf[0])
    {
        snprintf(errbuf, sizeof(errbuf), "%d:%s (last error %d: %s)",
                        err,
                        NDRXD_ERROR_DESCRIPTION(err),
                        M_ndrxd_error,
                        M_ndrxd_error_msg_buf);
    }
    else
    {
        snprintf(errbuf, sizeof(errbuf), "%d:%s",
                    err, NDRXD_ERROR_DESCRIPTION(err));
    }

    return errbuf;
}

/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
expublic int * _ndrxd_getNDRXD_errno_addr (void)
{
    return &M_ndrxd_error;
}

/**
 * Internetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void NDRXD_set_error(int error_code)
{
    if (!M_ndrxd_error)
    {
        NDRX_LOG(log_warn, "NDRXD_set_error: %d (%s)",
                                error_code, NDRXD_ERROR_DESCRIPTION(error_code));
        M_ndrxd_error_msg_buf[0] = EXEOS;
        M_ndrxd_error = error_code;
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void NDRXD_set_error_msg(int error_code, char *msg)
{
    int msg_len;
    int err_len;

    if (!M_ndrxd_error)
    {
        msg_len = strlen(msg);
        err_len = (msg_len>MAX_NDRXD_ERROR_LEN)?MAX_NDRXD_ERROR_LEN:msg_len;


        NDRX_LOG(log_warn, "NDRXD_set_error_msg: %d (%s) [%s]", error_code,
                                NDRXD_ERROR_DESCRIPTION(error_code), msg);
        M_ndrxd_error_msg_buf[0] = EXEOS;
        strncat(M_ndrxd_error_msg_buf, msg, err_len);
        M_ndrxd_error = error_code;
    }
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
expublic void NDRXD_set_error_fmt(int error_code, const char *fmt, ...)
{
    char msg[MAX_NDRXD_ERROR_LEN+1] = {EXEOS};
    va_list ap;

    if (!M_ndrxd_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        NDRX_STRCPY_SAFE(M_ndrxd_error_msg_buf, msg);
        M_ndrxd_error = error_code;

        NDRX_LOG(log_warn, "NDRXD_set_error_fmt: %d (%s) [%s]",
                        error_code, NDRXD_ERROR_DESCRIPTION(error_code),
                        M_ndrxd_error_msg_buf);
    }
}

/**
 * Unset any error data currently in use
 */
expublic void NDRXD_unset_error(void)
{
    M_ndrxd_error_msg_buf[0]=EXEOS;
    M_ndrxd_error = BMINVAL;
}
/**
 * Return >0 if error is set
 * @return
 */
expublic int NDRXD_is_error(void)
{
    return M_ndrxd_error;
}

/**
 * Append error message
 * @param msg
 */
expublic void NDRXD_append_error_msg(char *msg)
{
    int free_space = MAX_NDRXD_ERROR_LEN-strlen(M_ndrxd_error_msg_buf);
    int app_error_len = strlen(msg);
    int n;
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(M_ndrxd_error_msg_buf, msg, n);
}

/* vim: set ts=4 sw=4 et smartindent: */
