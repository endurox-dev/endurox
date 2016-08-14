/* 
** NDR Stnadard library error handling
**
** @file nerror.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <regex.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>
#include <nerror.h>
#include <errno.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Function returns error description
 */
#define NSTD_ERROR_DESCRIPTION(X) (M_nstd_error_defs[X<NMINVAL?NMINVAL:(X>NMAXVAL?NMAXVAL:X)].msg)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
__thread char M_nstd_error_msg_buf[MAX_ERROR_LEN+1] = {EOS};
__thread int M_nstd_error = BMINVAL;

/* Do we need this to be in message catalogue?
 * table bellow is indexed by id (i.e. direct array erorr lookup should work)
 */
struct err_msg
{
    int id;
    char *msg;
} M_nstd_error_defs [] =
{
    {BMINVAL,       "No error"},        /* 0 */
    {NEINVALINI,    "Invalid INI file"}, /* 1 */
    {NEMALLOC,      "Malloc failed"},    /* 2 */
};
/*---------------------------Prototypes---------------------------------*/
/**
 * Standard.
 * Printer error to stderr
 * @param str
 */
public void N_error (char *str)
{
    if (EOS!=M_nstd_error_msg_buf[0])
        fprintf(stderr, "%s:%d:%s (%s)\n", str, M_nstd_error, NSTD_ERROR_DESCRIPTION(M_nstd_error),
                                            M_nstd_error_msg_buf);
    else
        fprintf(stderr, "%s:%d:%s\n", str, M_nstd_error, NSTD_ERROR_DESCRIPTION(M_nstd_error));
}

/**
 * Extended version. This will print internal error message what happened.
 * This is not thread safe (as all other functions).
 * @param error_code
 */
public char * Nstrerror (int err)
{
    static __thread char errbuf[MAX_ERROR_LEN+1];

    if (EOS!=M_nstd_error_msg_buf[0])
    {
        snprintf(errbuf, sizeof(errbuf), "%d:%s (last error %d: %s)",
                                err,
                                NSTD_ERROR_DESCRIPTION(err),
                                M_nstd_error,
                                M_nstd_error_msg_buf);
    }
    else
    {
        snprintf(errbuf, sizeof(errbuf), "%d:%s",
                            err, NSTD_ERROR_DESCRIPTION(err));
    }

    return errbuf;
}

/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
public int * _Nget_Nerror_addr (void)
{
    return &M_nstd_error;
}

/**
 * Internetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
public void _Nset_error(int error_code)
{
    if (!M_nstd_error)
    {
        NDRX_LOG(log_warn, "_Nset_error: %d (%s)", 
                                error_code, NSTD_ERROR_DESCRIPTION(error_code));
        M_nstd_error_msg_buf[0] = EOS;
        M_nstd_error = error_code;
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
public void _Nset_error_msg(int error_code, char *msg)
{
    int msg_len;
    int err_len;

    if (!M_nstd_error)
    {
        msg_len = strlen(msg);
        err_len = (msg_len>MAX_ERROR_LEN)?MAX_ERROR_LEN:msg_len;


        NDRX_LOG(log_warn, "_Nset_error_msg: %d (%s) [%s]", error_code,
                                NSTD_ERROR_DESCRIPTION(error_code), msg);
        M_nstd_error_msg_buf[0] = EOS;
        strncat(M_nstd_error_msg_buf, msg, err_len);
        M_nstd_error = error_code;
    }
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
public void _Nset_error_fmt(int error_code, const char *fmt, ...)
{
    char msg[MAX_ERROR_LEN+1] = {EOS};
    va_list ap;

    if (!M_nstd_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        strcpy(M_nstd_error_msg_buf, msg);
        M_nstd_error = error_code;

        NDRX_LOG(log_warn, "_Nset_error_fmt: %d (%s) [%s]",
                        error_code, NSTD_ERROR_DESCRIPTION(error_code),
                        M_nstd_error_msg_buf);
    }
}

/**
 * Unset any error data currently in use
 */
public void _Nunset_error(void)
{
    M_nstd_error_msg_buf[0]=EOS;
    M_nstd_error = BMINVAL;
}
/**
 * Return >0 if error is set
 * @return 
 */
public int _Nis_error(void)
{
    return M_nstd_error;
}

/**
 * Append error message
 * @param msg
 */
public void _Nappend_error_msg(char *msg)
{
    int free_space = MAX_ERROR_LEN-strlen(M_nstd_error_msg_buf);
    int app_error_len = strlen(msg);
    int n;
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(M_nstd_error_msg_buf, msg, n);
}

