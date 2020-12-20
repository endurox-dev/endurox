/**
 * @brief Protocol view support
 *
 * @file pview.c
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
#include <ubfview.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Convert view XATMI buffer to protocol
 * Loop over the view, read the non null fields
 * NOTE: In case if this MBUF, then view data is encapsulated 
 * in ndrx_view_header 
 * If view comes from UBF then it is BVIEWFLD
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
expublic int exproto_build_ex2proto_view(cproto_t *fld, int level, long offset,
        char *ex_buf, long ex_len, char *proto_buf, long *proto_buf_offset,
        long proto_bufsz)
{
    int ret = EXSUCCEED;
    BFLDID bfldid;
    BFLDOCC occ;
    char *p;
    /* Indicators.. */
    short *C_count;
    short C_count_stor;
    unsigned short *L_length; /* will transfer as long */
    ndrx_typedview_t *v;
    ndrx_typedview_field_t *f;
    char *cstruct;
    char f_data_buf[sizeof(proto_ufb_fld_t)+sizeof(char *)]; /* just store ptr */
    ssize_t f_data_buf_len;
    proto_ufb_fld_t *fldata = (proto_ufb_fld_t*)f_data_buf;
    short accept_tags[] = {VIEW_TAG_CNAME, VIEW_TAG_BFLDLEN, 0, EXFAIL};
    xmsg_t tmp_cv;
    BVIEWFLD vheader;
    
    f_data_buf_len = sizeof(f_data_buf);
    
    UBF_LOG(log_debug, "%s enter at level %d", __func__, level);
    
    if (XATMIBUFPTR==fld->type)
    {
        BVIEWFLD *vdata = (BVIEWFLD *)ex_buf;
            
        NDRX_STRCPY_SAFE(vheader.vname, vdata->vname);
        vheader.vflags = vdata->vflags;
    }
    else
    {
        ndrx_view_header *p_hdr = (ndrx_view_header *)ex_buf;
        
        /* Resolve view descriptor */
        NDRX_STRCPY_SAFE(vheader.vname, p_hdr->vname);
        vheader.vflags = p_hdr->vflags;
        
    }
    
    /* write off header... */
    tmp_cv.descr = "VIEWHDR";
    tmp_cv.tab[0] = ndrx_G_view;
    tmp_cv.tabcnt=1;
    tmp_cv.command = 0; /* not used... */
    
    if (EXSUCCEED!=exproto_build_ex2proto(&tmp_cv, 0, 0,(char *)&vheader, 
        sizeof(vheader), proto_buf, proto_buf_offset,  
        NULL, fldata, proto_bufsz))
    {
        NDRX_LOG(log_error, "Failed to emit view[%s] header", vheader.vname);
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==vheader.vname[0])
    {
        NDRX_LOG(log_debug, "Empty view - no data conv");
        goto out;
    }

    if (NULL==(v = ndrx_view_get_view(vheader.vname)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", vheader.vname);
        EXFAIL_OUT(ret);
    }
    
    tmp_cv.descr = "VIEWLFLD";
    tmp_cv.tab[0] = ndrx_G_view_field;
    tmp_cv.tabcnt=1;
    tmp_cv.command = 0; /* not used... */
        
    DL_FOREACH(v->fields, f)
    {
        if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
        {
            C_count = (short *)(cstruct+f->count_fld_offset);
        }
        else
        {
            C_count_stor=f->count; 
            C_count = &C_count_stor;
        }
        
        /* extra check: */
        if (*C_count > f->count)
        {
            UBF_LOG(log_error, "Invalid count for field %s.%s in "
                    "view %hd, specified: %hd", v->vname, f->cname, 
                    f->count, *C_count);
            
            ndrx_Bset_error_fmt(BNOCNAME, "Invalid count for field %s.%s in "
                    "view %hd, specified: %hd", v->vname, f->cname, 
                    f->count, *C_count);
            EXFAIL_OUT(ret);
        }
        
        NDRX_STRCPY_SAFE(fldata->cname, f->cname);
        

        for (occ=0; occ<*C_count; occ++)
        {
            BFLDLEN dim_size = f->fldsize/f->count;
            p = cstruct+f->offset+occ*dim_size;
            
            /* just increment the field id for view.. */
            bfldid++;
            
            /* get the carray length  */
            if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
            {
                L_length = (unsigned short *)(cstruct+f->length_fld_offset+
                            occ*sizeof(unsigned short));
                fldata->bfldlen= (BFLDLEN)*L_length;
            }
            else
            {
                fldata->bfldlen=dim_size;
            }
            
            /* Optimize out the length field for fixed
             * data types
             */
            accept_tags[2] = ndrx_G_ubf_proto_tag_map[f->typecode_full];
            
            /* put the pointer value there */
            memcpy(fldata->buf, p, sizeof(char *));
            
            /* lets drive our structure? */
            ret = exproto_build_ex2proto(&tmp_cv, 0, 0,(char *)f, 
                    f_data_buf_len, proto_buf, proto_buf_offset,  
                    accept_tags, fldata, proto_bufsz);

            if (EXSUCCEED!=ret)
            {
                NDRX_LOG(log_error, "Failed to convert sub/tag %x: view: [%s] "
                        "cname: [%s] occ: %d"
                    "at offset %ld", fld->tag, v->vname, f->cname, occ);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
            
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
