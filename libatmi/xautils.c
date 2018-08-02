/* 
 * @brief ATMI lib part for XA api - utils
 *
 * @file xautils.c
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
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <xa_cmn.h>
#include <exuuid.h>

#include <tperror.h>
#include <Exfields.h>

#include <xa_cmn.h>
#include <atmi_tls.h>
#include <ubfutil.h>
#include <exbase64.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define     TM_CALL_FB_SZ           1024        /* default size for TM call */


#define _XAUTILS_DEBUG

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/*************************** XID manipulation *********************************/

/**
 * Extract info from xid.
 * @param xid xid
 * @param p_nodeid return nodeid
 * @param p_srvid return serverid
 */
expublic void atmi_xa_xid_get_info(XID *xid, short *p_nodeid, short *p_srvid)
{
    
    memcpy((char *)p_nodeid, xid->data + sizeof(exuuid_t) + sizeof(unsigned char), 
            sizeof(short));
    
    memcpy((char *)p_srvid, xid->data + sizeof(exuuid_t) 
            +sizeof(unsigned char) + sizeof(short), sizeof(short));
}


/**
 * Return XID info for XID string
 * @param xid_str
 * @param p_nodeid
 * @param p_srvid
 */
expublic void atmi_xa_xid_str_get_info(char *xid_str, short *p_nodeid, short *p_srvid)
{
    XID xid;
    memset(&xid, 0, sizeof(xid));
    atmi_xa_xid_get_info(atmi_xa_deserialize_xid((unsigned char *)xid_str, &xid), p_nodeid, p_srvid);
}

/**
 * Get `char *' representation of XID.
 * @param xid
 * @param xid_str_out
 */
expublic char * atmi_xa_serialize_xid(XID *xid, char *xid_str_out)
{
    int ret=EXSUCCEED;
    unsigned char tmp[XIDDATASIZE+64];
    int tot_len;
    int data_len = xid->gtrid_length+xid->bqual_length;
    size_t out_len = 0;
    /* we should serialize stuff in platform independent format... */
    
    NDRX_LOG(log_debug, "atmi_xa_serialize_xid - enter");
    /* serialize format id: */
    tmp[0] = (unsigned char)((xid->formatID >> 24) & 0xff);
    tmp[1] = (unsigned char)((xid->formatID >> 16) & 0xff);
    tmp[2] = (unsigned char)((xid->formatID >> 8) & 0xff);
    tmp[3] = (unsigned char)(xid->formatID & 0xff);
    tot_len=4;
    
    /* serialize gtrid_length */
    tmp[4] = (unsigned char)xid->gtrid_length;
    tot_len+=1;
    
    /* serialize bqual_length */
    tmp[5] = (unsigned char)xid->bqual_length;
    tot_len+=1;
    
    /* copy off the data: TODO - is data in uid endianness agnostic? */
    memcpy(tmp+6, xid->data, data_len);
    tot_len+=data_len;
    
    NDRX_DUMP(log_debug, "Original XID", xid, sizeof(*xid));
    
    NDRX_LOG(log_debug, "xid serialization total len: %d", tot_len);    
    NDRX_DUMP(log_debug, "XID data for serialization", tmp, tot_len);
    
    ndrx_xa_base64_encode(tmp, tot_len, &out_len, xid_str_out);
    xid_str_out[out_len] = EXEOS;
    
    NDRX_LOG(log_debug, "Serialized xid: [%s]", xid_str_out);    
    
    return xid_str_out;
    
}

/**
 * Deserialize - make system XID
 */
expublic XID* atmi_xa_deserialize_xid(unsigned char *xid_str, XID *xid_out)
{
    unsigned char tmp[XIDDATASIZE+64];
    size_t tot_len = 0;
    long l;
    
    NDRX_LOG(log_debug, "atmi_xa_deserialize_xid - enter");
    NDRX_LOG(log_debug, "Serialized xid: [%s]", xid_str);    
    
    ndrx_xa_base64_decode(xid_str, strlen((char *)xid_str), &tot_len, (char *)tmp);
    
    NDRX_LOG(log_debug, "xid deserialization total len: %d", tot_len);
    NDRX_DUMP(log_debug, "XID data for deserialization", tmp, tot_len);
    
    memset(xid_out, 0, sizeof(*xid_out));
    
    /* build the format id: */
    l = tmp[0];
    l <<=24;
    xid_out->formatID |= l;
    
    l = tmp[1];
    l <<=16;
    xid_out->formatID |= l;
    
    l = tmp[2];
    l <<=8;
    xid_out->formatID |= l;
    
    l = tmp[3];
    xid_out->formatID |= l;
    
    /* restore gtrid_length */
    
    xid_out->gtrid_length = tmp[4];
    
    /* restore bqual_length */
    xid_out->bqual_length = tmp[5];
    
    memcpy(xid_out->data, tmp+6, tot_len - 6);
 
    NDRX_DUMP(log_debug, "Original XID restored ", xid_out, sizeof(*xid_out));
    
    return xid_out;
       
}

/***************** Manipulation functions of current transaction **************/

/**
 * Return transaction from hash table 
 * @param tmxid - xid in string format
 * @return ptr or NULL
 */
expublic atmi_xa_tx_info_t * atmi_xa_curtx_get(char *tmxid)
{
    atmi_xa_tx_info_t *ret = NULL;
    ATMI_TLS_ENTRY;
    
    EXHASH_FIND_STR( G_atmi_tls->G_atmi_xa_curtx.tx_tab, tmxid, ret);    
    return ret;
}

/**
 * Add new current transaction to the hash list.
 * This does not register known list...
 * @param tmxid
 * @param tmrmid
 * @param tmnodeid
 * @param tmsrvid
 * @return ptr to entry or NULL
 */
expublic atmi_xa_tx_info_t * atmi_xa_curtx_add(char *tmxid,
        short tmrmid, short tmnodeid, short tmsrvid, char *tmknownrms)
{
    atmi_xa_tx_info_t * tmp = NDRX_CALLOC(1, sizeof(atmi_xa_tx_info_t));
    ATMI_TLS_ENTRY;
    
    if (NULL==tmp)
    {
        userlog("malloc failed: %s", strerror(errno));
        goto out;
    }
    
    NDRX_STRCPY_SAFE(tmp->tmxid, tmxid);
    tmp->tmrmid = tmrmid;
    tmp->tmnodeid = tmnodeid;
    tmp->tmsrvid = tmsrvid;
    NDRX_STRCPY_SAFE(tmp->tmknownrms, tmknownrms);
    
    EXHASH_ADD_STR( G_atmi_tls->G_atmi_xa_curtx.tx_tab, tmxid, tmp );
    
out:
    return tmp;
}

/**
 * Remove transaction from list of transaction in progress
 * @param p_txinfo
 */
expublic void atmi_xa_curtx_del(atmi_xa_tx_info_t *p_txinfo)
{
    ATMI_TLS_ENTRY;
    
    EXHASH_DEL( G_atmi_tls->G_atmi_xa_curtx.tx_tab, p_txinfo);
    /* Remove any cds involved... */
    /* TODO: Think about cd invalidating... */
    atmi_xa_cd_unregall(&(p_txinfo->call_cds));
    atmi_xa_cd_unregall(&(p_txinfo->conv_cds));
    
    NDRX_FREE((void *)p_txinfo);
    
    return;
}

/**
 * Convert call structure to xai
 * @param p_xai
 * @param call
 * @return 
 */
expublic atmi_xa_tx_info_t * atmi_xa_curtx_from_call(tp_command_call_t *call)
{
    atmi_xa_tx_info_t * ret = NULL;
    /* Firstly we should do the lookup (maybe already registered?)
     * If not then register the handle and return ptr
     */
    if (NULL==(ret = atmi_xa_curtx_get(call->tmxid)))
    {
     ret = atmi_xa_curtx_add(call->tmxid, call->tmrmid, 
             call->tmnodeid, call->tmsrvid, call->tmknownrms);
    }
    
    return ret;
}
#if 0
/**
 * Convert call to xai (only data copy, does not register)
 * @param call
 * @return 
 */
expublic void atmi_xa_xai_from_call(atmi_xa_tx_info_t * p_xai, tp_command_call_t *p_call)
{
    strcpy(p_xai->tmxid, p_call->tmxid);
    p_xai->tmrmid = p_call->tmrmid;
    p_xai->tmnodeid = p_call->tmnodeid;
    p_xai->tmsrvid = p_call->tmsrvid;
}
#endif

/*************************** Transaction info manipulation ********************/

/**
 * Load into bisubf buffer info about new transaction created.
 * @param p_ub
 * @param tmxid
 * @param tmrmid
 * @param tmnodeid
 * @param tmsrvid
 * @return 
 */
expublic int atmi_xa_load_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    char test[100] = {EXEOS};
    if (EXSUCCEED!=Bchg(p_ub, TMXID, 0, p_xai->tmxid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMRMID, 0, (char *)&p_xai->tmrmid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&p_xai->tmnodeid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&p_xai->tmsrvid, 0L) ||
            EXSUCCEED!=Bchg(p_ub, TMKNOWNRMS, 0, (char *)p_xai->tmknownrms, 0L)
            )
    {
        NDRX_LOG(log_error, "Failed to setup TMXID/TMRMID/TMNODEID/"
                "TMSRVID/TMKNOWNRMS! - %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    Bget(p_ub, TMKNOWNRMS, 0, test, 0L);
    
out:
    return ret;
}

/**
 * Read transaction info received from TM
 * @param p_ub
 * @param p_xai
 * @return 
 */
expublic int atmi_xa_read_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, p_xai->tmxid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMRMID, 0, (char *)&p_xai->tmrmid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&p_xai->tmnodeid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&p_xai->tmsrvid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMKNOWNRMS, 0, (char *)p_xai->tmknownrms, 0L)
            )
    {
        NDRX_LOG(log_error, "Failed to setup TMXID/TMRMID/TMNODEID/"
                "TMSRVID/TMKNOWNRMS! - %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Copy XA transaction info to tpcall() buffer
 * @param call
 * @param p_xai
 * @return 
 */
expublic void atmi_xa_cpy_xai_to_call(tp_command_call_t *call, atmi_xa_tx_info_t *p_xai)
{
    XA_TX_COPY(call, p_xai);
}

/**
 * Function prints the list known resource managers in transaction
 * @param tmknownrms
 * @return 
 */
expublic void atmi_xa_print_knownrms(int dbglev, char *msg, char *tmknownrms)
{
    int i;
    int cnt = strlen(tmknownrms);
    char tmp[128]={EXEOS};
    
    for (i=0; i<cnt; i++)
    {
        if (i<cnt-1)
        {
            sprintf(tmp+strlen(tmp), "%hd ", (short)tmknownrms[i]);
        }
        else
        {
            sprintf(tmp+strlen(tmp), "%hd", (short)tmknownrms[i]);
        }
    }
    NDRX_LOG(dbglev, "%s: %s", msg, tmp);
}

/**
 * Reset current transaction info (remove current transaction)
 * @return 
 */
expublic void atmi_xa_reset_curtx(void)
{
    ATMI_TLS_ENTRY;
    
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        atmi_xa_curtx_del(G_atmi_tls->G_atmi_xa_curtx.txinfo);
        G_atmi_tls->G_atmi_xa_curtx.txinfo = NULL;
    }
}

/**
 * Check is current RM known in the list
 * @param tmknownrms
 * @return 
 */
expublic int atmi_xa_is_current_rm_known(char *tmknownrms)
{
    if (NULL==strchr(tmknownrms, (unsigned char)G_atmi_env.xa_rmid))
    {
        return EXFALSE;
    }
    return EXTRUE;
}

/**
 * Update the list of known transaction resource managers
 * @param dst_tmknownrms
 * @param src_tmknownrms
 * @return 
 */
expublic int atmi_xa_update_known_rms(char *dst_tmknownrms, char *src_tmknownrms)
{
    int i;
    int len = strlen(src_tmknownrms);
    int len2;
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_error, "src len: %d", len);
    
    for (i=0; i<len; i++)
    {
        if (NULL==strchr(dst_tmknownrms, src_tmknownrms[i]))
        {
            len2=strlen(dst_tmknownrms);
            NDRX_LOG(log_debug, "len2=%d", len2);
            if (len2==NDRX_MAX_RMS)
            {
                NDRX_LOG(log_error, "Too much RMs: src: [%s] dest: [%s]!", 
                        src_tmknownrms, dst_tmknownrms);
                EXFAIL_OUT(ret);
            }
            NDRX_LOG(log_info, "1--> %c", dst_tmknownrms[len2]);
            NDRX_LOG(log_info, "2--> %c", src_tmknownrms[i]);
            
            dst_tmknownrms[len2] = src_tmknownrms[i];
            dst_tmknownrms[len2+1] = EXEOS;
        }
    }
    
out:
    return ret;
}

/**
 * Update known RMs, with info from xai
 * @param p_xai
 */
expublic int atmi_xa_curtx_set_cur_rmid(atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    int cnt;
    ATMI_TLS_ENTRY;
    
    if (NULL==strchr(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms, 
            (unsigned char)p_xai->tmrmid))
    {
        /* Updated known RMs */
        if ((cnt=strlen(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms)) > NDRX_MAX_RMS)
        {
            NDRX_LOG(log_error, "Maximum Resource Manager reached (%d)", 
                    NDRX_MAX_RMS);
            userlog("Maximum Resource Manager reached (%d) - Cannot join process "
                    "to XA transaction", NDRX_MAX_RMS);
            ret=EXFAIL;
            goto out;
        }
        
        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms[cnt] = (unsigned char)p_xai->tmrmid;
    }
    
    atmi_xa_print_knownrms(log_info, "Known RMs", 
            G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms);
out:

    return ret;
}
/**
 * Set current thread info from xai + updates known RMs..
 * We should search the hash list of the current transaction and make the ptr 
 * as current. If not found, then we shall register
 * @param p_xai
 * @return 
 */
expublic int atmi_xa_set_curtx_from_xai(atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    ATMI_TLS_ENTRY;
    
    /* Lookup the hash add if found ok switch ptr 
     * if not found, add and switch ptr too.
     */
    if (NULL==(G_atmi_tls->G_atmi_xa_curtx.txinfo = atmi_xa_curtx_get(p_xai->tmxid)) &&
         NULL==(G_atmi_tls->G_atmi_xa_curtx.txinfo = 
            atmi_xa_curtx_add(p_xai->tmxid, p_xai->tmrmid, 
            p_xai->tmnodeid, p_xai->tmsrvid, p_xai->tmknownrms)))
            
    {
        NDRX_LOG(log_error, "Set current transaction failed!");
        ret=EXFAIL;
        goto out;
    }
    
out:
        return ret;
}

/**
 * Set current tx info from tx buffer.
 * @param p_ub
 * @param p_xai
 * @return 
 */
expublic int atmi_xa_set_curtx_from_tm(UBFH *p_ub)
{
    int ret = EXSUCCEED;
    atmi_xa_tx_info_t xai;
    
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai))
    {
        ret=EXFAIL;
        goto out;
    }
    
    /* transfer stuff to current context */
    if (EXSUCCEED!=atmi_xa_set_curtx_from_xai(&xai))
    {
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Allocate stanard TM call FB
 * @param pp_ub
 * @return NULL (error) or allocated FB
 */
expublic UBFH * atmi_xa_alloc_tm_call(char cmd)
{
    UBFH *p_ub = NULL;
    int ret = EXSUCCEED;
    ATMI_TLS_ENTRY;
    
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, TM_CALL_FB_SZ)))
    {
        /* TM error should be already set */
        NDRX_LOG(log_error, "Failed to allocate TM call FB (%d)", 
                TM_CALL_FB_SZ);
        ret = EXFAIL;
        goto out;
    }
    
    /* install caller error */
    if (EXSUCCEED!=Bchg(p_ub, TMPROCESSID, 0, G_atmi_tls->G_atmi_conf.my_id, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMPROCESSID) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = EXFAIL;
        goto out;
    }
    
    /* install command code */
    if (EXSUCCEED!=Bchg(p_ub, TMCMD, 0, &cmd, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMCMD) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = EXFAIL;
        goto out;
    }
    
    /* Install caller RM code */
    if (EXSUCCEED!=Bchg(p_ub, TMCALLERRM, 0, (char *)&G_atmi_env.xa_rmid, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMCALLERRM) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = EXFAIL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "Call buffer setup OK");
    
out:

    if (EXSUCCEED!=ret && NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return p_ub;
}

/**
 * Setup basic admin call buffer
 * @param cmd
 * @param p_ub
 * @return 
 */
expublic int atmi_xa_tm_admincall(char cmd, UBFH *p_ub)
{
    int ret = EXSUCCEED;
    
    
    
out:
    return ret;
}


/**
 * Generic transaction manager call
 * @param cmd - TM command
 * @param call_any - should we call any our RMID
 * @param rmid - should we call specific RM
 * @param p_xai
 * @return 
 */
expublic UBFH* atmi_xa_call_tm_generic(char cmd, int call_any, short rmid, 
        atmi_xa_tx_info_t *p_xai)
{
    UBFH *p_ub = atmi_xa_alloc_tm_call(cmd);
    
    return atmi_xa_call_tm_generic_fb(cmd, NULL, call_any, rmid, p_xai, p_ub);
}

/**
 * Do generic call to TM server (using FB passed in)
 * @rmid - optional, FAIL if not set
 * @p_xai - optional, NULL if not set
 * @return SUCCEED/FAIL
 */
expublic UBFH* atmi_xa_call_tm_generic_fb(char cmd, char *svcnm_spec, int call_any, short rmid, 
        atmi_xa_tx_info_t *p_xai, UBFH *p_ub)
{
    int ret = EXSUCCEED;
    long rsplen;
    char svcnm[MAXTIDENT+1];
    
    ATMI_TLS_ENTRY;

    if (NULL==p_ub)
    {
        EXFAIL_OUT(ret);
    }
    
    /* Load the data into FB (if available) */
    if (NULL!=p_xai && EXSUCCEED!=atmi_xa_load_tx_info(p_ub, p_xai))
    {
        EXFAIL_OUT(ret);
    }
    
    if (svcnm_spec)
    {
       /* Override the service name! */
        NDRX_STRCPY_SAFE(svcnm, svcnm_spec);
    }
    else if (rmid>0)
    {
        /* Any entry of TM */
        snprintf(svcnm, sizeof(svcnm), NDRX_SVC_RM, rmid);
    }
    else if (call_any)
    {
        /* Any entry of TM */
        /*
        sprintf(svcnm, NDRX_SVC_TM, G_atmi_env.our_nodeid, G_atmi_env.xa_rmid);
         */
        snprintf(svcnm, sizeof(svcnm), NDRX_SVC_RM, G_atmi_env.xa_rmid);
    }
    else
    {
        /* TM + srvid*/
        /* TODO: Think - call local or RM or global!!! */
        
        if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
        {
            snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TM_I, 
                    G_atmi_tls->G_atmi_xa_curtx.txinfo->tmnodeid, 
                    G_atmi_tls->G_atmi_xa_curtx.txinfo->tmrmid, 
                    G_atmi_tls->G_atmi_xa_curtx.txinfo->tmsrvid);
        }
        else if (p_xai)
        {
            snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TM_I, p_xai->tmnodeid, 
                    p_xai->tmrmid, 
                    p_xai->tmsrvid);
        }
        else
        {
            NDRX_LOG(log_error, "No transaction RM info to call!");
            EXFAIL_OUT(ret);
        }
    }
    
    NDRX_LOG(log_debug, "About to call TM, service: [%s]", svcnm);
    
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    if (EXFAIL == tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTRAN))
    {
        NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
        /* TODO: Needs to set the XA error code!! 
        FAIL_OUT(ret);*/
    }
    
    NDRX_LOG(log_debug, "got response from [%s]", svcnm);
    /* TODO Might need debug lib for FB dumps.. */
    ndrx_debug_dump_UBF(log_info, "Response buffer:", p_ub); 
    
    /* Check the response code - load response to atmi_lib error handler...*/
    /* only if we really have an code back! */
    if (atmi_xa_is_error(p_ub))
    {
        atmi_xa2tperr(p_ub);
    }
            
    if (ndrx_TPis_error())
    {
        NDRX_LOG(log_error, "Failed to call RM: %d:[%s] ", 
                            tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
            
out:
            
    if (EXSUCCEED!=ret && NULL!=p_ub)
    {
        atmi_error_t err;
        
        /* Save the original error/needed later! */
        ndrx_TPsave_error(&err);
        tpfree((char *)p_ub);  /* This stuff removes ATMI error!!! */
        ndrx_TPrestore_error(&err);
        p_ub = NULL;
    }

    NDRX_LOG(log_debug, "atmi_xa_call_tm_generic returns %p", p_ub);
    return p_ub;
}

/**
 * Return current transactions XID in context of the branch.
 * We should deserialize & replace branch id
 * TODO: add cache.
 * @return 
 */
expublic XID* atmi_xa_get_branch_xid(atmi_xa_tx_info_t *p_xai)
{
    unsigned char rmid = (unsigned char)G_atmi_env.xa_rmid; /* max 255...! */
    ATMI_TLS_ENTRY;
    
    memset(&G_atmi_tls->xid, 0, sizeof(G_atmi_tls->xid));
    atmi_xa_deserialize_xid((unsigned char *)p_xai->tmxid, &G_atmi_tls->xid);
    
    /* set current branch id - do we need this actually? 
     * How about byte order?
     */
    memcpy(G_atmi_tls->xid.data + sizeof(exuuid_t), 
            &rmid, sizeof(unsigned char));
    
    return &G_atmi_tls->xid;
}   

/*************************** Call descriptor manipulations ********************/

/**
 * Register call descriptor as part of global tx
 * @param p_xai
 * @param cd
 * @return 
 */
expublic int atmi_xa_cd_reg(atmi_xa_tx_cd_t **cds, int in_cd)
{
    int ret = EXSUCCEED;
    
    atmi_xa_tx_cd_t *cdt = NDRX_CALLOC(1, sizeof(atmi_xa_tx_cd_t));
    
    if (NULL==cdt)
    {
        NDRX_LOG(log_error, "Failed to malloc: %s data for cd "
                "binding to global tx!", strerror(errno));
        userlog("Failed to malloc: %s data for cd "
                "binding to global tx!", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    cdt->cd = in_cd;
    
    EXHASH_ADD_INT((*cds), cd, cdt);
    
out:
    return ret;
}

/**
 * Find the cd element in hash
 * @param p_xai
 * @param cd
 * @return 
 */
expublic atmi_xa_tx_cd_t * atmi_xa_cd_find(atmi_xa_tx_cd_t **cds, int in_cd)
{
    atmi_xa_tx_cd_t *ret = NULL;
    EXHASH_FIND_INT( (*cds), &in_cd, ret);    
    return ret;
}

/**
 * Check is any call descriptor registered under global transaction
 * @param p_xai
 * @return 
 */
expublic int atmi_xa_cd_isanyreg(atmi_xa_tx_cd_t **cds)
{
    int ret = EXFALSE;
    atmi_xa_tx_cd_t *el = NULL;
    atmi_xa_tx_cd_t *elt = NULL;
    
    /* Iterate over the hash! */
    EXHASH_ITER(hh, (*cds), el, elt)
    {
        NDRX_LOG(log_error, "Found cd=%d linked to tx!", el->cd);
        ret = EXTRUE;
    }
    
out:
    return ret;
}

/**
 * Remove cd from global tx
 * @param p_xai
 * @param cd
 */
expublic void atmi_xa_cd_unreg(atmi_xa_tx_cd_t **cds, int in_cd)
{
    int ret = EXSUCCEED;
    
    atmi_xa_tx_cd_t *el = atmi_xa_cd_find(cds, in_cd);
    
    if (NULL!=el)
    {
        EXHASH_DEL((*cds), el);
        
        NDRX_FREE(el);
    }
}

/**
 * Clean up the hash - remove all registered call descriptors.
 * @param p_xai
 * @return 
 */
expublic int atmi_xa_cd_unregall(atmi_xa_tx_cd_t **cds)
{
    int ret = EXSUCCEED;
    
    atmi_xa_tx_cd_t *el = NULL;
    atmi_xa_tx_cd_t *elt = NULL;
    
    /* Iterate over the hash! */
    EXHASH_ITER(hh, (*cds), el, elt)
    {
         EXHASH_DEL((*cds), el);
         NDRX_FREE(el);
    }
    
out:
    return ret;
}



