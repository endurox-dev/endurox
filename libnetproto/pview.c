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
        short *accept_tags, proto_ufb_fld_t *p_ub_data, 
        long proto_bufsz)
{
    int ret = EXSUCCEED;
    BFLDID bfldid;
    BFLDLEN  len;
    BFLDOCC occ;
    char *p;
    int i;
    Bvnext_state_t bprint_state;
    /* Indicators.. */
    short *C_count;
    short C_count_stor;
    unsigned short *L_length; /* will transfer as long */
    BVIEWFLD *vdata = (ex_buf+offset+fld->offset);
    ndrx_typedview_t *v;
    ndrx_typedview_field_t *f;
    char *cstruct = vdata->data;
    
    UBF_LOG(log_debug, "%s enter at level %d", __func__, level);
    
    memset(&bprint_state, 0, sizeof(bprint_state));
    
    /* TODO: Process empty view... (just put no data... what so ever) */
    
    if (EXEOS==vdata->vname[0])
    {
        NDRX_LOG(log_debug, "Empty view - no data conv");
        goto out;
    }
    
    /* Resolve view descriptor */
    if (NULL==(v = ndrx_view_get_view(vdata->vname)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", vdata->vname);
        EXFAIL_OUT(ret);
    }
    
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
                len = (BFLDLEN)*L_length;
            }
            else
            {
                len=dim_size;
            }
            
            /* TODO: Do we need to print nulls? */
        }
    }
    
  
out:
    NDRX_LOG(log_debug, "** TLV END %d **", ret);
            
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
