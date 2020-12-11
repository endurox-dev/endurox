/**
 * @brief Serialize / Deserialize XATMI buffer
 *  Currently only deserialize here, as due to master buffer
 *  This becomes inner processing
 *
 * @file pxatmibuf.c
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
 * Read the ATMI buffer to Enduro/X
 * @param fld
 * @param proto_buf
 * @param proto_len
 * @param ex_buf
 * @param ex_offset
 * @param max_struct
 * @param level
 * @param p_x_fb
 * @param p_ub_data
 * @param ex_bufsz
 * @return 
 */
expublic int _exproto_proto2ex_atmibuf(cproto_t *fld, char *proto_buf, long proto_len, 
        char *ex_buf, long ex_offset, long *max_struct, int level, 
        UBFH *p_x_fb, proto_ufb_fld_t *p_ub_data, long ex_bufsz)
{
    /* This is special driver for ATMI buffer */
    unsigned buffer_type = *((unsigned*)(ex_buf+ex_offset+fld->buftype_offset));
    unsigned *buf_len = (unsigned *)(ex_buf+ex_offset+fld->counter_offset);
    char *data = (char *)(ex_buf+ex_offset+fld->offset);
    int f_type;

    buffer_type = NDRX_MBUF_TYPE(buffer_type);

    NDRX_LOG(log_debug, "Buffer type is: %u", 
            buffer_type);

    if (BUF_TYPE_UBF==buffer_type ||
            BUF_TYPE_VIEW==buffer_type)
    {
        UBFH *p_ub = (UBFH *)data;

        /* char f_data_buf[sizeof(proto_ufb_fld_t) + NDRX_MSGSIZEMAX + NDRX_PADDING_MAX];*/
        char *f_data_buf;
        ssize_t f_data_buf_len;
        proto_ufb_fld_t *f;
        BFLDOCC occ;

        short accept_tags[] = {UBF_TAG_BFLDID, UBF_TAG_BFLDLEN, 0, EXFAIL};

        /* Reserve space for Tag/Length */
        /* <sub tlv> */
        long len_offset;
        long off_start;
        long off_stop;

        xmsg_t tmp_cv;

        NDRX_SYSBUF_MALLOC_OUT(f_data_buf, f_data_buf_len, ret);

        f =  (proto_ufb_fld_t *)f_data_buf;

        /* This is sub tlv/ thus put tag... */
        if (EXSUCCEED!=ndrx_write_tag((short)p->tag, proto_buf, 
                proto_buf_offset, proto_bufsz))
        {
            NDRX_SYSBUF_FREE(f_data_buf);
            EXFAIL_OUT(ret);
        }

        len_offset = *proto_buf_offset;
        CHECK_PROTO_BUFSZ(ret, *proto_buf_offset, proto_bufsz, LEN_BYTES);
        *proto_buf_offset=*proto_buf_offset+LEN_BYTES;

        off_start = *proto_buf_offset;
        /* </sub tlv> */
        memcpy(&tmp_cv, cv, sizeof(tmp_cv));
        tmp_cv.tab[0] = M_ubf_field;

        /* <process field by field> */
        NDRX_LOG(log_debug, "Processing UBF buffer");

        /* loop over the buffer & process field by field */
        /*memset(f.buf, 0, sizeof(f.buf));  <<< HMMM Way too slow!!! */

        f->bfldlen = NDRX_MSGSIZEMAX - sizeof(*f);
        f->bfldid = BFIRSTFLDID;
        while(1==Bnext(p_ub, &f->bfldid, &occ, f->buf, &f->bfldlen))
        {
            f_type = Bfldtype(f->bfldid);

            accept_tags[2] = M_ubf_proto_tag_map[f_type];

            NDRX_LOG(log_debug, "UBF type %d mapped to "
                    "tag %hd", f_type, accept_tags[2]);

            /* Hmm lets drive our structure? */

            ret = exproto_build_ex2proto(&tmp_cv, 0, 0,
                (char *)f, f_data_buf_len, proto_buf, 
                proto_buf_offset,  accept_tags, f, proto_bufsz);

            if (EXSUCCEED!=ret)
            {
                NDRX_LOG(log_error, "Failed to convert "
                        "sub/tag %x: [%s] %ld"
                        "at offset %ld", 
                        p->tag, p->cname, p->offset);
                NDRX_SYSBUF_FREE(f_data_buf);
                EXFAIL_OUT(ret);
            }
            /*
             * why?
            memset(f.buf, 0, sizeof(f.buf));
             */
            f->bfldlen = f_data_buf_len - sizeof(*f);
        }

        /* </process field by field> */

        /* <sub tlv> */
        off_stop = *proto_buf_offset;
        /* Put back len there.. */
        len_written = (short)(off_stop - off_start);
        if (EXSUCCEED!=ndrx_write_len(len_written, proto_buf, &len_offset,
                proto_bufsz))
        {
            NDRX_SYSBUF_FREE(f_data_buf);
            EXFAIL_OUT(ret);
        }
        /* </sub tlv> */
        NDRX_SYSBUF_FREE(f_data_buf);
    }
    else
    {
        /* Should work for string buffers too, if EOS counts in len! */
        NDRX_LOG(log_debug, "Processing data block buffer");

        if (EXSUCCEED!=ndrx_write_tag((short)p->tag, 
                proto_buf, proto_buf_offset, proto_bufsz))
        {
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=ndrx_write_len((int)*buf_len, proto_buf, 
                proto_buf_offset, proto_bufsz))
        {
            EXFAIL_OUT(ret);
        }

        len_written = *buf_len;

        /* Put data on network */
        CHECK_PROTO_BUFSZ(ret, *proto_buf_offset, proto_bufsz, *buf_len);
        memcpy(proto_buf+(*proto_buf_offset), data, *buf_len);
        *proto_buf_offset=*proto_buf_offset + *buf_len;
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
