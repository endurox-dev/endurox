/* 
 * @brief UBF error handling routines.
 *   Also provides functions for general purpose error handling
 *   The emulator of UBF library
 *   Enduro Execution Library
 *
 * @file ferror.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
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
#include <ferror.h>
#include <errno.h>
#include <ubf_tls.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Function returns error description
 */
#define UBF_ERROR_DESCRIPTION(X) (M_ubf_error_defs[X<BMINVAL?BMINVAL:(X>BMAXVAL?BMAXVAL:X)].msg)
#define UBF_ERROR_CODESTR(X) (M_ubf_error_defs[X<BMINVAL?BMINVAL:(X>BMAXVAL?BMAXVAL:X)].codestr)


#define UBFE(X) X,  #X

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* Do we need this to be in message catalogue?
 * table bellow is indexed by id (i.e. direct array erorr lookup should work)
 */
struct err_msg
{
    int id;
    char *codestr;
    char *msg;
} M_ubf_error_defs [] =
{
    {UBFE(BMINVAL),   "No error"}, /* 0 */
    {UBFE(BERFU0),    "BERFU0"}, /* 1 */
    {UBFE(BALIGNERR), "Bisubf buffer not aligned"}, /* 2 */
    {UBFE(BNOTFLD),   "Buffer not bisubf"}, /* 3 */
    {UBFE(BNOSPACE),  "No space in bisubf buffer"}, /* 4 */
    {UBFE(BNOTPRES),  "Field not present"}, /* 5 */
    {UBFE(BBADFLD),   "Unknown field number or type"}, /* 6 */
    {UBFE(BTYPERR),   "Illegal field type"}, /* 7 */
    {UBFE(BEUNIX),    "Unix system call error"}, /* 8 */
    {UBFE(BBADNAME),  "Unknown field name"}, /* 9 */
    {UBFE(BMALLOC),   "Malloc failed"}, /* 10 */
    {UBFE(BSYNTAX),   "Bad syntax in boolean expression"}, /* 11 */
    {UBFE(BFTOPEN),   "Cannot find or open field table"}, /* 12 */
    {UBFE(BFTSYNTAX), "Syntax error in field table"}, /* 13 */
    {UBFE(BEINVAL),   "Invalid argument to function"}, /* 14 */
    {UBFE(BERFU1),    "BERFU1"}, /* 15 */
    {UBFE(BBADTBL),   "Bad access to field table"}, /* 16 */
    {UBFE(BBADVIEW),  "Bad view name/not loaded"}, /* 17 */
    {UBFE(BVFSYNTAX), "View file syntax error"}, /* 18 */
    {UBFE(BVFOPEN),   "Failed to open view file"}, /* 19 */
    {UBFE(BBADACM),   "View array count indicator negative"}, /* 20 */
    {UBFE(BNOCNAME),  "Structure field (cname) not found"},
    {UBFE(BEBADOP),  "Bad operation"}, /* 21 */
};
/*---------------------------Prototypes---------------------------------*/

/**
 * Return error code in string format
 * note in case of invalid error code, the max or min values will be returned.
 * @param err error code
 * @return ptr to constant string (error code)
 */
expublic char * Becodestr(int err)
{
    return UBF_ERROR_CODESTR(err);
}
/**
 * Standard.
 * Printer error to stderr
 * @param str
 */
expublic void B_error (char *str)
{
    UBF_TLS_ENTRY;
    if (EXEOS!=G_ubf_tls->M_ubf_error_msg_buf[0])
    {
        fprintf(stderr, "%s:%d:%s (%s)\n", str, G_ubf_tls->M_ubf_error, 
                UBF_ERROR_DESCRIPTION(G_ubf_tls->M_ubf_error),
                G_ubf_tls->M_ubf_error_msg_buf);
    }
    else
    {
        fprintf(stderr, "%s:%d:%s\n", str, G_ubf_tls->M_ubf_error, 
                UBF_ERROR_DESCRIPTION(G_ubf_tls->M_ubf_error));
    }
}

/**
 * Extended version. This will print internal error message what happened.
 * This is not thread safe (as all other functions).
 * @param error_code
 */
expublic char * Bstrerror (int err)
{
    UBF_TLS_ENTRY;

    if (EXEOS!=G_ubf_tls->M_ubf_error_msg_buf[0])
    {
        snprintf(G_ubf_tls->errbuf, sizeof(G_ubf_tls->errbuf), 
                "%d:%s (last error %d: %s)",
                err,
                UBF_ERROR_DESCRIPTION(err),
                G_ubf_tls->M_ubf_error,
                G_ubf_tls->M_ubf_error_msg_buf);
    }
    else
    {
        snprintf(G_ubf_tls->errbuf, sizeof(G_ubf_tls->errbuf), "%d:%s",
                            err, UBF_ERROR_DESCRIPTION(err));
    }

    return G_ubf_tls->errbuf;
}

/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
expublic int * ndrx_Bget_Ferror_addr (void)
{
    UBF_TLS_ENTRY;
    return &G_ubf_tls->M_ubf_error;
}

/**
 * For ABI compatiblity with older Enduro/X versions.
 */
expublic int * _Bget_Ferror_addr (void)
{
    return ndrx_Bget_Ferror_addr();
}

/**
 * Internetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void ndrx_Bset_error(int error_code)
{
    UBF_TLS_ENTRY;
    if (!G_ubf_tls->M_ubf_error)
    {
        UBF_LOG(log_warn, "%s: %d (%s)", __func__,
                                error_code, UBF_ERROR_DESCRIPTION(error_code));
        G_ubf_tls->M_ubf_error_msg_buf[0] = EXEOS;
        G_ubf_tls->M_ubf_error = error_code;
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void ndrx_Bset_error_msg(int error_code, char *msg)
{
    int msg_len;
    int err_len;

    UBF_TLS_ENTRY;
    
    if (!G_ubf_tls->M_ubf_error)
    {
        msg_len = strlen(msg);
        err_len = (msg_len>MAX_ERROR_LEN)?MAX_ERROR_LEN:msg_len;


        UBF_LOG(log_warn, "%s: %d (%s) [%s]", __func__, error_code,
                                UBF_ERROR_DESCRIPTION(error_code), msg);
        G_ubf_tls->M_ubf_error_msg_buf[0] = EXEOS;
        strncat(G_ubf_tls->M_ubf_error_msg_buf, msg, err_len);
        G_ubf_tls->M_ubf_error = error_code;
    }
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
expublic void ndrx_Bset_error_fmt(int error_code, const char *fmt, ...)
{
    char msg[MAX_ERROR_LEN+1] = {EXEOS};
    va_list ap;
    UBF_TLS_ENTRY;
    
    if (!G_ubf_tls->M_ubf_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        NDRX_STRCPY_SAFE(G_ubf_tls->M_ubf_error_msg_buf, msg);
        G_ubf_tls->M_ubf_error = error_code;

        UBF_LOG(log_warn, "%s: %d (%s) [%s]", __func__,
                        error_code, UBF_ERROR_DESCRIPTION(error_code),
                        G_ubf_tls->M_ubf_error_msg_buf);
    }
}

/**
 * Unset any error data currently in use
 */
expublic void ndrx_Bunset_error(void)
{
    UBF_TLS_ENTRY;
    G_ubf_tls->M_ubf_error_msg_buf[0]=EXEOS;
    G_ubf_tls->M_ubf_error = BMINVAL;
}
/**
 * Return >0 if error is set
 * @return 
 */
expublic int ndrx_Bis_error(void)
{
    UBF_TLS_ENTRY;
    return G_ubf_tls->M_ubf_error;
}

/**
 * Append error message
 * @param msg
 */
expublic void ndrx_Bappend_error_msg(char *msg)
{
    int free_space;
    int app_error_len = strlen(msg);
    int n;
    UBF_TLS_ENTRY;
    
    free_space = MAX_ERROR_LEN-strlen(G_ubf_tls->M_ubf_error_msg_buf);
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(G_ubf_tls->M_ubf_error_msg_buf, msg, n);
}

/**
 * Report regular expression error.
 * @param fun_nm
 * @param err
 * @param rp
 */
expublic void ndrx_report_regexp_error(char *fun_nm, int err, regex_t *rp)
{
    char *errmsg;
    int errlen;
    char errbuf[2048];
    errlen = (int) regerror(err, rp, NULL, 0);
    errmsg = (char *) NDRX_MALLOC(errlen*sizeof(char));
    regerror(err, rp, errmsg, errlen);

    snprintf(errbuf, sizeof(errbuf), "regexp err (%s, %d, \"%s\").",
                                        fun_nm, err, errmsg);
    UBF_LOG(log_error, "Failed to compile regexp: [%s]", errbuf);
    ndrx_Bset_error_msg(BSYNTAX, errbuf);

    NDRX_FREE(errmsg);
}

