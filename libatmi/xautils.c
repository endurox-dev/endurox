/* 
** ATMI lib part for XA api - utils
**
** @file xautils.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define     TM_CALL_FB_SZ           1024        /* default size for TM call */


#define _XAUTILS_DEBUG

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

static char encoding_table_xa[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '_'};

static char encoding_table_normal[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static char *decoding_table_xa = NULL;
static char *decoding_table_normal = NULL;

static int mod_table[] = {0, 2, 1};

/*---------------------------Prototypes---------------------------------*/
private char * build_decoding_table(char *encoding_table);

/* private void base64_cleanup(void); */

private char * b64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table);


private unsigned char *b64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *encoding_table);

/**
 * XA Version of Base64 encode
 * @param data
 * @param input_length
 * @param output_length
 * @param encoded_data
 * @return 
 */
public char * atmi_xa_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data) 
{
    return b64_encode(data, input_length, output_length, 
            encoded_data, encoding_table_xa);
}

/**
 * XA Version of base64 decode
 * @param data
 * @param input_length
 * @param output_length
 * @param decoded_data
 * @return 
 */
public unsigned char *atmi_xa_base64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data)
{
    if (decoding_table_xa == NULL)
    {
            decoding_table_xa =  build_decoding_table(encoding_table_xa);
    }
    
    return b64_decode((unsigned char *)data, input_length, output_length, 
            decoded_data, decoding_table_xa);
}


/**
 * Standard Version of Base64 encode
 * @param data
 * @param input_length
 * @param output_length
 * @param encoded_data
 * @return 
 */
char * atmi_base64_encode(unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data) 
{
    return b64_encode(data, input_length, output_length, 
            encoded_data, encoding_table_normal);
}

/**
 * Standard Version of base64 decode
 * @param data
 * @param input_length
 * @param output_length
 * @param decoded_data
 * @return 
 */
unsigned char *atmi_base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data)
{
    if (decoding_table_normal == NULL)
    {
            decoding_table_normal =  build_decoding_table(encoding_table_normal);
    }
    
    return b64_decode((unsigned char *)data, input_length, output_length, 
            decoded_data, decoding_table_normal);
}

/**
 * Encode the data
 * @param data
 * @param input_length
 * @param output_length
 * @return 
 */
private char * b64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length,
                    char *encoded_data, 
                    char *encoding_table) 
{
    int i;
    int j;
    *output_length = 4 * ((input_length + 2) / 3);

    /*
    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;*/

    for (i = 0, j = 0; i < input_length;) 
    {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


/**
 * Decode the data
 * @param data
 * @param input_length
 * @param output_length
 * @return 
 */
private unsigned char *b64_decode(unsigned char *data,
                             size_t input_length,
                             size_t *output_length,
                             char *decoded_data,
                             char *decoding_table)
{

    int i;
    int j;

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    /*
    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;
*/
    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return (unsigned char *)decoded_data;
}

/**
 * Build encoding table
 */
private char * build_decoding_table(char *encoding_table)
{
    int i;
    char *ptr = malloc(256);

    for (i = 0; i < 64; i++)
        ptr[(unsigned char) encoding_table[i]] = i;
    
    return ptr;
}

#if 0
/**
 * Cleanup table
 */
private void base64_cleanup(void)
{
    free(decoding_table);
}
#endif

/*************************** XID manipulation *********************************/

/**
 * Extract info from xid.
 * @param xid xid
 * @param p_nodeid return nodeid
 * @param p_srvid return serverid
 */
public void atmi_xa_xid_get_info(XID *xid, short *p_nodeid, short *p_srvid)
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
public void atmi_xa_xid_str_get_info(char *xid_str, short *p_nodeid, short *p_srvid)
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
public char * atmi_xa_serialize_xid(XID *xid, char *xid_str_out)
{
    int ret=SUCCEED;
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
    
    atmi_xa_base64_encode(tmp, tot_len, &out_len, xid_str_out);
    xid_str_out[out_len] = EOS;
    
    NDRX_LOG(log_debug, "Serialized xid: [%s]", xid_str_out);    
    
    return xid_str_out;
    
}

/**
 * Deserialize - make system XID
 */
public XID* atmi_xa_deserialize_xid(unsigned char *xid_str, XID *xid_out)
{
    unsigned char tmp[XIDDATASIZE+64];
    size_t tot_len = 0;
    long l;
    
    NDRX_LOG(log_debug, "atmi_xa_deserialize_xid - enter");
    NDRX_LOG(log_debug, "Serialized xid: [%s]", xid_str);    
    
    atmi_xa_base64_decode(xid_str, strlen((char *)xid_str), &tot_len, (char *)tmp);
    
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
public atmi_xa_tx_info_t * atmi_xa_curtx_get(char *tmxid)
{
    atmi_xa_tx_info_t *ret = NULL;
    HASH_FIND_STR( G_atmi_xa_curtx.tx_tab, tmxid, ret);    
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
public atmi_xa_tx_info_t * atmi_xa_curtx_add(char *tmxid,
        short tmrmid, short tmnodeid, short tmsrvid, char *tmknownrms)
{
    atmi_xa_tx_info_t * tmp = calloc(1, sizeof(atmi_xa_tx_info_t));
    if (NULL==tmp)
    {
        userlog("malloc failed: %s", strerror(errno));
        goto out;
    }
    
    strcpy(tmp->tmxid, tmxid);
    tmp->tmrmid = tmrmid;
    tmp->tmnodeid = tmnodeid;
    tmp->tmsrvid = tmsrvid;
    strcpy(tmp->tmknownrms, tmknownrms);
    
    HASH_ADD_STR( G_atmi_xa_curtx.tx_tab, tmxid, tmp );
    
out:
    return tmp;
}

/**
 * Remove transaction from list of transaction in progress
 * @param p_txinfo
 */
public void atmi_xa_curtx_del(atmi_xa_tx_info_t *p_txinfo)
{
    HASH_DEL( G_atmi_xa_curtx.tx_tab, p_txinfo);
    /* Remove any cds involved... */
    /* TODO: Think about cd invalidating... */
    atmi_xa_cd_unregall(&(p_txinfo->call_cds));
    atmi_xa_cd_unregall(&(p_txinfo->conv_cds));
    
    free((void *)p_txinfo);
    
    return;
}

/**
 * Convert call structure to xai
 * @param p_xai
 * @param call
 * @return 
 */
public atmi_xa_tx_info_t * atmi_xa_curtx_from_call(tp_command_call_t *call)
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
public void atmi_xa_xai_from_call(atmi_xa_tx_info_t * p_xai, tp_command_call_t *p_call)
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
public int atmi_xa_load_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    char test[100] = {EOS};
    if (SUCCEED!=Bchg(p_ub, TMXID, 0, p_xai->tmxid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMRMID, 0, (char *)&p_xai->tmrmid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMNODEID, 0, (char *)&p_xai->tmnodeid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMSRVID, 0, (char *)&p_xai->tmsrvid, 0L) ||
            SUCCEED!=Bchg(p_ub, TMKNOWNRMS, 0, (char *)p_xai->tmknownrms, 0L)
            )
    {
        NDRX_LOG(log_error, "Failed to setup TMXID/TMRMID/TMNODEID/"
                "TMSRVID/TMKNOWNRMS!");
        FAIL_OUT(ret);
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
public int atmi_xa_read_tx_info(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    
    if (SUCCEED!=Bget(p_ub, TMXID, 0, p_xai->tmxid, 0L) ||
            SUCCEED!=Bget(p_ub, TMRMID, 0, (char *)&p_xai->tmrmid, 0L) ||
            SUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&p_xai->tmnodeid, 0L) ||
            SUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&p_xai->tmsrvid, 0L) ||
            SUCCEED!=Bget(p_ub, TMKNOWNRMS, 0, (char *)p_xai->tmknownrms, 0L)
            )
    {
        NDRX_LOG(log_error, "Failed to setup TMXID/TMRMID/TMNODEID/"
                "TMSRVID/TMKNOWNRMS!");
        FAIL_OUT(ret);
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
public void atmi_xa_cpy_xai_to_call(tp_command_call_t *call, atmi_xa_tx_info_t *p_xai)
{
    XA_TX_COPY(call, p_xai);
}

/**
 * Function prints the list known resource managers in transaction
 * @param tmknownrms
 * @return 
 */
public void atmi_xa_print_knownrms(int dbglev, char *msg, char *tmknownrms)
{
    int i;
    int cnt = strlen(tmknownrms);
    char tmp[128]={EOS};
    
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
public void atmi_xa_reset_curtx(void)
{
    if (G_atmi_xa_curtx.txinfo)
    {
        atmi_xa_curtx_del(G_atmi_xa_curtx.txinfo);
        G_atmi_xa_curtx.txinfo = NULL;
    }
}

/**
 * Check is current RM known in the list
 * @param tmknownrms
 * @return 
 */
public int atmi_xa_is_current_rm_known(char *tmknownrms)
{
    if (NULL==strchr(tmknownrms, (unsigned char)G_atmi_env.xa_rmid))
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * Update the list of known transaction resource managers
 * @param dst_tmknownrms
 * @param src_tmknownrms
 * @return 
 */
public int atmi_xa_update_known_rms(char *dst_tmknownrms, char *src_tmknownrms)
{
    int i;
    int len = strlen(src_tmknownrms);
    int len2;
    int ret = SUCCEED;
    
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
                FAIL_OUT(ret);
            }
            NDRX_LOG(log_info, "1--> %c", dst_tmknownrms[len2]);
            NDRX_LOG(log_info, "2--> %c", src_tmknownrms[i]);
            
            dst_tmknownrms[len2] = src_tmknownrms[i];
            dst_tmknownrms[len2+1] = EOS;
        }
    }
    
out:
    return ret;
}

/**
 * Update known RMs, with info from xai
 * @param p_xai
 */
public int atmi_xa_curtx_set_cur_rmid(atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    int cnt;
    if (NULL==strchr(G_atmi_xa_curtx.txinfo->tmknownrms, (unsigned char)p_xai->tmrmid))
    {
        /* Updated known RMs */
        if ((cnt=strlen(G_atmi_xa_curtx.txinfo->tmknownrms)) > NDRX_MAX_RMS)
        {
            NDRX_LOG(log_error, "Maximum Resource Manager reached (%d)", 
                    NDRX_MAX_RMS);
            userlog("Maximum Resource Manager reached (%d) - Cannot join process "
                    "to XA transaction", NDRX_MAX_RMS);
            ret=FAIL;
            goto out;
        }
        
        G_atmi_xa_curtx.txinfo->tmknownrms[cnt] = (unsigned char)p_xai->tmrmid;
    }
    
    atmi_xa_print_knownrms(log_info, "Known RMs", G_atmi_xa_curtx.txinfo->tmknownrms);
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
public int atmi_xa_set_curtx_from_xai(atmi_xa_tx_info_t *p_xai)
{
    int cnt;
    int ret = SUCCEED;
    
    /* Lookup the hash add if found ok switch ptr 
     * if not found, add and switch ptr too.
     */
    if (NULL==(G_atmi_xa_curtx.txinfo = atmi_xa_curtx_get(p_xai->tmxid)) &&
         NULL==(G_atmi_xa_curtx.txinfo = 
            atmi_xa_curtx_add(p_xai->tmxid, p_xai->tmrmid, 
            p_xai->tmnodeid, p_xai->tmsrvid, p_xai->tmknownrms)))
            
    {
        NDRX_LOG(log_error, "Set current transaction failed!");
        ret=FAIL;
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
public int atmi_xa_set_curtx_from_tm(UBFH *p_ub)
{
    int ret = SUCCEED;
    atmi_xa_tx_info_t xai;
    
    if (SUCCEED!=atmi_xa_read_tx_info(p_ub, &xai))
    {
        ret=FAIL;
        goto out;
    }
    
    /* transfer stuff to current context */
    if (SUCCEED!=atmi_xa_set_curtx_from_xai(&xai))
    {
        ret=FAIL;
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
public UBFH * atmi_xa_alloc_tm_call(char cmd)
{
    UBFH *p_ub = NULL;
    int ret = SUCCEED;
    
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, TM_CALL_FB_SZ)))
    {
        /* TM error should be already set */
        NDRX_LOG(log_error, "Failed to allocate TM call FB (%d)", 
                TM_CALL_FB_SZ);
        ret = FAIL;
        goto out;
    }
    
    /* install caller error */
    if (SUCCEED!=Bchg(p_ub, TMPROCESSID, 0, G_atmi_conf.my_id, 0L))
    {
        _TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMPROCESSID) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = FAIL;
        goto out;
    }
    
    /* install command code */
    if (SUCCEED!=Bchg(p_ub, TMCMD, 0, &cmd, 0L))
    {
        _TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMCMD) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = FAIL;
        goto out;
    }
    
    /* Install caller RM code */
    if (SUCCEED!=Bchg(p_ub, TMCALLERRM, 0, (char *)&G_atmi_env.xa_rmid, 0L))
    {
        _TPset_error_fmt(TPESYSTEM,  "Failed to setup TM call buffer (TMCALLERRM) %d:[%s]", 
                                        Berror, Bstrerror(Berror));
        
        ret = FAIL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "Call buffer setup OK");
    
out:

    if (SUCCEED!=ret && NULL!=p_ub)
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
public int atmi_xa_tm_admincall(char cmd, UBFH *p_ub)
{
    int ret = SUCCEED;
    
    
    
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
public UBFH* atmi_xa_call_tm_generic(char cmd, int call_any, short rmid, 
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
public UBFH* atmi_xa_call_tm_generic_fb(char cmd, char *svcnm_spec, int call_any, short rmid, 
        atmi_xa_tx_info_t *p_xai, UBFH *p_ub)
{
    int ret = SUCCEED;
    long rsplen;
    char svcnm[MAXTIDENT+1];

    if (NULL==p_ub)
    {
        FAIL_OUT(ret);
    }
    
    /* Load the data into FB (if available) */
    if (NULL!=p_xai && SUCCEED!=atmi_xa_load_tx_info(p_ub, p_xai))
    {
        FAIL_OUT(ret);
    }
    
    if (svcnm_spec)
    {
       /* Override the service name! */
        strcpy(svcnm, svcnm_spec);
    }
    else if (rmid>0)
    {
        /* Any entry of TM */
        sprintf(svcnm, NDRX_SVC_RM, rmid);
    }
    else if (call_any)
    {
        /* Any entry of TM */
        /*
        sprintf(svcnm, NDRX_SVC_TM, G_atmi_env.our_nodeid, G_atmi_env.xa_rmid);
         */
        sprintf(svcnm, NDRX_SVC_RM, G_atmi_env.xa_rmid);
    }
    else
    {
        /* TM + srvid*/
        /* TODO: Think - call local or RM or global!!! */
        
        if (G_atmi_xa_curtx.txinfo)
        {
            sprintf(svcnm, NDRX_SVC_TM_I, G_atmi_xa_curtx.txinfo->tmnodeid, 
                    G_atmi_xa_curtx.txinfo->tmrmid, 
                    G_atmi_xa_curtx.txinfo->tmsrvid);
        }
        else if (p_xai)
        {
            sprintf(svcnm, NDRX_SVC_TM_I, p_xai->tmnodeid, 
                    p_xai->tmrmid, 
                    p_xai->tmsrvid);
        }
        else
        {
            NDRX_LOG(log_error, "No transaction RM info to call!");
            FAIL_OUT(ret);
        }
    }
    
    NDRX_LOG(log_debug, "About to call TM, service: [%s]", svcnm);
    
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    if (FAIL == tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen,TPNOTRAN))
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
            
    if (_TPis_error())
    {
        NDRX_LOG(log_error, "Failed to call RM: %d:[%s] ", 
                            tperrno, tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
            
out:
            
    if (SUCCEED!=ret && NULL!=p_ub)
    {
        atmi_error_t err;
        
        /* Save the original error/needed later! */
        _TPsave_error(&err);
        tpfree((char *)p_ub);  /* This stuff removes ATMI error!!! */
        _TPrestore_error(&err);
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
public XID* atmi_xa_get_branch_xid(atmi_xa_tx_info_t *p_xai)
{
    static XID __thread xid; /* handler for new XID */
    unsigned char rmid = (unsigned char)G_atmi_env.xa_rmid; /* max 255...! */
    memset(&xid, 0, sizeof(xid));
    atmi_xa_deserialize_xid((unsigned char *)p_xai->tmxid, &xid);
    
    /* set current branch id - do we need this actually? 
     * How about byte order?
     */
    memcpy(xid.data + sizeof(exuuid_t), &rmid, sizeof(unsigned char));
    
    return &xid;
}   

/*************************** Call descriptor manipulations ********************/

/**
 * Register call descriptor as part of global tx
 * @param p_xai
 * @param cd
 * @return 
 */
public int atmi_xa_cd_reg(atmi_xa_tx_cd_t **cds, int in_cd)
{
    int ret = SUCCEED;
    
    atmi_xa_tx_cd_t *cdt = calloc(1, sizeof(atmi_xa_tx_cd_t));
    
    if (NULL==cdt)
    {
        NDRX_LOG(log_error, "Failed to malloc: %s data for cd "
                "binding to global tx!", strerror(errno));
        userlog("Failed to malloc: %s data for cd "
                "binding to global tx!", strerror(errno));
        FAIL_OUT(ret);
    }
    
    cdt->cd = in_cd;
    
    HASH_ADD_INT((*cds), cd, cdt);
    
out:
    return ret;
}

/**
 * Find the cd element in hash
 * @param p_xai
 * @param cd
 * @return 
 */
public atmi_xa_tx_cd_t * atmi_xa_cd_find(atmi_xa_tx_cd_t **cds, int in_cd)
{
    atmi_xa_tx_cd_t *ret = NULL;
    HASH_FIND_INT( (*cds), &in_cd, ret);    
    return ret;
}

/**
 * Check is any call descriptor registered under global transaction
 * @param p_xai
 * @return 
 */
public int atmi_xa_cd_isanyreg(atmi_xa_tx_cd_t **cds)
{
    int ret = FALSE;
    atmi_xa_tx_cd_t *el = NULL;
    atmi_xa_tx_cd_t *elt = NULL;
    
    /* Iterate over the hash! */
    HASH_ITER(hh, (*cds), el, elt)
    {
        NDRX_LOG(log_error, "Found cd=%d linked to tx!", el->cd);
        ret = TRUE;
    }
    
out:
    return ret;
}

/**
 * Remove cd from global tx
 * @param p_xai
 * @param cd
 */
public void atmi_xa_cd_unreg(atmi_xa_tx_cd_t **cds, int in_cd)
{
    int ret = SUCCEED;
    
    atmi_xa_tx_cd_t *el = atmi_xa_cd_find(cds, in_cd);
    
    if (NULL!=el)
    {
        HASH_DEL((*cds), el);
        
        free(el);
    }
}

/**
 * Clean up the hash - remove all registered call descriptors.
 * @param p_xai
 * @return 
 */
public int atmi_xa_cd_unregall(atmi_xa_tx_cd_t **cds)
{
    int ret = SUCCEED;
    
    atmi_xa_tx_cd_t *el = NULL;
    atmi_xa_tx_cd_t *elt = NULL;
    
    /* Iterate over the hash! */
    HASH_ITER(hh, (*cds), el, elt)
    {
         HASH_DEL((*cds), el);
         free(el);
    }
    
out:
    return ret;
}



