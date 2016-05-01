/* 
** Persistent queue commons (used between tmqsrv and "userspace" api)
**
** @file qcommon.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
#include <Exfields.h>
#include <qcommon.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   OFFSET(s,e), ELEM_SIZE(s,e)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * TPQCTL structure mappings.
 */
static ubf_c_map_t M_tpqctl_map[] = 
{
    {EX_QFLAGS,         0, OFSZ(TPQCTL, flags),         BFLD_LONG}, /* 0 */
    {EX_QDEQ_TIME,      0, OFSZ(TPQCTL, deq_time),      BFLD_LONG}, /* 1 */
    {EX_QPRIORITY,      0, OFSZ(TPQCTL, priority),      BFLD_LONG}, /* 2 */
    {EX_QDIAGNOSTIC,    0, OFSZ(TPQCTL, diagnostic),    BFLD_LONG}, /* 3 */
    {EX_QMSGID,         0, OFSZ(TPQCTL, msgid),         BFLD_CARRAY}, /* 4 */
    {EX_QCORRID,        0, OFSZ(TPQCTL, corrid),        BFLD_STRING}, /* 5 */
    {EX_QREPLYQUEUE,    0, OFSZ(TPQCTL, replyqueue),    BFLD_STRING}, /* 6 */
    {EX_QFAILUREQUEUE,  0, OFSZ(TPQCTL, failurequeue),  BFLD_STRING}, /* 7 */
    {EX_CLTID,          0, OFSZ(TPQCTL, cltid),         BFLD_STRING}, /* 8 */
    {EX_QURCODE,        0, OFSZ(TPQCTL, urcode),        BFLD_LONG}, /* 9 */
    {EX_QAPPKEY,        0, OFSZ(TPQCTL, appkey),        BFLD_LONG}, /* 10 */
    {EX_QDELIVERY_QOS,  0, OFSZ(TPQCTL, delivery_qos),  BFLD_LONG}, /* 11 */
    {EX_QREPLY_QOS,     0, OFSZ(TPQCTL, reply_qos),     BFLD_LONG}, /* 12 */
    {EX_QEXP_TIME,      0, OFSZ(TPQCTL, exp_time),      BFLD_LONG}, /* 13 */
    {EX_QDIAGMSG,       0, OFSZ(TPQCTL, diagmsg),       BFLD_STRING}, /* 14 */
    {BBADFLDID}
};

/**
 * Enqueue request structure
 */
static long M_tpqctl_enqreq[] = 
{
    UBFUTIL_EXPORT,/* 0 - EX_QFLAGS*/
    UBFUTIL_EXPORT,/* 1 - EX_QDEQ_TIME*/
    UBFUTIL_EXPORT,/* 2 - EX_QPRIORITY*/
    0,             /* 3 - EX_QDIAGNOSTIC*/
    UBFUTIL_EXPORT,/* 4 - EX_QMSGID*/
    UBFUTIL_EXPORT,/* 5 - EX_QCORRID*/
    UBFUTIL_EXPORT,/* 6 - EX_QREPLYQUEUE*/
    UBFUTIL_EXPORT,/* 7 - EX_QFAILUREQUEUE*/
    UBFUTIL_EXPORT,/* 8 - EX_CLTID*/
    UBFUTIL_EXPORT,/* 9 - EX_QURCODE*/
    UBFUTIL_EXPORT,/* 10 - EX_QAPPKEY*/
    UBFUTIL_EXPORT,/* 11 - EX_QDELIVERY_QOS*/
    UBFUTIL_EXPORT,/* 12 - EX_QREPLY_QOS*/
    UBFUTIL_EXPORT,/* 13 - EX_QEXP_TIME*/
    0              /* 14 - EX_QDIAGMSG*/
};

/**
 * Enqueue response structure
 */
static long M_tpqctl_enqrsp[] = 
{
    UBFUTIL_EXPORT,/* 0 - EX_QFLAGS*/
    0,/* 1 - EX_QDEQ_TIME*/
    0,/* 2 - EX_QPRIORITY*/
    UBFUTIL_EXPORT,/* 3 - EX_QDIAGNOSTIC*/
    UBFUTIL_EXPORT,/* 4 - EX_QMSGID*/
    0,/* 5 - EX_QCORRID*/
    0,/* 6 - EX_QREPLYQUEUE*/
    0,/* 7 - EX_QFAILUREQUEUE*/
    0,/* 8 - EX_CLTID*/
    0,/* 9 - EX_QURCODE*/
    0,/* 10 - EX_QAPPKEY*/
    0,/* 11 - EX_QDELIVERY_QOS*/
    0,/* 12 - EX_QREPLY_QOS*/
    0,/* 13 - EX_QEXP_TIME*/
    UBFUTIL_EXPORT              /* 14 - EX_QDIAGMSG*/
};


/**
 * dequeue request structure
 */
static long M_tpqctl_deqreq[] = 
{
    UBFUTIL_EXPORT,/* 0 - EX_QFLAGS*/
    0,/* 1 - EX_QDEQ_TIME*/
    0,/* 2 - EX_QPRIORITY*/
    0,             /* 3 - EX_QDIAGNOSTIC*/
    UBFUTIL_EXPORT,/* 4 - EX_QMSGID*/
    UBFUTIL_EXPORT,/* 5 - EX_QCORRID*/
    0,/* 6 - EX_QREPLYQUEUE*/
    0,/* 7 - EX_QFAILUREQUEUE*/
    0,/* 8 - EX_CLTID*/
    0,/* 9 - EX_QURCODE*/
    0,/* 10 - EX_QAPPKEY*/
    0,/* 11 - EX_QDELIVERY_QOS*/
    0,/* 12 - EX_QREPLY_QOS*/
    0,/* 13 - EX_QEXP_TIME*/
    0              /* 14 - EX_QDIAGMSG*/
};

/**
 * Dequeue response structure
 */
static long M_tpqctl_deqrsp[] = 
{
    UBFUTIL_EXPORT,/* 0 - EX_QFLAGS*/
    0,/* 1 - EX_QDEQ_TIME*/
    UBFUTIL_EXPORT,/* 2 - EX_QPRIORITY*/
    UBFUTIL_EXPORT,/* 3 - EX_QDIAGNOSTIC*/
    UBFUTIL_EXPORT,/* 4 - EX_QMSGID*/
    UBFUTIL_EXPORT,/* 5 - EX_QCORRID*/
    UBFUTIL_EXPORT,/* 6 - EX_QREPLYQUEUE*/
    UBFUTIL_EXPORT,/* 7 - EX_QFAILUREQUEUE*/
    UBFUTIL_EXPORT,/* 8 - EX_CLTID*/
    UBFUTIL_EXPORT,/* 9 - EX_QURCODE*/
    UBFUTIL_EXPORT,/* 10 - EX_QAPPKEY*/
    UBFUTIL_EXPORT,/* 11 - EX_QDELIVERY_QOS*/
    UBFUTIL_EXPORT,/* 12 - EX_QREPLY_QOS*/
    0,/* 13 - EX_QEXP_TIME*/
    UBFUTIL_EXPORT              /* 14 - EX_QDIAGMSG*/
};


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_enqreq);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, p_ub, ctl, M_tpqctl_enqreq);
    
    return ret;
}


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_enqrsp);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, p_ub, ctl, M_tpqctl_enqrsp);
    
    return ret;
}

/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_deqreq);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, p_ub, ctl, M_tpqctl_deqreq);
    
    return ret;
}


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_to_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_deqrsp);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
public int tmq_tpqctl_from_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = SUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, p_ub, ctl, M_tpqctl_deqrsp);
    
    return ret;
}

/**************************** API SECTION *************************************/

/**
 * Internal version of message enqueue.
 * @param qspace service name
 * @param qname queue name
 * @param ctl control data
 * @param data data to enqueue
 * @param len data len
 * @param flags flags (for tpcall)
 * @return SUCCEED/FAIL
 */
public int _tpenqueue (char *qspace, char *qname, TPQCTL *ctl, 
        char *data, long len, long flags)
{
    int ret = SUCCEED;
    long rsplen;
    char cmd = TMQ_CMD_ENQUEUE;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", TMQ_DEFAULT_BUFSZ+len);
    
    
    if (NULL==data)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: data is null!");
        FAIL_OUT(ret);
    }
    
    if (NULL==qspace || EOS==*qspace)
    {
        _TPset_error_msg(TPEINVAL,  "_tpenqueue: empty or NULL qspace!");
        FAIL_OUT(ret);
    }
    
    if (NULL==qname || EOS==*qname)
    {
        _TPset_error_msg(TPEINVAL,  "_tpenqueue: empty or NULL qname!");
        FAIL_OUT(ret);
    }
    
    if (NULL==ctl)
    {
        _TPset_error_msg(TPEINVAL,  "_tpenqueue: NULL ctl!");
        FAIL_OUT(ret);
    }
    
    /* Alloc the request buffer */
    if (NULL == p_ub)
    {
        _TPset_error_msg(TPESYSTEM,  "_tpenqueue: Failed to allocate req buffer: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* Convert the structure to UBF */
    if (SUCCEED!=tmq_tpqctl_to_ubf_enqreq(p_ub, ctl))
    {
        
        _TPset_error_msg(TPEINVAL,  "_tpenqueue: failed convert ctl "
                "to internal UBF buf!");
        FAIL_OUT(ret);
    }
    
    /* set the data field */
    if (SUCCEED!=Bchg(p_ub, EX_DATA, 0, data, len))
    {
        _TPset_error_msg(TPESYSTEM,  "_tpenqueue: Failed to set data field: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* Setup the command in EX_QCMD */
    if (SUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        _TPset_error_msg(TPESYSTEM,  "_tpenqueue: Failed to set cmd field: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_QNAME, 0, qname, 0L))
    {
        _TPset_error_msg(TPESYSTEM,  "_tpenqueue: Failed to set qname field: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* do the call to queue system */
    if (FAIL == tpcall(qspace, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, flags))
    {
        int tpe = tperrno;
        NDRX_LOG(log_error, "%s failed: %s", qspace, tpstrerror(tpe));
        if (TPESVCFAIL!=tpe)
        {
            FAIL_OUT(ret);
        }
    }
    
    /* the call is ok (or app failed), convert back. */
    if (SUCCEED!=tmq_tpqctl_from_ubf_enqrsp(p_ub, ctl))
    {
        
        _TPset_error_msg(TPEINVAL,  "_tpenqueue: failed convert ctl "
                "to internal UBF buf!");
        FAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (ctl->diagnostic)
    {
        ret = TPEDIAGNOSTIC;
    }

    NDRX_LOG(log_info, "_tpenqueue: return %d", ret);

    return ret;    
}

/**
 * Internal version of message dequeue.
 * @param qspace service name
 * @param qname queue name
 * @param ctl control data
 * @param data data to enqueue
 * @param len data len
 * @param flags flags (for tpcall)
 * @return SUCCEED/FAIL
 */
public int _tpdequeue (char *qspace, char *qname, TPQCTL *ctl, 
        char **data, long *len, long flags)
{
    int ret = SUCCEED;
    long rsplen;
    char cmd = TMQ_CMD_ENQUEUE;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", TMQ_DEFAULT_BUFSZ);
    
    if (NULL==qspace || EOS==*qspace)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: empty or NULL qspace!");
        FAIL_OUT(ret);
    }
    
    if (NULL==qname || EOS==*qname)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: empty or NULL qname!");
        FAIL_OUT(ret);
    }
    
    if (NULL==ctl)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: NULL ctl!");
        FAIL_OUT(ret);
    }
    
    if (NULL==data)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: data is null!");
        FAIL_OUT(ret);
    }
    
    if (NULL==len)
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: len is null!");
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tptypes(*data, NULL, NULL))
    {
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: data buffer not allocated by "
                "tpalloc()");
        FAIL_OUT(ret);
    }
    
    /* Alloc the request buffer */
    if (NULL == p_ub)
    {
        _TPset_error_msg(TPESYSTEM,  "_tpdequeue: Failed to allocate req buffer: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* Convert the structure to UBF */
    if (SUCCEED!=tmq_tpqctl_to_ubf_deqreq(p_ub, ctl))
    {
        
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: failed convert ctl "
                "to internal UBF buf!");
        FAIL_OUT(ret);
    }
    
    /* set the data field */
    
    if (SUCCEED!=Bchg(p_ub, EX_QNAME, 0, qname, 0L))
    {
        _TPset_error_msg(TPESYSTEM,  "_tpdequeue: Failed to set qname field: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* Setup the command in EX_QCMD */
    if (SUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        _TPset_error_msg(TPESYSTEM,  "_tpdequeue: Failed to set cmd field: %s", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }
    
    /* do the call to queue system */
    if (FAIL == tpcall(qspace, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, flags))
    {
        int tpe = tperrno;
        NDRX_LOG(log_error, "%s failed: %s", qspace, tpstrerror(tpe));
        if (TPESVCFAIL!=tpe)
        {
            FAIL_OUT(ret);
        }
    }
    else
    {
        BFLDLEN len_extra=0;
        char *data_extra;
        if (SUCCEED!=(data_extra=Bgetalloc(p_ub, EX_DATA, 0, &len_extra)))
        {
            _TPset_error_msg(TPESYSTEM,  "_tpdequeue: Failed to get EX_DATA: %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        /* Test the dest buffer */
        if (NULL==(*data = tprealloc(*data, len_extra)))
        {
            free(data_extra);
            _TPset_error_msg(TPESYSTEM,  "_tpdequeue: Failed to realloc output buffer: %s", 
                    Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        /* Copy off the data to dest */
        memcpy(*data, data_extra, len_extra);
        *len = len_extra;
        free(data_extra);
    }
    
    /* the call is ok (or app failed), convert back. */
    if (SUCCEED!=tmq_tpqctl_from_ubf_deqrsp(p_ub, ctl))
    {
        
        _TPset_error_msg(TPEINVAL,  "_tpdequeue: failed convert ctl "
                "to internal UBF buf!");
        FAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (ctl->diagnostic)
    {
        ret = TPEDIAGNOSTIC;
    }

    NDRX_LOG(log_info, "_tpdequeue: return %d", ret);

    return ret;    
}