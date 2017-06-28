/* 
** ATMI error library
** Also used by XA lib & TM process.
**
** @file tperror.c
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
#include <atmi.h>
#include <ndebug.h>
#include <tperror.h>
#include <Exfields.h>

#include <xa.h>
#include <atmi_int.h>
#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/**
 * Function returns error description
 */
#define ATMI_ERROR_DESCRIPTION(X) (M_atmi_error_defs[X<TPMINVAL?TPMINVAL:(X>TPMAXVAL?TPMAXVAL:X)].msg)
#define TP_ERROR(X) X, #X
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
	char *msg;
} M_atmi_error_defs [] =
{
    {TP_ERROR(TPMINVAL)},	/* 0 - minimum error message */
    {TP_ERROR(TPEABORT)}, /* 1 */
    {TP_ERROR(TPEBADDESC)}, /* 2 */
    {TP_ERROR(TPEBLOCK)}, /* 3 */
    {TP_ERROR(TPEINVAL)}, /* 4 */
    {TP_ERROR(TPELIMIT)}, /* 5 */
    {TP_ERROR(TPENOENT)}, /* 6 */
    {TP_ERROR(TPEOS)}, /* 7 */
    {TP_ERROR(TPEPERM)}, /* 8 */
    {TP_ERROR(TPEPROTO)}, /* 9 */
    {TP_ERROR(TPESVCERR)}, /* 10 */
    {TP_ERROR(TPESVCFAIL)}, /* 11 */
    {TP_ERROR(TPESYSTEM)}, /* 12 */
    {TP_ERROR(TPETIME)}, /* 13 */
    {TP_ERROR(TPETRAN)}, /* 14 */
    {TP_ERROR(TPGOTSIG)}, /* 15 */
    {TP_ERROR(TPERMERR)}, /* 16 */
    {TP_ERROR(TPEITYPE)}, /* 17 */
    {TP_ERROR(TPEOTYPE)}, /* 18 */
    {TP_ERROR(TPERELEASE)}, /* 19 */
    {TP_ERROR(TPEHAZARD)}, /* 20 */
    {TP_ERROR(TPEHEURISTIC)}, /* 21 */
    {TP_ERROR(TPEEVENT)}, /* 22 */
    {TP_ERROR(TPEMATCH)}, /* 23 */
    {TP_ERROR(TPEDIAGNOSTIC)}, /* 24 */
    {TP_ERROR(TPEMIB)}, /* 25 */
    {TP_ERROR(TPERFU26)}, /* 26 */
    {TP_ERROR(TPERFU27)}, /* 27 */
    {TP_ERROR(TPERFU28)}, /* 28 */
    {TP_ERROR(TPERFU29)}, /* 29 */
    {TP_ERROR(TPINITFAIL)}, /* 30 */
    {TP_ERROR(TPMAXVAL)} /* 31 - maximum error message */
};

/**
 * XA api error codes
 */
struct err_msg_xa
{
	short errcode;
	char *msg;
} M_atmi_xa_error_defs [] =
{
{XA_RBBASE, "the inclusive lower bound of the rollback codes"},
{XA_RBROLLBACK, "the rollback was caused by an unspecified reason"},
{XA_RBCOMMFAIL, "the rollback was caused by a communication failure"},
{XA_RBDEADLOCK, "a deadlock was detected"},
{XA_RBINTEGRITY, "a condition that violates the integrity of the resources was detected"},
{XA_RBOTHER, "the resource manager rolled back the transaction branch for a reason not on this list"},
{XA_RBPROTO, "a protocol error occurred in the resource manager"},
{XA_RBTIMEOUT, "a transaction branch took too long"},
{XA_RBTRANSIENT, "may retry the transaction branch"},
{XA_RBEND, " the inclusive upper bound of the rollback codes"},
{XA_NOMIGRATE, "resumption must occur where suspension occurred"},
{XA_HEURHAZ, "the transaction branch may have been heuristically completed"},
{XA_HEURCOM, "the transaction branch has been heuristically committed"},
{XA_HEURRB, "the transaction branch has been heuristically rolled back"},
{XA_HEURMIX, "the transaction branch has been heuristically committed and rolled back"},
{XA_RETRY, "routine returned with no effect and may be reissued"},
{XA_RDONLY, "the transaction branch was read-only and has been committed"},
{XA_OK, "normal execution"},
{XAER_ASYNC, "asynchronous operation already outstanding"},
{XAER_RMERR, "a resource manager error occurred in the transaction branch"},
{XAER_NOTA, "the XID is not valid"},
{XAER_INVAL, "invalid arguments were given"},
{XAER_PROTO, "routine invoked in an improper context"},
{XAER_RMFAIL, "resource manager unavailable"},
{XAER_DUPID, "the XID already exists"},
{XAER_OUTSIDE, "resource manager doing work outside global transaction"},
};

/*---------------------------Prototypes---------------------------------*/
/**
 * Standard.
 * Printer error to stderr
 * @param str
 */
expublic void TP_error (char *str)
{
    ATMI_TLS_ENTRY;
    
    if (EXEOS!=G_atmi_tls->M_atmi_error_msg_buf[0])
    {
        fprintf(stderr, "%s:%d:%s (%s)\n", str, 
                G_atmi_tls->M_atmi_error, 
                ATMI_ERROR_DESCRIPTION(G_atmi_tls->M_atmi_error),
                G_atmi_tls->M_atmi_error_msg_buf);
    }
    else
    {
        fprintf(stderr, "%s:%d:%s\n", str, G_atmi_tls->M_atmi_error, 
                ATMI_ERROR_DESCRIPTION(G_atmi_tls->M_atmi_error));
    }
}

/**
 * Extended version. This will print internal error message what happened.
 * This is not thread safe (as all other functions).
 * @param error_code
 */
expublic char * tpstrerror (int err)
{
    ATMI_TLS_ENTRY;
    if (EXEOS!=G_atmi_tls->M_atmi_error_msg_buf[0])
    {
        snprintf(G_atmi_tls->errbuf, sizeof(G_atmi_tls->errbuf), 
                "%d:%s (last error %d: %s)",
                err,
                ATMI_ERROR_DESCRIPTION(err),
                G_atmi_tls->M_atmi_error,
                G_atmi_tls->M_atmi_error_msg_buf);
    }
    else
    {
        snprintf(G_atmi_tls->errbuf, sizeof(G_atmi_tls->errbuf), "%d:%s",
                    err, ATMI_ERROR_DESCRIPTION(err));
    }

    return G_atmi_tls->errbuf;
}

/**
 * ATMI standard
 * @return - pointer to int holding error code?
 */
expublic int * _exget_tperrno_addr (void)
{
    ATMI_TLS_ENTRY;
    return &G_atmi_tls->M_atmi_error;
}

/**
 * Internetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void _TPset_error(int error_code)
{
    ATMI_TLS_ENTRY;
    if (!G_atmi_tls->M_atmi_error)
    {
        NDRX_LOG(log_warn, "_TPset_error: %d (%s)",
                error_code, ATMI_ERROR_DESCRIPTION(error_code));
        
        G_atmi_tls->M_atmi_error_msg_buf[0] = EXEOS;
        G_atmi_tls->M_atmi_error = error_code;
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void _TPset_error_msg(int error_code, char *msg)
{
    int msg_len;
    int err_len;
    ATMI_TLS_ENTRY;

    if (!G_atmi_tls->M_atmi_error)
    {
        msg_len = strlen(msg);
        err_len = (msg_len>MAX_TP_ERROR_LEN)?MAX_TP_ERROR_LEN:msg_len;


        NDRX_LOG(log_warn, "_TPset_error_msg: %d (%s) [%s]", error_code,
                                ATMI_ERROR_DESCRIPTION(error_code), msg);
        G_atmi_tls->M_atmi_error_msg_buf[0] = EXEOS;
        strncat(G_atmi_tls->M_atmi_error_msg_buf, msg, err_len);
        G_atmi_tls->M_atmi_error = error_code;
    }
}


/**
 * Override the current error code.
 * @param error_code
 */
expublic void _TPoverride_code(int error_code)
{
    ATMI_TLS_ENTRY;
    G_atmi_tls->M_atmi_error = error_code;
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
expublic void _TPset_error_fmt(int error_code, const char *fmt, ...)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    va_list ap;
    ATMI_TLS_ENTRY;

    if (!G_atmi_tls->M_atmi_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        strcpy(G_atmi_tls->M_atmi_error_msg_buf, msg);
        G_atmi_tls->M_atmi_error = error_code;

        NDRX_LOG(log_warn, "_TPset_error_fmt: %d (%s) [%s]",
                        error_code, ATMI_ERROR_DESCRIPTION(error_code),
                        G_atmi_tls->M_atmi_error_msg_buf);
    }
}

/**
 * Extended error setter, including reason (for XA mostly)
 * @param error_code
 * @param reason
 * @param fmt
 * @param ...
 */
expublic void _TPset_error_fmt_rsn(int error_code, short reason, const char *fmt, ...)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    va_list ap;
    ATMI_TLS_ENTRY;

    if (!G_atmi_tls->M_atmi_error)
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        strcpy(G_atmi_tls->M_atmi_error_msg_buf, msg);
        G_atmi_tls->M_atmi_error = error_code;
        G_atmi_tls->M_atmi_reason = reason;

        NDRX_LOG(log_warn, "_TPset_error_fmt_rsn: %d (%s) reason: %d [%s]",
                        error_code, ATMI_ERROR_DESCRIPTION(error_code), reason,
                        G_atmi_tls->M_atmi_error_msg_buf);
    }
}

/**
 * Unset any error data currently in use
 */
expublic void _TPunset_error(void)
{
    ATMI_TLS_ENTRY;
    
    G_atmi_tls->M_atmi_error_msg_buf[0]=EXEOS;
    G_atmi_tls->M_atmi_error = BMINVAL;
    G_atmi_tls->M_atmi_reason = NDRX_XA_ERSN_NONE;
}
/**
 * Return >0 if error is set
 * @return 
 */
expublic int _TPis_error(void)
{
    ATMI_TLS_ENTRY;
    return G_atmi_tls->M_atmi_error;
}

/**
 * Append error message
 * @param msg
 */
expublic void _TPappend_error_msg(char *msg)
{
    int free_space;
    int app_error_len = strlen(msg);
    int n;
    
    ATMI_TLS_ENTRY;
    
    free_space = MAX_TP_ERROR_LEN-strlen(G_atmi_tls->M_atmi_error_msg_buf);
    
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(G_atmi_tls->M_atmi_error_msg_buf, msg, n);
}

/* <XA Error handling - used by ATMI lib & TM server>*/

/**
 * Return XA reason code (if set)
 * @return 
 */
expublic short atmi_xa_get_reason(void)
{   
    ATMI_TLS_ENTRY;
    return G_atmi_tls->M_atmi_reason;
}

/**
 * Internal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void atmi_xa_set_error(UBFH *p_ub, short error_code, short reason)
{
    if (!atmi_xa_is_error(p_ub))
    {
        NDRX_LOG(log_warn, "atmi_xa_set_error: %d (%s)",
                                error_code, ATMI_ERROR_DESCRIPTION(error_code));
        Bchg(p_ub, TMERR_CODE, 0, (char *)&error_code, 0L);
        Bchg(p_ub, TMERR_REASON, 0, (char *)&reason, 0L);
    }
}

/**
 * I nternetal function for setting
 * @param error_code
 * @param msg
 * @return
 */
expublic void atmi_xa_set_error_msg(UBFH *p_ub, short error_code, short reason, char *msg)
{
    if (!atmi_xa_is_error(p_ub))
    {
        NDRX_LOG(log_warn, "atmi_xa_set_error_msg: %d (%s) [%s]", error_code,
                                ATMI_ERROR_DESCRIPTION(error_code), msg);
        
        Bchg(p_ub, TMERR_CODE, 0, (char *)&error_code, 0L);
        Bchg(p_ub, TMERR_REASON, 0, (char *)&reason, 0L);
        Bchg(p_ub, TMERR_MSG, 0, msg, 0L);
    }
}

/**
 * Advanced error setting function, uses format list
 * Use this only in case where it is really needed.
 * @param error_code - error code
 * @param fmt - format stirng
 * @param ... - format details
 */
expublic void atmi_xa_set_error_fmt(UBFH *p_ub, short error_code, short reason, const char *fmt, ...)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    va_list ap;

    if (!atmi_xa_is_error(p_ub))
    {
        va_start(ap, fmt);
        (void) vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);

        NDRX_LOG(log_warn, "atmi_xa_set_error_fmt: %d (%s) [%s]",
                        error_code, ATMI_ERROR_DESCRIPTION(error_code),
                        msg);
        Bchg(p_ub, TMERR_CODE, 0, (char *)&error_code, 0L);
        Bchg(p_ub, TMERR_REASON, 0, (char *)&reason, 0L);
        Bchg(p_ub, TMERR_MSG, 0, msg, 0L);
    }
}

/**
 * Override XA error code.
 * @param p_ub
 * @param error_code
 */
expublic void atmi_xa_override_error(UBFH *p_ub, short error_code)
{
    NDRX_LOG(log_warn, "atmi_xa_override_error: %d (%s)",
                    error_code, ATMI_ERROR_DESCRIPTION(error_code));
    Bchg(p_ub, TMERR_CODE, 0, (char *)&error_code, 0L);
}


/**
 * Unset any error data currently in use
 */
expublic void atmi_xa_unset_error(UBFH *p_ub)
{
        Bdel(p_ub, TMERR_CODE, 0);
        Bdel(p_ub, TMERR_REASON, 0);
        Bdel(p_ub, TMERR_MSG, 0);
}
/**
 * Return >0 if error is set
 * @return 
 */
expublic int atmi_xa_is_error(UBFH *p_ub)
{
    short errcode = TPMINVAL;
    Bget(p_ub, TMERR_CODE, 0, (char *)&errcode, 0L);
    
    return (int)errcode;
}

/**
 * Append error message
 * @param msg
 */
expublic void atmi_xa_error_msg(UBFH *p_ub, char *msg)
{
    char tmp[MAX_TP_ERROR_LEN+1] = {EXEOS};
    
    Bget(p_ub, TMERR_MSG, 0, tmp, 0L);
    
    int free_space = MAX_TP_ERROR_LEN-strlen(tmp);
    int app_error_len = strlen(msg);
    int n;
    n = free_space<app_error_len?free_space:app_error_len;
    strncat(tmp, msg, n);
    
    Bchg(p_ub, TMERR_MSG, 0, tmp, 0L);
}

/**
 * Convert XA FB error to ATMI lib error
 * @param p_ub - response from TM.
 */
expublic void atmi_xa2tperr(UBFH *p_ub)
{
    char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
    short code;
    short reason = 0;
    ATMI_TLS_ENTRY;
    
    /* unset the ATMI error. */
    if (Bpres(p_ub, TMERR_CODE, 0))
    {
        _TPunset_error();

        Bget(p_ub, TMERR_CODE, 0, (char *)&code, 0L);
        Bget(p_ub, TMERR_MSG, 0, msg, 0L);
        Bget(p_ub, TMERR_REASON, 0, (char *)&reason, 0L);

        _TPset_error_msg((int)code, msg);

        /* Append with reason code. */
        if (!G_atmi_tls->M_atmi_reason)
        {
            G_atmi_tls->M_atmi_reason = reason;
        }
        
    }
}

/**
 * return human readable error code for XA api
 * @param 
 * @return 
 */
expublic char *atmi_xa_geterrstr(int code)
{
    int i;
    static char* unknown_err = "Unknown error";
    char *ret = unknown_err;
    
    for (i=0; i<N_DIM(M_atmi_xa_error_defs); i++)
    {
        if (code == M_atmi_xa_error_defs[i].errcode)
        {
            ret= M_atmi_xa_error_defs[i].msg;
            goto out;
        }
    }
    
out:
        
    return ret;
}

/**
 * Approve request
 * @param code
 * @return 
 */
expublic void atmi_xa_approve(UBFH *p_ub)
{
    atmi_xa_set_error_msg(p_ub, 0, NDRX_XA_ERSN_NONE, "Success");
}
/* </XA Error handling>*/



/**
 * Save current error
 * @param p_err
 */
expublic void _TPsave_error(atmi_error_t *p_err)
{
    ATMI_TLS_ENTRY;
    
    p_err->atmi_error = G_atmi_tls->M_atmi_error;
    p_err->atmi_reason = G_atmi_tls->M_atmi_reason;
    strcpy(p_err->atmi_error_msg_buf, G_atmi_tls->M_atmi_error_msg_buf);
}

/**
 * Restore current error
 * @param p_err
 */
expublic void _TPrestore_error(atmi_error_t *p_err)
{
    ATMI_TLS_ENTRY;
    G_atmi_tls->M_atmi_error = p_err->atmi_error;
    G_atmi_tls->M_atmi_reason = p_err->atmi_reason;
    strcpy(G_atmi_tls->M_atmi_error_msg_buf, p_err->atmi_error_msg_buf);
}
