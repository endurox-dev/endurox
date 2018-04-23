/* 
** Enduro/X standard library - error handling
**
** @file nerror.c
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
#include <nstd_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Function returns error description
 */
#define NSTD_ERROR_DESCRIPTION(X) (M_nstd_error_defs[X<NMINVAL?NMINVAL:(X>NMAXVAL?NMAXVAL:X)].msg)
#define NSTD_ERROR_CODESTR(X) (M_nstd_error_defs[X<NMINVAL?NMINVAL:(X>NMAXVAL?NMAXVAL:X)].codestr)

#define STDE(X) X,  #X

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
} M_nstd_error_defs [] =
{
    {STDE(BMINVAL),       "No error"},        /* 0 */
    {STDE(NEINVALINI),    "Invalid INI file"}, /* 1 */
    {STDE(NEMALLOC),      "Malloc failed"},    /* 2 */
    {STDE(NEUNIX),        "Unix system call failed"},    /* 3 */
    {STDE(NEINVAL),       "Invalid paramter passed to function"}, /* 4 */
    {STDE(NESYSTEM),      "System failure"}, /* 5 */
    {STDE(NEMANDATORY),   "Mandatory key/field missing"}, /* 6 */
    {STDE(NEFORMAT),      "Invalid format"}, /* 7 */
    {STDE(NETOUT),        "Timed-out"}, /* 8 */
    {STDE(NENOCONN),      "No Connection"}, /* 9 */
    {STDE(NELIMIT),       "Limit reached"}, /* 10 */
    {STDE(NEPLUGIN),      "Plugin error"}, /* 11 */
    {STDE(NENOSPACE),     "No space to store output buffer"}, /* 12 */
    {STDE(NEINVALKEY),    "Probably invalid encryption key"} /* 13 */
};
/*---------------------------Prototypes---------------------------------*/
/**
 * Standard.
 * Printer error to stderr
 * @param str
 */
expublic void N_error (char *str)
{
    NSTD_TLS_ENTRY;
    if (EXEOS!=G_nstd_tls->M_nstd_error_msg_buf[0])
        fprintf(stderr, "%s:%d:%s (%s)\n", str, 
                G_nstd_tls->M_nstd_error, 
                NSTD_ERROR_DESCRIPTION(G_nstd_tls->M_nstd_error),
                G_nstd_tls->M_nstd_error_msg_buf);
    else
        fprintf(stderr, "%s:%d:%s\n", str, 
                G_nstd_tls->M_nstd_error, 
                NSTD_ERROR_DESCRIPTION(G_nstd_tls->M_nstd_error));
}

/**
 * Return error code in string format
 * note in case of invalid error code, the max or min values will be returned.
 * @param err error code
 * @return ptr to constant string (error code)
 */
expublic char * ndrx_Necodestr(int err)
{
    return NSTD_ERROR_CODESTR(err);
}

/**
 * Extended version. This will print internal error message what happened.
 * This is not thread safe (as all other functions).
 * @param error_code
 */
expublic char * Nstrerror (int err)
{
    NSTD_TLS_ENTRY;
   
    if (EXEOS!=G_nstd_tls->M_nstd_error_msg_buf[0])
    {
        snprintf(G_nstd_tls->errbuf, sizeof(G_nstd_tls->errbuf), 
                                "%d:%s (last error %d: %s)",
                                err,
                                NSTD_ERROR_DESCRIPTION(err),
                                G_nstd_tls->M_nstd_error,
                                G_nstd_tls->M_nstd_error_msg_buf);
    }
    else
    {
        snprintf(G_nstd_tls->errbuf, sizeof(G_nstd_tls->errbuf), "%d:%s",
                            err, NSTD_ERROR_DESCRIPTION(err));
    }

    return G_nstd_tls->errbuf;
}

/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
expublic int * _Nget_Nerror_addr (void)
{
    NSTD_TLS_ENTRY;
    return &(G_nstd_tls->M_nstd_error);
}

/**
 * Internetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void _Nset_error(int error_code)
{
    NSTD_TLS_ENTRY;
    if (!G_nstd_tls->M_nstd_error)
    {
/*
        NDRX_LOG(log_warn, "_Nset_error: %d (%s)", 
                                error_code, NSTD_ERROR_DESCRIPTION(error_code));
 */
        G_nstd_tls->M_nstd_error_msg_buf[0] = EXEOS;
        G_nstd_tls->M_nstd_error = error_code;
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void _Nset_error_msg(int error_code, char *msg)
{
    int msg_len;
    int err_len;
    NSTD_TLS_ENTRY;
    
    if (!G_nstd_tls->M_nstd_error)
    {
        msg_len = strlen(msg);
        err_len = (msg_len>MAX_ERROR_LEN)?MAX_ERROR_LEN:msg_len;

        G_nstd_tls->M_nstd_error_msg_buf[0] = EXEOS;
        strncat(G_nstd_tls->M_nstd_error_msg_buf, msg, err_len);
        G_nstd_tls->M_nstd_error = error_code;
    }
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
expublic void _Nset_error_fmt(int error_code, const char *fmt, ...)
{
    char msg[MAX_ERROR_LEN+1] = {EXEOS};
    va_list ap;
    NSTD_TLS_ENTRY;

    if (!G_nstd_tls->M_nstd_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        NDRX_STRCPY_SAFE(G_nstd_tls->M_nstd_error_msg_buf, msg);
        G_nstd_tls->M_nstd_error = error_code;
        
    }
}

/**
 * Unset any error data currently in use
 */
expublic void _Nunset_error(void)
{
    NSTD_TLS_ENTRY;
    G_nstd_tls->M_nstd_error_msg_buf[0]=EXEOS;
    G_nstd_tls->M_nstd_error = BMINVAL;
}
/**
 * Return >0 if error is set
 * @return 
 */
expublic int _Nis_error(void)
{
    NSTD_TLS_ENTRY;   
    return G_nstd_tls->M_nstd_error;
}

/**
 * Append error message
 * @param msg
 */
expublic void _Nappend_error_msg(char *msg)
{
    int free_space;
    int app_error_len = strlen(msg);
    int n;
    NSTD_TLS_ENTRY;
    
    free_space = MAX_ERROR_LEN-strlen(G_nstd_tls->M_nstd_error_msg_buf);
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(G_nstd_tls->M_nstd_error_msg_buf, msg, n);
}

