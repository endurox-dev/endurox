/**
 * @brief Compare view buffer values
 *
 * @file view_cmp.c
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
#include <math.h>
#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>

#include <userlog.h>
#include <typed_view.h>
#include <view_cmn.h>
#include <atmi_tls.h>

#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NUM_COMP(A, B) \
        do \
        {\
            if ((A) > (B)) \
            {\
                ret=1;\
                UBF_LOG(log_debug, "NUM_COMP non eq cname=[%s] occ=%d (A=%ld > B=%ld)", f->cname, occ, (long)(A), (long)(B));\
                goto out;\
            }\
            else if ((A) < (B)) \
            {\
                ret=-1;\
                UBF_LOG(log_debug, "NUM_COMP non eq cname=[%s] occ=%d (A=%ld< B=%ld)", f->cname, occ, (long)(A), (long)(B));\
                goto out;\
            }\
    } while (0)\

#define FLOAT_COMP(A, B) \
    do \
    {\
        if (fabs((A)-(B)) > FLOAT_EQUAL)\
        {\
            if ((A) > (B))\
            {\
                ret=1;\
                UBF_LOG(log_debug, "FLOAT_COMP non eq cname=[%s] occ=%d (A=%f > B=%f)", f->cname, occ, (float)(A), (float)(B));\
                goto out;\
            }\
            else\
            {\
                ret=-1;\
                UBF_LOG(log_debug, "FLOAT_COMP non eq cname=[%s] occ=%d (A=%f < B=%f)", f->cname, occ, (float)(A), (float)(B));\
                goto out;\
            }\
        }\
    } while (0)

#define DOUBLE_COMP(A, B) \
    do \
    {\
        if (fabs((A)-(B)) > DOUBLE_EQUAL) \
        {\
            if ((A) > (B)) \
            {\
                ret=1;\
                UBF_LOG(log_debug, "DOUBLE_COMP non eq cname=[%s] occ=%d (A=%lf > B=%lf)", f->cname, occ, (double)(A), (double)(B));\
                goto out;\
            } \
            else \
            {\
                ret=-1;\
                UBF_LOG(log_debug, "DOUBLE_COMP non eq cname=[%s] occ=%d (A=%lf < B=%lf)", f->cname, occ, (double)(A), (double)(B));\
                goto out;\
            }\
        }\
     } while (0)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compare to views
 * @param cstruct1 view structure 1 ptr
 * @param view1 view name 1
 * @param cstruct2 view structure 2 ptr
 * @param view2 view name 2
 * @return  0 - view name/data are equal
 *  -1 - first view field is less than view2/cstruct
 *  1 - first view is greater than view2
 *  -2 - there is error (view not found)
 */
expublic int ndrx_Bvcmp(char *cstruct1, char *view1, char *cstruct2, char *view2)
{
    int ret = 0;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f;
    short *C_count1, *C_count2;
    short C_count_stor1, C_count_stor2;
    unsigned short *L_length1, *L_length2;
    BFLDOCC occ;
    short *sv1, *sv2;
    int *iv1, *iv2;
    long *lv1, *lv2;
    float *fv1, *fv2;
    double *dv1, *dv2;
    int len_cmp;
    
    UBF_LOG(log_debug, "Compare views by data %p [%s] %p [%s]", 
            cstruct1, view1, cstruct2, view2);

    if (EXEOS==view1[0] && EXEOS==view2[0])
    {
        UBF_LOG(log_debug, "Both empty views - assume equals");
        goto out;
    }
    
    if (0!=(ret=strcmp(view1, view2)))
    {
        UBF_LOG(log_debug, "Name differs");
        
        /* normalize values */
        if (ret<-1)
        {
            ret=-1;
        }
        else if (ret>1)
        {
            ret=1;
        }
        
        goto out;
    }
    
    /* find the descriptor, only one needed, as matches*/
    if (NULL==(v = ndrx_view_get_view(view1)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view1);
        ret=-2;
        goto out;
    }
    
    DL_FOREACH(v->fields, f)
    {        
        NDRX_LOG(log_dump, "Processing field: [%s]", f->cname);
        /* Check do we have length indicator? */
        if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
        {
            C_count1 = (short *)(cstruct1+f->count_fld_offset);
            C_count2 = (short *)(cstruct2+f->count_fld_offset);
        }
        else
        {
            C_count_stor1=f->count; 
            C_count1 = &C_count_stor1;
            
            C_count_stor2=f->count; 
            C_count2 = &C_count_stor2;
        }
        
        NUM_COMP(*C_count1, *C_count2);
        
        /* well we must support arrays too...! of any types
         * Arrays we will load into occurrences of the same buffer
         */
        /* loop over the occurrences 
         * we might want to send less count then max struct size..*/
        for (occ=0; occ<*C_count1; occ++)
        {
            int dim_size = f->fldsize/f->count;
            /* OK we need to understand following:
             * size (data len) of each of the array elements..
             * the header plotter needs to support occurrences of the
             * length elements for the arrays...
             * 
             * For outgoing if there is only one occurrence for field
             * then we can can detect the field to be NULL,
             * if NULL, then we do not send empty fields over..
            */ 
            char *fld_offs1 = cstruct1+f->offset+occ*dim_size;
            char *fld_offs2 = cstruct2+f->offset+occ*dim_size;
            
            
            switch(f->typecode_full)
            {
                case BFLD_SHORT:
                    sv1 = (short *)fld_offs1;
                    sv2 = (short *)fld_offs2;

                    NUM_COMP(*sv1, *sv2);
                    
                    break;
                case BFLD_INT:
                    iv1 = (int *)fld_offs1;
                    iv2 = (int *)fld_offs2;

                    NUM_COMP(*iv1, *iv2);
                    
                    break;
                case BFLD_LONG:
                    lv1 = (long *)fld_offs1;
                    lv2 = (long *)fld_offs2;

                    NUM_COMP(*lv1, *lv2);
                    
                    break;
                case BFLD_CHAR:
                    
                    NUM_COMP(*fld_offs1, *fld_offs2);

                    break;
                case BFLD_FLOAT:
                    fv1 = (float *)fld_offs1;
                    fv2 = (float *)fld_offs2;
                    FLOAT_COMP(*fv1, *fv2);
                    break;
                case BFLD_DOUBLE:
                    dv1 = (double *)fld_offs1;
                    dv2 = (double *)fld_offs2;

                    DOUBLE_COMP(*dv1, *dv2);
                    break;
                case BFLD_STRING:

                    if (0!=(ret=strcmp(fld_offs1, fld_offs2)))
                    {
                        if (ret<-1)
                        {
                            ret=-1;
                        }
                        else if (ret>1)
                        {
                            ret=1;
                        }

                        goto out;
                    }
                    
                    break;
                case BFLD_CARRAY:

                    if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
                    {

                        L_length1 = (unsigned short *)(cstruct1+f->length_fld_offset+
                                occ*sizeof(unsigned short));
                        
                        L_length2 = (unsigned short *)(cstruct2+f->length_fld_offset+
                                occ*sizeof(unsigned short));
                        
                        NUM_COMP(*L_length1, *L_length2);
                        
                        len_cmp = *L_length1;
                    }
                    else
                    {
                        len_cmp = dim_size;
                    }
                    
                    if (0!=(ret=memcmp(fld_offs1, fld_offs2, len_cmp)))
                    {
                        if (ret<-1)
                        {
                            ret=-1;
                        }
                        else if (ret>1)
                        {
                            ret=1;
                        }
                        goto out;
                    }
                    
                    break;
            }
        } /* for occ */
    }
    
out:
    
    NDRX_LOG(log_debug, "Result %d", ret);
    return ret;

}

/* vim: set ts=4 sw=4 et smartindent: */
