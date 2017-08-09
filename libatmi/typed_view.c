/* 
** VIEW buffer type support
** Basically all data is going to be stored in FB.
** For view not using base as 3000. And the UBF field numbers follows the 
** the field number in the view (1, 2, 3, 4, ...) 
** Sample file:
** 
**# 
**#type    cname   fbname count flag  size  null
**#
**int     in    -   1   -   -   -
**short    sh    -   2   -   -   -
**long     lo    -   3   -   -   -
**char     ch    -   1   -   -   -
**float    fl    -   1   -   -   -
**double    db    -   1   -   -   -
**string    st    -   1   -   15   -
**carray    ca    -   1   -   15   -
**END
** 
**
** @file typed_view.c
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
 * Allocate the view object
 * @param descr
 * @param subtype
 * @param len
 * @return 
 */
expublic char * VIEW_tpalloc (typed_buffer_descr_t *descr, char *subtype, long len)
{
    char *ret=NULL;
    
    ndrx_typedview_t *v = ndrx_view_get_view(subtype);
    
    if (NULL==v)
    {
         NDRX_LOG(log_error, "%s: VIEW [%s] NOT FOUND!", __func__, subtype);
        ndrx_TPset_error_fmt(TPENOENT, "%s: VIEW [%s] NOT FOUND!", 
                __func__, subtype);
        goto out;
    }
    
    if (NDRX_VIEW_SIZE_DEFAULT_SIZE>len)
    {
        len = NDRX_VIEW_SIZE_DEFAULT_SIZE;
    }
    
    /* Allocate VIEW buffer */
    /* Maybe still malloc? */
    ret=(char *)NDRX_CALLOC(1, len);

    if (NULL==ret)
    {
        NDRX_LOG(log_error, "%s: Failed to allocate VIEW buffer!", __func__);
        ndrx_TPset_error_msg(TPEOS, Bstrerror(Berror));
        goto out;
    }
    
    /* Check the size of the view, if buffer is smaller then view
     * the have some working in ULOG and logfile... */
    if (v->ssize < len)
    {
        NDRX_LOG(log_info, "tpalloc'ed %ld bytes, but VIEW [%s] structure size if %ld",
                len, subtype, v->ssize);
    }

out:
    return ret;
}

/**
 * Re-allocate CARRAY buffer. Firstly we will find it in the list.
 * @param ptr
 * @param size
 * @return
 */
expublic char * VIEW_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len)
{
    char *ret=NULL;

    if (0==len)
    {
        len = NDRX_VIEW_SIZE_DEFAULT_SIZE;
    }

    /* Allocate CARRAY buffer */
    ret=(char *)NDRX_REALLOC(cur_ptr, len);
    

    return ret;
}

/**
 * this will add data the the buffer. If data size is too small, allocate extra
 * 1024 bytes and add again.
 * @param pp_ub double ptr to buffer
 * @param bfldid field id
 * @param occ occurrence
 * @param buf buffer
 * @param len len
 * @return EXSUCCED/EXFAIL
 */
exprivate  int sized_Bchg (UBFH **pp_ub, BFLDID bfldid, 
        BFLDOCC occ, char * buf, BFLDLEN len)
{
    int ret = EXSUCCEED;
    

    while (EXSUCCEED!=(ret=Bchg(*pp_ub, bfldid, occ, buf, len))
            &&  BNOSPACE==Berror)
    {
        if (NULL==(*pp_ub = (UBFH *)tprealloc((char *)*pp_ub, Bsizeof(*pp_ub) + 1024)))
        {
            NDRX_LOG(log_error, "Failed to realloc the buffer!");            
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;
}

/**
 * Build the outgoing buffer...
 * @param descr
 * @param idata
 * @param ilen
 * @param obuf
 * @param olen
 * @param flags
 * @return 
 */
expublic int VIEW_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    buffer_obj_t *bo;
    ndrx_typedview_t *v;
    ndrx_typedview_field_t *f;
    long cksum;
    long fldid;
    int i = NDRX_VIEW_UBF_BASE;
    
    /* Indicators.. */
    short *C_count;
    unsigned short *L_length; /* will transfer as long */
    long L_len_long;
    
    int *int_fix_ptr;
    char *ptr;
    long int_fix_l;
    
    int occ;
    
    /* get the view */
    
    bo = ndrx_find_buffer(idata);
 
    if (NULL==bo)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Input buffer not allocated by tpalloc!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Preparing outgoing for VIEW [%s]", bo->sub_type);
    
    v = ndrx_view_get_view(bo->sub_type);
    
    if (NULL==v)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "View not found [%s]!", v->vname);
        EXFAIL_OUT(ret);
    }
   
    /* Build outgoing UBF...
     * We will make chunks by 1024 bytes. Thus if Bchg op fails with nospace
     * error then reallocate and try again. 
     */
   
    p_ub = (UBFH*)tpalloc("UBF", NULL, 1024);
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to allocate UBF buffer!");
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_VIEW_NAME, 0, bo->sub_type, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup EX_VIEW_NAME to [%s]: %s", 
                v->vname, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    cksum = (long)v->cksum;
    
    if (EXSUCCEED!=Bchg(p_ub, EX_VIEW_CKSUM, 0, (char *)&cksum, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup EX_VIEW_CKSUM to [%ld]: %s", 
                cksum, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* Now setup the fields in the buffer according to loaded view object 
     * we will load all fields into the buffer
     */
    i = 0;
    DL_FOREACH(v->fields, f)
    {
        i++;
        
        NDRX_LOG(log_dump, "Processing field: [%s]", f->cname);
        /* Check do we have length indicator? */
        if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
        {
            C_count = (short *)(idata+f->count_fld_offset);
            NDRX_LOG(log_dump, "C_count=%hd", *C_count);
            
            fldid = Bmkfldid(BFLD_SHORT, i);
            
            if (EXSUCCEED!=Bchg(p_ub, fldid, 0, (char *)C_count, 0L))
            {
                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup C_count at field %d: %s", 
                    i, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            i++;
        }
        
        if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
        {
            L_length = (unsigned short *)(idata+f->length_fld_offset);
            L_len_long = (long)L_length;
            NDRX_LOG(log_dump, "L_length=%hu (long: %ld", 
                    *L_length, L_len_long);
            
            fldid = Bmkfldid(BFLD_LONG, i);
            
            if (EXSUCCEED!=Bchg(p_ub, fldid, 0, (char *)&L_len_long, 0L))
            {
                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup L_len_long "
                        "at field %d: %s", 
                    i, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            i++;
        }
        
        fldid = Bmkfldid(f->typecode, i);
        
        
        /* well we must support arrays too...! of any types
         * Arrays we will load into occurrences of the same buffer
         */
        /* loop over the occurrences */
        for (occ=0; occ<*C_count; occ++)
        {
            /* OK we need to understand following:
             * size (data len) of each of the array elements..
             * TODO: the header plotter needs to support occurrances of the
             * lenght elements for the arrays...
             */
            if (BFLD_INT==f->typecode)
            {
                int_fix_ptr = (int *)(idata+f->offset);
                int_fix_l = (long)*int_fix_ptr;

                if (EXSUCCEED!=sized_Bchg(&p_ub, fldid, 0, (char *)&int_fix_l, 0L))
                {
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup field %d", 
                        fldid);
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                ptr = (idata+f->offset);

                if (EXSUCCEED!=sized_Bchg(&p_ub, fldid, 0, ptr, 0L))
                {
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to setup field %d", 
                        fldid);
                    EXFAIL_OUT(ret);
                }
            }
        }
        
    }
    
    
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
}

expublic int VIEW_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags)
{
    int ret = EXSUCCEED;
    
out:
    return ret;
}