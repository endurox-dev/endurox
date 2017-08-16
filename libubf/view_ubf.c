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
#include <ferror.h>
#include <ubfutil.h>

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
            dim_size = f->fldsize/f->count;
            
            if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
            {
                C_count = (short *)(cstruct+f->count_fld_offset);
            }
            else
            {
                C_count = &C_count_stor;
            }
            
            for (occ=0; occ<f->count; occ++)
            {
                fld_offs = cstruct+f->offset+occ*dim_size;
                
                if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
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
    ndrx_typedview_field_t *f;
    UBFH *temp_ub = NULL;
    long bsize = v->ssize*3+1024;
    short *C_count;
    short C_count_stor;
    char *fld_offs;
    unsigned short *L_length; /* will transfer as long */
    unsigned short L_length_stor;
    
    int dim_size;
    
    int *int_fix_ptr;
    long int_fix_l;
    int occ;
    BFLDLEN len;
    
    temp_ub = (UBFH *)NDRX_MALLOC(bsize);

    if (NULL==temp_ub)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to allocate %ld bytes in temporary UBF buffer: %s", 
                bsize, strerror(errno));
        ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate %ld bytes in temporary UBF buffer: %s", 
                bsize, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Binit(temp_ub, bsize))
    {
        NDRX_LOG(log_error, "Failed to init UBF buffer: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* TODO: 
     * - Build new UBF buffer from v. Loop over the v, add data to FB, realloc if needed.
     * - call the BUPDATE, (BOJOIN - RFU), (BJOIN - RFU), BCONCAT
     */
    DL_FOREACH(v->fields, f)
    {
        dim_size = f->fldsize/f->count;
        
        if (f->flags & NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F)
        {
            UBF_LOG(log_debug, "Processing field: [%s] ubf [%s]", 
                    f->cname, f->fbname);

            /* Check do we have length indicator? */
            if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
            {
                C_count = (short *)(cstruct+f->count_fld_offset);

                NDRX_LOG(log_dump, "%s.C_%s=%hd", 
                        v->vname, f->cname, *C_count);
            }
            else
            {
                C_count_stor=f->count; 
                C_count = &C_count_stor;
            }

            for (occ=0; occ<*C_count; occ++)
            {
             
                fld_offs = cstruct+f->offset+occ*dim_size;
                
                if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
                {
                    L_length = (unsigned short *)(cstruct+f->length_fld_offset+
                            occ*sizeof(unsigned short));
                }
                else
                {
                    L_length_stor = dim_size;
                    L_length = &L_length_stor;
                }
                
                len = *L_length;
                
                /* If field is not NULL 
                 * BUPDATE does not make NULL appear in target UBF
                 */
                if (BUPDATE != mode || !ndrx_Bvnull_int(v, f, occ, cstruct))
                {
                    if (BFLD_INT==f->typecode_full)
                    {
                        int_fix_ptr = (int *)fld_offs;
                        int_fix_l = (long)*int_fix_ptr;
    
                        if (EXSUCCEED!=CBchg(temp_ub, f->ubfid, occ, 
                                (char *)&int_fix_l, 0L, BFLD_LONG))
                        {
                            UBF_LOG(log_error, "Failed to add field [%s]/%d as long!", 
                                    f->fbname, f->ubfid);
                            /* error will be set already */
                            EXFAIL_OUT(ret);
                        }
                    }
                    else
                    {
                        if (EXSUCCEED!=CBchg(temp_ub, f->ubfid, occ, 
                                fld_offs, len, f->typecode_full))
                        {
                            UBF_LOG(log_error, "Failed to add field [%s]/%d as long!", 
                                    f->fbname, f->ubfid);
                            
                            /* error will be set already */
                            EXFAIL_OUT(ret);
                        }
                    }
                }
                else
                {
                    /* Not interested in any more occurrences 
                     * cause it will cause NULL occurrances...
                     */
                    break; 
                }
            }
        }
    }
    
    /* Temporary buffer built ok... now merge. */
    
    ndrx_debug_dump_UBF_ubflogger(log_info, "Temporary buffer built", 
            temp_ub);
    
    ndrx_debug_dump_UBF_ubflogger(log_info, "Output buffer before merge",
            p_ub);
    
    switch (mode)
    {
        case BUPDATE:
            UBF_LOG(log_info, "About to run Bupdate");
            if (EXSUCCEED!=Bupdate(p_ub, temp_ub))
            {
                UBF_LOG(log_error, "Failed to Bupdate(): %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            break;
        case BJOIN:
            UBF_LOG(log_info, "About to run Bjoin");
            if (EXSUCCEED!=Bjoin(p_ub, temp_ub))
            {
                UBF_LOG(log_error, "Failed to Bjoin(): %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            break;
        case BOJOIN:
            UBF_LOG(log_info, "About to run Bojoin");
            if (EXSUCCEED!=Bojoin(p_ub, temp_ub))
            {
                UBF_LOG(log_error, "Failed to Bojoin(): %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            break;
        case BCONCAT:
            UBF_LOG(log_info, "About to run Bconcat");
            if (EXSUCCEED!=Bconcat(p_ub, temp_ub))
            {
                UBF_LOG(log_error, "Failed to Bconcat(): %s", Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            break;
        default:
            ndrx_Bset_error_fmt(BEINVAL, "Invalid %s option: %d", __func__, mode);
            EXFAIL_OUT(ret);
            break;
    }
    
    ndrx_debug_dump_UBF_ubflogger(log_info, "Output buffer after merge",
            p_ub);
    
out:
    if (NULL!=temp_ub)
    {
        NDRX_FREE(p_ub);
    }

    return ret;
}

/**
 * Copy data from C struct to UBF
 * @param p_ub ptr to UBF buffer
 * @param cstruct ptr to memroy block
 * @param mode BUPDATE, BOJOIN, BJOIN, BCONCAT
 * @param view view name of th ec struct
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvstof(UBFH *p_ub, char *cstruct, int mode, char *view)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t *v = NULL;
    
    /* Resolve view */
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=ndrx_Bvstof_int(p_ub, v, cstruct, mode))
    {
        UBF_LOG(log_error, "ndrx_Bvftos_int failed");
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

