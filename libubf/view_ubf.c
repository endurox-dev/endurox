/* 
** VIEW UBF operations (UBF->C struct, C struct->UBF)
**
** @file view_ubf.c
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

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>

#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/*
 * TODO: Bvftos, Bvstof
 */

/**
 * Fill the C structure with UBF data
 * @param p_ub ptr to UBF buffer
 * @param v resolved view
 * @param cstruct ptr to memory block where view instance lives
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvftos_int(UBFH *p_ub, ndrx_typedview_t *v, char *cstruct)
{
    int ret = EXSUCCEED;
    int occ;
    int dim_size;
    char *fld_offs;
    BFLDLEN len;
    short *C_count;
    short C_count_stor;
    unsigned short *L_length;
    unsigned short L_length_stor;
    long l;
    

    ndrx_typedview_field_t *f;
    
    UBF_LOG(log_info, "Into %s", __func__);
    
    /* Go over the c struct  */
    DL_FOREACH(v->fields, f)
    {
        if (f->flags & NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S)
        {
            for (occ=0; occ<f->count; occ++)
            {
                fld_offs = cstruct+f->offset+occ*dim_size;
                dim_size = f->fldsize/f->count;
                
                
                if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
                {
                    C_count = (short *)(cstruct+f->count_fld_offset);
                }
                else
                {
                    C_count = &C_count_stor;
                }
                
                if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
                {
                    L_length = (unsigned short *)(cstruct+f->length_fld_offset+
                            occ*sizeof(unsigned short));
                }
                else
                {
                    L_length = &L_length_stor;
                }
                
                *C_count = 0;
                *L_length = 0;
            
                
                len = dim_size;

                if (
                        (
                            BFLD_INT==f->typecode_full && 
                            EXSUCCEED!=CBget(p_ub, f->ubfid, occ, (char *)&l, 0L, BFLD_LONG)
                         )
                         ||

                        (
                            BFLD_INT!=f->typecode_full && 
                            EXSUCCEED!=CBget(p_ub, f->ubfid, occ, fld_offs, &len, f->typecode_full)
                        )
                    )
                {
                    UBF_LOG(log_info, "Failed to get %d field: %s",
                            f->ubfid, Bstrerror(Berror));

                    if (BNOTPRES!=Berror)
                    {
                        UBF_LOG(log_error, "Error getting field: %s", 
                                Bstrerror(Berror));
                        /* error already set */
                        EXFAIL_OUT(ret);

                    }
                    else
                    {
                        /* unset UBF error */
                        ndrx_Bunset_error();

                        /* Setup NULL at given occ*/
                        if (EXSUCCEED!=ndrx_Bvselinit_int(v, f, cstruct))
                        {
                            ndrx_Bset_error_fmt(BBADVIEW, "Failed to set NULL to %s.%s",
                                    v->vname, f->cname);
                            EXFAIL_OUT(ret);
                        }

                    }
                }
                else
                {
                    if (BFLD_INT==f->typecode_full)
                    {
                        int *vi = (int *)fld_offs;
                        *vi = (int)l;
                    }
                    
                    (*(C_count))++;
                    
                    if (BFLD_STRING==f->typecode_full || 
                            BFLD_CARRAY==f->typecode_full)
                    {
                        *L_length = (unsigned short)len;
                    }
                    else
                    {
                        /* not used for others.. */
                        *L_length = 0;
                    }
                }
            } /* for occ */
        } /* if UBF->C flag */
    } /* for fields in v */
    
out:
    return ret;
}

/**
 * Fill the C struct from UBF
 * @param p_ub UBF buffer to fill
 * @param cstruct memptr of the struct
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvftos(UBFH *p_ub, char *cstruct, char *view)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    /* Resolve view */
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_Bvftos_int(p_ub, v, cstruct))
    {
        UBF_LOG(log_error, "ndrx_Bvftos_int failed");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Copy C struct data to UBF buffer
 * @param p_ub UBF buffer
 * @param v resolved view
 * @param cstruct ptr c struct to update
 * @param mode BUPDATE, BOJOIN, BJOIN, BCONCAT
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvstof_int(UBFH *p_ub, ndrx_typedview_t *v, char *cstruct, int mode)
{
    int ret=EXSUCCEED;
    
    /* TODO: */
    
out:
    return ret;
}

