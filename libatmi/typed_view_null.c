/* 
** NULL Value handling in views
**
** @file typed_view_null.c
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
#include <tperror.h>

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
        BFLDOCC occ, char *view)
{
    int ret = EXFALSE;
    int dim_size = f->fldsize/f->count;
    char *fld_offs = view+f->offset+occ*dim_size;
    short *sv;
    int *iv;
    long *lv;
    float *fv;
    double *dv;
    int i, j;
    int len;
    
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
            
            if (fabs( *dv - f->nullval_float)<DOUBLE_EQUAL)
            {
                ret=EXTRUE;
                goto out;
            }
            break;
        case BFLD_STRING:
            
            /* nullval_bin EOS is set by CALLOC at the parser.. */
            len = strlen(fld_offs);
            
            if (f->flags & NDRX_VIEW_FLAG_NULLFILLER_P)
            {
                /* test the filler */
                ret=EXTRUE;
                for (i=0; i<f->nullval_bin_len; i++)
                {
                    if (i==f->nullval_bin_len-1 && i<len)
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
                    else if (fld_offs[i]!=f->nullval_bin[i] && 
                            i<len)
                    {
                        ret=EXFALSE;
                        goto out;
                    }
                    else if (i>=len)
                    {
                        ret=EXFALSE;
                        goto out;
                    }
                }
            }
            else
            {
                /* EOS will be set there by calloc... */
                NDRX_LOG(log_dump, "STR_CMP: data: [%s] vs obj: [%s]",
                        fld_offs, f->nullval_bin);
                if (0==strcmp(fld_offs, f->nullval_bin))
                {
                    ret=EXTRUE;
                    goto out;
                }
            }
            
            break;
        case BFLD_CARRAY:
            break;
    }
    
out:
    return ret;
       
}



