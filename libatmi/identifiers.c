/* 
** Enduro/X identifiers routines (queue names, client ids, etc...)
**
** @file identifiers.c
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
#include <ndrx_config.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys_mqueue.h>
#include <errno.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys_unix.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi_int.h>
#include <ndrxdcmn.h>
#include <utlist.h>
#include <atmi_shm.h>
#include <tperror.h>

#include "gencall.h"
#include "userlog.h"
#include "exsha1.h"
#include "exbase64.h"
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Q type prefix mapping table
 */
struct prefixmap
{
    char *prefix;
    char *offset;
    int len;
    int type;
    char *descr;
};
typedef struct prefixmap prefixmap_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/


/**
 * Prefix mapping table for detecting Q type
 */
expublic prefixmap_t M_prefixmap[] =
{  
    /* Qprefix format string, match off, len, q type classifier */
    {NDRX_NDRXD,                NULL, 0, NDRX_QTYPE_NDRXD,      "ndrxd Q"},
    {NDRX_SVC_QFMT_PFX,         NULL, 0, NDRX_QTYPE_SVC,        "service Q"},
    {NDRX_ADMIN_FMT_PFX,        NULL, 0, NDRX_QTYPE_SRVADM,     "svc admin Q"},
    {NDRX_SVR_QREPLY_PFX,       NULL, 0, NDRX_QTYPE_SRVRPLY,    "server rply Q"},
    {NDRX_CLT_QREPLY_PFX,       NULL, 0, NDRX_QTYPE_CLTRPLY,    "client rply Q"},
    {NDRX_CONV_INITATOR_Q_PFX,  NULL, 0, NDRX_QTYPE_CONVINIT,   "conv initi Q"},
    {NDRX_CONV_SRV_Q_PFX,       NULL, 0, NDRX_QTYPE_CONVSRVQ,   "conv server Q"},
    {NULL}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Parse Q. This will parse only the id, it will ignore the Q prefix
 * @param qname full queue named
 * @param p_myid parsed ID
 * @return SUCCEED/FAIL
 */
expublic int ndrx_cvnq_parse_client(char *qname, TPMYID *p_myid)
{
    int ret = EXSUCCEED;
    char *p;
    
    /* can be, example:
    - /dom2,cnv,c,srv,atmisv35,11,32159,0,2,1
    - /dom1,cnv,c,clt,atmiclt35,32218,5,1,1
     */
    
    if (NULL==(p = strchr(qname, NDRX_FMT_SEP)))
    {
        NDRX_LOG(log_error, "Invalid conversational initiator/client Q (1): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p++;
    
    if (0!=strncmp(p, "cnv"NDRX_FMT_SEP_STR, 4))
    {
        NDRX_LOG(log_error, "Invalid conversational initiator/client Q (2): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p+=4;
    
    if (0!=strncmp(p, "c"NDRX_FMT_SEP_STR, 2))
    {
        NDRX_LOG(log_error, "Invalid conversational initiator/client Q (3): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p+=2;
    
    ret = ndrx_myid_parse(p, p_myid, EXTRUE);
    
    
out:
    
    return ret;
}


/**
 * Step forward number of separators
 * @param qname
 * @param num
 * @return 
 */
exprivate char * move_forward(char *qname, int num)
{
    char *p = qname;
    int i;
    
    for (i=0; i<num; i++)
    {
        if (NULL==(p=strchr(p, NDRX_FMT_SEP)))
        {
            NDRX_LOG(log_error, "Search for %d %c seps in [%s], step %d- fail",
                    num, NDRX_FMT_SEP, qname, i);
            goto out;
        }
        p++;
    }
    
out:
    return p;
       
}
/**
 * Parse conversational server Q. Which is compiled of two initiator and acceptor:
 * e.g. 
 * - /dom2,cnv,s,srv,atmisv35,13,32163,0,2,1,srv,atmisv35,14,32165,0,2
 * - /dom2,cnv,s,clt,atmiclt35,32218,2,1,1,srv,atmisv35,11,32159,0,2
 * @param qname
 * @param p_myid_first
 * @param p_myid_second
 * @return 
 */
expublic int ndrx_cvnq_parse_server(char *qname, TPMYID *p_myid_first,  TPMYID *p_myid_second)
{
    int ret = EXSUCCEED;
    char tmpq[NDRX_MAX_Q_SIZE+1];
    char *p;
    char *p2;
    /* we might want to get specs about count of separator commas 
     * symbols in each of the ids. Thus we could step forward for compiled
     * Q identifiers.
     */
    
    NDRX_STRCPY_SAFE(tmpq, qname);
    
    /* /dom2,cnv,s,clt,atmiclt35,32218,2,1,1,srv,atmisv35,11,32159,0,2 */
    
    if (NULL==(p = strchr(tmpq, NDRX_FMT_SEP)))
    {
        NDRX_LOG(log_error, "Invalid conversational server Q (1): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p++;
    
    if (0!=strncmp(p, "cnv"NDRX_FMT_SEP_STR, 4))
    {
        NDRX_LOG(log_error, "Invalid conversational server Q (2): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p+=4;
    
    if (0!=strncmp(p, "s"NDRX_FMT_SEP_STR, 2))
    {
        NDRX_LOG(log_error, "Invalid conversational server Q (3): [%s]", 
                qname);
        EXFAIL_OUT(ret);
    }
    p+=2;
    
    if (0==strncmp(p, "srv"NDRX_FMT_SEP_STR, 4))
    {
        /* +1 because we want to get till the end of the our component */
        if (NULL==(p2=move_forward(p, NDRX_MY_ID_SRV_CNV_NRSEPS+1)))
        {
            NDRX_LOG(log_error, "Failed to decode server myid seps count: [%s]",
                   p);
        }
        p2--;
        *p2 = EXEOS;
        p2++;
        
        if (strlen(p2)==0)
        {
            NDRX_LOG(log_error, "Invalid server queue");
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strncmp(p, "clt"NDRX_FMT_SEP_STR, 4))
    {
        /* +1 because we want to get till the end of the our component */
        if (NULL==(p2=move_forward(p, NDRX_MY_ID_CLT_CNV_NRSEPS+1)))
        {
            NDRX_LOG(log_error, "Failed to decode client myid seps count: [%s]",
                   p);
        }
        p2--;
        *p2 = EXEOS;
        p2++;
        
        if (strlen(p2)==0)
        {
            NDRX_LOG(log_error, "Invalid client queue of server q [%s]", qname);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_error, "Cannot detect myid type of conversational Q: "
                "[%s]", qname);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Parsing Q: [%s] first part: [%s] "
            "second part: [%s]",qname, p, p2);
    if (EXSUCCEED!=ndrx_myid_parse(p, p_myid_first, EXTRUE) || 
            EXSUCCEED!=ndrx_myid_parse(p2, p_myid_second, EXFALSE))
    {
        NDRX_LOG(log_error, "Failed to parse Q: [%s] first part: [%s] "
            "second part: [%s]",qname, p, p2);
        EXFAIL_OUT(ret);
    }
    
out:
    NDRX_LOG(log_error, "ndrx_parse_cnv_srv_q returns with %d", ret);
    return ret;
}

/**
 * Prase myid (it will detect client or server)
 * @param my_id myid string
 * @param out parsed myid
 * @return SUCCEED/FAIL
 */
expublic int ndrx_myid_parse(char *my_id, TPMYID *out, int iscnv_initator)
{
    int ret = EXSUCCEED;
    
    if (0==strncmp(my_id, "srv"NDRX_FMT_SEP_STR, 4))
    {
        NDRX_LOG(log_debug, "Parsing server myid: [%s]", my_id);
        return ndrx_myid_parse_srv(my_id, out, iscnv_initator);
    }
    else if (0==strncmp(my_id, "clt"NDRX_FMT_SEP_STR, 4))
    {
        NDRX_LOG(log_debug, "Parsing client myid: [%s]", my_id);
        return ndrx_myid_parse_clt(my_id, out, iscnv_initator);
    }
    else
    {
        NDRX_LOG(log_error, "Cannot detect myid type: [%s]", my_id);
        ret=EXFAIL;
    }
    
    return ret;
}


/**
 * Parse client id
 * @param my_id client id string
 * @param out client id out struct
 * @return SUCCEED
 */
expublic int ndrx_myid_parse_clt(char *my_id, TPMYID *out, int iscnv_initator)
{
    int ret = EXSUCCEED;
    int len;
    int i;
    char tmp[NDRX_MAX_Q_SIZE+1];
    
    NDRX_STRCPY_SAFE(tmp, my_id);
    len = strlen(tmp);
    for (i=0; i<len; i++)
    {
        if (NDRX_FMT_SEP==tmp[i])
            tmp[i]=' ';
    }
    
    NDRX_LOG(log_debug, "Parsing: [%s]", tmp);
    if (iscnv_initator)
    {
        sscanf(tmp, NDRX_MY_ID_CLT_CNV_PARSE, 
                out->binary_name
                ,&(out->pid)
                ,&(out->contextid)
                ,&(out->nodeid)
                ,&(out->cd));
        out->isconv = EXTRUE;
    }
    else 
    {

        sscanf(tmp, NDRX_MY_ID_CLT_PARSE, 
                out->binary_name
                ,&(out->pid)
                ,&(out->contextid)
                ,&(out->nodeid));
        out->isconv = EXFALSE;
    }

    out->tpmyidtyp = TPMYIDTYP_CLIENT;
    
    ndrx_myid_dump(log_debug, out, "Parsed myid");
    
    return ret;
}

/**
 * Parse myid of the server process
 * @param my_id myid/NDRX_MY_ID_SRV formatted id
 * @param out filled structure of the parse id
 * @return SUCCEED
 */
expublic int ndrx_myid_parse_srv(char *my_id, TPMYID *out, int iscnv_initator)
{
    int ret = EXSUCCEED;
    int len;
    int i;
    char tmp[NDRX_MAX_Q_SIZE+1];
    
    NDRX_STRCPY_SAFE(tmp, my_id);
    len = strlen(tmp);
    for (i=0; i<len; i++)
    {
        if (NDRX_FMT_SEP==tmp[i])
            tmp[i]=' ';
    }
    
    NDRX_LOG(log_debug, "Parsing: [%s]", tmp);
    if (iscnv_initator)
    {
        sscanf(tmp, NDRX_MY_ID_SRV_CNV_PARSE, 
                out->binary_name
                ,&(out->srv_id)
                ,&(out->pid)
                ,&(out->contextid)
                ,&(out->nodeid)
                ,&(out->cd));
        out->isconv = EXTRUE;
    }
    else
    {
        sscanf(tmp, NDRX_MY_ID_SRV_PARSE, 
                out->binary_name
                ,&(out->srv_id)
                ,&(out->pid)
                ,&(out->contextid)
                ,&(out->nodeid));
        out->isconv = EXFALSE;
    }
    
    out->tpmyidtyp = TPMYIDTYP_SERVER;
    
    ndrx_myid_dump(log_debug, out, "Parsed myid output");
    
    return ret;
}

/**
 * Check is process a live
 * @param 
 * @return FAIL (not our node)/TRUE (live)/FALSE (dead)
 */
expublic int ndrx_myid_is_alive(TPMYID *p_myid)
{
    /* Bug #291 2018/03/25 */
    if (p_myid->nodeid==G_atmi_env.our_nodeid)
    {
        /* cltname same pos as server proc name */
        return ndrx_sys_is_process_running(p_myid->pid, p_myid->binary_name);
    }
    else
    {
        return EXFAIL;
    }
}

/**
 * Dump the MYID struct to the log
 * @param p_myid ptr to myid
 * @param lev debug level to print of
 */
expublic void ndrx_myid_dump(int lev, TPMYID *p_myid, char *msg)
{
    
    NDRX_LOG(lev, "=== %s ===", msg);
    
    NDRX_LOG(lev, "binary_name:[%s]", p_myid->binary_name);
    NDRX_LOG(lev, "pid        :%d", p_myid->pid);
    NDRX_LOG(lev, "contextid  :%ld", p_myid->contextid);
    NDRX_LOG(lev, "nodeid     :%d", p_myid->nodeid);
    NDRX_LOG(lev, "typ        :%s (%d)", 
            p_myid->tpmyidtyp==TPMYIDTYP_SERVER?"server":"client", 
            p_myid->tpmyidtyp);
    
    if (p_myid->tpmyidtyp==TPMYIDTYP_SERVER)
    {
        NDRX_LOG(lev, "srv_id     :%d", p_myid->srv_id);
    }
    NDRX_LOG(lev, "cnv initia :%s", p_myid->isconv?"TRUE":"FALSE");
    
    if (p_myid->isconv)
    {
        NDRX_LOG(lev, "cd         :%d", p_myid->cd);
    }
    NDRX_LOG(lev, "=================");
            
}

/**
 * Translate the given my_id to reply q
 * This should work for servers and clients.
 * The rply Q is built locally...
 * 
 * @param my_id String version of my_id
 * @param rply_q String version (full version with pfx) of the reply Q
 */
expublic int ndrx_myid_convert_to_q(TPMYID *p_myid, char *rply_q, int rply_q_buflen)
{
    int ret = EXSUCCEED;
    
    /* Now build the reply Qs */
    if (TPMYIDTYP_SERVER==p_myid->tpmyidtyp)
    {
        /* build server q */
        /*#define NDRX_SVR_QREPLY   "%s,srv,reply,%s,%d,%d"  qpfx, procname, serverid, pid */
        snprintf(rply_q, rply_q_buflen, NDRX_SVR_QREPLY, G_atmi_env.qprefix, 
                p_myid->binary_name, p_myid->srv_id, p_myid->pid);
        
    }
    else
    {
        /* build client q */
        /*#define NDRX_CLT_QREPLY   "%s,clt,reply,%s,%d,%ld"  pfx, name, pid, context id*/
        snprintf(rply_q, rply_q_buflen, NDRX_CLT_QREPLY, G_atmi_env.qprefix, 
                p_myid->binary_name, p_myid->pid, p_myid->contextid);
    }
    
    NDRX_LOG(log_info, "Translated into [%s] reply q", rply_q);
    
out:
    return ret;
}


/**
 * Dump the MYID struct to the log
 * @param p_myid ptr to myid
 * @param lev debug level to print of
 */
expublic void ndrx_qdet_dump(int lev, ndrx_qdet_t *qdet, char *msg)
{
    
    NDRX_LOG(lev, "=== %s ===", msg);
    /* I */
    NDRX_LOG(lev, "binary_name:[%s]", qdet->binary_name);
    NDRX_LOG(lev, "pid        :%d", qdet->pid);
    NDRX_LOG(lev, "contextid  :%ld", qdet->contextid);
    NDRX_LOG(lev, "typ        :%d",  qdet->qtype);
    
    NDRX_LOG(lev, "=================");
            
}

/**
 * Parse client qstr 
 * @param qdet queue details where to store
 * @param qstr Queue string to parse
 * @return SUCCEED
 */
expublic int ndrx_qdet_parse_cltqstr(ndrx_qdet_t *qdet, char *qstr)
{   
    int ret = EXSUCCEED;
    int len;
    int i;
    char tmp[NDRX_MAX_Q_SIZE+1];
    
    NDRX_STRCPY_SAFE(tmp, qstr);
    len = strlen(tmp);
    for (i=0; i<len; i++)
    {
        if (NDRX_FMT_SEP==tmp[i])
            tmp[i]=' ';
    }
    
    NDRX_LOG(log_debug, "Parsing: [%s]", tmp);
    
    sscanf(tmp, NDRX_CLT_QREPLY_PARSE, 
            qdet->qprefix,
            qdet->binary_name
            ,&(qdet->pid)
            ,&(qdet->contextid));

    
    qdet->qtype = NDRX_QTYPE_CLTRPLY;
    
    ndrx_qdet_dump(log_debug, qdet, "Parsed qdet client output");
    
out:
    return ret;
}
/**
 * Build myid from reply q. 
 * @param p_myid
 * @param rply_q
 * @param nodeid - Cluster node id, as q does not encode cluster id
 * @return SUCCEED/FAIL
 */
expublic int ndrx_myid_convert_from_qdet(TPMYID *p_myid, ndrx_qdet_t *qdet, long nodeid)
{
    int ret = EXSUCCEED;
    
    if (NDRX_QTYPE_CLTRPLY==qdet->qtype)
    {
        NDRX_STRCPY_SAFE(p_myid->binary_name, qdet->binary_name);
        p_myid->contextid = qdet->contextid;
        p_myid->pid = qdet->pid;
        p_myid->nodeid = nodeid;
    }
    else
    {
        NDRX_LOG(log_error, "%s: Unsupported qtype for building myid: %d", 
                __func__, qdet->qtype);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Convert TPMYID to string
 * @param p_myid
 * @param my_id
 * @return SUCCEED
 */
expublic void ndrx_myid_to_my_id_str(TPMYID *p_myid, char *my_id)
{
    snprintf(my_id, NDRX_MAX_ID_SIZE+1, NDRX_MY_ID_CLT, 
        p_myid->binary_name,
        p_myid->pid,
        p_myid->contextid,
        p_myid->nodeid
    );
    
    NDRX_LOG(log_debug, "%s: built my_id: [%s]", __func__, my_id);
}

/**
 * Initialize ATMI util lib.
 * Setup the queue testing strings for local host
 * @return SUCCEED/FAIL
 */
expublic int ndrx_atmiutil_init(void)
{
    int ret = EXSUCCEED;
    prefixmap_t *p = M_prefixmap;
    
    while (NULL!=p->prefix)
    {
        p->offset = strchr(p->prefix, NDRX_FMT_SEP);
        
        if (NULL==p->offset)
        {
            NDRX_LOG(log_error, "%s failed to search for [%c] in [%s]", __func__, 
                    NDRX_FMT_SEP, p->prefix);
            EXFAIL_OUT(ret);
        }
        
        /* calculate match length for the Q */
        p->len = strlen(p->offset);
        
        p++;
    }
    
out:
    return ret;
}

/**
 * Return queue type classifier
 * @param q full queue (with prefix)
 * @return see NDRX_QTYPE_* or FAIL
 */
expublic int ndrx_q_type_get(char *q)
{
    int ret = EXFAIL;
    prefixmap_t *p = M_prefixmap;
    char *q_wo_pfx = strchr(q, NDRX_FMT_SEP);
    
    if (NULL==q_wo_pfx)
    {
        NDRX_LOG(log_error, "Invalid Enduro/X Q (possible not Enduro/X): [%s]", 
                q_wo_pfx);
        EXFAIL_OUT(ret);
    }
    
    while (NULL!=p->prefix)
    {
        if (0==strncmp(p->offset, q_wo_pfx, p->len))
        {
            /* matched */
            break;
        }
        
        p++;
    }
    
    if (NULL!=p)
    {
        ret = p->type;
        NDRX_LOG(log_debug, "[%s] matched type [%d/%s]", q, ret, p->descr);
    }
    
out:
    return ret;
}

/**
 * Internal version of tpconvert()
 * @param str in/out string format, the buffer size should be at least 
 * @param bin binary data
 * @param flags flags (see API descr)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tpconvert(char *str, char *bin, long flags)
{
    int ret = EXSUCCEED;
    CLIENTID *cltid;
    size_t out_len;
    
    if (flags & TPTOSTRING)
    {
        out_len = TPCONVMAXSTR;
        NDRX_LOG(log_debug, "%s: convert to string: %"PRIx64, __func__, flags);
        
        if (flags & TPCONVCLTID)
        {
            /* client id is already string... */
            cltid = (CLIENTID *)bin;
            NDRX_STRNCPY_SAFE(str, cltid->clientdata, TPCONVMAXSTR);
        }
        else if (flags & TPCONVTRANID)
        {
            /* maybe needs to think about cross platform way?
             * but currently do not see reason for this
             */
            ndrx_xa_base64_encode((unsigned char *)bin, sizeof(TPTRANID), &out_len, str);
            str[out_len] = EXEOS;
        }
        else if (flags & TPCONVXID)
        {
            atmi_xa_serialize_xid((XID *)bin, str);
        }
        else
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Invalid convert flags: %"PRIx64, 
                    __func__, flags);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "%s: convert to bin: %"PRIx64, __func__, flags);
        
        if (flags & TPCONVCLTID)
        {
            /* client id is already string... */
            cltid = (CLIENTID *)bin;
            NDRX_STRCPY_SAFE(cltid->clientdata, str);
        }
        else if (flags & TPCONVTRANID)
        {
            /* Decode binary data: */
            out_len = sizeof(TPTRANID);
            if (NULL==(ndrx_xa_base64_decode((unsigned char *)str, strlen(str), &out_len, bin)))
            {
                ndrx_TPset_error_msg(TPEINVAL, "Failed to decode string, possible "
                        "bad base64 coding.");
                EXFAIL_OUT(ret);
            }
        }
        else if (flags & TPCONVXID)
        {
            /* Deserialize xid. */
            atmi_xa_deserialize_xid(str, (XID *)bin);
        }
        else
        {
            ndrx_TPset_error_fmt(TPEINVAL, "Invalid convert flags: %"PRIx64, flags);
            EXFAIL_OUT(ret);
        }
    }
    
out:
    NDRX_LOG(log_debug, "%s returns %d", __func__, ret);

    return ret;
}

