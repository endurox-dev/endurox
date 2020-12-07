/**
 * @brief Master Buffer serialization (aligned TLV of tags/len and XATMI buffers)
 *
 * @file pmultibuf.c
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

/*---------------------------Includes-----------------------------------*/
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <atmi.h>

#include <stdio.h>
#include <stdlib.h>

#include <exnet.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>

#include <utlist.h>
#include <ubf_int.h>            /* FLOAT_RESOLUTION, DOUBLE_RESOLUTION */

#include <typed_buf.h>
#include <ubfutil.h>
#include <math.h>
#include <xatmi.h>
#include <userlog.h>

#include "fdatatype.h"
#include <multibuf.h>
#include "exnetproto.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Loop over the master buffer,
 * this will emit the custom tags of MBUFs
 * 
 * @param fld Current proto field
 * @param level recursion level
 * @param offset offset in C struct (not to the field)
 * @param ex_buf start of the C struct ptr
 * @param ex_len C len
 * @param proto_buf output buffer
 * @param proto_buf_offset current offset at output buf
 * @param accept_tags which flags shall be generated in output
 * @param p_ub_data not used
 * @param proto_bufsz output buffer size
 * @return EXSUCCEED/EXFAIL
 */
expublic int exproto_build_ex2proto_mbuf(cproto_t *fld, int level, long offset,
        char *ex_buf, long ex_len, char *proto_buf, long *proto_buf_offset,
        short *accept_tags, proto_ufb_fld_t *p_ub_data, 
        long proto_bufsz)
{
    ndrx_mbuf_tlv_t *tlv_hdr;
    long tlv_pos;
    unsigned int tag_exp=0;
    int ret = EXSUCCEED;
    long *buf_len = (long *)(ex_buf+offset+fld->counter_offset);
    xmsg_t tmp_cv;
    long len_offset;
    long off_start;
    long off_stop;
    int len_written; /* The length we used  */

    memset(&tmp_cv, 0, sizeof(tmp_cv));
    
    /* write tag */
    if (EXSUCCEED!=ndrx_write_tag((short)fld->tag, proto_buf, 
            proto_buf_offset, proto_bufsz))
    {
        EXFAIL_OUT(ret);
    }

    NDRX_DUMP(log_error, "YOPT EXBUF", ex_buf+offset+fld->offset, ex_len-(offset+fld->offset));
    NDRX_LOG(log_debug, "XINC tag: 0x%x (%s), current offset=%ld fld=%ld", 
            fld->tag, fld->cname, offset, offset+fld->offset);

    /* remember the TLV offset in protobuf  */
    len_offset = *proto_buf_offset;
    CHECK_PROTO_BUFSZ(ret, *proto_buf_offset, proto_bufsz, LEN_BYTES);
    *proto_buf_offset=*proto_buf_offset+LEN_BYTES;

    off_start = *proto_buf_offset; 
    
    /* temp table */
    tmp_cv.descr="MBUF";
    tmp_cv.tabcnt=1;
    tmp_cv.tab[0] = ndrx_G_ndrx_mbuf_tlv_x;

    /* OK load the stuff ... */
    NDRX_LOG(log_debug, "** TLV START **");
    /* so from current position  till then current position + buffer len... */
    for (tlv_pos=offset+fld->offset; tlv_pos< (offset+fld->offset+*buf_len); 
            tlv_pos+=(ALIGNED_GEN(tlv_hdr->len)+sizeof(ndrx_mbuf_tlv_t)), tag_exp++)
    {
        int is_callinfo;
        unsigned int tag;
        int btype;
        
        tlv_hdr=(ndrx_mbuf_tlv_t *)(ex_buf + tlv_pos);
        
        tag = NDRX_MBUF_TAGTAG(tlv_hdr->tag);
        btype = NDRX_MBUF_TYPE(tlv_hdr->tag);
        is_callinfo = !!(tlv_hdr->tag & NDRX_MBUF_CALLINFOBIT);
        
        NDRX_LOG(log_debug, "Buffer tag: %u type: %d callinfo: %d len: %u aligned: %d",
                tag, btype, is_callinfo, tlv_hdr->len, ALIGNED_GEN(tlv_hdr->len));
        
        if (tag!=tag_exp)
        {
            NDRX_LOG(log_error, "ERROR: Expected tag %u but got %u", tag_exp, tag);
            return EXFAIL;
        }
        
        /* NDRX_DUMP(log_debug, "TAG data", tlv_hdr->data, tlv_hdr->len); */
        
        /* Write off the TLV to proto TLV */
        ret = exproto_build_ex2proto(&tmp_cv, 0, 
                tlv_pos, /* < where the data starts*/
                ex_buf, ex_len, proto_buf, proto_buf_offset,
                accept_tags, p_ub_data, proto_bufsz);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Failed to serialize multi-buf");
            EXFAIL_OUT(ret);
        }
    }
    
    
    /* write the len: (back to front) */
    off_stop = *proto_buf_offset;
    /* Put back len there.. */
    len_written = (int)(off_stop - off_start);

    NDRX_LOG(log_debug, "len_written=%d len_offset=%ld", 
            len_written, len_offset);

    if (EXSUCCEED!=ndrx_write_len(len_written, proto_buf, &len_offset, 
            proto_bufsz))
    {
        EXFAIL_OUT(ret);
    }

out:
    NDRX_LOG(log_debug, "** TLV END %d **", ret);
            
    return ret;
}

/**
 * Convert multi-buffer to buffer from network to EX
 * Start to restore the TLV of the C, restore the TLV LEN
 * Restore TAG and start off the XATMI TLV DRIVER
 * @param fld Current conv field, this must be data
 * @param proto_buf protobuf ptr (data rcvd), current position in buf
 *  this points to our data. The data now again is our MBUF TLV in EXPROTO
 * @param proto_len buffer len (current tag len)
 * @param ex_buf Enduro/X output buffer C
 * @param ex_offset current offset in C, updated to offset after data unload
 *  ptr
 * @param max_struct where we are currently. As
 * @param level recursion level
 * @param p_x_fb current UBF building (not used here)
 * @param p_ub_data UBF data (not used here)
 * @param ex_bufsz (Enduro/X output C buffer size)
 * @return EXSUCCED/EXFAIL
 */
expublic int _exproto_proto2ex_mbuf(cproto_t *fld, char *proto_buf, long proto_len, 
        char *ex_buf, long *ex_offset, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data, long ex_bufsz)
{
    
    /* got to de-serialize current tag.... (this is parent) */
    long *buf_len = (long *)(ex_buf+(*max_struct)+fld->counter_offset);
    unsigned step_size;
    int ret = EXSUCCEED;
    ndrx_mbuf_tlv_t *tlv_hdr = (ndrx_mbuf_tlv_t *)(ex_buf+(*max_struct)+fld->offset);
    
    /*
     * The same C offset, yet nothing unloaded.
     */
    ret = _exproto_proto2ex(ndrx_G_ndrx_mbuf_tlv_x, proto_buf, proto_len, 
            ex_buf, *ex_offset, max_struct, 
            0, NULL, NULL, ex_bufsz);

    if (EXSUCCEED!=ret)
    {
        goto out;
    }

    /* add the master-len value (i.e. increment the f and publish to  buf_len */
    step_size=ALIGNED_GEN(tlv_hdr->len)+sizeof(ndrx_mbuf_tlv_t);
    
    /* we might get several calls here */
    p_ub_data->bfldlen+=step_size;
    (*ex_offset)+=step_size;
    
    /* update the call mater len */
    (*buf_len)+=p_ub_data->bfldlen;
    
out:
    
    return ret;    
}

/* vim: set ts=4 sw=4 et smartindent: */
