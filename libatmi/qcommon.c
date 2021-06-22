/**
 * @brief Persistent queue commons (used between tmqsrv and "userspace" api)
 *
 * @file qcommon.c
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
#include <typed_buf.h>
#include <qcommon.h>
#include <exbase64.h>
#include <atmi_tls.h>

#include "tperror.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   EXOFFSET(s,e), EXELEM_SIZE(s,e)
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
    {EX_QCORRID,        0, OFSZ(TPQCTL, corrid),        BFLD_CARRAY}, /* 5 */
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
    0 /* 14 - EX_QDIAGMSG*/
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
expublic int tmq_tpqctl_to_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_enqreq);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_from_ubf_enqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_ubf_to_c(M_tpqctl_map, p_ub, ctl, M_tpqctl_enqreq);
    
    return ret;
}


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_to_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_enqrsp);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_from_ubf_enqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_ubf_to_c(M_tpqctl_map, p_ub, ctl, M_tpqctl_enqrsp);
    
    return ret;
}

/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_to_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_deqreq);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_from_ubf_deqreq(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_ubf_to_c(M_tpqctl_map, p_ub, ctl, M_tpqctl_deqreq);
    
    return ret;
}


/**
 * Copy the TPQCTL data to buffer, request data
 * @param p_ub destination buffer
 * @param ctl source struct
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_to_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_c_to_ubf(M_tpqctl_map, ctl, p_ub, M_tpqctl_deqrsp);
    
    return ret;
}

/**
 * Build the TPQCTL from UBF buffer, request data
 * @param p_ub source buffer
 * @param ctl destination strcture
 * @return SUCCEED/FAIL
 */
expublic int tmq_tpqctl_from_ubf_deqrsp(UBFH *p_ub, TPQCTL *ctl)
{
    int ret = EXSUCCEED;
    
    ret=atmi_cvt_ubf_to_c(M_tpqctl_map, p_ub, ctl, M_tpqctl_deqrsp);
    
    return ret;
}


/**
 * Generate serialized version of the string
 * @param msgid_in, length defined by constant TMMSGIDLEN
 * @param msgidstr_out
 * @return msgidstr_out
 */
expublic char * tmq_msgid_serialize(char *msgid_in, char *msgid_str_out)
{
    size_t out_len = 0;
    
    NDRX_DUMP(log_debug, "Original MSGID", msgid_in, TMMSGIDLEN);
    
    ndrx_xa_base64_encode((unsigned char *)msgid_in, TMMSGIDLEN, &out_len, 
            msgid_str_out);

    /* msgid_str_out[out_len] = EXEOS; */
    
    NDRX_LOG(log_debug, "MSGID after serialize: [%s]", msgid_str_out);
    
    return msgid_str_out;
}

/**
 * Get binary message id
 * @param msgid_str_in, length defined by constant TMMSGIDLEN
 * @param msgid_out
 * @return msgid_out 
 */
expublic char * tmq_msgid_deserialize(char *msgid_str_in, char *msgid_out)
{
    size_t tot_len = 0;
    
    NDRX_LOG(log_debug, "Serialized MSGID: [%s]", msgid_str_in);
    
    memset(msgid_out, 0, TMMSGIDLEN);
        
    ndrx_xa_base64_decode((unsigned char *)msgid_str_in, strlen(msgid_str_in), 
            &tot_len, msgid_out);
    
    NDRX_DUMP(log_debug, "Deserialized MSGID", msgid_out, TMMSGIDLEN);
    
    return msgid_out;
}

/**************************** API SECTION *************************************/

/**
 * Internal version of message enqueue.
 * TODO: forward ATMI error!
 * 
 * @param qspace service name
 * @param qname queue name
 * @param ctl control data
 * @param data data to enqueue
 * @param len data len
 * @param flags flags (for tpcall)
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tpenqueue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char *data, long len, long flags)
{
    int ret = EXSUCCEED;
    long rsplen;
    char cmd = TMQ_CMD_ENQUEUE;
    char *tmp=NULL;
    long tmp_len;
    UBFH *p_ub = NULL;
    atmi_error_t errbuf;
    char qspacesvc[XATMI_SERVICE_NAME_LENGTH+1]; /* real service name */
    
    NDRX_SYSBUF_MALLOC_WERR_OUT(tmp, tmp_len, ret);
    
    /*
     * Support #403
    if (NULL==data)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: data is null!", __func__);
        EXFAIL_OUT(ret);
    }
    */
    
    if (NULL==qspace || (EXEOS==*qspace && !nodeid && !srvid))
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: empty or NULL qspace!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==qname || EXEOS==*qname)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: empty or NULL qname!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==ctl)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: NULL ctl!", __func__);
        EXFAIL_OUT(ret);
    }
    
    ctl->diagnostic=0;
    
    if (EXFAIL==tptypes(data, NULL, NULL))
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: data buffer not allocated by "
                "tpalloc()", __func__);
        EXFAIL_OUT(ret);
    }
    
    /* prepare buffer for call */
    if (EXSUCCEED!=ndrx_mbuf_prepare_outgoing(data, len, tmp, &tmp_len, 0, 
            NDRX_MBUF_FLAG_NOCALLINFO))
    {
        /* not good - error should be already set */
        EXFAIL_OUT(ret);
    }
    
    NDRX_DUMP(log_debug, "Buffer for sending data out", tmp, tmp_len);
    
    /* Alloc the FB */
    
    if (NULL == (p_ub = (UBFH *)tpalloc("UBF", "", TMQ_DEFAULT_BUFSZ+tmp_len)))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to allocate req buffer: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Convert the structure to UBF */
    if (EXSUCCEED!=tmq_tpqctl_to_ubf_enqreq(p_ub, ctl))
    {
        
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: failed convert ctl "
                "to internal UBF buf!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_DATA, 0, tmp, tmp_len))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to set data field: %s", 
                Bstrerror(Berror), __func__);
        EXFAIL_OUT(ret);
    }
    
    /* Setup the command in EX_QCMD */
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to set cmd field: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QNAME, 0, qname, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to set qname field: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "QSPACE enqueue request buffer", p_ub);
    
    /* do the call to queue system */
    if (EXEOS!=*qspace)
    {
        snprintf(qspacesvc, sizeof(qspacesvc), NDRX_SVC_QSPACE, qspace);
    }
    else
    {
        snprintf(qspacesvc, sizeof(qspacesvc), NDRX_SVC_TMQ, (long)nodeid, (int)srvid);
    }
    
    if (EXFAIL == tpcall(qspacesvc, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, flags|TPNOABORT))
    {
        int tpe = tperrno;
        
        NDRX_LOG(log_error, "%s failed: %s", qspace, tpstrerror(tpe));
        if (TPESVCFAIL!=tpe)
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            ret=EXFAIL;
        }
    }
    
    ndrx_debug_dump_UBF(log_debug, "QSPACE enqueue response buffer", p_ub);
    
    /* the call is ok (or app failed), convert back. */
    if (EXSUCCEED!=tmq_tpqctl_from_ubf_enqrsp(p_ub, ctl))
    {
        NDRX_LOG(log_error, "Failed convert ctl to internal UBF buf!");
        ndrx_TPoverride_code(TPESYSTEM);   
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        atmi_error_t err;
        /* save any error here... */
        ndrx_TPsave_error(&err);
        tpfree((char *)p_ub);
        ndrx_TPrestore_error(&err);
    }

    /* restore the error if have */
    if (0!=tperrno)
    {
        atmi_error_t err;
        ndrx_TPsave_error(&err);
        
        if (ctl->diagnostic)
        {
            err.atmi_error = TPEDIAGNOSTIC;
            NDRX_STRCPY_SAFE(err.atmi_error_msg_buf, 
                    "error details in TPQCTL diag fields");
        }
        ndrx_TPrestore_error(&err);
        
        /* Special abort section */
        NDRX_ABORT_START(EXFALSE)
                   
        if (TPEDIAGNOSTIC==tperrno &&
                (   QMEINVAL==ctl->diagnostic 
                 || QMEBADQUEUE==ctl->diagnostic
                )
            )
        {
            abort_needed=EXFALSE;
        }

        NDRX_ABORT_END(EXFALSE)
                
    }
    else
    {
        ctl->diagnostic = EXFALSE;
    }

    if (NULL!=tmp)
    {
        NDRX_SYSBUF_FREE(tmp);
    }


    NDRX_LOG(log_info, "%s: return %d", __func__, ret);

    return ret;    
}

/**
 * Internal version of message dequeue.
 * TODO: forward ATMI error!
 * 
 * @param qspace service name
 * @param qname queue name
 * @param ctl control data
 * @param data data to enqueue
 * @param len data len
 * @param flags flags (for tpcall)
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tpdequeue (char *qspace, short nodeid, short srvid, char *qname, TPQCTL *ctl, 
        char **data, long *len, long flags)
{
    int ret = EXSUCCEED;
    long rsplen;
    char cmd = TMQ_CMD_DEQUEUE;
    atmi_error_t errbuf;
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", TMQ_DEFAULT_BUFSZ);
    char qspacesvc[XATMI_SERVICE_NAME_LENGTH+1]; /* real service name */
    
    if (NULL==qspace || (EXEOS==*qspace && !nodeid && !srvid))
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: empty or NULL qspace!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==qname || EXEOS==*qname)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: empty or NULL qname!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==ctl)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: NULL ctl!", __func__);
        EXFAIL_OUT(ret);
    }
    
    ctl->diagnostic=0;
    
    if (NULL==data)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: data is null!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==len)
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: len is null!", __func__);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==tptypes(*data, NULL, NULL))
    {
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: data buffer not allocated by "
                "tpalloc()", __func__);
        EXFAIL_OUT(ret);
    }
    
    /* Alloc the request buffer */
    if (NULL == p_ub)
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to allocate req buffer: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Convert the structure to UBF */
    if (EXSUCCEED!=tmq_tpqctl_to_ubf_deqreq(p_ub, ctl))
    {
        
        ndrx_TPset_error_fmt(TPEINVAL,  "%s: failed convert ctl "
                "to internal UBF buf!", __func__);
        EXFAIL_OUT(ret);
    }
    
    /* set the data field */
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QNAME, 0, qname, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to set qname field: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Setup the command in EX_QCMD */
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to set cmd field: %s", 
                __func__, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* do the call to queue system */
    ndrx_debug_dump_UBF(log_debug, "QSPACE dequeue request buffer", p_ub);
    
    if (EXEOS!=*qspace)
    {
        snprintf(qspacesvc, sizeof(qspacesvc), NDRX_SVC_QSPACE, qspace);
    }
    else
    {
        snprintf(qspacesvc, sizeof(qspacesvc), NDRX_SVC_TMQ, (long)nodeid, (int)srvid);
    }
    
    if (EXFAIL == tpcall(qspacesvc, (char *)p_ub, 0L, (char **)&p_ub, &rsplen, 
            flags | TPNOABORT))
    {
        int tpe = tperrno;                
        NDRX_LOG(log_error, "%s failed: %s", qspace, tpstrerror(tpe));
        if (TPESVCFAIL!=tpe)
        {
            EXFAIL_OUT(ret);
        }
        else
        {
            ret=EXFAIL;
        }
        
        ndrx_debug_dump_UBF(log_debug, "QSPACE dequeue response buffer", p_ub);
        
    }
    else
    {
        BFLDLEN len_extra=0;
        char *data_extra = NULL;
        
        ndrx_debug_dump_UBF(log_debug, "QSPACE dequeue response buffer", p_ub);
        
        if (NULL==(data_extra=Bgetalloc(p_ub, EX_DATA, 0, &len_extra)))
        {
            ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to get EX_DATA: %s", 
                    __func__, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        ret=ndrx_mbuf_prepare_incoming(data_extra,
                        len_extra,
                        data,
                        len,
                        flags, 0);
        if (EXSUCCEED!=ret)
        {
            ndrx_TPset_error_fmt(TPESYSTEM,  "%s: Failed to prepare incoming buffer: %s", 
                    __func__, Bstrerror(Berror));
            
            NDRX_FREE(data_extra);
            EXFAIL_OUT(ret);
        }
        NDRX_FREE(data_extra);
    }
    
    /* the call is ok (or app failed), convert back. */
    if (EXSUCCEED!=tmq_tpqctl_from_ubf_deqrsp(p_ub, ctl))
    {
        NDRX_LOG(log_error, "Failed convert ctl to internal UBF buf!");
        ndrx_TPoverride_code(TPESYSTEM);
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=p_ub)
    {
        atmi_error_t err;
        /* save any error here... */
        ndrx_TPsave_error(&err);
        tpfree((char *)p_ub);
        ndrx_TPrestore_error(&err);
    }

    /* restore the error if have */
    if (0!=tperrno)
    {
        atmi_error_t err;
        ndrx_TPsave_error(&err);
        
        if (ctl->diagnostic)
        {
            err.atmi_error = TPEDIAGNOSTIC;
            NDRX_STRCPY_SAFE(err.atmi_error_msg_buf, 
                    "error details in TPQCTL diag fields");
        }
        ndrx_TPrestore_error(&err);
        
        /* Special abort section */
        NDRX_ABORT_START(EXFALSE)
                   
        if (TPEDIAGNOSTIC==tperrno &&
                (      QMEINVAL==ctl->diagnostic 
                    || QMENOMSG==ctl->diagnostic
                    || QMEBADQUEUE==ctl->diagnostic))
        {
            abort_needed=EXFALSE;
        }

        NDRX_ABORT_END(EXFALSE)
    }


    NDRX_LOG(log_info, "%s: return %d", __func__, ret);

    return ret;    
}
/* vim: set ts=4 sw=4 et smartindent: */
