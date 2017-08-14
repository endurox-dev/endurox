/* 
** UBF library
** The emulator of UBF library
** Enduro Execution Library
** Implementation of projection functions.
**
** @file fproj_impl.c
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
/* #define UBF_API_DEBUG */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Compare two integers
 * @param a
 * @param b
 * @return
 */
int compare (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

/**
 * Performs binary search on passed in list
 * @param array - array it selft
 * @param left - min value
 * @param right - max value
 * @param number - number to search for
 * @return TRUE(found)/FALSE(not found)
 */
int is_fld_pres (BFLDID * array, BFLDID left, BFLDID right, BFLDID number)
{
    int middle = 0;
    int bsearch = EXFALSE;

    while(bsearch == EXFALSE && left <= right)
    {
        middle = (left + right) / 2;

        if(number == array[middle])
        {
            bsearch = EXTRUE;
        }
        else
        {
                if(number < array[middle]) right = middle - 1;
                if(number > array[middle]) left = middle + 1;
        }
    }
    
    UBF_LOG(log_debug, "is_fld_pres: [%p/%s] in%s list",
                                        number, _Bfname_int(number), bsearch?"":" NOT");

    return bsearch;
}

/**
 * Delete & clean up the buffer.
 * @param hdr - ptr header
 * @param del_start - ptr to start of removal
 * @param del_stop - ptr to end of the removal
 */
exprivate void delete_buffer_data(UBFH *p_ub, char *del_start, char *del_stop,
                                BFLDID **p_nextfld)
{
    char *last;
    int remove_size;
    int move_size;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    char fn[] = "delete_buffer_data";
    char *p;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
#endif
/*******************************************************************************/
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = NDRX_MALLOC(hdr->buf_len);
    memcpy(__p_ub_copy, p_ub, hdr->buf_len);

    UBF_LOG(log_debug, "%s: entry\n"
                                    "FBbuflen=%d FBused=%d FBfree=%d ",fn,
                                    hdr->buf_len, hdr->bytes_used,
                                    (hdr->buf_len - hdr->bytes_used));
#endif
/*******************************************************************************/
        /* We should remove here stuff out of the buffer
         * In range from del_bfldid_start..p_bfldid
         * and update the pointers
         */

        remove_size = del_stop-del_start;

        UBF_LOG(log_debug, "About to delete from buffer: %d bytes",
                remove_size);

        last = (char *)hdr;
        last+=(hdr->bytes_used-1);

        /* Remove the data */
        move_size = (last-del_start+1) - remove_size;

        UBF_LOG(log_debug, "delete_buffer_data: to %p, from %p size: %d",
                        del_start, del_start+remove_size, move_size);
        
        memmove(del_start, del_start+remove_size, move_size);
        hdr->bytes_used-=remove_size;
                
        /* Now reset the tail to zeros */
        last = (char *)hdr;
        last+=(hdr->bytes_used-1);

        /* Ensure that we reset last elements... So that we do not get
         * used elements
         */
        UBF_LOG(log_debug, "resetting: %p to 0 - %d bytes",
                        last+1, remove_size);
        memset(last+1, 0, remove_size);

        /* Update the pointer to next */
        p = (char *)*p_nextfld;
        p-=remove_size;
        *p_nextfld= (BFLDID *)p;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = __p_ub_copy->bytes_used;
    __dbg_newuse = hdr->bytes_used;

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "%s: org_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                fn,
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
    UBF_LOG(log_debug, "%s: last %d bytes of data\n org=%p new %p",
                          fn, sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After _Bproj", __p_ub_copy, p_ub, hdr->buf_len);
    UBF_DUMP(log_always, "Used buffer dump after delete_buffer_data: ",
                                p_ub, hdr->bytes_used);
    NDRX_FREE(__p_ub_copy);
#endif
/*******************************************************************************/
}

/**
 * Function leaves only required fields into the buffer
 * This version will also serve as core for Bdelete and Bdeleteall
 * @param p_ub
 * @param fldlist - list of fields to be kept into buffer. If NULL or just BBADFLDID,
 *                  then buffer is reinitialized.
 * @param mode  - operating mode, see PROJ_MODE_*
 * @param processed - ptr to int, which will return the count of chunks processed
 * @return SUCCEED/FAIL
 */
expublic int _Bproj (UBFH * p_ub, BFLDID * fldlist,
                                    int mode, int *processed)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    int ret=EXSUCCEED;
    BFLDID   *p_bfldid = &hdr->bfldid;
    char *p = (char *)&hdr->bfldid;
    BFLDID *del_bfldid_start = NULL; /* Mark the point when we should delete the field */

    dtype_str_t *dtype=NULL;
    int fld_count = 0;
    BFLDID *f_p=fldlist;
    int step;
    char fn[] = "_Bproj";
    int type;
    int mark;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
    dtype_str_t *__dbg_dtype;
#endif
/*******************************************************************************/

/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = NDRX_MALLOC(hdr->buf_len);
    memcpy(__p_ub_copy, p_ub, hdr->buf_len);

    UBF_LOG(log_debug, "%s: entry\n FBbuflen=%d FBused=%d FBfree=%d ",fn,
                                    hdr->buf_len, hdr->bytes_used,
                                    (hdr->buf_len - hdr->bytes_used));
    
#endif
/*******************************************************************************/
    *processed=0;
    /* In this case we delete all items - run the init */
    if (NULL==fldlist || BBADFLDID==*fldlist)
    {
        ret=Binit (p_ub, hdr->buf_len);
    }
    else
    {

        /* Get the count */
        if (PROJ_MODE_DELALL!=mode)
        {
            while(BBADFLDID!=*f_p)
            {
                fld_count++;
                f_p++;
            }
        }
        else
        {
            /* In this case we have only one field */
            fld_count=1;
        }

        /* In all other cases
         * 1. Sort the incoming array
         * 2. Loop throught the buffer, and delete all un-needed items.
         */
        qsort (fldlist, fld_count, sizeof(int), compare);

        /* OK stuff is sorted */
        /* Do not count in bad field by it self. It is not needed for us! */
        while(BBADFLDID!=*p_bfldid)
        {
            /* Should we start removal? */
            if (PROJ_MODE_PROJ==mode)
            {
                /* if not in list then about to delete. */
                mark = !is_fld_pres(fldlist, 0, fld_count-1, *p_bfldid);
            }
            else if (PROJ_MODE_DELETE==mode)
            {
                mark = is_fld_pres(fldlist, 0, fld_count-1, *p_bfldid);
            }
            else if (PROJ_MODE_DELALL==mode)
            {
                /* single field mode, if in list then about to delete*/
                mark = *fldlist==*p_bfldid;
            }
            else
            {
                UBF_LOG(log_error, "Unknown proj mode %d", mode);
                return EXFAIL; /* <<< RETURN! */
            }
            
            /* if (NULL!=del_bfldid_start && *del_bfldid_start != *p_bfldid && !pres)*/
            if (NULL!=del_bfldid_start && !mark)
            {
                UBF_LOG(log_debug, "Current BFLDID before removal: %p",
                                                    *p_bfldid);

                delete_buffer_data(p_ub, (char *)del_bfldid_start, 
                        (char *)p_bfldid, &p_bfldid);

                UBF_LOG(log_debug, "Current BFLDID after  removal: %p",
                                                    *p_bfldid);
                /* Mark that we have all done! */
                del_bfldid_start=NULL; /* Mark that we have finished with this. */
                *processed=*processed+1;
            }

            if (mark && NULL==del_bfldid_start)
            {
                del_bfldid_start = p_bfldid;
                UBF_LOG(log_debug, "Marking field %p for deletion at %p",
                                        *del_bfldid_start, del_bfldid_start);
            }
            /* Get type descriptor */
            type = *p_bfldid>>EFFECTIVE_BITS;

            if (IS_TYPE_INVALID(type))
            {
                ret=EXFAIL;
                ndrx_Bset_error_fmt(BALIGNERR, "%s: Unknown data type found in "
                                        "buffer: %d", fn, type);
                break; /* <<< BREAK; */
            }

            /* Check type alignity */
            dtype = &G_dtype_str_map[type];
            p = (char *)p_bfldid;
            /* Get the step */
            step = dtype->p_next(dtype, p, NULL);

            /* Move us forward. */
            p+=step;
            /* Align error */
            if (CHECK_ALIGN(p, p_ub, hdr))
            {
                ret=EXFAIL;
                ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to unbisubf area: %p",
                                            fn, p);
                break; /* <<< BREAK; */
            }
            p_bfldid = (BFLDID *)p;

        }

        /* If last field was Bad field, then still we need to delete the stuff out! */
        if (EXSUCCEED==ret && NULL!=del_bfldid_start && *del_bfldid_start != *p_bfldid)
        {

            delete_buffer_data(p_ub, (char *)del_bfldid_start,
                                (char *)p_bfldid, &p_bfldid);
            /* Mark that we have all done! */
            del_bfldid_start=NULL; /* Mark that we have finished with this. */
            *processed=*processed+1;
        }
    } /* Else  of use of Binit */
    
    if (EXSUCCEED!=ubf_cache_update(p_ub))
    {
        ndrx_Bset_error_fmt(BALIGNERR, "%s: Failed to update cache!");
        EXFAIL_OUT(ret);
    }
    
out:
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = __p_ub_copy->bytes_used;
    __dbg_newuse = hdr->bytes_used;

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "%s: returns=%d\norg_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                fn,
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
    UBF_LOG(log_debug, "%s: last %d bytes of data\n org=%p new %p",
                          fn, sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After _Bproj", __p_ub_copy, p_ub, hdr->buf_len);
    UBF_DUMP(log_always, "Used buffer dump after _Bproj: ",p_ub, hdr->bytes_used);
    NDRX_FREE(__p_ub_copy);
#endif
/*******************************************************************************/

    return ret;     
}


/**
 * Copy buffer from one to another
 * @param hdr - ptr header
 * @param del_start 
 * @param del_stop 
 */
exprivate int copy_buffer_data(UBFH *p_ub_dst,
                        char *cpy_start, char *cpy_stop, BFLDID **p_nextfld_dst)
{
    int ret=EXSUCCEED;
    int cpy_size;
    UBF_header_t *hdr_dst = (UBF_header_t *)p_ub_dst;

    char fn[] = "delete_buffer_data";
    char *p;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
    dtype_str_t *__dbg_dtype;
#endif
/*******************************************************************************/
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = NDRX_MALLOC(hdr_dst->buf_len);
    memcpy(__p_ub_copy, p_ub_dst, hdr_dst->buf_len);

    UBF_LOG(log_debug, "%s: entry\n"
                                    "FBbuflen=%d FBused=%d FBfree=%d ",fn,
                                    hdr_dst->buf_len, hdr_dst->bytes_used,
                                    (hdr_dst->buf_len - hdr_dst->bytes_used));
#endif
/*******************************************************************************/
        /* We should remove here stuff out of the buffer
         * In range from del_bfldid_start..p_bfldid
         * and update the pointers
         */

        cpy_size = cpy_stop-cpy_start;

        UBF_LOG(log_debug, "About to copy from buffer: %d bytes",
                cpy_size);
        /*
         * Check dest buffer length.
         */
        if (hdr_dst->bytes_used+cpy_size > hdr_dst->buf_len)
        {
            ndrx_Bset_error_fmt(BNOSPACE, "No space in dest buffer, free: "
                                      "%d bytes required: %d bytes",
                                (hdr_dst->buf_len - hdr_dst->bytes_used), cpy_size);
            ret=EXFAIL;
        }
        else
        {                  
            memcpy(*p_nextfld_dst, cpy_start, cpy_size);

            /* Update dest pointer */
            p = (char *)*p_nextfld_dst;
            p+=cpy_size;
            *p_nextfld_dst = (BFLDID *)p;
            hdr_dst->bytes_used+=cpy_size;
            
        }
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = __p_ub_copy->bytes_used;
    __dbg_newuse = hdr_dst->bytes_used;

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr_dst;
    __dbg_p_new+= (hdr_dst->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "%s: org_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                fn,
                                __dbg_olduse,
                                __dbg_newuse,
                                (__dbg_olduse - __dbg_newuse),
                                __p_ub_copy->bfldid, __p_ub_copy->bfldid,
                                hdr_dst->bfldid, hdr_dst->bfldid,
                                *__dbg_fldptr_org, *__dbg_fldptr_org,
                                *__dbg_fldptr_new, *__dbg_fldptr_new);
    /* Check the last four bytes before the end */
    __dbg_p_org-= sizeof(BFLDID);
    __dbg_p_new-= sizeof(BFLDID);
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "%s: last %d bytes of data\n org=%p new %p",
                          fn, sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After _Bproj", __p_ub_copy, p_ub_dst, hdr_dst->buf_len);
    UBF_DUMP(log_always, "Used buffer dump after delete_buffer_data: ",
                                p_ub_dst, hdr_dst->bytes_used);
    NDRX_FREE(__p_ub_copy);
#endif
/*******************************************************************************/
    return ret;
}

/**
 * Projection copy from one buffer to another buffer
 * @param p_ub_dst - dst buffer (being modified)
 * @param p_ub_src - src buffer (not modified)
 * @param fldlist - list of fields to be kept into buffer. If NULL or just BBADFLDID,
 *                  then buffer is reinitialized.
 * @return SUCCEED/FAIL
 */
expublic int _Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src,
                                    BFLDID * fldlist)
{
    int ret=EXSUCCEED;

    UBF_header_t *hdr_src = (UBF_header_t *)p_ub_src;
    BFLDID   *p_bfldid_src = &hdr_src->bfldid;
    char *p = (char *)&hdr_src->bfldid;
    BFLDID *cpy_bfldid_start = NULL; /* Mark the point when we should delete the field */
    
    
    UBF_header_t *hdr_dst = (UBF_header_t *)p_ub_dst;
    BFLDID   *p_bfldid_dst = &hdr_dst->bfldid;


    dtype_str_t *dtype=NULL;
    int fld_count = 0;
    BFLDID *f_p=fldlist;
    int step;
    char fn[] = "_Bprojcpy";
    int type;
    int mark;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
#endif
/*******************************************************************************/

/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = NDRX_MALLOC(hdr_dst->buf_len);
    memcpy(__p_ub_copy, p_ub_dst, hdr_dst->buf_len);

    UBF_LOG(log_debug, "%s: dst buf: entry\n FBbuflen=%d FBused=%d FBfree=%d ",fn,
                                    hdr_dst->buf_len, hdr_dst->bytes_used,
                                    (hdr_dst->buf_len - hdr_dst->bytes_used));

#endif
/*******************************************************************************/
    /* In this case we delete all items - run the init */
    
    if (EXSUCCEED!=Binit (p_ub_dst, hdr_dst->buf_len))
    {
        EXFAIL_OUT(ret);
    }

    if ((NULL==fldlist || BBADFLDID==*fldlist))
    {
        UBF_LOG(log_debug, "Copy list empty - nothing to do!");
    }
    else
    {

        /* Get the count */
        while(BBADFLDID!=*f_p)
        {
            fld_count++;
            f_p++;
        }

        /* In all other cases
         * 1. Sort the incoming array
         * 2. Loop throught the buffer, and delete all un-needed items.
         */
        qsort (fldlist, fld_count, sizeof(int), compare);

        /* OK stuff is sorted */
        /* Do not count in bad field by it self. It is not needed for us! */
        while(EXSUCCEED==ret && BBADFLDID!=*p_bfldid_src)
        {
            mark = is_fld_pres(fldlist, 0, fld_count-1, *p_bfldid_src);
            
            if (NULL!=cpy_bfldid_start && !mark)
            {
                if (EXSUCCEED!=copy_buffer_data(p_ub_dst,
                            (char *)cpy_bfldid_start,
                            (char *)p_bfldid_src, &p_bfldid_dst))
                {
                    EXFAIL_OUT(ret);
                }
                /* Mark that we have all done! */
                cpy_bfldid_start=NULL; /* Mark that we have finished with this. */
                
            }

            /* Should we start removal? */
            if (mark &&
                    NULL==cpy_bfldid_start)
            {
                cpy_bfldid_start = p_bfldid_src;
                UBF_LOG(log_debug, "Marking field %p for copy at %p",
                                        *cpy_bfldid_start, cpy_bfldid_start);
            }
            /* Get type descriptor */
            type = *p_bfldid_src>>EFFECTIVE_BITS;

            if (IS_TYPE_INVALID(type))
            {
                ndrx_Bset_error_fmt(BALIGNERR, "%s: Unknown data type found in "
                                        "buffer: %d", fn, type);
                EXFAIL_OUT(ret);
            }

            /* Check type alignity */
            dtype = &G_dtype_str_map[type];
            p = (char *)p_bfldid_src;
            /* Get the step */
            step = dtype->p_next(dtype, p, NULL);

            /* Move us forward. */
            p+=step;
            /* Align error */
            if (CHECK_ALIGN(p, p_ub_src, hdr_src))
            {
                ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to non UBF area: %p",
                                            fn, p);
                EXFAIL_OUT(ret);
            }
            p_bfldid_src = (BFLDID *)p;

        }

        /* If last field was Bad field, then still we need to delete the stuff out! */
        if (NULL!=cpy_bfldid_start && *cpy_bfldid_start != *p_bfldid_src)
        {

            ret=copy_buffer_data(p_ub_dst,
                            (char *)cpy_bfldid_start,
                            (char *)p_bfldid_src, &p_bfldid_dst);
            /* Mark that we have all done! */
            cpy_bfldid_start=NULL; /* Mark that we have finished with this. */
            
            if (EXSUCCEED!=ret)
            {
                EXFAIL_OUT(ret);
            }
        }
    } /* Else  of use of Binit */

    if (EXSUCCEED!=ubf_cache_update(p_ub_dst))
    {
        ndrx_Bset_error_fmt(BALIGNERR, "%s: Failed to update cache!");
        EXFAIL_OUT(ret);
    }

out:
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = __p_ub_copy->bytes_used;
    __dbg_newuse = hdr_dst->bytes_used;

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - sizeof(BFLDID));

    __dbg_p_new = (char *)hdr_dst;
    __dbg_p_new+= (hdr_dst->bytes_used - sizeof(BFLDID));

    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;

    UBF_LOG(log_debug, "%s: dst returns=%d\norg_used=%d new_used=%d diff=%d "
                                "org_start=%d/%p new_start=%d/%p\n"
                                "old_finish=%d/%p, new_finish=%d/%p",
                                fn,
                                ret,
                                __dbg_olduse,
                                __dbg_newuse,
                                (__dbg_olduse - __dbg_newuse),
                                __p_ub_copy->bfldid, __p_ub_copy->bfldid,
                                hdr_dst->bfldid, hdr_dst->bfldid,
                                *__dbg_fldptr_org, *__dbg_fldptr_org,
                                *__dbg_fldptr_new, *__dbg_fldptr_new);
    /* Check the last four bytes before the end */
    __dbg_p_org-= sizeof(BFLDID);
    __dbg_p_new-= sizeof(BFLDID);
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "%s: last %d bytes of dst data\n org=%p new %p",
                          fn, sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After _Bproj", __p_ub_copy, p_ub_dst, hdr_dst->buf_len);
    UBF_DUMP(log_always, "Used buffer dump after _Bprojcpy: ",p_ub_dst,
                                                        hdr_dst->bytes_used);
    NDRX_FREE(__p_ub_copy);
#endif
/*******************************************************************************/

    return ret;
}

