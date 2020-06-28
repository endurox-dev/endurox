/**
 * @brief UBF library
 *   The emulator of UBF library
 *   Enduro Execution Library
 *   Internal implementation of the library - no entry error checks.
 *   Errors are checked on entry pointers only in ubf.c!
 *   TODO: Add checks for user passed bfldid type. If bigger than BFLD_CARRAY, then reject with BBADFLD.
 *
 * @file ubf_impl.c
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
    EXOFFSET(UBF_header_t,cache_long_off), /* LONG */
    EXOFFSET(UBF_header_t,cache_char_off), /* CHAR */
    EXOFFSET(UBF_header_t,cache_float_off), /* FLOAT */
    EXOFFSET(UBF_header_t,cache_double_off), /* DOUBLE */
    EXOFFSET(UBF_header_t,cache_string_off), /* STRING */
    EXOFFSET(UBF_header_t,cache_carray_off), /* CARRAY */
};

/*---------------------------Prototypes---------------------------------*/
exprivate inline void ubf_cache_set(UBFH *p_ub, BFLDID fldid, int next_offset);

/**
 * Dump the UBF cache
 */
expublic void ubf_cache_dump(UBFH *p_ub, char *msg)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    UBF_LOG(log_debug, "%s: ubf cache short, 0: %d", msg, 0);
    UBF_LOG(log_debug, "%s: ubf cache long, 1: %d", msg, hdr->cache_long_off);
    UBF_LOG(log_debug, "%s: ubf cache char, 2: %d", msg, hdr->cache_char_off);
    UBF_LOG(log_debug, "%s: ubf cache float, 3: %d", msg, hdr->cache_float_off);
    UBF_LOG(log_debug, "%s: ubf cache double, 4: %d", msg, hdr->cache_double_off);
    UBF_LOG(log_debug, "%s: ubf cache string, 5: %d", msg, hdr->cache_string_off);
    UBF_LOG(log_debug, "%s: ubf cache carray, 6: %d", msg, hdr->cache_carray_off);
    
    UBF_LOG(log_debug, "%s: ubf cache ptr, 6: %d", msg, hdr->cache_ptr_off);
    UBF_LOG(log_debug, "%s: ubf cache ubf, 6: %d", msg, hdr->cache_view_off);
    UBF_LOG(log_debug, "%s: ubf cache view, 6: %d", msg, hdr->cache_ubf_off);
}
/**
 * Update the cache (usable after merge)...
 */
expublic int ubf_cache_update(UBFH *p_ub)
{
    int type;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    BFLDID   *p_bfldid_start = &hdr->bfldid;
    BFLDID   *p_cur;
    char *p = (char *)&hdr->bfldid;
    dtype_str_t *dtype=NULL;
    char *fn = "ubf_cache_update";
    int typenext;
    int step;
    int ret = EXSUCCEED;
    int i;
    
    /* reset cache... */
    for (i=1; i<N_DIM(M_ubf_type_cache); i++)
    {
        BFLDLEN *offset = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[i].cache_offset);
        *offset = 0;
    }

#ifdef BIN_SEARCH_DEBUG
    UBF_LOG(log_debug, "%s: About to update ubf bin-search cache", fn);
#endif
    
    while (!UBF_EOF(hdr, p_bfldid))
    {
        /* Got to next position */
        /* Get type */
        type = (*p_bfldid>>EFFECTIVE_BITS);
        
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "%s: Got field: [%d], type %d", fn, *p_bfldid, type);
#endif

        /* Check data type alignity */
        if (IS_TYPE_INVALID(type))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Invalid field type (%d)", fn, *p_bfldid);
            EXFAIL_OUT(ret);
        }
        
        p_cur = p_bfldid;
        
        /* Get type descriptor */
        dtype = &G_dtype_str_map[type];
        step = dtype->p_next(dtype, p, NULL);
        p+=step;
        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to non UBF area: %p",
                                        fn, p);
            EXFAIL_OUT(ret);
        }
        p_bfldid = (BFLDID *)p;
        
        if (UBF_EOF(hdr, p_bfldid))
        {
            typenext=EXFAIL;
        }
        else
        {
            typenext = (*p_bfldid>>EFFECTIVE_BITS);
        }
        
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "%s: Next field: [%d], type %d", fn, *p_bfldid, typenext);
#endif
        
        if (type!=typenext)
        {
            /* Update the cache */
            int offset = (((char *)p_bfldid) - ((char *)p_bfldid_start));
            ubf_cache_set(p_ub, *p_cur, offset);
        }
        else
        {
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: Not updating type=%d, typenext=%d", fn, type, typenext);
#endif
        }
    }
    
out:
    return ret;
}

/**
 * Set cache absolute values
 * @param p_ub UBF buffer
 * @param fldid last field ID
 * @param next_offset offset of the end of the fldid
 */
exprivate inline void ubf_cache_set(UBFH *p_ub, BFLDID fldid, int next_offset)
{
    UBF_header_t *uh = (UBF_header_t *)p_ub;
    char *fn = "ubf_cache_shift";
    int type = (fldid>>EFFECTIVE_BITS);
    
    switch (type)
    {
        case BFLD_SHORT:
            uh->cache_long_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_SHORT, uh->cache_long_off => %d", 
                    fn, uh->cache_long_off);
#endif
        case BFLD_LONG:
            uh->cache_char_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_LONG, uh->cache_char_off=> %d", 
                    fn, uh->cache_char_off);
#endif
        case BFLD_CHAR:
            uh->cache_float_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_CHAR, uh->cache_float_off=> %d", 
                    fn, uh->cache_float_off);
#endif
        case BFLD_FLOAT:
            uh->cache_double_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_FLOAT, uh->cache_double_off=> %d", 
                    fn, uh->cache_double_off);
#endif
        case BFLD_DOUBLE:
            uh->cache_string_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_DOUBLE, uh->cache_string_off=> %d", 
                    fn, uh->cache_string_off);
#endif
        case BFLD_STRING:
            uh->cache_carray_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_STRING, uh->cache_carray_off=> %d", 
                    fn, uh->cache_carray_off);
#endif      
        case BFLD_CARRAY:
            uh->cache_ptr_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_CARRAY, uh->cache_ptr_off=> %d", 
                    fn, uh->cache_ptr_off);
#endif      
        case BFLD_PTR:
            uh->cache_ubf_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_PTR, uh->cache_ubf_off=> %d", 
                    fn, uh->cache_ubf_off);
#endif      
        case BFLD_UBF:
            uh->cache_view_off=next_offset;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_UBF, uh->cache_view_off=> %d", 
                    fn, uh->cache_view_off);
#endif
            break;
            
    }
}
/**
 * Update fielded buffer cache according to 
 * @param p_ub
 * @param fldid
 * @param size_diff
 */
expublic inline void ubf_cache_shift(UBFH *p_ub, BFLDID fldid, int size_diff)
{
    UBF_header_t *uh = (UBF_header_t *)p_ub;
    char *fn = "ubf_cache_shift";
    int type = (fldid>>EFFECTIVE_BITS);
    
    switch (type)
    {
        case BFLD_SHORT:
            uh->cache_long_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_SHORT, uh->cache_long_off+=%d => %d", 
                    fn, size_diff, uh->cache_long_off);
#endif
        case BFLD_LONG:
            uh->cache_char_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_LONG, uh->cache_char_off+=%d => %d", 
                    fn, size_diff, uh->cache_char_off);
#endif
        case BFLD_CHAR:
            uh->cache_float_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_CHAR, uh->cache_float_off+=%d => %d", 
                    fn, size_diff, uh->cache_float_off);
#endif
        case BFLD_FLOAT:
            uh->cache_double_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_FLOAT, uh->cache_double_off+=%d => %d", 
                    fn, size_diff, uh->cache_double_off);
#endif
        case BFLD_DOUBLE:
            uh->cache_string_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_DOUBLE, uh->cache_string_off+=%d => %d", 
                    fn, size_diff, uh->cache_string_off);
#endif
        case BFLD_STRING:
            uh->cache_carray_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_STRING, uh->cache_carray_off+=%d => %d", 
                    fn, size_diff, uh->cache_carray_off);
#endif      
        case BFLD_CARRAY:
            uh->cache_ptr_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_CARRAY, uh->cache_ptr_off+=%d => %d", 
                    fn, size_diff, uh->cache_ptr_off);
#endif
        case BFLD_PTR:
            uh->cache_ubf_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_PTR, uh->cache_ubf_off+=%d => %d", 
                    fn, size_diff, uh->cache_ubf_off);
#endif
        case BFLD_UBF:
            uh->cache_view_off+=size_diff;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "%s: BFLD_UBF, uh->cache_view_off+=%d => %d", 
                    fn, size_diff, uh->cache_view_off);
#endif
            break;
    }
   return;
}

/**
 * Get field ID at index
 * @param start
 * @param i
 * @param step
 * @return 
 */
exprivate inline BFLDID get_fldid_at_idx(char *start, int i, int step)
{
    BFLDID fld = *((BFLDID   *)(start + i*step));
    
    return fld;
}

/**
 * return the occurrence of current field.
 * @param start
 * @param f
 * @param i
 * @param step
 * @return G
 */
exprivate inline int get_fld_occ_from_idx(char *start, BFLDID f, int i, int step)
{
    char *cur = start + i*step;
    BFLDID cur_fld = *((BFLDID   *)cur);
    int occ = -1;
    
    while (cur_fld == f && cur >=start)
    {
        occ++;
        i--;
        cur = start + i*step;
        
        if (cur>=start)
        {
            cur_fld = *((BFLDID   *)cur);
        }
    }
    
    return occ;
    
}

/**
 * Get the ptr to field (or NULL) if given occurrence is not found
 * @param start
 * @param stop
 * @param i
 * @param step
 * @param req_occ
 * @return 
 */
exprivate inline char * get_field(char *start, char *stop, BFLDID f, int i, int step, 
        int req_occ, int get_last, int *last_occ, char ** last_match, char ** last_checked)
{
    char *tmp;
    char *cur;
    BFLDID   *f1;
    
    int iocc = get_fld_occ_from_idx(start, f, i, step);
    
    if (get_last & UBF_BINSRCH_GET_LAST)
    {
get_last:
        /* operate in last off mode */
        tmp = cur = start + step * i;
        
        /* try to search for last one.... */
        while (cur < stop)
        {
            tmp = start + step * (i+1);
            f1 = (BFLDID *)tmp;
            if (tmp >= stop)
            {
                break;
            }
            else if (*f1 > f)
            {
                break;
            }
            else if (*f1==f)
            {
                i++;
                iocc++;
                cur = tmp;
            }
        }
        if (NULL!=last_occ)
        {
            /* hm, not sure is it ok? maybe just i? but maint get_loc works as this */
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "*last_occ = %d", *last_occ);
#endif
            *last_occ = iocc;
        }

        /* for change, we set to next field actually */
        if (get_last & UBF_BINSRCH_GET_LAST_CHG)
        {
            cur = tmp;
        }
        
        if (NULL!=last_match)
        {
            *last_match = cur;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "*last_match = %p", last_match);
#endif
        }

        if (NULL!=last_checked)
        {
            *last_checked = cur;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "*last_match = %p", last_match);
#endif
        }
        
        return NULL;
    }
    else if (req_occ<=iocc)
    {
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "req_occ<=iocc -> %d<=%d", req_occ, iocc);
#endif
        return start + step*(i-(iocc-req_occ)); /* step back positions needed */
    }
    else
    {
        char *cur = start + step * (i + req_occ-iocc);
        BFLDID cur_fld; 
        
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "req_occ %d/ iocc %d", req_occ, iocc);
#endif
        
        if (cur >= stop)
        {
            /* so for bchg we need here to locate last valid occ.... */
            if (get_last & UBF_BINSRCH_GET_LAST_CHG)
            {
#ifdef BIN_SEARCH_DEBUG
                UBF_LOG(log_debug, "going to get_last 1");
#endif
                goto get_last;
            }
            return NULL; /* field not found (out of bounds) */
        }
        
        cur_fld = *((BFLDID   *)cur);
        
        if (cur_fld==f)
        {
            return cur;
        }
        
        /* For change mode, roll to last field */
        if (get_last & UBF_BINSRCH_GET_LAST_CHG)
        {
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "going to get_last 2");
#endif
            goto get_last;
        }
        
    }
    
    return NULL;
    
}

/**
 * Get the field by performing binary search. Must be called for proper field types (
 * i.e. non string & non carray)
 * @param p_ub
 * @param bfldid
 * @param last_matched - last matched field (can be used together with last_occ),
 *                       It is optional (pas NULL if not needed).
 * @param last_checked  - if not NULL, then function will try find the last occ 
 *      of the type (even not matched).
 *      In case of EOF/not found, this might point to address outside 
 *      the bytes_used scope!!!
 * @param occ - occurrence to get. If less than -1, then get out the count
 * @param last_occ last check occurrence
 * @return - ptr to field.
 */
expublic char * get_fld_loc_binary_search(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype, int get_last, 
                            int *last_occ, char ** last_checked, char ** last_match)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid_start = &hdr->bfldid;
    BFLDID   *p_bfldid_stop  = &hdr->bfldid;
    BFLDID   *curf;
    BFLDLEN tmp = 0;
    BFLDLEN *to_add1 = &tmp; /* default for short */
    BFLDLEN *to_add2;
    char *start = (char *)&hdr->bfldid;
    char *stop;
    dtype_str_t *dtype=NULL;
    int type = (bfldid>>EFFECTIVE_BITS);
    int step;
    int fld_got;
    char *tmp1;
    char *cur;
    char * ret=NULL;
    int did_search = EXFALSE;
    int first, last, middle, last_middle;
    int was_found_fldid = EXFALSE;
    char fn[] = "get_fld_loc_binary_search";
    
    if (type > BFLD_SHORT) /* Short is first, thus no need to cache the type */
    {
        /* start from the typed offset (type cache) */
        to_add1 = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[type].cache_offset);
        p_bfldid_start= (BFLDID *)(((char *)p_bfldid_start) + *to_add1);
        start = (char *)p_bfldid_start;
    }
    else if (type > BFLD_CARRAY)
    {
	    
    }
    
    /* stop will be bigger than start.... anyway (take the next field cache offset) */
    /* start from the typed offset (type cache) */
    to_add2 = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[type+1].cache_offset);
    p_bfldid_stop= (BFLDID *)(((char *)p_bfldid_stop) + *to_add2);
    stop = (char *)p_bfldid_stop;
    
#ifdef BIN_SEARCH_DEBUG
    /* if type is present then there must be difference between start & stop */
    UBF_LOG(log_error, "start = %p stop = %p diff = %d (off 1 %d (%d) off 2 %d (%d))", 
        start, stop, stop-start,
        *to_add1,
        type,
        *to_add2,
        type+1
    );
    
    ubf_cache_dump(p_ub, "Offsets...");
#endif
    
    if (stop-start <=0)
    {
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_warn, "Field not found stop-start < 0!");
#endif
        if (NULL!=last_checked )
        {
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "Last search = start");
#endif
            *last_checked =start;
        }
        goto out;
    }
    
    /* get the step of the field... (size of block) */
    dtype = &G_dtype_str_map[type];
    *fld_dtype=dtype;
    step = dtype->p_next(dtype, start, NULL);
    
    first = 0;
    last = (stop-start) / step - 1;
    
#ifdef BIN_SEARCH_DEBUG
    UBF_LOG(log_error, "start %p stop %p, last=%d", start, stop, last);
#endif
    
    middle = (first+last)/2;
    
    if (first<=last)
    {
#ifdef BIN_SEARCH_DEBUG
        int jj;
        
        UBF_LOG(log_debug, "---- Dumping the search area ----")
        for (jj=first; jj<=last; jj++)
        {
            cur = start + step * (jj);
            curf = (BFLDID*)cur;
                        
            UBF_LOG(log_debug, "Area: %p Field %x (%d)", 
                cur, *curf, *curf);
        }
        UBF_LOG(log_debug, "---------------------------------")
#endif
        did_search = EXTRUE;
    }
    
    while (first <= last)
    {
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "Looking for middle: %d (first: %d, last: %d)", 
                middle, first, last);
#endif
        last_middle = middle;
        fld_got = get_fldid_at_idx(start, middle, step);
        
#ifdef BIN_SEARCH_DEBUG
        UBF_LOG(log_debug, "Got field %x (%d) search %x (%d)", 
                fld_got, fld_got, bfldid, bfldid);
#endif
        
        if ( fld_got < bfldid)
        {
           first = middle + 1;    
        }
        else if (fld_got == bfldid)
        {
            was_found_fldid = EXTRUE;
            ret=get_field(start, stop, bfldid, middle, step, occ, get_last, 
                   last_occ, last_match, last_checked);

           break;
        }
        else
        {
           last = middle - 1;
        }

        middle = (first + last)/2;
    }
    
    /* Not found, my provide some support for add 
     * Search for the end of the last matched occurrence, i.e. we provide the address
     * of the next. If it is end of the buffer, it must be 4x bytes of zero...
     * even if we matched, we must get the last field.
     */
    if (NULL!=last_checked && !was_found_fldid)
    {
        if (did_search)
        {
            if (NULL==ret)
            {
                char *last_ok;
                last_ok = cur = start + step * last_middle;
                
                if (fld_got < bfldid)
                {
                    /* Look forward */
                    curf = (BFLDID*)cur;
                    /* try to search for last one.... */
                    while (cur < stop && *curf < bfldid)
                    {
                        /* last_ok = cur; */
                        last_middle++;
                        cur = start + step * (last_middle);
                        curf = (BFLDID*)cur;
#ifdef BIN_SEARCH_DEBUG
                        UBF_LOG(log_debug, "Stepping forward %p", cur);
#endif
                    }
                }
                else
                {
                    /* Look back - we must stay at field that is first bigger than
                     * ours field...
                     */
                    curf = (BFLDID*)cur;
                    /* try to search for last one.... */
                    while (cur > start && *curf > bfldid)
                    {   
                        last_ok = cur;
                        last_middle--;
                        cur = start + step * (last_middle);
                        curf = (BFLDID*)cur;
#ifdef BIN_SEARCH_DEBUG
                        UBF_LOG(log_debug, "Stepping back %p", cur);
#endif
                    }
                    
                    cur = last_ok;
                    
                }
                
                *last_checked = cur;
#ifdef BIN_SEARCH_DEBUG
                UBF_LOG(log_debug, "*last_checked = %p", *last_checked);
#endif
            }
            else
            {   
                *last_checked = ret;
#ifdef BIN_SEARCH_DEBUG
                UBF_LOG(log_debug, "*last_checked = %p", *last_checked);
#endif
            }
        }
        else
        {
            *last_checked = start;
#ifdef BIN_SEARCH_DEBUG
            UBF_LOG(log_debug, "*last_checked = %p", *last_checked);
#endif
        }
    }
    
out:
            
#ifdef BIN_SEARCH_DEBUG
    UBF_LOG(log_debug, "%s ret %p", fn, ret);
#endif

    return ret;
}


/**
 * Returns pointer to Fb of for specified field
 * @param p_ub
 * @param bfldid
 * @param last_matched - last matched field (can be used together with last_occ),
 *                       It is optional (pas NULL if not needed).
 * @param occ - occurrence to get. If less than -1, then get out the count
 * @param last_occ last check occurrence
 * @param last_checked - BE WARNED! In case of not found, this will point to hdr+bytes_used 
 * (to non existing field address)!
 * @return - ptr to field.
 */
expublic char * get_fld_loc(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype,
                            char ** last_checked,
                            char **last_matched,
                            int *last_occ,
                            Bfld_loc_info_t *last_start)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    dtype_str_t *dtype=NULL;
    int iocc=EXFAIL;
    int type = (bfldid>>EFFECTIVE_BITS);
    int step;
    char * ret=NULL;
    *fld_dtype=NULL;
    int stat = EXSUCCEED;
    char fn[] = "get_fld_loc";
    int eof;

    *last_occ = EXFAIL;
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
    
    /* ok we match first... then step to next. */
    /* test only if we are not at the end... */
    if (!UBF_EOF(hdr, p_bfldid) && bfldid == *p_bfldid)
    {
        iocc++;
        
        /* Save last matched position */
        if (NULL!=last_matched)
            *last_matched = p;
    }

    while ( !(eof=UBF_EOF(hdr, p_bfldid))  &&
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
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Found invalid data type in buffer %d", 
                                        fn, type);
            stat=EXFAIL;
            goto out;
        }

        /* Get type descriptor */
        dtype = &G_dtype_str_map[type];
        step = dtype->p_next(dtype, p, NULL);
        p+=step;
        
        /* Align error */
#if 0
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p "
                    "(offset: %ld) field: %d, len: %d, overrun: %d", 
                    fn, p, (long)(p-((char *)hdr)),
                    *p_bfldid, step, (p-((char *)hdr)) - hdr->bytes_used );
            stat=EXFAIL;
            goto out;
        }
#endif

        if (EXTRUE==(eof=UBF_EOF(hdr, p)))
        {
            break;
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
    if (!eof && bfldid ==*p_bfldid && iocc==occ)
    {
        type = (*p_bfldid>>EFFECTIVE_BITS);
        /* Check data type alignity */
        if (IS_TYPE_INVALID(type))
        {
            
            ndrx_Bset_error_fmt(BALIGNERR, "Found invalid data type in buffer %d", type);
            stat=EXFAIL;
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
    
    if (!eof)
    {
        *last_checked=(char *)p_bfldid;
    }
    else
    {
        /* Last check will go over the boundaries! */
        *last_checked=(char *)hdr + hdr->bytes_used;
    }
    
    
    UBF_LOG(log_debug, "*last_checked %p, last_occ=%d", *last_checked, *last_occ);
    
out:
    return ret;
}

/**
 * Checks the buffer to so do we have enought place for new data
 * @param p_ub bisubf buffer
 * @param add_size data to be added
 * @return
 */
expublic int have_buffer_size(UBFH *p_ub, int add_size, int set_err)
{
    int ret=EXFALSE;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int buf_free = hdr->buf_len - hdr->bytes_used;

    /* Keep last ID as BBADFLDID (already included in size) */
    if ( buf_free < add_size)
    {
        if (set_err)
            ndrx_Bset_error_fmt(BNOSPACE, "Buffsize free [%d] new data size [%d]",
                    buf_free, add_size);
        return EXFALSE;
    }
    else
    {
        return EXTRUE;
    }
/*
    return ret;
*/
}

/**
 * Validates parameters entered into function. If not valid, then error will be set
 * @param p_ub
 * @return
 */
expublic inline int validate_entry(UBFH *p_ub, BFLDID bfldid, int occ, int mode)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *) p_ub;
    BFLDID *last;
    char *p;
    if (NULL==p_ub)
    {
        /* Null buffer */
        ndrx_Bset_error_msg(BNOTFLD, "ptr to UBFH is NULL");
        EXFAIL_OUT(ret);
    }
    else if (0!=strncmp(hdr->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "Invalid FB magic");
        EXFAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && BBADFLDID==bfldid)
    {
        /* Invalid arguments? */
        ndrx_Bset_error_msg(BBADFLD, "bfldid == BBADFLDID");
        EXFAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && IS_TYPE_INVALID(bfldid>>EFFECTIVE_BITS))
    {   /* Invalid field id */
        ndrx_Bset_error_fmt(BBADFLD, "Invalid bfldid (type %d not correct)",
                ((int)bfldid>>EFFECTIVE_BITS));
        EXFAIL_OUT(ret);
    }
    else if (!(mode & VALIDATE_MODE_NO_FLD) && occ < -1)
    {
        ndrx_Bset_error_msg(BEINVAL, "occ < -1");
        EXFAIL_OUT(ret);
    }
#if 0
    /* Validate the buffer. Last 4 bytes must be empty! */
    /* Get the end of the buffer */
    p = (char *)p_ub;
    p+=hdr->bytes_used;
    p-= FF_USED_BYTES;

    last=(BFLDID *)(p);
    if (*last!=BBADFLDID)
    {
        ndrx_Bset_error_fmt(BALIGNERR, "last %d bytes of buffer not equal to "
                                    "%p (got %p)",
                                    FF_USED_BYTES, BBADFLDID, *last);
        EXFAIL_OUT(ret);
    }
#endif
   
out:
    return ret;
}

/**
 * Add value to FB...
 * TODO: Move to binary search.
 * If adding the first value, the buffer should be large enought and last
 * BFLDID should stay at BADFLID, because will not be overwritten.
 * Also last entry always must at BBADFLDID! This is the rule.
 * @param next_fld ptr to next field after this is added
 */
expublic int ndrx_Badd (UBFH *p_ub, BFLDID bfldid, 
                    char *buf, BFLDLEN len,
                    Bfld_loc_info_t *last_start, Bfld_loc_info_t *next_fld)
{
    int ret=EXSUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    BFLDID   *p_bfldid = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    char *last;
    int move_size;
    int actual_data_size;
    int type = (bfldid>>EFFECTIVE_BITS);
    char fn[] = "_Badd";
    int ntype;
    dtype_str_t *ndtype;
    int new_dat_size;
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
    
    __p_ub_copy = NDRX_MALLOC(hdr->buf_len);
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
                                    ndrx_Bfname_int(bfldid), bfldid, bfldid);
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "Adding data", buf, &len);
    UBF_DUMP(log_always, "_Badd data to buffer:", buf, actual_data_size);
#endif
/*******************************************************************************/

    UBF_LOG(log_debug, "Badd: bfldid: %d", bfldid);
    
    ntype = (bfldid>>EFFECTIVE_BITS);
    ndtype = &G_dtype_str_map[ntype];
    /* Move memory around (i.e. prepare free space to put data in) */
    new_dat_size=ndtype->p_get_data_size(ndtype, buf, len, &actual_data_size);

    /* Check required buffer size */
    if (!have_buffer_size(p_ub, new_dat_size, EXTRUE))
    {
        UBF_LOG(log_warn, "Badd failed - out of buffer memory!");
        EXFAIL_OUT(ret);
    }

    if (NULL!=next_fld && NULL!=next_fld->last_checked)
    {
	p_bfldid = next_fld->last_checked;
	p = (char *)next_fld->last_checked;
    }
    /* Allow to continue - better performance for concat */
    else if (NULL!=last_start)
    {
        p_bfldid = last_start->last_checked;
        p = (char *)last_start->last_checked;
    }
    else if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        dtype_str_t *tmp;
        get_fld_loc_binary_search(p_ub, bfldid, EXFAIL,
                            &tmp, UBF_BINSRCH_GET_LAST_CHG, 
                            NULL, &p, NULL);
        p_bfldid= (BFLDID *)p;
    }
    else
    {
        BFLDLEN *to_add = (BFLDLEN *)(((char *)hdr) + M_ubf_type_cache[type].cache_offset);
        p_bfldid= (BFLDID *)(((char *)p_bfldid) + *to_add);
        p = (char *)p_bfldid;
    }

    /* Seek position where we should insert the data... */
    while (!UBF_EOF(hdr, p_bfldid) && bfldid >= *p_bfldid)
    {
        dtype_str_t *dtype;
        int step;
        int type;
	
        /*
         * Save the point from which we can continue (suitable for Bconcat)
         */
        if (NULL!=last_start && *last_start->last_checked!=*p_bfldid)
        {
            last_start->last_checked = p_bfldid;
        }

        /* Got to next position */
        /* Get type */
        type = (*p_bfldid>>EFFECTIVE_BITS);
        if (IS_TYPE_INVALID(type))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Unknown data type referenced %d",
                                        fn, type);
            EXFAIL_OUT(ret);
        }
        /* Get type descriptor */
        dtype = &G_dtype_str_map[type];
        step = dtype->p_next(dtype, p, NULL);

        /* Move to next... */
        p+=step;
        /* Align error */
        if (CHECK_ALIGN(p, p_ub, hdr))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p "
                    "(offset: %ld)", fn, p, (long)(p-((char *)hdr)));
            EXFAIL_OUT(ret);
        }
        p_bfldid = (BFLDID *)p;
    }

    if (UBF_EOF(hdr, p_bfldid))
    {
        /* Copy data here! */
        if (EXSUCCEED!=ndtype->p_put_data(ndtype, p, bfldid, buf, len))
        {
            EXFAIL_OUT(ret);
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
         * so we get length. to which we should move.
         */
        memmove(p+new_dat_size, p, move_size);
        /* Put the data in! */
        if (EXSUCCEED!=ndtype->p_put_data(ndtype, p, bfldid, buf, len))
        {
            EXFAIL_OUT(ret);
        }
        /* Update the pointer of last bit! */
        hdr->bytes_used+=new_dat_size;
        
        /* Update type offset cache: */
        ubf_cache_shift(p_ub, bfldid, new_dat_size);
        
    }
    
    if (NULL!=next_fld)
    {
        /* so mark as next by data size */
        next_fld->last_checked = (BFLDID *)(p + new_dat_size);
    }
    
out:
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = (__p_ub_copy->buf_len - __p_ub_copy->bytes_used);
    __dbg_newuse = (hdr->buf_len - hdr->bytes_used);

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - FF_USED_BYTES);

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - FF_USED_BYTES);

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
    __dbg_p_org-= FF_USED_BYTES;
    __dbg_p_new-= FF_USED_BYTES;
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "Badd: last %d bytes of data\n org=%p new %p",
                          FF_USED_BYTES, *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After Badd", __p_ub_copy, p_ub, hdr->buf_len);
    __dump_size=hdr->bytes_used;
    UBF_DUMP(log_always, "Used buffer dump after: ",p_ub, __dump_size);
    NDRX_FREE(__p_ub_copy);
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
 * @param upd_only - when is set to EXTRUE update only existing fields 
 * and not add new fields in p_ub, otherwise update existing fields and 
 * add new fields when they not exist in p_fb
 * @return
 */
expublic int ndrx_Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len,
                            Bfld_loc_info_t *last_start, int upd_only)
{
    int ret=EXSUCCEED;

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
    __p_ub_copy = NDRX_MALLOC(hdr->buf_len);
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
                                    ndrx_Bfname_int(bfldid), bfldid, bfldid);
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "Bchg data", buf, &len);
    UBF_DUMP(log_always, "Bchg data to buffer:", buf, actual_data_size);
#endif
/*******************************************************************************/
    UBF_LOG(log_debug, "%s: changing %d occ=%d", __func__, bfldid, occ);
    
    if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        p = get_fld_loc_binary_search(p_ub, bfldid, occ, &dtype,
                            UBF_BINSRCH_GET_LAST_CHG, &last_occ, &last_checked, NULL);
    }
    else
    {
       p=get_fld_loc(p_ub, bfldid, occ, &dtype, 
                                &last_checked, NULL, &last_occ, last_start);
    }
    
    if (NULL!=p)
    {
       /* Play slightly differently here - get the existing data size */
        int existing_size;
        int must_have_size;
        UBF_LOG(log_debug, "Bchg: Field present, checking buff sizes");

        existing_size = dtype->p_next(dtype, p, NULL);
        target_elem_size = dtype->p_get_data_size(dtype, buf, len, &actual_data_size);

        /* Which may happen for badly formatted data! */
        if (EXFAIL==target_elem_size)
        {
            ndrx_Bset_error_msg(BEINVAL, "Failed to get data size - corrupted data?");
            EXFAIL_OUT(ret);
        }

        /* how much we are going to add */
        must_have_size = target_elem_size - existing_size;
        if ( must_have_size>0 && !have_buffer_size(p_ub, must_have_size, EXTRUE))
        {
            EXFAIL_OUT(ret);
        }

        if (must_have_size!=0)
        { 
            BFLDLEN debug_before;
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
            
            debug_before = hdr->bytes_used;
            
            hdr->bytes_used+=must_have_size;
            
            UBF_LOG(log_debug, "%s: bytes_used: %d (+%d)-> %d",
                __func__, debug_before, must_have_size, hdr->bytes_used);
           
            /* Update type offset cache: */
            ubf_cache_shift(p_ub, bfldid, must_have_size);
            
            /* Reset last bytes to 0 */
            if (must_have_size < 0)
            {
                /* Reset trailing stuff to 0 - this should be tested! */
                /*TODO: Opt ???
                memset(p+existing_size + must_have_size+move_size, 0, real_move);
                 * */
            }
        }

        /* Put the actual data there, buffer sizes already resized above */
        if (EXSUCCEED!=dtype->p_put_data(dtype, p, bfldid, buf, len))
        {
            ndrx_Bset_error_msg(BEINVAL, "Failed to put data into FB - corrupted data?");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        if (EXTRUE == upd_only)
        {
            UBF_LOG(log_debug, "%s: Update only!! Field %d not found occ=%d",
                    __func__, bfldid, occ);
            goto out;
        }
        /* We know something about last field?  */
        int missing_occ;
        int must_have_size;
        int empty_elem_tot_size;
        BFLDLEN debug_before;
        
#if 0
        -- this is always set, even to invalid addr.
        if (NULL==last_checked)
        {
            /* so element was not found, calculate "virtual address" */
            last_checked = ((char *)hdr) + hdr->bytes_used;
        }
#endif
        
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
        UBF_LOG(log_debug, "Missing empty positions: %d (occ: %d, last_occ: %d)", 
                missing_occ, occ, last_occ);
        
        elem_empty_size = ext1_map->p_empty_sz(ext1_map);
        empty_elem_tot_size = missing_occ * ext1_map->p_empty_sz(ext1_map);

        target_elem_size = dtype->p_get_data_size(dtype, buf, len, &actual_data_size);


        if (EXFAIL==target_elem_size)
        {
            ndrx_Bset_error_msg(BEINVAL, "Failed to get data size - corrupted data?");
            EXFAIL_OUT(ret);
        }

        must_have_size=empty_elem_tot_size+target_elem_size;
        UBF_LOG(log_debug, "About to add data %d bytes (total used: %d, total: %d), "
                "target_elem_size: %d, empty_elem_tot_size: %d, missing_occ: %d",
                must_have_size, hdr->bytes_used, hdr->buf_len,
                target_elem_size, empty_elem_tot_size, missing_occ);
        
        if (!have_buffer_size(p_ub, must_have_size, EXTRUE))
        {
            EXFAIL_OUT(ret);
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
        if (EXSUCCEED!=dtype->p_put_data(dtype, p, bfldid, buf, len))
        {
            /* We have failed! */
            ndrx_Bset_error_msg(BEINVAL, "Failed to put data into FB - corrupted data?");
            EXFAIL_OUT(ret);
        }       
        debug_before = hdr->bytes_used;
        /* Finally increase the buffer usage! */
        hdr->bytes_used+=must_have_size;
        
        UBF_LOG(log_debug, "%s: bytes_used: %d (+%d)-> %d",
            __func__, debug_before, must_have_size, hdr->bytes_used);
        
        
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
    __dbg_p_org+= (__p_ub_copy->bytes_used - FF_USED_BYTES);

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - FF_USED_BYTES);

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
    __dbg_p_org-= FF_USED_BYTES;
    __dbg_p_new-= FF_USED_BYTES;
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "Bchg: last %d bytes of data\n org=%p new %p",
                          FF_USED_BYTES, *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After Bchg diff: ", __p_ub_copy, p_ub, hdr->buf_len);

    __dump_size=hdr->bytes_used;
    UBF_DUMP(log_always, "Used buffer dump after: ",p_ub, __dump_size);

    NDRX_FREE(__p_ub_copy);
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
expublic BFLDOCC ndrx_Boccur (UBFH * p_ub, BFLDID bfldid)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int ret=EXFAIL;

    UBF_LOG(log_debug, "_Boccur: bfldid: %d", bfldid);

    /* using -2 for looping throught te all occurrances! */
    if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        get_fld_loc_binary_search(p_ub, bfldid, EXFAIL, &fld_dtype, 
                    UBF_BINSRCH_GET_LAST, &ret, NULL, NULL);
    }
    else
    {
        get_fld_loc(p_ub, bfldid, -2,
                                &fld_dtype,
                                (char **)&p_last,
                                NULL,
                                &ret,
                                NULL);
    }
    if (EXFAIL==ret)
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
expublic int _Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int last_occ;
    int ret=EXTRUE;
    char *ret_ptr;

    UBF_LOG(log_debug, "_Bpres: bfldid: %d occ: %d", bfldid, occ);
    

    if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        ret_ptr = get_fld_loc_binary_search(p_ub, bfldid, occ, &fld_dtype, 
                UBF_BINSRCH_GET_LAST_NONE, NULL, NULL, NULL);
    }
    else
    {
        ret_ptr = get_fld_loc(p_ub, bfldid, occ,
                                &fld_dtype,
                                (char **)&p_last,
                                NULL,
                                &last_occ,
                                NULL);
    }
    
    if (NULL!=ret_ptr)
    {
        ret=EXTRUE;
    }
    else
    {
        ret=EXFALSE;
    }


    UBF_LOG(log_debug, "_Boccur: return %d", ret);

    return ret;
}

/**
 * Get the next occurrence. This will iterate over the all FB. Inside it uses
 * static pointer & counter for fields.
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
expublic int ndrx_Bnext(Bnext_state_t *state, UBFH *p_ub, BFLDID *bfldid,
                                BFLDOCC *occ, char *buf, BFLDLEN *len,
                                char **d_ptr)
{
    int found=EXSUCCEED;
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
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Invalid data type: %d", type, fn);
            found=EXFAIL;
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
            ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p", fn, p);
            found=EXFAIL;
            goto out;
        }
        
        /* Move to next */
        state->p_cur_bfldid = (BFLDID *)p;
        if (!UBF_EOF(hdr, state->p_cur_bfldid) && prev_fld==*state->p_cur_bfldid)
        {
            state->cur_occ++;
        }
        else
        {
            state->cur_occ=0;
        }
    }
    
    /* return the results */
    if (!UBF_EOF(hdr, state->p_cur_bfldid))
    {
        /* Return the value if needed */
        *bfldid = *state->p_cur_bfldid;
        *occ = state->cur_occ;
        UBF_LOG(log_debug, "%s: Found field buf=%p fldid=%d occ %d",
                                            fn, p_ub, *bfldid, *occ);

        found = 1;
        /* Return the value */
        type=*state->p_cur_bfldid>>EFFECTIVE_BITS;

        if (IS_TYPE_INVALID(type))
        {
            ndrx_Bset_error_fmt(BALIGNERR, "Invalid data type: %d", type);
            found=EXFAIL;
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
            dtype_ext1 = &G_dtype_ext1_map[type];
            dlen = dtype_ext1->hdr_size;
            
            if (NULL!=dtype_ext1->p_prep_ubfp)
            {
                *d_ptr=dtype_ext1->p_prep_ubfp(dtype_ext1, &state->vstorage, p);
            }
            else
            {
                *d_ptr=p+dlen;
            }
        }

        if (NULL!=buf)
        {
            if (EXSUCCEED!=dtype->p_get_data(dtype, (char *)p, buf, len))
            {
                found=EXFAIL;
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
        else if (NULL!=len)
        {
            /* return length only... */
            dtype->p_next(dtype, p, len);
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
expublic char * ndrx_Btypcvt (BFLDLEN * to_len, int to_type,
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
    if (NULL==(ret=ndrx_ubf_get_cbuf(from_type, to_type, NULL, from_buf, from_len, &alloc_buf,
                                &cvn_len, CB_MODE_ALLOC, 0)))
    {
        /* error should be already set */
        UBF_LOG(log_error, "%s: Malloc failed!", fn);
        goto out;
    }
    
    /* Run the conversation */
    if (NULL==ndrx_ubf_convert(from_type, CNV_DIR_OUT, from_buf, from_len,
                        to_type, ret, &cvn_len))
    {
        /* if fails, error should be already set! */
        /* remove allocated memory */
        NDRX_FREE(alloc_buf);
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
expublic int ndrx_Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    dtype_str_t *fld_dtype;
    BFLDID *p_last=NULL;
    int ret=EXSUCCEED;
    char *p;

    UBF_LOG(log_debug, "_Blen: bfldid: %d, occ: %d", bfldid, occ);

    /* using -2 for looping through all occurrences! */
    
    
    if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        p=get_fld_loc_binary_search(p_ub, bfldid, occ, &fld_dtype, 
                    UBF_BINSRCH_GET_LAST_NONE, NULL, NULL, NULL);
    }
    else
    {
        p=get_fld_loc(p_ub, bfldid, occ,
                                &fld_dtype,
                                (char **)&p_last,
                                NULL,
                                &ret,
                                NULL);
    }
    
    if (EXFAIL!=ret && NULL!=p)
    {
        
        fld_dtype->p_next(fld_dtype, p, &ret);
    }
    else
    {
        /* Field not found */
        ndrx_Bset_error(BNOTPRES);
        ret=EXFAIL;
    }

    UBF_LOG(log_debug, "_Boccur: return %d", ret);

    return ret;
}

/**
 * Count all occurrences of the fields in the buffer
 * @param p_ub - UBF buffer
 * @return Number of all occurrences of the fields in buffer, -1 FAIL
 */
expublic BFLDOCC ndrx_Bnum(UBFH *p_ub)
{
    BFLDOCC fldcount = 0;
    char fn[] = "_Bnum";
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif
    /* Seems this caused tricks for multi threading.*/
    static __thread Bnext_state_t state;
    BFLDID bfldid;
    BFLDOCC occ;

    memset(&state, 0, sizeof(state));
    bfldid= BFIRSTFLDID;

    while(1==ndrx_Bnext(&state, p_ub, &bfldid, &occ, NULL, NULL, NULL))
    {
        fldcount++;
    }

    return fldcount;
}

/**
 * Allocates buffer & do the initialisation
 * Assuming that using GLIBC which returns already aligned.
 * @param f - number of fields
 * @param v - field size
 * @param len_set directly allocate from given bytes
 * @return ptr to UBF buffer or NULL
 */
expublic UBFH * ndrx_Balloc (BFLDOCC f, BFLDLEN v, long len_set)
{
    UBFH *p_ub=NULL;
    long alloc_size;
    
    if (EXFAIL!=len_set)
    {
        alloc_size = len_set;
    }
    else
    {
        /* Bug #394 */
        alloc_size = ndrx_Bneeded(f, v);
    }
    
    if ( alloc_size > MAXUBFLEN)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Requesting %ld, but min is 1 and max is %ld bytes",
                alloc_size, MAXUBFLEN);
    }
    else
    {
        p_ub=NDRX_MALLOC(alloc_size);
        if (NULL==p_ub)
        {
            ndrx_Bset_error_fmt(BMALLOC, "Failed to alloc %ld bytes", alloc_size);
        }
        else
        {
            if (EXSUCCEED!=Binit(p_ub, alloc_size))
            {
                NDRX_FREE(p_ub); /* Free up allocated memory! */
                p_ub=NULL;
                UBF_LOG(log_error, "Balloc failed - abort Balloc!");
            }
        }
    }
    
    UBF_LOG(log_debug, "Balloc: Returning %p!", p_ub);
    
    return p_ub;
}


/**
 * Reallocate the memory
 * Assuming that using GLIBC which is already aligned
 * 
 * @param p_Fb
 * @param f
 * @param v
 * @param[in] len_set preferred length 
 * @return reallocated UBF buffer or NULL on failure
 */
expublic UBFH * ndrx_Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v, long len_set)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    long alloc_size;
    
    UBF_LOG(log_debug, "Brealloc: enter p_ub=%p f=%d v=%d len_set=%ld", 
        p_ub, (int)f, (int)v, len_set);
    
    if (EXFAIL!=len_set)
    {
        alloc_size = len_set;
    }
    else
    {
        alloc_size = ndrx_Bneeded(f, v);
    }

    /*
     * New buffer size should not be smaller that used.
     */
    if ( alloc_size < hdr->bytes_used || alloc_size > MAXUBFLEN)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Requesting %ld, but min is %ld and max is %ld bytes",
                alloc_size, hdr->buf_len+1, MAXUBFLEN);
        Bfree(p_ub);
        p_ub=NULL;
    }
    else
    {
        p_ub=NDRX_REALLOC(p_ub, alloc_size);
        if (NULL==p_ub)
        {
            ndrx_Bset_error_fmt(BMALLOC, "Failed to alloc %ld bytes", alloc_size);
            p_ub=NULL;
        }
        else
        {
            long reset_size;
            char * p=(char *)p_ub;
            /* reset the header pointer */
            hdr = (UBF_header_t *)p_ub;
            reset_size = alloc_size-hdr->buf_len;
#if 0
            if (reset_size>0)
            {
                /* Now we need to set buffer ending to 0
                 * and we should increase
                 */
                UBF_LOG(log_debug, "Resetting reallocated memory to 0. "
                                    "From %p %d bytes",
                                    p+hdr->buf_len, reset_size);

                memset(p+hdr->buf_len, 0, reset_size);
            }
#endif
            /* Update FB to new size. */
            hdr->buf_len+=reset_size;
        }
    }
    
    UBF_LOG(log_debug, "Brealloc: Returning %p!", p_ub);

    return p_ub;
}

/* vim: set ts=4 sw=4 et smartindent: */
