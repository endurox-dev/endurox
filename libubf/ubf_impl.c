/* 
** UBF library
** The emulator of UBF library
** Enduro Execution Library
** Internal implementation of the library - no entry error checks.
** Errors are checked on entry pointers only in ubf.c!
**
** @file ubf_impl.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
#include <ubf_impl.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
struct ubf_type_cache
{
    size_t  cache_offset;
};
typedef struct ubf_type_cache ubf_type_cache_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * UBF cache offset table
 */
static ubf_type_cache_t M_ubf_type_cache[] = 
{
    0, /* SHORT */
    OFFSET(UBF_header_t,cache_long_off), /* LONG */
    OFFSET(UBF_header_t,cache_char_off), /* CHAR */
    OFFSET(UBF_header_t,cache_float_off), /* FLOAT */
    OFFSET(UBF_header_t,cache_double_off), /* DOUBLE */
    OFFSET(UBF_header_t,cache_string_off), /* STRING */
    OFFSET(UBF_header_t,cache_carray_off), /* CARRAY */
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Dump the UBF cache
 */
public void ubf_cache_dump(UBFH *p_ub, char *msg)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    UBF_LOG(log_debug, "%s: ubf cache short, 0: %d", msg, 0);
    UBF_LOG(log_debug, "%s: ubf cache long, 1: %d", msg, hdr->cache_long_off);
    UBF_LOG(log_debug, "%s: ubf cache char, 2: %d", msg, hdr->cache_char_off);
    UBF_LOG(log_debug, "%s: ubf cache float, 3: %d", msg, hdr->cache_float_off);
    UBF_LOG(log_debug, "%s: ubf cache double, 4: %d", msg, hdr->cache_double_off);
    UBF_LOG(log_debug, "%s: ubf cache string, 5: %d", msg, hdr->cache_string_off);
    UBF_LOG(log_debug, "%s: ubf cache carray, 6: %d", msg, hdr->cache_carray_off);
}
/**
 * Update the cache (usable after merge)...
 */
public int ubf_cache_update(UBFH *p_ub)
{
    int type;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    BFLDID   *p_bfldid_start = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    dtype_str_t *dtype=NULL;
    char *fn = "ubf_cache_update";
    int typenext;
    int step;
    int ret = SUCCEED;
    int i;
    
    for (i=1; i<N_DIM(M_ubf_type_cache); i++)
    {
        BFLDLEN *offset = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[i].cache_offset);
        *offset = 0;
    }

    
    while (BBADFLDID!=*p_bfldid)
    {
        /* Got to next position */
        /* Get type */
        type = (*p_bfldid>>EFFECTIVE_BITS);

        /* Check data type alignity */
        if (IS_TYPE_INVALID(type))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Invalid field type (%d)", fn, *p_bfldid);
            FAIL_OUT(ret);
        }
        
        /* Get type descriptor */
        dtype = &G_dtype_str_map[type];
        step = dtype->p_next(dtype, p, NULL);
        p+=step;
        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Pointing to non UBF area: %p",
                                        fn, p);
            FAIL_OUT(ret);
        }
        p_bfldid = (BFLDID *)p;
        
        typenext = (*p_bfldid>>EFFECTIVE_BITS);
        
        if (type!=typenext && typenext > BFLD_SHORT)
        {
            /* Update the cache */
            BFLDLEN *offset = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[typenext].cache_offset);
            *offset = (((char *)p_bfldid) - ((char *)p_bfldid_start));
            
            ubf_cache_dump(p_ub, "Cache refresh");
        }
    }
    
out:
    return;
}

/**
 * Update fielded buffer cache according to 
 * @param p_ub
 * @param fldid
 * @param size_diff
 */
public void ubf_cache_shift(UBFH *p_ub, BFLDID fldid, int size_diff)
{
    UBF_header_t *uh = (UBF_header_t *)p_ub; 
    int off = 0;
    int * shift_list[7][7] = 
    {
        {&uh->cache_long_off, &uh->cache_char_off, &uh->cache_float_off, &uh->cache_double_off, &uh->cache_string_off, &uh->cache_carray_off, NULL}, /* BFLD_SHORT */
        {&uh->cache_char_off, &uh->cache_float_off, &uh->cache_double_off, &uh->cache_string_off, &uh->cache_carray_off, NULL}, /* BFLD_LONG */
        {&uh->cache_float_off, &uh->cache_double_off, &uh->cache_string_off, &uh->cache_carray_off, NULL, NULL}, /* BFLD_CHAR */
        {&uh->cache_double_off, &uh->cache_string_off, &uh->cache_carray_off, NULL, NULL, NULL}, /* BFLD_FLOAT */
        {&uh->cache_string_off, &uh->cache_carray_off, NULL, NULL, NULL}, /* BFLD_DOUBLE */
        {&uh->cache_carray_off, NULL, NULL, NULL, NULL}, /* BFLD_STRING */
        {NULL, NULL, NULL, NULL, NULL, NULL}  /* BFLD_CARRAY */    
    };
   
   int typ = Bfldtype(fldid);
   
   while (NULL!=shift_list[typ][off])
   {
       BFLDLEN *p_cache = shift_list[typ][off];
       *p_cache = (*p_cache) +  size_diff;
       off++;
   }
   
   return;
}

/**
 * Returns pointer to Fb of for specified field
 * @param p_ub
 * @param bfldid
 * @param last_matched - last matched field (can be used together with last_occ),
 *                       It is optional (pas NULL if not needed).
 * @param occ - occurrence to get. If less than -1, then get out the count
 * @param last_occ last check occurrence
 * @return - ptr to field.
 */
public char * get_fld_loc(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype,
                            char ** last_checked,
                            char **last_matched,
                            int *last_occ,
                            get_fld_loc_info_t *last_start)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    dtype_str_t *dtype=NULL;
    int iocc=FAIL;
    int type = (bfldid>>EFFECTIVE_BITS);
    int step;
    char * ret=NULL;
    *fld_dtype=NULL;
    int stat = SUCCEED;
    char fn[] = "get_fld_loc";
    

    *last_occ = FAIL;
    /*
     * Roll the field till the
     */
    if (NULL!=last_start)
    {
        p_bfldid = (BFLDID *)last_start->last_checked;
        p = (char *)last_start->last_checked;
    }
    else if (type > BFLD_SHORT) /* Short is first, thus no need to cache the type */
    {
        /* start from the typed offset (type cache) */
        BFLDLEN *to_add = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[type].cache_offset);
        p_bfldid= (BFLDID *)(((char *)p_bfldid) + *to_add);
        p = (char *)p_bfldid;
    }
    
    if (bfldid == *p_bfldid)
    {
        iocc++;
        /* Save last matched position */
        if (NULL!=last_matched)
            *last_matched = p;
    }

    while (BBADFLDID!=*p_bfldid &&
            ( (bfldid != *p_bfldid) || (bfldid == *p_bfldid && (iocc<occ || occ<-1))) &&
            bfldid >= *p_bfldid)
    {
        /*
         * Update the start of the new field
         * This keeps us in track what was last normal field check.
         * This is useful for Update function. As we know buffers are in osrted
         * order and any new fields that are find by Bnext in case of update
         * should be appended starting for current position in buffer.
         */
        if (NULL!=last_start && *last_start->last_checked!=*p_bfldid)
        {
            last_start->last_checked = p_bfldid;
        }

        /* Got to next position */
        /* Get type */
        type = (*p_bfldid>>EFFECTIVE_BITS);

        /* Check data type alignity */
        if (IS_TYPE_INVALID(type))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Found invalid data type in buffer %d", 
                                        fn, type);
            stat=FAIL;
            goto out;
        }

        /* Get type descriptor */
        dtype = &G_dtype_str_map[type];
        step = dtype->p_next(dtype, p, NULL);
        p+=step;
        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p",
                                        fn, p);
            stat=FAIL;
            goto out;
        }
        p_bfldid = (BFLDID *)p;

        if (bfldid == *p_bfldid)
        {
            iocc++;
            /* Save last matched position */
            if (NULL!=last_matched)
                *last_matched = p;
        }
    }

    /*
     * Check that we found correct field!?!
     * if not then responding with NULL!
     */
    if (BBADFLDID!=*p_bfldid && bfldid ==*p_bfldid && iocc==occ)
    {
        type = (*p_bfldid>>EFFECTIVE_BITS);
        /* Check data type alignity */
        if (IS_TYPE_INVALID(type))
        {
            
            _Fset_error_fmt(BALIGNERR, "Found invalid data type in buffer %d", type);
            stat=FAIL;
            goto out;
        }
        else
        {
            dtype = &G_dtype_str_map[type];
            *fld_dtype=dtype;
            ret=(char *)p_bfldid;
        }
    }

    *last_occ = iocc;
    /* set up last checked, it could be even next element! */
    *last_checked=(char *)p_bfldid;
    
    
    NDRX_LOG(log_debug, "*last_checked [%d] %p", **last_checked, *last_checked);
    
out:
    return ret;
}

/**
 * Checks the buffer to so do we have enought place for new data
 * @param p_ub bisubf buffer
 * @param add_size data to be added
 * @return
 */
public bool have_buffer_size(UBFH *p_ub, int add_size, bool set_err)
{
    bool ret=FALSE;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int buf_free = hdr->buf_len - hdr->bytes_used;

    /* Keep last ID as BBADFLDID (already included in size) */
    if ( buf_free < add_size)
    {
        if (set_err)
            _Fset_error_fmt(BNOSPACE, "Buffsize free [%d] new data size [%d]",
                    buf_free, add_size);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
    return ret;
}

/**
 * Validates paramters entered into function. If not valid, then error will be set
 * @param p_ub
 * @return
 */
public int validate_entry(UBFH *p_ub, BFLDID bfldid, int occ, int mode)
{
    int ret=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *) p_ub;
    BFLDID *last;
    char *p;
    if (NULL==p_ub)
    {
        /* Null buffer */
        _Fset_error_msg(BNOTFLD, "ptr to UBFH is NULL");
        FAIL_OUT(ret);
    }
    else if (0!=strncmp(hdr->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        _Fset_error_msg(BNOTFLD, "Invalid FB magic");
        FAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && BBADFLDID==bfldid)
    {
        /* Invalid arguments? */
        _Fset_error_msg(BBADFLD, "bfldid == BBADFLDID");
        FAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && IS_TYPE_INVALID(bfldid>>EFFECTIVE_BITS))
    {   /* Invalid field id */
        _Fset_error_msg(BBADFLD, "Invalid bfldid (type not correct)");
        FAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && occ < -1)
    {
        _Fset_error_msg(BEINVAL, "occ < -1");
        FAIL_OUT(ret);
    }
    /* Validate the buffer. Last 4 bytes must be empty! */
    /* Get the end of the buffer */
    p = (char *)p_ub;
    p+=hdr->bytes_used;
    p-= sizeof(BFLDID);

    last=(BFLDID *)(p);
    if (*last!=BBADFLDID)
    {
        _Fset_error_fmt(BALIGNERR, "last %d bytes of buffer not equal to "
                                    "%p (got %p)",
                                    sizeof(BFLDID), BBADFLDID, *last);
        FAIL_OUT(ret);
    }
   
out:
    return ret;
}

/**
 * Add value to FB...
 *
 * If adding the first value, the buffer should be large enought and last
 * BFLDID should stay at BADFLID, because will not be overwritten.
 * Also last entry always must at BBADFLDID! This is the rule.
 */
public int _Badd (UBFH *p_ub, BFLDID bfldid, 
                    char *buf, BFLDLEN len,
                    get_fld_loc_info_t *last_start)
{
    int ret=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    char *last;
    int move_size;
    int actual_data_size;
    int type = (bfldid>>EFFECTIVE_BITS);
    char fn[] = "_Badd";
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int __dbg_type;
    int __dbg_vallen;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
    int __dump_size;
    dtype_str_t *__dbg_dtype;
    dtype_ext1_t *__dbg_dtype_ext1;
    
    __p_ub_copy = malloc(hdr->buf_len);
    memcpy(__p_ub_copy, p_ub, hdr->buf_len);
    __dbg_type = (bfldid>>EFFECTIVE_BITS);
    __dbg_dtype = &G_dtype_str_map[__dbg_type];
    __dbg_dtype_ext1 = &G_dtype_ext1_map[__dbg_type];
    __dbg_vallen = __dbg_dtype->p_get_data_size(__dbg_dtype, buf, len,
                                                &actual_data_size);
    UBF_LOG(log_debug, "Badd: entry, adding\nfld=[%d/%p] "
                                    "spec len=%d type[%hd/%s] datalen=%d\n"
                                    "FBbuflen=%d FBused=%d FBfree=%d "
                                    "FBstart fld=[%s/%d/%p] ",
                                    bfldid, bfldid, len,
                                    __dbg_type, __dbg_dtype->fldname,
                                    __dbg_vallen,
                                    hdr->buf_len, hdr->bytes_used,
                                    (hdr->buf_len - hdr->bytes_used),
                                    _Bfname_int(bfldid), bfldid, bfldid);
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "Adding data", buf, &len);
    UBF_DUMP(log_always, "_Badd data to buffer:", buf, actual_data_size);
#endif
/*******************************************************************************/

    UBF_LOG(log_debug, "Badd: bfldid: %x", bfldid);
    
    int ntype = (bfldid>>EFFECTIVE_BITS);
    dtype_str_t *ndtype = &G_dtype_str_map[ntype];
    /* Move memory around (i.e. prepare free space to put data in) */
    int new_dat_size=ndtype->p_get_data_size(ndtype, buf, len, &actual_data_size);

    /* Check required buffer size */
    if (!have_buffer_size(p_ub, new_dat_size, TRUE))
    {
        UBF_LOG(log_warn, "Badd failed - out of buffer memory!");
        FAIL_OUT(ret);
    }

    /* Allow to continue - better performance for concat */
    if (NULL!=last_start)
    {
        p_bfldid = last_start->last_checked;
        p = (char *)last_start->last_checked;
    }
    else if (type > BFLD_SHORT)
    {
        BFLDLEN *to_add = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[type].cache_offset);
        p_bfldid= (BFLDID *)(((char *)p_bfldid) + *to_add);
        p = (char *)p_bfldid;
    }

    /* Seek position where we should insert the data... */
    while (BBADFLDID!=*p_bfldid && bfldid >= *p_bfldid)
    {
        /*
         * Save the point from which we can continue (suitable for Bconcat)
         */
        if (NULL!=last_start && *last_start->last_checked!=*p_bfldid)
        {
            last_start->last_checked = p_bfldid;
        }

        /* Got to next position */
        /* Get type */
        int type = (*p_bfldid>>EFFECTIVE_BITS);
        if (IS_TYPE_INVALID(type))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Unknown data type referenced %d",
                                        fn, type);
            FAIL_OUT(ret);
        }
        /* Get type descriptor */
        dtype_str_t *dtype = &G_dtype_str_map[type];
        int step = dtype->p_next(dtype, p, NULL);

        /* Move to next... */
        p+=step;
        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p",
                                        fn, p);
            FAIL_OUT(ret);
        }
        p_bfldid = (BFLDID *)p;
    }

    if (BBADFLDID==*p_bfldid)
    {
        /* Copy data here! */
        if (SUCCEED!=ndtype->p_put_data(ndtype, p, bfldid, buf, len))
        {
            FAIL_OUT(ret);
        }
        
        hdr->bytes_used+=new_dat_size;
        
        /* Update type offset cache: */
        ubf_cache_shift(p_ub, bfldid, new_dat_size);
        
    }
    else
    {
        last = (char *)hdr;
        last+=(hdr->bytes_used-1);

        /* Get the size to be moved */
        move_size = (last-p+1);
        /* Get some free space here!
         * So from last element we take off current position,
         * so we get lenght. to which we should move.
         */
        memmove(p+new_dat_size, p, move_size);
        /* Put the data in! */
        if (SUCCEED!=ndtype->p_put_data(ndtype, p, bfldid, buf, len))
        {
            FAIL_OUT(ret);
        }
        /* Update the pointer of last bit! */
        hdr->bytes_used+=new_dat_size;
        
        /* Update type offset cache: */
        ubf_cache_shift(p_ub, bfldid, new_dat_size);
        
    }
out:
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = (__p_ub_copy->buf_len - __p_ub_copy->bytes_used);
    __dbg_newuse = (hdr->buf_len - hdr->bytes_used);

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "Badd: returns=%d\norg_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                ret,
                                __dbg_olduse,
                                __dbg_newuse,
                                (__dbg_olduse - __dbg_newuse),
                                __p_ub_copy->bfldid, __p_ub_copy->bfldid,
                                hdr->bfldid, hdr->bfldid,
                                *__dbg_fldptr_org, *__dbg_fldptr_org,
                                *__dbg_fldptr_new, *__dbg_fldptr_new);
    /* Check the last four bytes before the end */
    __dbg_p_org-= sizeof(BFLDID);
    __dbg_p_new-= sizeof(BFLDID);
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "Badd: last %d bytes of data\n org=%p new %p",
                          sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After Badd", __p_ub_copy, p_ub, hdr->buf_len);
    __dump_size=hdr->bytes_used;
    UBF_DUMP(log_always, "Used buffer dump after: ",p_ub, __dump_size);
    free(__p_ub_copy);
#endif
/*******************************************************************************/
    return ret;
}

/**
 * Change field content...
 * @param
 * @param
 * @param
 * @param
 * @param
 * @return
 */
public int _Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len,
                            get_fld_loc_info_t *last_start)
{
    int ret=SUCCEED;

    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    dtype_str_t *dtype;
    char *p;
    int last_occ=-1;
    dtype_ext1_t *ext1_map;
    int i;
    char *last;
    int move_size;
    char *last_checked=NULL;
    int elem_empty_size;
    int target_elem_size;
    int actual_data_size;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int __dbg_type;
    int __dbg_vallen;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
    dtype_str_t *__dbg_dtype;
    dtype_ext1_t *__dbg_dtype_ext1;
    int __dump_size;
#endif
/*******************************************************************************/
    /* Call other functions on standard criteria */
    if (occ == -1)
    {
        UBF_LOG(log_debug, "Bchg: calling Badd, because occ == -1!");
        return Badd(p_ub, bfldid, buf, len); /* <<<< RETURN HERE! */
    }
    else if (NULL==buf)
    {
        UBF_LOG(log_debug, "Bchg: calling Bdel, because buf == NULL!");
        return Bdel(p_ub, bfldid, occ); /* <<<< RETURN HERE! */
    }
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = malloc(hdr->buf_len);
    memcpy(__p_ub_copy, p_ub, hdr->buf_len);
    __dbg_type = (bfldid>>EFFECTIVE_BITS);
    __dbg_dtype = &G_dtype_str_map[__dbg_type];
    __dbg_dtype_ext1 = &G_dtype_ext1_map[__dbg_type];
    __dbg_vallen = __dbg_dtype->p_get_data_size(__dbg_dtype, buf, len,
                                                &actual_data_size);
    UBF_LOG(log_debug, "Bchg: entry, adding\nfld=[%d/%p] occ=[%d] "
                                    "spec len=%d type[%hd/%s] datalen=%d\n"
                                    "FBbuflen=%d FBused=%d FBfree=%d "
                                    "FBstart fld=[%s/%d/%p] ",
                                    bfldid, bfldid, occ, len,
                                    __dbg_type, __dbg_dtype->fldname,
                                    __dbg_vallen,
                                    hdr->buf_len, hdr->bytes_used,
                                    (hdr->buf_len - hdr->bytes_used),
                                    _Bfname_int(bfldid), bfldid, bfldid);
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "Bchg data", buf, &len);
    UBF_DUMP(log_always, "Bchg data to buffer:", buf, actual_data_size);
#endif
/*******************************************************************************/

    if (NULL!=(p=get_fld_loc(p_ub, bfldid, occ, &dtype, 
                                &last_checked, NULL, &last_occ, last_start)))
    {
       /* Play slightly differently here - get the existing data size */
        int existing_size;
        int must_have_size;
        UBF_LOG(log_debug, "Bchg: Field present, checking buff sizes");

        existing_size = dtype->p_next(dtype, p, NULL);
        target_elem_size = dtype->p_get_data_size(dtype, buf, len, &actual_data_size);

        /* Which may happen for badly formatted data! */
        if (FAIL==target_elem_size)
        {
            _Fset_error_msg(BEINVAL, "Failed to get data size - corrupted data?");
            FAIL_OUT(ret);
        }

        /* how much we are going to add */
        must_have_size = target_elem_size - existing_size;
        if ( must_have_size>0 && !have_buffer_size(p_ub, must_have_size, TRUE))
        {
            FAIL_OUT(ret);
        }

        if (must_have_size!=0)
        { 
            int real_move = must_have_size;
            if (real_move < 0 )
                real_move = -real_move;
            /* Free up space in memory if required (i.e. do the move) */
            last = (char *)hdr;
            last+=(hdr->bytes_used-1);
            move_size = (last-(p+existing_size)+1);

            UBF_LOG(log_debug, "Bchg: memmove: %d bytes "
                            "from addr %p to addr %p", real_move,
                             p+existing_size, p+existing_size+must_have_size);

            /* Free up, or make more memory to be used! */
            memmove(p+existing_size + must_have_size, p+existing_size, move_size); 
            hdr->bytes_used+=must_have_size;
           
            /* Update type offset cache: */
            ubf_cache_shift(p_ub, bfldid, must_have_size);
            
            /* Reset last bytes to 0 */
            if (must_have_size < 0)
            {
                /* Reset trailing stuff to 0 - this should be tested! */
                memset(p+existing_size + must_have_size+move_size, 0, real_move);
            }
        }

        /* Put the actual data there, buffer sizes already resized above */
        if (SUCCEED!=dtype->p_put_data(dtype, p, bfldid, buf, len))
        {
            _Fset_error_msg(BEINVAL, "Failed to put data into FB - corrupted data?");
            FAIL_OUT(ret);
        }
    }
    else
    {
        /* We know something about last field?  */
        int missing_occ;
        int must_have_size;
        int empty_elem_tot_size;
        p = last_checked;
        p_bfldid = (BFLDID *)last_checked;
        int type;

        UBF_LOG(log_debug, "Bchg: Field not present. last_occ=%d",
                last_occ);
        /*
         * Read data type again, because we do not have it in case if it is not
         * exact match!
         */
        type = (bfldid>>EFFECTIVE_BITS);
        dtype = &G_dtype_str_map[type];

        ext1_map = &G_dtype_ext1_map[dtype->fld_type];

        /* 1. Have to calculate new size, if elem was not preset at all (last_occ = -1),
         * then for example occ=0, last_occ = -1, which makes:
         * 0 - (-1) - 1 = 0, meaning that there is nothing missing.
         */
        missing_occ = occ - last_occ - 1; /* -1 cos end elem is ours */
        UBF_LOG(log_debug, "Missing empty positions = %d", missing_occ);
        elem_empty_size = ext1_map->p_empty_sz(ext1_map);
        empty_elem_tot_size = missing_occ * ext1_map->p_empty_sz(ext1_map);

        target_elem_size = dtype->p_get_data_size(dtype, buf, len, &actual_data_size);


        if (FAIL==target_elem_size)
        {
            _Fset_error_msg(BEINVAL, "Failed to get data size - corrupted data?");
            FAIL_OUT(ret);
        }

        must_have_size=empty_elem_tot_size+target_elem_size;
        UBF_LOG(log_debug, "About to add data %d bytes",
                                        must_have_size);
        
        if (!have_buffer_size(p_ub, must_have_size, TRUE))
        {
            FAIL_OUT(ret);
        }
        
        /* Free up space in memory if required (i.e. do the move) */
        last = (char *)hdr;
        last+=(hdr->bytes_used-1);
        /* Get the size to be moved */
        move_size = (last-p+1); /* <<< p is incorrect here! */
        /* Get some free space here!
         * So from last element we take off current position,
         * so we get lenght. to which we should move.
         */
        if (move_size > 0)
        {
            UBF_LOG(log_debug, "Bchg: memmove: %d bytes "
                            "from addr %p to addr %p", move_size,
                            p, p+must_have_size);
            memmove(p+must_have_size, p, move_size);
        }

        /* We have space, so now produce empty nodes */
        for (i=0; i<missing_occ; i++)
        {
            ext1_map->p_put_empty(ext1_map, p, bfldid);
            p+=elem_empty_size;
        }
        /* Now load the data by itself - do not check the
         * result it should work out OK.
         */
        if (SUCCEED!=dtype->p_put_data(dtype, p, bfldid, buf, len))
        {
            /* We have failed! */
            _Fset_error_msg(BEINVAL, "Failed to put data into FB - corrupted data?");
            FAIL_OUT(ret);
        }       
        /* Finally increase the buffer usage! */
        hdr->bytes_used+=must_have_size;
        
        /* Update type offset cache: */
        ubf_cache_shift(p_ub, bfldid, must_have_size);
        
    }
    
out:
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = (__p_ub_copy->buf_len - __p_ub_copy->bytes_used);
    __dbg_newuse = (hdr->buf_len - hdr->bytes_used);

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "Bchg: returns=%d\norg_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                ret,
                                __dbg_olduse,
                                __dbg_newuse,
                                (__dbg_olduse - __dbg_newuse),
                                __p_ub_copy->bfldid, __p_ub_copy->bfldid,
                                hdr->bfldid, hdr->bfldid,
                                *__dbg_fldptr_org, *__dbg_fldptr_org,
                                *__dbg_fldptr_new, *__dbg_fldptr_new);
    /* Check the last four bytes before the end */
    __dbg_p_org-= sizeof(BFLDID);
    __dbg_p_new-= sizeof(BFLDID);
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "Bchg: last %d bytes of data\n org=%p new %p",
                          sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After Bchg diff: ", __p_ub_copy, p_ub, hdr->buf_len);

    __dump_size=hdr->bytes_used;
    UBF_DUMP(log_always, "Used buffer dump after: ",p_ub, __dump_size);

    free(__p_ub_copy);
#endif
/*******************************************************************************/

    return ret;
}

/**
 * Get the total occurrance count.
 * Internal version - no error checking.
 * @param p_ub
 * @param bfldid
 * @return
 */
public BFLDOCC _Boccur (UBFH * p_ub, BFLDID bfldid)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int ret=SUCCEED;

    UBF_LOG(log_debug, "_Boccur: bfldid: %d", bfldid);

    /* using -2 for looping throught te all occurrances! */
    get_fld_loc(p_ub, bfldid, -2,
                            &fld_dtype,
                            (char **)&p_last,
                            NULL,
                            &ret,
                            NULL);
    if (FAIL==ret)
    {
        /* field not found! */
        ret=0;
    }
    else
    {
        /* found (but zero based) so have to increment! */
        ret+=1;
    }

    UBF_LOG(log_debug, "_Boccur: return %d", ret);

    return ret;
}

/**
 * Check the field presence
 * Internal version - no error checking.
 */
public int _Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int last_occ;
    int ret=TRUE;

    UBF_LOG(log_debug, "_Bpres: bfldid: %d occ: %d", bfldid, occ);
    

    if (NULL!=get_fld_loc(p_ub, bfldid, occ,
                            &fld_dtype,
                            (char **)&p_last,
                            NULL,
                            &last_occ,
                            NULL))
    {
        ret=TRUE;
    }
    else
    {
        ret=FALSE;
    }


    UBF_LOG(log_debug, "_Boccur: return %d", ret);

    return ret;
}

/**
 * Get the next occurrance. This will iterate over the all FB. Inside it it uses
 * static pointer & counter for fields. So function should not be used for two
 * searches in the same time.
 *
 * Search must start with BFIRSTFLDID
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @param len
 * @param d_ptr - pointer to start of the data (result is similar of Bfind result)
 * @return 0 - not found/ 1 - entry found.
 */
public int _Bnext(Bnext_state_t *state, UBFH *p_ub, BFLDID *bfldid,
                                BFLDOCC *occ, char *buf, BFLDLEN *len,
                                char **d_ptr)
{
    int found=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID prev_fld;
    int step;
    int type;
    dtype_str_t *dtype;
    char *p;
    char fn[] = "_Bnext";
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif

    if (*bfldid == BFIRSTFLDID)
    {
        state->p_cur_bfldid = &hdr->bfldid;
        state->cur_occ = 0;
        state->p_ub = p_ub;
        state->size = hdr->bytes_used;
        p = (char *)&hdr->bfldid;
    }
    else
    {
        /* Get current field type */
        prev_fld = *state->p_cur_bfldid;
        /* Get data type */
        type=*state->p_cur_bfldid>>EFFECTIVE_BITS;

        /* Align error */
        if (IS_TYPE_INVALID(type))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Invalid data type: %d", type, fn);
            found=FAIL;
            goto out;
        }

        dtype=&G_dtype_str_map[type];
        p=(char *)state->p_cur_bfldid;
        /* Get step to next */
        step = dtype->p_next(dtype, p, NULL);
        p+=step;


        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            _Fset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p", fn, p);
            found=FAIL;
            goto out;
        }
        
        /* Move to next */
        state->p_cur_bfldid = (BFLDID *)p;
        if (prev_fld==*state->p_cur_bfldid)
        {
            state->cur_occ++;
        }
        else
        {
            state->cur_occ=0;
        }
    }
    
    /* return the results */
    if (BBADFLDID!=*state->p_cur_bfldid)
    {
        /* Return the value if needed */
        *bfldid = *state->p_cur_bfldid;
        *occ = state->cur_occ;
        UBF_LOG(log_debug, "%s: Found field %p occ %d",
                                            fn, *bfldid, *occ);

        found = 1;
        /* Return the value */
        type=*state->p_cur_bfldid>>EFFECTIVE_BITS;

        if (IS_TYPE_INVALID(type))
        {
            _Fset_error_fmt(BALIGNERR, "Invalid data type: %d", type);
            found=FAIL;
            goto out;
        }
        dtype=&G_dtype_str_map[type];
        /*
         * Return the pointer to start of the field.
         */
        if (NULL!=d_ptr)
        {
            int dlen;
            dtype_ext1_t *dtype_ext1;
            /* Return the pointer to the data */
            *d_ptr = p;
            dtype_ext1 = &G_dtype_ext1_map[type];
            dlen = dtype_ext1->hdr_size;
            *d_ptr=p+dlen;

            /* Now return the len if needed */
            if (NULL!=len)
            {
               /* *len = data_len;*/
               dtype->p_next(dtype, (char *)p, len);
            }
        }

        if (NULL!=buf)
        {
            if (SUCCEED!=dtype->p_get_data(dtype, (char *)p, buf, len))
            {
                found=FAIL;
                goto out;
            }
#ifdef UBF_API_DEBUG
            else
            {
                /* Dump found value */
                __dbg_dtype_ext1 = &G_dtype_ext1_map[type];
                __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Bnext got data",
                                                buf, len);
            }
#endif
        }
        else
        {
            UBF_LOG(log_warn, "%s: Buffer null - not returning value", fn);
        }
    }
    else
    {
        UBF_LOG(log_debug, "%s: Reached End Of Buffer", fn);
        /* do not return anything */
        found = 0; /* End Of Buffer */
    }
    
out:
    return found;
}

/**
 * Internal version of data type conversation.
 * This allocates output buffer.
 * @param to_len
 * @param to_type
 * @param from_buf
 * @param from_type
 * @param from_len - THIS MUST BE SET! Provided for user by API function,
 *                      but internally for performance reasons, this is not processed
 *                      here.
 * @return NULL on failure/ptr to allocted memory if OK.
 */
public char * _Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len)
{
    char *alloc_buf=NULL;
    BFLDLEN  cvn_len=0;
    char *ret=NULL;
    char fn[]="_Btypcvt";

/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif
/*******************************************************************************/

    UBF_LOG(log_debug, "%s: entered, from %d to %d", fn,
                                        from_type, to_type);

    /* Allocate the buffer dynamically */
    if (NULL==(ret=get_cbuf(from_type, to_type, NULL, from_buf, from_len, &alloc_buf,
                                &cvn_len, CB_MODE_ALLOC, 0)))
    {
        /* error should be already set */
        UBF_LOG(log_error, "%s: Malloc failed!", fn);
        goto out;
    }
    
    /* Run the conversation */
    if (NULL==ubf_convert(from_type, CNV_DIR_OUT, from_buf, from_len,
                        to_type, ret, &cvn_len))
    {
        /* if fails, error should be already set! */
        /* remove allocated memory */
        free(alloc_buf);
        alloc_buf=NULL;
        ret=NULL;
        goto out;
    }

    /* return output len if requested */
    if (NULL!=to_len)
        *to_len=cvn_len;

out:
    UBF_LOG(log_debug, "%s: return %p", fn, ret);

/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (NULL!=ret)
    {
        __dbg_dtype_ext1 = &G_dtype_ext1_map[to_type];
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Btypcvt got data", ret,
                                                                           to_len);
    }
    #endif
/*******************************************************************************/

    return ret;
}

/**
 * Back-end for Blen
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return 
 */
public int _Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int ret=SUCCEED;
    char *p;

    UBF_LOG(log_debug, "_Blen: bfldid: %d, occ: %d", bfldid, occ);

    /* using -2 for looping throught te all occurrances! */
    p=get_fld_loc(p_ub, bfldid, occ,
                            &fld_dtype,
                            (char **)&p_last,
                            NULL,
                            &ret,
                            NULL);
    if (FAIL!=ret && NULL!=p)
    {
        
        fld_dtype->p_next(fld_dtype, p, &ret);
    }
    else
    {
        /* Field not found */
        _Fset_error(BNOTPRES);
        ret=FAIL;
    }

    UBF_LOG(log_debug, "_Boccur: return %d", ret);

    return ret;
}
