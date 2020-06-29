/**
 * @brief View print & parse functions to/from UBF style output
 *
 * @file view_print.c
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
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>
#include <atmi_tls.h>
#include <cf.h>
#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print VIEW data to file pointer 
 * @param cstruct ptr to the view struct (c)
 * @param view view name
 * @param outf file pointer to which to print
 * @param p_writef masking function if any (shared with UBF)
 * @param dataptr1 RFU data pointer
 * @param level level (front tabs at which to print)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvfprint (char *cstruct, char *view, FILE * outf,
          ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1, int level)
{
    int ret=EXSUCCEED;
    BFLDID bfldid;
    BFLDLEN  len;
    BFLDOCC occ;
    char *p;
    int fldtype;
    char *cnv_buf = NULL;
    char *tmp_buf = NULL;
    BFLDLEN cnv_len;
    char fmt_wdata[256];
    char fmt_ndata[256];
    int i;
    BVIEWFLD *vdata;
    Bvnext_state_t bprint_state;
    char cname[NDRX_VIEW_CNAME_LEN+1];
    BFLDOCC maxocc;
    long dim_size;
    Bvnext_state_t state;
    char *p_view = view;
    
    /* Indicators.. */
    short *C_count;
    short C_count_stor;
    unsigned short *L_length; /* will transfer as long */
    long L_len_long;
    
    ndrx_typedview_t *v;
    ndrx_typedview_field_t *f;
    
    UBF_LOG(log_debug, "%s enter at level %d", __func__, level);
    
    memset(&bprint_state, 0, sizeof(bprint_state));
    
    for (i=0; i<level; i++)
    {
        fmt_wdata[i]='\t';
        fmt_ndata[i]='\t';
    }
    
    fmt_wdata[i]='%';
    fmt_ndata[i]='%';
    
    i++;
    fmt_wdata[i]='s';
    fmt_ndata[i]='s';
    
    i++;
    fmt_wdata[i]='\\';
    fmt_ndata[i]='\\';
            
    i++;
    fmt_wdata[i]='t';
    fmt_ndata[i]='t';
    
    i++;
    fmt_wdata[i]='%';
    fmt_ndata[i]='\\';
    
    i++;
    fmt_wdata[i]='s';
    fmt_ndata[i]='n';
    fmt_ndata[i+1]=EXEOS;
    
    i++;
    fmt_wdata[i]='\\';
    
    i++;
    fmt_wdata[i]='n';
    fmt_ndata[i+1]=EXEOS;
    
    /*
    struct MYVIEW1 v;
    
    init_MYVIEW1(&v);
    
    assert_equal(Bvnext (&state, "MYVIEW1", cname, &fldtype, &maxocc, &dim_size), 1);
    assert_string_equal(cname, "tshort1");
    assert_equal(fldtype, 0);
    assert_equal(maxocc, 1);
    assert_equal(dim_size, sizeof(short));
    
    
    assert_equal(Bvnext (&state, NULL, cname, &fldtype, &maxocc, &dim_size), 1);
    */
    
    bfldid = BFIRSTFLDID;

    DL_FOREACH(v->fields, f)
    {
        p_view=NULL;
        
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
            int dim_size = f->fldsize/f->count;
            char *fld_offs = cstruct+f->offset+occ*dim_size;
        }
        
        
        
        /* now check are we printable? */
        if (CBvget())
        {
            int temp_len;
            
            /* For strings we must count off trailing EOS */
            if (BFLD_STRING==fldtype)
            {
                len--;
            }

            temp_len = ndrx_get_nonprintable_char_tmpspace(p, len);

            if (temp_len!=len) /* for carray we need EOS at end! */
            {
                UBF_LOG(log_debug, "Containing special characters -"
                                    " needs to temp buffer for prefixing");
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                if (NULL==tmp_buf)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: Failed to allocate ",
                            __func__, temp_len+1);
                    EXFAIL_OUT(ret);
                }

                /* build the printable string */
                ndrx_build_printable_string(tmp_buf, temp_len+1, p, len);
                p = tmp_buf;
            }
            else if (BFLD_CARRAY==fldtype) /* we need EOS for carray... */
            {
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                
                memcpy(tmp_buf, p, temp_len);
                
                if (NULL==tmp_buf)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: Failed to allocate ", 
                            __func__, temp_len+1);
                    EXFAIL_OUT(ret);
                }
                tmp_buf[temp_len] = EXEOS;
                p = tmp_buf;
            }
        }

        /* value is kept in p */
        if (len>0)
        {
/* #define OUTPUT_FORMAT_WDATA "%s\t%s\n", ndrx_Bfname_int(bfldid), p*/

            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                int do_write = EXFALSE;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_WDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(&tmp, tmp_len, dataptr1, &do_write, 
                        outf, bfldid)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s]", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
                }
                
                if (do_write)
                {
                    fprintf(outf, "%s", tmp);
                }
                        
                NDRX_FREE(tmp);
            }
            else
            {
                fprintf(outf, OUTPUT_FORMAT_WDATA);
            }
            
        }
        else
        {
            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                int do_write = EXFALSE;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_NDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed 2", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(&tmp, tmp_len, dataptr1, &do_write, outf,
                        bfldid)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s] 2", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
                }
                
                if (do_write)
                {
                    fprintf(outf, "%s", tmp);
                }
                
                NDRX_FREE(tmp);
            }
            else
            {
                fprintf(outf, OUTPUT_FORMAT_NDATA);
            }
   
        }
        
        if (NULL!=outf && ferror(outf))
        {
            ndrx_Bset_error_fmt(BEUNIX, "Failed to write to file with error: [%s]",
                        strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* Step into printing the inner ubf */
        if (BFLD_UBF==fldtype)
        {
            if (EXSUCCEED!=ndrx_Bfprint ((UBFH *)p, outf, p_writef, dataptr1, level+1))
            {
                UBF_LOG(log_error, "ndrx_Bfprint failed at level %d", level+1);
                EXFAIL_OUT(ret);
            }
        }
        else if (BFLD_VIEW==fldtype)
        {
            /* at this step we shall print the VIEW at given indentation level */
            if (EXSUCCEED!=ndrx_Bvfprint (vdata->data, vdata->vname, outf,
                    p_writef, dataptr1, level+1))
            {
                UBF_LOG(log_error, "ndrx_Bvfprint failed at level %d", level+1);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    /* Free up allocated resources */
    if (NULL!=tmp_buf)
    {
        NDRX_FREE(tmp_buf);
    }

    if (NULL!=cnv_buf)
    {
        NDRX_FREE(cnv_buf);
    }

    /* release the stuff... */
    fflush(outf);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
