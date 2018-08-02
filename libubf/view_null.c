/* 
 * @brief NULL Value handling in views
 *
 * @file view_null.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Test is given field empty in "memory"
 * @param v resolved view object
 * @param f resolved view field object
 * @param occ occurrence of the field
 * @param view user data memory block to analyze
 * @return EXFALSE/EXTRUE/EXFAIL
 */
expublic int ndrx_Bvnull_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f, 
        BFLDOCC occ, char *cstruct)
{
    int ret = EXFALSE;
    int dim_size = f->fldsize/f->count;
    char *fld_offs = cstruct+f->offset+occ*dim_size;
    short *sv;
    int *iv;
    long *lv;
    float *fv;
    double *dv;
    int i, j;
    int len;
    
    if (f->nullval_none)
    {
        UBF_LOG(log_debug, "field set to NONE, no NULL value...");
        ret=EXFALSE;
        goto out;
    }
    
    switch(f->typecode_full)
    {
        case BFLD_SHORT:
            sv = (short *)fld_offs;
            
            if (*sv==f->nullval_short)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_INT:
            iv = (int *)fld_offs;
            
            if (*iv==f->nullval_int)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_LONG:
            lv = (long *)fld_offs;
            
            if (*lv==f->nullval_long)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_CHAR:
            if (*fld_offs == f->nullval_bin[0])
            {
                ret=EXTRUE;
                goto out;
            }
            
            break;
        case BFLD_FLOAT:
            fv = (float *)fld_offs;
            
            if (fabs( *fv - f->nullval_float)<FLOAT_EQUAL)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_DOUBLE:
            dv = (double *)fld_offs;
            
            if (fabs( *dv - f->nullval_double)<DOUBLE_EQUAL)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_STRING:
            
            /* nullval_bin EOS is set by CALLOC at the parser.. */
            len = dim_size-1; /* EOS do not count in */
            
            /*
             * Well we somehow need to check the length 
             * of the default and the data.
             * For filler the first part must match (length at-least)
             * For non-filler it must be full match
             */
            if (f->flags & NDRX_VIEW_FLAG_NULLFILLER_P)
            {
                /* test the filler */
                ret=EXTRUE;
                
                if (len < f->nullval_bin_len)
                {
                    /* No match , it is shorter.. */
                    ret=EXFALSE;
                    goto out;
                }
                
                for (i=0; i<f->nullval_bin_len; i++)
                {
                    if (i>=len)
                    {
                        ret=EXFALSE;
                        goto out;
                    }
                    else if (i==f->nullval_bin_len-1)
                    {
                        /* compare last bits... */
                        for (j=i; j<len; j++)
                        {
                            if (fld_offs[j]!=f->nullval_bin[i])
                            {
                                ret=EXFALSE;
                                goto out;
                            }
                        }
                    }
                    else if (fld_offs[i]!=f->nullval_bin[i])
                    {
                        ret=EXFALSE;
                        goto out;
                    }
                    
                }
            }
            else
            {
                /* EOS will be set there by calloc... */
                UBF_LOG(log_dump, "STR_CMP: data: [%s] vs obj: [%s]",
                        fld_offs, f->nullval_bin);
                if (0==strcmp(fld_offs, f->nullval_bin))
                {
                    ret=EXTRUE;
                    goto out;
                }
            }
            
            break;
        case BFLD_CARRAY:
            
            len = dim_size;
            
            if (!(f->flags & NDRX_VIEW_FLAG_NULLFILLER_P))
            {
                if (len < f->nullval_bin_len)
                {
                    ret=EXFALSE;
                    goto out;
                }
            }
            
            /* test the filler */
            ret=EXTRUE;
            
            for (i=0; i<f->nullval_bin_len; i++)
            {
                if (i>=len)
                {
                    ret=EXFALSE;
                    goto out;
                }
                else if (f->flags & NDRX_VIEW_FLAG_NULLFILLER_P && 
                        i==f->nullval_bin_len-1)
                {
                    /* compare last bits... */
                    for (j=i; j<len; j++)
                    {
                        if (fld_offs[j]!=f->nullval_bin[i])
                        {
                            ret=EXFALSE;
                            goto out;
                        }
                    }
                }
                else if (fld_offs[i]!=f->nullval_bin[i])
                {
                    ret=EXFALSE;
                    goto out;
                }
                
            }
            
            break;
    }
    
out:
    UBF_LOG(log_debug, "%s: %s.%s presence %d", __func__, v->vname, f->cname, ret);
    return ret;
       
}

/**
 * Test is field null or not 
 * @param cstruct
 * @param cname
 * @param occ
 * @param view
 * @return EXFAIL(-1)/EXFALSE(0)/EXTRUE(1)
 */
expublic int ndrx_Bvnull(char *cstruct, char *cname, BFLDOCC occ, char *view)
{
    int ret = EXFALSE;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;
    
    /* resolve view & field, call ndrx_Bvnull_int */
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }
    
    if (occ > f->count-1 || occ<0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Invalid occurrence requested for field "
                "%s.%s, count=%d occ=%d (zero base)",
                v->vname, f->cname, f->count, occ);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==(ret=ndrx_Bvnull_int(v, f, occ, cstruct)))
    {
        /* should not get here.. */
        ndrx_Bset_error_fmt(BBADVIEW, "System error occurred.");
        goto out;
    }
    
out:
    return ret;
}

/**
 * Initialize single structure element to NULL
 * @param v view object
 * @param f field object
 * @param cstruct c structure
 * @return 
 */
expublic int ndrx_Bvselinit_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f,  
        BFLDLEN single_occ, char *cstruct)
{
    int ret = EXSUCCEED;
    int dim_size;
    char *fld_offs;
    short *sv;
    int *iv;
    long *lv;
    float *fv;
    double *dv;
    int i, j;
    int len;
    int occ;
    short *C_count;
    unsigned short *L_length;
    int start_occ;
    int stop_occ;
    
    if (f->nullval_none)
    {
        UBF_LOG(log_debug, "field set to NONE, no NULL value...");
        goto out;
    } 
    
    if (EXFAIL!=single_occ)
    {
        start_occ = single_occ;
        stop_occ = single_occ+1;
    }
    else
    {
        start_occ = 0;
        stop_occ = f->count;
    }
    
    for (occ=start_occ; occ<stop_occ; occ++)
    {   
     
        if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
        {
            C_count = (short *)(cstruct+f->count_fld_offset);
            
            /* Reset only if full NULLing */
            if (EXFAIL==single_occ)
            {
                *C_count = 0;
            }
        }
        
        dim_size = f->fldsize/f->count;
        fld_offs = cstruct+f->offset+occ*dim_size;   
        
        switch(f->typecode_full)
        {
            case BFLD_SHORT:
                sv = (short *)fld_offs;
                *sv=f->nullval_short;
                
                break;
            case BFLD_INT:
                iv = (int *)fld_offs;
                *iv=f->nullval_int;
                break;
            case BFLD_LONG:
                lv = (long *)fld_offs;
                *lv=f->nullval_long;
                break;
            case BFLD_CHAR:
                *fld_offs = f->nullval_bin[0];
                break;
            case BFLD_FLOAT:
                fv = (float *)fld_offs;
                *fv = f->nullval_float;
                break;
            case BFLD_DOUBLE:
                dv = (double *)fld_offs;
                *dv = f->nullval_double;
                break;
            case BFLD_STRING:
                
                /* nullval_bin EOS is set by CALLOC at the parser.. */
                if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
                {
                    
                    L_length = (unsigned short *)(cstruct+f->length_fld_offset+
                                occ*sizeof(unsigned short));
                    *L_length = 0;
                }
                
                if (f->flags & NDRX_VIEW_FLAG_NULLFILLER_P)
                {
                    for (i=0; i<f->nullval_bin_len; i++)
                    {
                        if (i>=dim_size-1)
                        {
                            /* Put EOS */
                            fld_offs[i] = EXEOS;
                            break;
                        } /* if we are at the end... */
                        else if (i==f->nullval_bin_len-1)
                        {
                            /* Transfer last char as multiple copies.. */
                            for (j=i; j<dim_size-1; j++)
                            {
                                fld_offs[j]=f->nullval_bin[i];
                            }
                            
                            fld_offs[dim_size-1] = EXEOS;
                            break;
                        }
                        else
                        {
                            /* transfer the value.. */
                            fld_offs[i]=f->nullval_bin[i];
                        }
                    }
                }
                else
                {   
                    len = f->nullval_bin_len;
                    
                    if (len>dim_size)
                    {
                        len = dim_size-1; /* Place for eos */
                    }
                    
                    NDRX_STRNCPY(fld_offs, f->nullval_bin, len);
                    
                    fld_offs[len] = EXEOS;                    
                }
                
                break;
            case BFLD_CARRAY:

                /* nullval_bin EOS is set by CALLOC at the parser.. */
                if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
                {
                     L_length = (unsigned short *)(cstruct+f->length_fld_offset+
                                occ*sizeof(unsigned short));
                    *L_length = 0;
                }
                
                len = dim_size;

                /* test the filler */
                for (i=0; i<f->nullval_bin_len; i++)
                {
                    if (i>=len)
                    {
                        break;
                    }
                    else if (f->flags & NDRX_VIEW_FLAG_NULLFILLER_P && 
                            i==f->nullval_bin_len-1 && i<len)
                    {
                        /* compare last bits... */
                        for (j=i; j<len; j++)
                        {
                            fld_offs[j]=f->nullval_bin[i];
                        }
                    }
                    else
                    {
                        fld_offs[i]=f->nullval_bin[i];
                    }
                }
                break;
        } /* switch typecode */
    } /* for occ */
    
out:
    
    return ret;
       
}

/**
 * Init VIEW element to zero
 * @param cstruct c struct
 * @param cname field name
 * @param view view name
 * @return  EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvselinit(char *cstruct, char *cname, char *view) 
{
    int ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }
        
    if (EXFAIL==ndrx_Bvselinit_int(v, f, EXFAIL, cstruct))
    {
        /* should not get here.. */
        ndrx_Bset_error_fmt(BBADVIEW, "System error occurred.");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/* Next todo: Fvsinit */

/**
 * Initialize view to default values.
 * @param v resolved view object
 * @param cstruct memory bloc to initialize
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvsinit_int(ndrx_typedview_t *v, char *cstruct)
{
    int ret =EXSUCCEED;
    ndrx_typedview_field_t *f;
    
    /* Go over the c struct  */
    DL_FOREACH(v->fields, f)
    {
        if (EXSUCCEED!=ndrx_Bvselinit_int(v, f, EXFAIL, cstruct))
        {
            ndrx_Bset_error_fmt(BBADVIEW, "System error occurred.");
            goto out;
        }
    }
    
out:
    return ret;
}

/**
 * Initialize full structure by given view
 * @param cstruct memory addr
 * @param view view name
 * @return  EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvsinit(char *cstruct, char *view)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_Bvsinit_int(v, cstruct))
    {
        UBF_LOG(log_error, "ndrx_Bvsinit_int failed!");
        EXFAIL_OUT(ret);
    }
    
out:
                
    return ret;
}


