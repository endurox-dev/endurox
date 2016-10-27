/* 
** UBF library
** The emulator of UBF library
** Enduro Execution Library
** Debug ideas:
** - on API entry take copy of FB
** - on API exit make diff of the FB memory. Also diff the memory usage
** - provide the information about count of bytes from source element added
** - on exit, dump the first BFLDID element address
**
** @file ubf.c
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
#include <expr.h>
#include <thlock.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define API_ENTRY {_Bunset_error(); \
    if (!M_init) { \
	MUTEX_LOCK; \
        UBF_DBG_INIT(("UBF", UBFDEBUGLEV));\
        M_init=TRUE;\
	MUTEX_UNLOCK;\
    }\
}\

/*
 * Validates user type specified
 */
#define VALIDATE_USER_TYPE(X, Y) \
            /* Check data type alignity */ \
            if (IS_TYPE_INVALID(X)) \
            { \
                _Fset_error_fmt(BTYPERR, "Invalid user type %d", X);\
                Y;\
            }

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
static volatile bool M_init = FALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize bisubf buffer
 * Size already includes last BBADFLDID! Which always must stay as BBADFLDID!
 */
public int Binit (UBFH * p_ub, BFLDLEN len)
{
    int ret=SUCCEED;
    UBF_header_t *ubf_h = (UBF_header_t *)p_ub;
    
    UBF_LOG(log_debug, "Binit p_ub=%p len=%d", p_ub, len);
            
    if (NULL==p_ub)
    {
        /* Null buffer */
        _Fset_error_msg(BNOTFLD, "ptr to UBFH is NULL");
        ret=FAIL;
    }
    else if (len < sizeof(UBF_header_t))
    {
        _Fset_error_fmt(BNOSPACE, "Minimum: %d, but got: %d",
                                    sizeof(UBF_header_t), len);
        ret=FAIL;
    }
    else
    {
        /* Initialize buffer */
        memset((char *)p_ub, 0, len); /* TODO: Do we need all to be set to 0? */
        ubf_h->version = UBF_VERSION; /* Reset options to (all disabled) */
        ubf_h->buffer_type = 0; /* not used currently */
        memcpy(ubf_h->magic, UBF_MAGIC, sizeof(UBF_MAGIC)-1);
        ubf_h->buf_len = len;
        ubf_h->opts = 0; 
        ubf_h->bytes_used = sizeof(UBF_header_t);
        
    }
    
    return ret;
}

/**
 * Add value to FB...
 *
 * If adding the first value, the buffer should be large enought and last
 * BFLDID should stay at BADFLID, because will not be overwritten.
 * Also last entry always must at BBADFLDID! This is the rule.
 */
public int Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len)
{
    API_ENTRY;
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "Badd: arguments fail!");
        return FAIL;
    }
    
    return _Badd (p_ub, bfldid, buf, len, NULL);
}

/**
 * Read the data from buffer and loaded it into output buff
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @param buflen
 * @return 
 */
public int Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN * buflen)
{
    API_ENTRY;

    if (SUCCEED!=validate_entry(p_ub, bfldid,occ,0))
    {
        UBF_LOG(log_warn, "Bget: arguments fail!");
        return FAIL; /* <<<< RETURN HERE! */
    }

    return _Bget (p_ub, bfldid, occ, buf, buflen);
}

/**
 * Delete specific field from buffer
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return
 */
public int Bdel (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ)
{
    int ret=SUCCEED;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    /* BFLDID   *p_bfldid = &hdr->bfldid; */
    dtype_str_t *dtype;
    char *p;
    BFLDID   *p_bfldid;
    char *last_checked=NULL;
    int last_occ=-1;
    char *last;
    int remove_size;
    int move_size;
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    /* Real debug stuff!! */
    UBF_header_t *__p_ub_copy;
    int __dbg_type;
    int *__dbg_fldptr_org;
    int *__dbg_fldptr_new;
    char *__dbg_p_org;
    char *__dbg_p_new;
    int __dbg_newuse;
    int __dbg_olduse;
    dtype_str_t *__dbg_dtype;
#endif
/*******************************************************************************/
    API_ENTRY;
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bdel: arguments fail!");
        ret=FAIL;
        return ret; /* <<<< RETURN HERE! */
    }
/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __p_ub_copy = NDRX_MALLOC(hdr->buf_len);
    memcpy(__p_ub_copy, p_ub, hdr->buf_len);
    __dbg_type = (bfldid>>EFFECTIVE_BITS);
    __dbg_dtype = &G_dtype_str_map[__dbg_type];
    UBF_LOG(log_debug, "Bdel: entry, adding\nfld=[%d/%p] occ=%d "
                                    "type[%hd/%s]\n"
                                    "FBbuflen=%d FBused=%d FBfree=%d "
                                    "FBstart fld=[%s/%d/%p] ",
                                    bfldid, bfldid, occ,
                                    __dbg_type, __dbg_dtype->fldname,
                                    hdr->buf_len, hdr->bytes_used,
                                    (hdr->buf_len - hdr->bytes_used),
                                    _Bfname_int(bfldid), bfldid, bfldid);
#endif
/*******************************************************************************/
    if (NULL!=(p=get_fld_loc(p_ub, bfldid, occ, &dtype, &last_checked, NULL, &last_occ,
                                            NULL)))
    {
        p_bfldid = (BFLDID *)p;
        /* Wipe the data out! */
        last = (char *)hdr;
        last+=(hdr->bytes_used-1);
        remove_size = dtype->p_next(dtype, p, NULL);
        /* Get the size to be moved */
        move_size = (last-p+1) - remove_size;
        /* Get some free space here!
         * So from last element we take off current position,
         * so we get lenght. to which we should move.
         */
        UBF_LOG(log_debug, "moving: to %p from %p %d bytes",
                            p, p+remove_size, move_size);
        
        memmove(p, p+remove_size, move_size);
        hdr->bytes_used-=remove_size;
        
        /* Update type offset cache: */
        ubf_cache_shift(p_ub, bfldid, -1*remove_size);
        
        last = (char *)hdr;
        last+=(hdr->bytes_used-1);
        /* Ensure that we reset last elements... So that we do not get
         * used elements
         */
        UBF_LOG(log_debug, "resetting: %p to 0 - %d bytes",
                            last+1, 0, remove_size);
        memset(last+1, 0, remove_size);
    }
    else
    {
        _Fset_error(BNOTPRES);
        ret=FAIL;
    }

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

    UBF_LOG(log_debug, "Bdel: returns=%d\norg_used=%d new_used=%d diff=%d "
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
    UBF_LOG(log_debug, "Bdel: last %d bytes of data\n org=%p new %p",
                          sizeof(BFLDID), *__dbg_fldptr_org, *__dbg_fldptr_new);
    UBF_DUMP_DIFF(log_always, "After Badd", __p_ub_copy, p_ub, hdr->buf_len);
    UBF_DUMP(log_always, "Used buffer dump after: ",p_ub, hdr->bytes_used);
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
 * @return
 */
public int Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len)
{
    API_ENTRY; /* Standard initialization */

    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bchg: arguments fail!");
        return FAIL; /* <<<< RETURN HERE! */
    }

    return _Bchg(p_ub, bfldid, occ, buf, len, NULL);
}

/**
 * Return BFLDID from name!
 */
public BFLDID Bfldid (char *fldnm)
{
    UBF_field_def_t *p_fld=NULL;

    API_ENTRY;

    if (SUCCEED!=prepare_type_tables())
    {
            return BBADFLDID;
    }
    /* Now we can try to do lookup */
    p_fld = _fldnmhash_get(fldnm);

    if (NULL!=p_fld)
    {
            return p_fld->bfldid;
    }
    else
    {
            _Fset_error(BBADNAME);
            return BBADFLDID;
    }
}
/**
 * Return field name from given id
 */
public char * Bfname (BFLDID bfldid)
{
    UBF_field_def_t *p_fld;
    API_ENTRY;

    if (SUCCEED!=prepare_type_tables())
    {
    goto out;
    }

    /* Now try to find the data! */
    p_fld = _bfldidhash_get(bfldid);
    if (NULL==p_fld)
    {
            _Fset_error(BBADFLD);
    goto out;
    }
    else
    {
            return p_fld->fldname;
    }
    
out:
    return NULL;
}

/**
 * Find the entry into buffer.
 * Return raw pointer directly
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param p_len
 * @return
 */
public char * Bfind (UBFH * p_ub, BFLDID bfldid,
                                        BFLDOCC occ, BFLDLEN * p_len)
{
    API_ENTRY;

    UBF_LOG(log_debug, "Bfind: bfldid: %d occ: %hd", bfldid, occ);
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bdel: arguments fail!");
        return NULL;
    }

    return _Bfind(p_ub, bfldid, occ, p_len, NULL);
}


/**
 * Whilte this is not called from other internal functions
 * we are leaving implementation in API
 * @param p_ub
 * @param bfldid
 * @param buf
 * @param len
 * @param usrtype
 * @return
 */
public int CBadd (UBFH *p_ub, BFLDID bfldid, char * buf,
                                BFLDLEN len, int usrtype)
{
    int ret=SUCCEED;
    int cvn_len=0;
    char *cvn_buf;

    char tmp_buf[CF_TEMP_BUF_MAX];
    int to_type = (bfldid>>EFFECTIVE_BITS);
    /* Buffer management */
    char *alloc_buf = NULL;
    char *p;
    char *fn = "CBadd";
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1 = &G_dtype_ext1_map[usrtype];
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "CBadd adding data", buf, &len);
    #endif
/*******************************************************************************/
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "CBadd: arguments fail!");
        return FAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return FAIL);

    /* if types are the same then do direct call */
    if (usrtype==to_type)
    {
        UBF_LOG(log_debug, "CBadd: the same types - direct call!");
        return _Badd(p_ub, bfldid, buf, len, NULL); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */

    /* Allocate the buffer dynamically */
    if (NULL==(p=get_cbuf(usrtype, to_type, tmp_buf, buf, len, &alloc_buf, 
                                &cvn_len, CB_MODE_DEFAULT, 0)))
    {
        UBF_LOG(log_error, "%s: Malloc failed!", fn);
        return FAIL; /* <<<< RETURN!!!! */
    }

    cvn_buf = ubf_convert(usrtype, CNV_DIR_IN, buf, len,
                        to_type, p, &cvn_len);

    if (NULL!=cvn_buf)
    {
        ret=_Badd (p_ub, bfldid, cvn_buf, cvn_len, NULL);
    }
    else
    {
        UBF_LOG(log_error, "%s: failed to convert data!", fn);
        /* Error should be provided by conversation function */
        ret=FAIL;
    }

    /* Free up buffer */
    if (NULL!=alloc_buf)
    {
        UBF_LOG(log_debug, "%s: free alloc_buf", fn);
        NDRX_FREE(alloc_buf);
    }

    return ret;
}
/**
 * Will we support zero lenght entries of BFLD_CARRAY? Currently it looks like yes!
 * But if so then we can get un-predictable results in case when doing zero lenght
 * conversion to other data types!
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @param len
 * @param usrtype
 * @return
 */
public int CBchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                                char * buf, BFLDLEN len, int usrtype)
{
    int ret=SUCCEED;
    int cvn_len=0;
    char *cvn_buf;
    char tmp_buf[CF_TEMP_BUF_MAX];
    int to_type = (bfldid>>EFFECTIVE_BITS);
    /* Buffer management */
    char *alloc_buf = NULL;
    char *p;
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1 = &G_dtype_ext1_map[usrtype];
    __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "CBchg putting data", buf, &len);
    #endif
/*******************************************************************************/

    API_ENTRY; /* Standard initialization */

    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "CBchg: arguments fail!");
        return FAIL; /* <<<< RETURN HERE! */
    }
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return FAIL);

    /* if types are the same then do direct call */
    if (usrtype==to_type)
    {
        UBF_LOG(log_debug, "CBchg: the same types - direct call!");
        return _Bchg(p_ub, bfldid, occ, buf, len, NULL); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */
    
    /* Allocate the buffer dynamically */
    if (NULL==(p=get_cbuf(usrtype, to_type, tmp_buf, buf, len, &alloc_buf, &cvn_len,
                                CB_MODE_DEFAULT, 0)))
    {
        UBF_LOG(log_error, "CBchg: Malloc failed!");
        return FAIL; /* <<<< RETURN!!!! */
    }

    cvn_buf = ubf_convert(usrtype, CNV_DIR_IN, buf, len,
                        to_type, p, &cvn_len);

    if (NULL!=cvn_buf)
    {
        ret=Bchg (p_ub, bfldid, occ, cvn_buf, cvn_len);
    }
    else
    {
        UBF_LOG(log_error, "CBchg: failed to convert data!");
        /* Error should be provided by conversation function */
        ret=FAIL;
    }

    /* Free up buffer */
    if (NULL!=alloc_buf)
    {
        UBF_LOG(log_debug, "CBchg: free alloc_buf");
        NDRX_FREE(alloc_buf);
    }
    
    return ret;
}

/* Have to think here, not?  */
public int CBget (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ ,
                                char *buf, BFLDLEN *len, int usrtype)
{
    int ret=SUCCEED;
    int from_type = (bfldid>>EFFECTIVE_BITS);
    BFLDLEN tmp_len = 0;
    char *cvn_buf;
    char *fb_data;
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1 = &G_dtype_ext1_map[usrtype];
    #endif
/*******************************************************************************/
    
    API_ENTRY;
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "CBget: arguments fail!");
        return FAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return FAIL);

    /* if types are the same then do direct call */
    if (usrtype==from_type)
    {
        UBF_LOG(log_debug, "CBget: the same types - direct call!");
        return Bget(p_ub, bfldid, occ, buf, len); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */
    
    fb_data=_Bfind (p_ub, bfldid, occ, &tmp_len, NULL);

    if (NULL!=fb_data)
    {

        cvn_buf = ubf_convert(from_type, CNV_DIR_OUT, fb_data, tmp_len,
                                    usrtype, buf, len);
        if (NULL==cvn_buf)
        {
            UBF_LOG(log_error, "CBget: failed to convert data!");
            /* Error should be provided by conversation function */
            ret=FAIL;
        }
    }
    else
    {
        UBF_LOG(log_error, "CBget: Field not present!");
        ret=FAIL;
    }
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (SUCCEED==ret)
    {
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "CBget got data", buf, len);
    }
    #endif
/*******************************************************************************/
    return ret;
}

/**
 * As all other functions. Currently this does not check to see is field
 * aligned or not. It should provide
 * @param p_ub_dst
 * @param p_ub_src
 * @return
 */
int Bcpy (UBFH * p_ub_dst, UBFH * p_ub_src)
{
    int ret=SUCCEED;
    UBF_header_t *src_h = (UBF_header_t *)p_ub_src;
    UBF_header_t *dst_h = (UBF_header_t *)p_ub_dst;
    int dst_buf_len;
    API_ENTRY;

    UBF_LOG(log_debug, "Bcpy: About to copy from FB=%p to FB=%p",
                                    p_ub_src, p_ub_dst);

    if (SUCCEED==ret && NULL==p_ub_src)
    {
        _Fset_error_msg(BNOTFLD, "p_ub_src is NULL!");
        ret=FAIL;
    }

    if (SUCCEED==ret && NULL==p_ub_dst)
    {
        _Fset_error_msg(BNOTFLD, "p_ub_dst is NULL!");
        ret=FAIL;
    }

    if (SUCCEED==ret && 0!=strncmp(src_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        _Fset_error_msg(BNOTFLD, "source buffer magic failed!");
        ret=FAIL;
    }

    if (SUCCEED==ret && 0!=strncmp(dst_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        _Fset_error_msg(BNOTFLD, "destination buffer magic failed!");
        ret=FAIL;
    }

    if (SUCCEED==ret && dst_h->buf_len < src_h->bytes_used)
    {
        _Fset_error_fmt(BNOSPACE, "Destination buffer too short. "
                                    "Source len: %d dest used: %d",
                                    dst_h->buf_len, src_h->bytes_used);
        ret=FAIL;
    }

    if (SUCCEED==ret)
    {
        /* save some original characteristics of dest buffer */
        dst_buf_len = dst_h->buf_len;
        memset(p_ub_dst, 0, dst_h->bytes_used);
        memcpy(p_ub_dst, p_ub_src, src_h->bytes_used);
        dst_h->buf_len = dst_buf_len;
        dst_h->bytes_used = src_h->bytes_used;
        
        /* save cache fields: */
        dst_h->cache_long_off = src_h->cache_long_off;
        dst_h->cache_char_off = src_h->cache_char_off;
        dst_h->cache_float_off = src_h->cache_float_off;
        dst_h->cache_double_off = src_h->cache_double_off;
        dst_h->cache_string_off = src_h->cache_string_off;
        dst_h->cache_carray_off = src_h->cache_carray_off;
    
        
    }
    UBF_LOG(log_debug, "Bcpy: return %d", ret);
    
    return ret;
}

/**
 * Used buffer amount.
 * @param p_ub
 * @return
 */
long Bused (UBFH *p_ub)
{
    long ret=FAIL;
    char fn[] = "Bused";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FALSE;
    }
    else
    {
        ret=hdr->bytes_used;
        UBF_LOG(log_debug, "%s: Buffer used: %ld!", fn, ret);
    }

    return ret;
}

/**
 * Generate valid field ID
 * @param fldtype
 * @param bfldid
 * @return 
 */
BFLDID Bmkfldid (int fldtype, BFLDID bfldid)
{

    BFLDID ret = fldtype << EFFECTIVE_BITS;
    ret |= bfldid;
    
    return ret;
}
/**
 * Get the total occurrance count.
 * API version - no error checking.
 * @param p_ub
 * @param bfldid
 * @return
 */
BFLDOCC Boccur (UBFH * p_ub, BFLDID bfldid)
{
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "_Boccur: arguments fail!");
        return FAIL;
    }

    return _Boccur (p_ub, bfldid);
}

/**
 * Check the field presence
 * API version - no error checking.
 */
int Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    API_ENTRY;
    
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "_Bpres: arguments fail!");
        return FALSE;
    }

    return _Bpres(p_ub, bfldid, occ);
}

/**
 * Public version of Bfldtype
 * @param bfldid
 * @return
 */
public int Bfldtype (BFLDID bfldid)
{
    return bfldid >> EFFECTIVE_BITS;
}

public char * Bboolco (char * expr)
{
    API_ENTRY;
    {
    /* Lock the region */
    MUTEX_LOCK;
    {
        char *ret;
        ret = _Bboolco (expr);
        MUTEX_UNLOCK;
        return ret;
    }
    }
}

public int Bboolev (UBFH * p_ub, char *tree)
{
    API_ENTRY;
    return _Bboolev (p_ub, tree);
}

/**
 * This function is not tested hardly. But it should work fine.
 * @param p_ub
 * @param tree
 * @return
 */
public double Bfloatev (UBFH * p_ub, char *tree)
{
    API_ENTRY;
    return _Bfloatev (p_ub, tree);
}

public void Btreefree (char *tree)
{
    API_ENTRY;
    _Btreefree (tree);
}

/**
 * This function keeps it's internal state in special buffer.
 * So this should not be used recursively or by multiple threads.
 * Also check against buffer sizes is done. Buffer shall not be modified during
 * the enumeration!
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @param len
 * @return -1 (FAIL)/0 (End of Buffer)/1 (Found)
 */
public int  Bnext(UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len)
{
    char fn[] = "Bnext";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    /* Seems this caused tricks for multi threading.*/
    static __thread Bnext_state_t state;

    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    else if (NULL==bfldid || NULL==occ)
    {
        _Fset_error_msg(BEINVAL, "Bnext: ptr of bfldid or occ is NULL!");
        return FAIL;
    }
    else if (*bfldid != BFIRSTFLDID && state.p_ub != p_ub)
    {
        _Fset_error_fmt(BEINVAL, "%s: Different buffer [state: %p used: %p] "
                                    "used for different state", fn,
                                    state.p_ub, p_ub);
        return FAIL;
    }
    else if (*bfldid != BFIRSTFLDID && state.size!=hdr->bytes_used)
    {
        _Fset_error_fmt(BEINVAL, "%s: Buffer size changed [state: %d used: %d] "
                                    "from last search", fn,
                                    state.size, hdr->bytes_used);
        return FAIL;
    }
    else
    {
        if (*bfldid == BFIRSTFLDID)
        {
            memset(&state, 0, sizeof(state));
        }

        return _Bnext(&state, p_ub, bfldid, occ, buf, len, NULL);
    }
}

/**
 * API entry for Bproj
 * @param p_ub
 * @param fldlist
 * @return SUCCEED/FAIL
 */
public int Bproj (UBFH * p_ub, BFLDID * fldlist)
{
    char fn[] = "Bproj";
    int processed;
    API_ENTRY;
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    else
    {
        /* Call the implementation */
        return _Bproj (p_ub, fldlist, PROJ_MODE_PROJ, &processed);
    }
}

/**
 * Projection copy on 
 * @param p_ub_dst
 * @param p_ub_src
 * @param fldlist
 * @return
 */
public int Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src,
                                    BFLDID * fldlist)
{
    char fn[] = "Bprojcpy";
    API_ENTRY;
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", fn);
        _Bappend_error_msg("(Bprojcpy: arguments fail for src buf!)");
        return FAIL;
    }
    else if (SUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", fn);
        _Bappend_error_msg("(Bprojcpy: arguments fail for dst buf!)");
        return FAIL;
    }
    else
    {
        /* Call the implementation */
        return _Bprojcpy (p_ub_dst, p_ub_src, fldlist);
    }
}

/**
 * Will not implement.
 * @param
 * @param
 * @return
 */
public int Bindex (UBFH * p_ub, BFLDOCC occ)
{
    UBF_LOG(log_debug, "Bindex: Not implemented - ignore!");
    return SUCCEED;
}

/**
 * Not implemented & not planning to do so
 * @param p_ub
 * @return 0 - number of fields indexed.
 */
public BFLDOCC Bunindex (UBFH * p_ub)
{
    UBF_LOG(log_debug, "Bunindex: Not implemented - ignore!");
    return 0;
}

/**
 * Not using indexes (Not implemented & not supported)
 * @param p_ub
 * @return 0 - size used for index
 */
public long Bidxused (UBFH * p_ub)
{
    UBF_LOG(log_debug, "Bidxused: Not implemented - ignore!");
    return 0;
}

/**
 * Not supported & will not be implemented.
 * @param p_ub
 * @param occ
 * @return
 */
public int Brstrindex (UBFH * p_ub, BFLDOCC occ)
{
    UBF_LOG(log_debug, "Brstrindex: Not implemented - ignore!");
    return SUCCEED;
}

/**
 * Will re-use _Bproj for this
 * @param p_ub
 * @param bfldid
 * @return
 */
public int Bdelall (UBFH *p_ub, BFLDID bfldid)
{
    int ret=SUCCEED;
    char fn[] = "Bdelall";
    int processed;
    API_ENTRY;
    
    UBF_LOG(log_warn, "%s: enter", fn);
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FAIL;
    }
    else
    {
        /* Call the implementation */
        ret=_Bproj (p_ub, &bfldid, PROJ_MODE_DELALL, &processed);
    }

    if (SUCCEED==ret && 0==processed)
    {
        /* Set error that none of fields have been deleted */
        ret=FAIL;
        _Fset_error_msg(BNOTPRES, "No fields have been deleted");
    }
    
    UBF_LOG(log_warn, "%s: return %d", fn, ret);
    return ret;
}

/**
 * Will re-use _Bproj for this
 * @param p_ub
 * @param fldlist
 * @return
 */
public int Bdelete (UBFH *p_ub, BFLDID *fldlist)
{
    int ret=SUCCEED;
    char fn[] = "Bdelete";
    int processed;
    API_ENTRY;

    UBF_LOG(log_warn, "%s: enter", fn);
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FAIL;
    }
    else
    {
        /* Call the implementation */
        ret=_Bproj (p_ub, fldlist, PROJ_MODE_DELETE, &processed);
    }

    if (SUCCEED==ret && 0==processed)
    {
        /* Set error that none of fields have been deleted */
        ret=FAIL;
        _Fset_error_msg(BNOTPRES, "No fields have been deleted");
    }
    
    UBF_LOG(log_warn, "%s: return %d", fn, ret);
    return ret;
}

/**
 * Returns field number out of field ID
 * @param bfldid
 * @return 
 */
public BFLDOCC Bfldno (BFLDID bfldid)
{
    UBF_LOG(log_debug, "Bfldno: Mask: %d", EFFECTIVE_BITS_MASK);
    return (BFLDOCC) bfldid & EFFECTIVE_BITS_MASK;
}

/**
 * returns TRUE if buffer is bisubf
 * @param p_ub - FB to test.
 * @return TRUE/FALSE
 */
public int Bisubf (UBFH *p_ub)
{
    int ret=TRUE;
    char fn[] = "Bisubf";
    API_ENTRY;
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FALSE;
        _Bunset_error();
    }
    
    return ret;
}

/**
 * Returns free buffer space that could be used by fields.
 * @param p_Fb
 * @return -1 on error or free buffer space
 */
public long Bunused (UBFH *p_ub)
{
    long ret=FAIL;
    char fn[] = "Bunused";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FALSE;
    }
    else
    {
        ret=(hdr->buf_len - hdr->bytes_used);
        UBF_LOG(log_debug, "%s: Buffer free: %ld!", fn, ret);
    }

    return ret;
}

/**
 * Return buffer size
 * @param p_ub
 * @return
 */
public long Bsizeof (UBFH *p_ub)
{
    long ret=FAIL;
    char fn[] = "Bsizeof";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FALSE;
    }
    else
    {
        ret=hdr->buf_len;
        UBF_LOG(log_debug, "%s: Buffer size: %ld!", fn, ret);
    }

    return ret;
}

/**
 * Return string descrypting the data type
 * @param bfldid
 * @return NULL (on error)/field type descr
 */
public char * Btype (BFLDID bfldid)
{
    int type = bfldid >> EFFECTIVE_BITS;
    API_ENTRY;

    if (IS_TYPE_INVALID(type))
    {
        _Fset_error_fmt(BTYPERR, "Unknown type number %d", type);
        return NULL;
    }
    else
    {
        return G_dtype_str_map[type].fldname;
    }
}

/**
 * Free up dynamically allocated buffer.
 * @param p_ub
 * @return 
 */
public int Bfree (UBFH *p_ub)
{
    int ret=SUCCEED;
    char fn[] = "Bfree";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;
    
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=FAIL;
    }
    else
    {
        memset(hdr->magic, 0, UBF_MAGIC_SIZE);
        NDRX_FREE(p_ub);
    }
    
    return ret;
}

/**
 * Allocates buffer & do the initialisation
 * Assuming that using GLIBC which returns already aligned.
 * @param f - number of fields
 * @param v - field size
 * @return
 */
public UBFH * Balloc (BFLDOCC f, BFLDLEN v)
{
    UBFH *p_ub=NULL;
    long alloc_size = f*(sizeof(BFLDID)) + f*v + sizeof(UBF_header_t);
    API_ENTRY;
    
    if ( alloc_size > MAXUBFLEN)
    {
        _Fset_error_fmt(BEINVAL, "Requesting %ld, but min is 1 and max is %ld bytes",
                alloc_size, MAXUBFLEN);
    }
    else
    {
        p_ub=NDRX_MALLOC(alloc_size);
        if (NULL==p_ub)
        {
            _Fset_error_fmt(BMALLOC, "Failed to alloc %ld bytes", alloc_size);
        }
        else
        {
            if (SUCCEED!=Binit(p_ub, alloc_size))
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
 * @return 
 */
public UBFH * Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v)
{
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    long alloc_size = f*(sizeof(BFLDID)) + f*v + sizeof(UBF_header_t);
    char fn[] = "Brealloc";
    API_ENTRY;

    UBF_LOG(log_debug, "Brealloc: enter p_ub=%p!", p_ub);
    
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        p_ub=NULL;
    }

    /*
     * New buffer size should not be smaller that used.
     */
    if ( alloc_size < hdr->bytes_used || alloc_size > MAXUBFLEN)
    {
        _Fset_error_fmt(BEINVAL, "Requesting %ld, but min is %ld and max is %ld bytes",
                alloc_size, hdr->buf_len+1, MAXUBFLEN);
        p_ub=NULL;
    }
    else
    {
        p_ub=NDRX_REALLOC(p_ub, alloc_size);
        if (NULL==p_ub)
        {
            _Fset_error_fmt(BMALLOC, "Failed to alloc %ld bytes", alloc_size);
            p_ub=NULL;
        }
        else
        {
            long reset_size;
            char * p=(char *)p_ub;
            /* reset the header pointer */
            hdr = (UBF_header_t *)p_ub;
            reset_size = alloc_size-hdr->buf_len;

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
            /* Update FB to new size. */
            hdr->buf_len+=reset_size;
        }
    }
    
    UBF_LOG(log_debug, "Brealloc: Returning %p!", p_ub);

    return p_ub;
}

/**
 * API function for Bupdate.
 * @param p_ub_dst - dest buffer to update
 * @param p_ub_src - source buffer to take values out from.
 * @return SUCEED/FAIL
 */
public int Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    char fn[] = "Bupdate";
    int ret=SUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", fn);
    if (SUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", fn);
        _Bappend_error_msg("(Bupdate: arguments fail for src buf!)");
        ret=FAIL;
    }
    else if (SUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", fn);
        _Bappend_error_msg("(Bupdate: arguments fail for dst buf!)");
        ret=FAIL;
    }
    else
    {
        /* Call the implementation */
        ret=_Bupdate (p_ub_dst, p_ub_src);
    }
    UBF_LOG(log_debug, "Return %s %d", fn, ret);
    return ret;
}

/**
 * Concat two buffers
 * @param p_ub_dst
 * @param p_ub_src
 * @return 
 */
public int Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    char fn[] = "Bconcat";
    int ret=SUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", fn);
    if (SUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", fn);
        _Bappend_error_msg("(Bconcat: arguments fail for src buf!)");
        ret=FAIL;
    }
    else if (SUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", fn);
        _Bappend_error_msg("(Bconcat: arguments fail for dst buf!)");
        ret=FAIL;
    }
    else
    {
        /* Call the implementation */
        ret=_Bconcat (p_ub_dst, p_ub_src);
    }
    UBF_LOG(log_debug, "Return %s %d", fn, ret);
    return ret;
}

/**
 * CBfind API entry
 * @return NULL/ptr to value
 */
public char * CBfind (UBFH * p_ub, 
                    BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype)
{
    char *fn = "CBfind";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d occ: %hd", fn, bfldid, occ);

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return NULL;
    }
    
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return NULL);

    /* Call the implementation */
    return _CBfind (p_ub, bfldid, occ, len, usrtype, CB_MODE_TEMPSPACE, 0);
}

/**
 * This almost is the same as CBfind only difference is that buffer is allocated
 * for user. Ant it is user's responsibility to free up memory.
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param usrtype
 * @param extralen (IN) - extra buffer to allocate/ (OUT) - size of bufer in use.
 * @return NULL (failure)/ptr - in case of success
 */
public char * CBgetalloc (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC occ, int usrtype, BFLDLEN *extralen)
{
    char *ret=NULL;
    char fn[] = "CBgetalloc";
    API_ENTRY;
    UBF_LOG(log_debug, "%s: bfldid: %d occ: %hd", fn, bfldid, occ);

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "CBgetalloc: arguments fail!");
        return NULL;
    }
    
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return NULL);

    /* Call the implementation */
    ret=_CBfind (p_ub, bfldid, occ, extralen, usrtype, CB_MODE_ALLOC, 
                    (NULL!=extralen?*extralen:0));

    UBF_LOG(log_debug, "%s: returns ret=%p", fn, ret);

    return ret;
}

/**
 * API entry for Bfindocc
 * @param p_ub
 * @param bfldid
 * @param buf
 * @param len
 * @return
 */
public BFLDOCC Bfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len)
{
    char *fn = "Bfindocc";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    if (NULL==buf)
    {
         _Fset_error_fmt(BEINVAL, "buf is NULL");
         return FAIL;
    }
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }


    /* validate user type */

    /* Call the implementation */
    return _Bfindocc (p_ub, bfldid, buf, len);
}

/**
 * API entry for CBfindocc
 * @param p_ub
 * @param bfldid
 * @param buf
 * @param len
 * @return
 */
public BFLDOCC CBfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len, int usrtype)
{
    char *fn = "CBfindocc";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    if (NULL==buf)
    {
         _Fset_error_fmt(BEINVAL, "buf is NULL");
         return FAIL;
    }
 
    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return FAIL);

    /* validate user type */

    /* Call the implementation */
    return _CBfindocc (p_ub, bfldid, buf, len, usrtype);
}

/**
 * API version of Bgetalloc
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param len
 * @return NULL (on failure)/ ptr on success.
 */
public char * Bgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen)
{
    char *fn = "Bgetalloc";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return NULL;
    }

    /* Call the implementation */
    return _Bgetalloc (p_ub, bfldid, occ, extralen);
}

/**
 * Bfindlast API entry
 * @return NULL/ptr to value
 */
public char * Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC *occ, BFLDLEN *len)
{
    char *fn = "Bfindlast";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return NULL;
    }

    /* Call the implementation */
    return _Bfindlast (p_ub, bfldid, occ, len);
}

/**
 * Bfindlast API entry
 * @return NULL/ptr to value
 */
public int Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len)
{
    char *fn = "Bgetlast";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }

    /* Call the implementation */
    return _Bgetlast (p_ub, bfldid, occ, buf, len);
}


/**
 * API function for Bfprint
 * @param p_ub
 * @param outf
 * @return
 */
public int Bfprint (UBFH *p_ub, FILE * outf)
{
    char *fn = "Bfprint";
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    /* check output file */
    if (NULL==outf)
    {
        _Fset_error_msg(BEINVAL, "output file cannot be NULL!");
        return FAIL;
    }

    return _Bfprint (p_ub, outf);
}

/**
 * API function for Bprint
 * uses _Bfprint in core with stdout.
 * @param p_ub
 * @param outf
 * @return
 */
public int Bprint (UBFH *p_ub)
{
    char *fn = "Bprint";
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }

    return _Bfprint (p_ub, stdout);
}

/**
 * API function for Btypcvt
 * @param to_len
 * @param to_type
 * @param from_buf
 * @param from_type
 * @param from_len
 * @return
 */
public char * Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len)
{
    char *fn = "Btypcvt";
    dtype_str_t * dtype;
    API_ENTRY;


    if (NULL==from_buf)
    {
        _Fset_error_fmt(BEINVAL, "%s:from buf cannot be NULL!", fn);
        return NULL; /* <<< RETURN! */
    }

    if (IS_TYPE_INVALID(from_type))
    {
        _Fset_error_fmt(BTYPERR, "%s: Invalid from_type %d", fn, from_type);
        return NULL; /* <<< RETURN! */
    }

    if (IS_TYPE_INVALID(to_type))
    {
        _Fset_error_fmt(BTYPERR, "%s: Invalid from_type %d", fn, to_type);
        return NULL; /* <<< RETURN! */
    }

    /*
     * We provide from length, it is needed for core implementation.
     */
    if (BFLD_CARRAY!=from_type)
    {
        int payload_size=0;
        dtype = &G_dtype_str_map[from_type];
        /* calculate the actual data len, required by type converter. */
        dtype->p_get_data_size(dtype, from_buf, from_len, &from_len);
    }
    
    /* Call implementation */
    return _Btypcvt(to_len, to_type, from_buf, from_type, from_len);
}

/**
 * API entry for Bextread
 * @param p_ub - bisubf buffer
 * @param inf - file to read from 
 * @return SUCCEED/FAIL
 */
public int Bextread (UBFH * p_ub, FILE *inf)
{
    char *fn = "Bextread";
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    /* check output file */
    if (NULL==inf)
    {
        _Fset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return FAIL;
    }
    
    return _Bextread (p_ub, inf);
}

/**
 * API entry for Bboolpr
 * @param tree - compiled expression tree
 * @param outf - file to print to tree
 */
public void Bboolpr (char * tree, FILE *outf)
{
    char *fn = "Bboolpr";
    API_ENTRY;

    /* Do standard validation */
    if (NULL==tree)
    {
        _Fset_error_msg(BEINVAL, "Evaluation tree cannot be NULL!");
        return;
    }
    /* check output file */
    if (NULL==outf)
    {
        _Fset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return;
    }

    _Bboolpr (tree, outf);
    /* put newline at the end. */
    fprintf(outf, "\n");
}

/**
 * String macro for CBadd
 * @param p_ub
 * @param bfldid
 * @param buf
 * @return
 */
public int Badds (UBFH *p_ub, BFLDID bfldid, char *buf)
{
    return CBadd(p_ub, bfldid, buf, 0, BFLD_STRING);
}

/**
 * String macro for CBchg
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @return
 */
public int Bchgs (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf)
{
    return CBchg(p_ub, bfldid, occ, buf, 0, BFLD_STRING);
}

/**
 * String macro for CBgets
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @return
 */
public int Bgets (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf)
{
    return CBget(p_ub, bfldid, occ, buf, 0, BFLD_STRING);
}

/**
 * String macro for CBgetalloc
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param extralen
 * @return
 */
public char * Bgetsa (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen)
{
    return CBgetalloc(p_ub, bfldid, occ, BFLD_STRING, extralen);
}

/**
 * String macro for CBfind
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return
 */
public char * Bfinds (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    return CBfind(p_ub, bfldid, occ, 0, BFLD_STRING);
}

/**
 * API entry for Bread
 * @param p_ub
 * @param inf
 * @return
 */
public int Bread  (UBFH * p_ub, FILE * inf)
{
    char *fn = "Bread";
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    /* check output file */
    if (NULL==inf)
    {
        _Fset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return FAIL;
    }

    return _Bread (p_ub, inf);
}

/**
 * API entry for Bwrite
 * @param p_ub
 * @param outf
 * @return
 */
public int Bwrite (UBFH *p_ub, FILE * outf)
{
    char *fn = "_Bwrite";
    API_ENTRY;

    /* Do standard validation */
    if (SUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return FAIL;
    }
    /* check output file */
    if (NULL==outf)
    {
        _Fset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return FAIL;
    }

    return _Bwrite (p_ub, outf);
}


/**
 * Get the length of the field
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return 
 */
public int Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    API_ENTRY;

    if (SUCCEED!=validate_entry(p_ub, bfldid,occ,0))
    {
        UBF_LOG(log_warn, "Bget: arguments fail!");
        return FAIL; /* <<<< RETURN HERE! */
    }

    return _Blen (p_ub, bfldid, occ);
}

/**
 * Set callback function that can be used in expressions for long value.
 * @param funcname
 * @param functionPtr
 * @return SUCCEED/FAIL
 */
public int Bboolsetcbf (char *funcname, 
            long (*functionPtr)(UBFH *p_ub, char *funcname))
{
    API_ENTRY;
    {
/* Lock the region */
    MUTEX_LOCK;
    {
        int ret;
        ret = _Bboolsetcbf (funcname, functionPtr);
        MUTEX_UNLOCK;
        return ret;
    }
    }
}

