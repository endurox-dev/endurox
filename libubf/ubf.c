/**
 * @brief UBF library
 *   The emulator of UBF library
 *   Enduro Execution Library
 *   Debug ideas:
 *   - on API entry take copy of FB
 *   - on API exit make diff of the FB memory. Also diff the memory usage
 *   - provide the information about count of bytes from source element added
 *   - on exit, dump the first BFLDID element address
 *
 * @file ubf.c
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
#include <expr.h>
#include <thlock.h>
#include <typed_view.h>
#include <ubfdb.h>
#include <ubfutil.h>

#include "expluginbase.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define API_ENTRY {ndrx_Bunset_error(); \
    if (!M_init) { \
	MUTEX_LOCK; \
        UBF_DBG_INIT(("UBF", UBFDEBUGLEV));\
        M_init=EXTRUE;\
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
                ndrx_Bset_error_fmt(BTYPERR, "Invalid user type %d", X);\
                Y;\
            }\
            if (IS_TYPE_COMPLEX(X)) \
            { \
                ndrx_Bset_error_fmt(BEBADOP, "Invalid user type %d", X);\
                Y;\
            }

#define VIEW_ENTRY if (EXSUCCEED!=ndrx_view_init()) {EXFAIL_OUT(ret);}
#define VIEW_ENTRY2 if (EXSUCCEED!=ndrx_view_init()) {ret=-2; goto out;}

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
static volatile int M_init = EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Initialize bisubf buffer
 * Size already includes last BBADFLDID! Which always must stay as BBADFLDID!
 */
expublic int Binit (UBFH * p_ub, BFLDLEN len)
{
    int ret=EXSUCCEED;
    UBF_header_t *ubf_h = (UBF_header_t *)p_ub;
    char *p;
    
    UBF_LOG(log_debug, "Binit p_ub=%p len=%d", p_ub, len);
            
    if (NULL==p_ub)
    {
        /* Null buffer */
        ndrx_Bset_error_msg(BNOTFLD, "ptr to UBFH is NULL");
        ret=EXFAIL;
    }
    else if (len < sizeof(UBF_header_t))
    {
        ndrx_Bset_error_fmt(BNOSPACE, "Minimum: %d, but got: %d",
                                    sizeof(UBF_header_t), len);
        ret=EXFAIL;
    }
    else
    {
#ifdef UBF_API_DEBUG
        /* randomize memory... */
        int i;
        char *buf = (char *)p_ub;

        srand ( time(NULL) );
        for (i=0; i<len; i++)
        {
            buf[i] = (char)(rand() % 255);
        }

#endif
        /* Initialise buffer */
        /* the last element bfldid is set to zero */
        memset((char *)p_ub, 0, sizeof(UBF_header_t)); /* Do we need all to be set to 0? */
        ubf_h->version = UBF_VERSION; /* Reset options to (all disabled) */
        ubf_h->buffer_type = 0; /* not used currently */
        memcpy(ubf_h->magic, UBF_MAGIC, sizeof(UBF_MAGIC)-1);
        ubf_h->buf_len = len;
        ubf_h->opts = 0; 
        ubf_h->bytes_used = sizeof(UBF_header_t) - FF_USED_BYTES;
	
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
expublic int Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len)
{
    API_ENTRY;
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "Badd: arguments fail!");
        return EXFAIL;
    }
    
    return ndrx_Badd (p_ub, bfldid, buf, len, NULL, NULL);
}

/**
 * Fast path adding of data to buffer, by providing reference to last field added
 * ending. the next_fld shall be used only if adding occurrences of the same field
 * to buffer. Or you are sure that you are adding new fldid, which is greater than
 * fldid in last add operation
 * @param p_ub UBF buffer
 * @param bfldid field to add
 * @param buf value to add
 * @param len len of value (if needed)
 * @param next_fld saved from previous call. Initially memset to 0
 * @return EXSUCCEED/EXFAIL
 */
expublic int Baddfast (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len, 
	Bfld_loc_info_t *next_fld)
{
    API_ENTRY;
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "Badd: arguments fail!");
        return EXFAIL;
    }
    
    if (NULL==next_fld)
    {
        ndrx_Bset_error_msg(BEINVAL, "next_fld must not be NULL!");
        return EXFAIL;
    }
    
    return ndrx_Badd (p_ub, bfldid, buf, len, NULL, next_fld);
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
expublic int Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN * buflen)
{
    API_ENTRY;

    if (EXSUCCEED!=validate_entry(p_ub, bfldid,occ,0))
    {
        UBF_LOG(log_warn, "Bget: arguments fail!");
        return EXFAIL; /* <<<< RETURN HERE! */
    }

    return ndrx_Bget (p_ub, bfldid, occ, buf, buflen);
}

/**
 * Delete specific field from buffer
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return
 */
expublic int Bdel (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ)
{
    int ret=EXSUCCEED;
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
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bdel: arguments fail!");
        ret=EXFAIL;
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
                                    ndrx_Bfname_int(bfldid), bfldid, bfldid);
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
#if 0
        /* Ensure that we reset last elements... So that we do not get
         * used elements
         */
        UBF_LOG(log_debug, "resetting: %p to 0 - %d bytes",
                            last+1, 0, remove_size);
        memset(last+1, 0, remove_size);
#endif
    }
    else
    {
        ndrx_Bset_error(BNOTPRES);
        ret=EXFAIL;
    }

/***************************************** DEBUG *******************************/
#ifdef UBF_API_DEBUG
    __dbg_olduse = __p_ub_copy->bytes_used;
    __dbg_newuse = hdr->bytes_used;

    /* Do bellow to print out end element (last) of the array - should be bbadfldid */
    __dbg_p_org = (char *)__p_ub_copy;
    __dbg_p_org+= (__p_ub_copy->bytes_used - FF_USED_BYTES);

    __dbg_p_new = (char *)hdr;
    __dbg_p_new+= (hdr->bytes_used - FF_USED_BYTES);

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
    __dbg_p_org-= FF_USED_BYTES;
    __dbg_p_new-= FF_USED_BYTES;
    __dbg_fldptr_org = (int *)__dbg_p_org;
    __dbg_fldptr_new = (int *)__dbg_p_new;
    UBF_LOG(log_debug, "Bdel: last %d bytes of data\n org=%p new %p",
                          FF_USED_BYTES, *__dbg_fldptr_org, *__dbg_fldptr_new);
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
expublic int Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len)
{
    API_ENTRY; /* Standard initialization */

    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bchg: arguments fail!");
        return EXFAIL; /* <<<< RETURN HERE! */
    }

    return ndrx_Bchg(p_ub, bfldid, occ, buf, len, NULL, EXFALSE);
}

/**
 * Return BFLDID from name!
 */
expublic BFLDID Bfldid (char *fldnm)
{
    UBF_field_def_t *p_fld=NULL;

    API_ENTRY;
    
    if (EXSUCCEED!=ndrx_prepare_type_tables())
    {
        return BBADFLDID;
    }
    
    /* Now we can try to do lookup */
    p_fld = ndrx_fldnmhash_get(fldnm);

    if (NULL!=p_fld)
    {
        return p_fld->bfldid;
    }
    else if (NULL!=ndrx_G_ubf_db)
    {
        int ret;
        /* lookup into db */
        ret=ndrx_ubfdb_Bflddbid(fldnm);
        return ret;
    }
    else
    {
        ndrx_Bset_error(BBADNAME);
        return BBADFLDID;
    }
}
/**
 * Return field name from given id
 */
expublic char * Bfname (BFLDID bfldid)
{
    UBF_field_def_t *p_fld;
    API_ENTRY;
    
    if (bfldid < 0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "bfldid (%d) < 0", (int)bfldid);
        goto out;
    }

    if (EXSUCCEED!=ndrx_prepare_type_tables())
    {
        goto out;
    }

    /* Now try to find the data! */
    p_fld = _bfldidhash_get(bfldid);
    
    if (NULL==p_fld)
    {
        if (NULL!=ndrx_G_ubf_db)
        {
            return ndrx_ubfdb_Bflddbname(bfldid);
        }
        else
        {
            ndrx_Bset_error(BBADFLD);
            goto out;
        }
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
expublic char * Bfind (UBFH * p_ub, BFLDID bfldid,
                                        BFLDOCC occ, BFLDLEN * p_len)
{
    API_ENTRY;

    UBF_LOG(log_debug, "Bfind: bfldid: %d occ: %hd", bfldid, occ);
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "Bdel: arguments fail!");
        return NULL;
    }

    return ndrx_Bfind(p_ub, bfldid, occ, p_len, NULL);
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
expublic int CBadd (UBFH *p_ub, BFLDID bfldid, char * buf,
                                BFLDLEN len, int usrtype)
{
    int ret=EXSUCCEED;
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
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "CBadd: arguments fail!");
        return EXFAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return EXFAIL);
    
    /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(to_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", to_type);
        return EXFAIL;
    }

    /* if types are the same then do direct call */
    if (usrtype==to_type)
    {
        UBF_LOG(log_debug, "CBadd: the same types - direct call!");
        return ndrx_Badd(p_ub, bfldid, buf, len, NULL, NULL); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */

    /* Allocate the buffer dynamically */
    if (NULL==(p=ndrx_ubf_get_cbuf(usrtype, to_type, tmp_buf, buf, len, &alloc_buf, 
                                &cvn_len, CB_MODE_DEFAULT, 0)))
    {
        UBF_LOG(log_error, "%s: Malloc failed!", fn);
        return EXFAIL; /* <<<< RETURN!!!! */
    }

    cvn_buf = ndrx_ubf_convert(usrtype, CNV_DIR_IN, buf, len,
                        to_type, p, &cvn_len);

    if (NULL!=cvn_buf)
    {
        ret=ndrx_Badd (p_ub, bfldid, cvn_buf, cvn_len, NULL, NULL);
    }
    else
    {
        UBF_LOG(log_error, "%s: failed to convert data!", fn);
        /* Error should be provided by conversation function */
        ret=EXFAIL;
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
expublic int CBchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                                char * buf, BFLDLEN len, int usrtype)
{
    int ret=EXSUCCEED;
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

    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "CBchg: arguments fail!");
        return EXFAIL; /* <<<< RETURN HERE! */
    }
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return EXFAIL);
    
    
        /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(to_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", to_type);
        return EXFAIL;
    }

    /* if types are the same then do direct call */
    if (usrtype==to_type)
    {
        UBF_LOG(log_debug, "CBchg: the same types - direct call!");
        return ndrx_Bchg(p_ub, bfldid, occ, buf, len, NULL, EXFALSE); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */
    
    /* Allocate the buffer dynamically */
    if (NULL==(p=ndrx_ubf_get_cbuf(usrtype, to_type, tmp_buf, buf, len, &alloc_buf, &cvn_len,
                                CB_MODE_DEFAULT, 0)))
    {
        UBF_LOG(log_error, "CBchg: Malloc failed!");
        return EXFAIL; /* <<<< RETURN!!!! */
    }

    cvn_buf = ndrx_ubf_convert(usrtype, CNV_DIR_IN, buf, len,
                        to_type, p, &cvn_len);

    if (NULL!=cvn_buf)
    {
        ret=Bchg (p_ub, bfldid, occ, cvn_buf, cvn_len);
    }
    else
    {
        UBF_LOG(log_error, "CBchg: failed to convert data!");
        /* Error should be provided by conversation function */
        ret=EXFAIL;
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
expublic int CBget (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ ,
                                char *buf, BFLDLEN *len, int usrtype)
{
    int ret=EXSUCCEED;
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
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "CBget: arguments fail!");
        return EXFAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return EXFAIL);
    
        /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(from_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", from_type);
        return EXFAIL;
    }

    /* if types are the same then do direct call */
    if (usrtype==from_type)
    {
        UBF_LOG(log_debug, "CBget: the same types - direct call!");
        return Bget(p_ub, bfldid, occ, buf, len); /* <<<< RETURN!!! */
    }
    /* if types are not the same then go the long way... */
    
    fb_data=ndrx_Bfind (p_ub, bfldid, occ, &tmp_len, NULL);

    if (NULL!=fb_data)
    {

        cvn_buf = ndrx_ubf_convert(from_type, CNV_DIR_OUT, fb_data, tmp_len,
                                    usrtype, buf, len);
        if (NULL==cvn_buf)
        {
            UBF_LOG(log_error, "CBget: failed to convert data!");
            /* Error should be provided by conversation function */
            ret=EXFAIL;
        }
    }
    else
    {
        UBF_LOG(log_error, "CBget: Field not present!");
        ret=EXFAIL;
    }
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (EXSUCCEED==ret)
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
expublic int Bcpy (UBFH * p_ub_dst, UBFH * p_ub_src)
{
    int ret=EXSUCCEED;
    UBF_header_t *src_h = (UBF_header_t *)p_ub_src;
    UBF_header_t *dst_h = (UBF_header_t *)p_ub_dst;
    int dst_buf_len;
    API_ENTRY;

    UBF_LOG(log_debug, "Bcpy: About to copy from FB=%p to FB=%p",
                                    p_ub_src, p_ub_dst);

    if (EXSUCCEED==ret && NULL==p_ub_src)
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ub_src is NULL!");
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && NULL==p_ub_dst)
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ub_dst is NULL!");
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && 0!=strncmp(src_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "source buffer magic failed!");
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && 0!=strncmp(dst_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "destination buffer magic failed!");
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret && dst_h->buf_len < src_h->bytes_used)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "Destination buffer too short. "
                                    "Dest buf len: %d source buf bytes used: %d",
                                    dst_h->buf_len, src_h->bytes_used);
        ret=EXFAIL;
    }

    if (EXSUCCEED==ret)
    {
        /* save some original characteristics of dest buffer */
        dst_buf_len = dst_h->buf_len;
        /*memset(p_ub_dst, 0, dst_h->bytes_used);*/
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
        
        dst_h->cache_ptr_off = src_h->cache_ptr_off;
        dst_h->cache_ubf_off = src_h->cache_ubf_off;
        dst_h->cache_view_off = src_h->cache_view_off;
    
        
    }
    UBF_LOG(log_debug, "Bcpy: return %d", ret);
    
    return ret;
}

/**
 * Used buffer amount.
 * @param p_ub
 * @return
 */
expublic long Bused (UBFH *p_ub)
{
    long ret=EXFAIL;
    char fn[] = "Bused";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFALSE;
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
expublic BFLDID Bmkfldid (int fldtype, BFLDID bfldid)
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
expublic BFLDOCC Boccur (UBFH * p_ub, BFLDID bfldid)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "_Boccur: arguments fail!");
        return EXFAIL;
    }

    return ndrx_Boccur (p_ub, bfldid);
}

/**
 * Check the field presence
 * API version - no error checking.
 */
expublic int Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    API_ENTRY;
    
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "_Bpres: arguments fail!");
        return EXFALSE;
    }

    return _Bpres(p_ub, bfldid, occ);
}

/**
 * Public version of Bfldtype
 * @param bfldid
 * @return
 */
expublic int Bfldtype (BFLDID bfldid)
{
    return bfldid >> EFFECTIVE_BITS;
}

expublic char * Bboolco (char * expr)
{
    API_ENTRY;
    {
    /* Lock the region */
    MUTEX_LOCK;
    {
        char *ret;
        ret = ndrx_Bboolco (expr);
        MUTEX_UNLOCK;
        return ret;
    }
    }
}

/**
 * Evaluate boolean expression
 * @param[in] p_ub UBF buffer
 * @param[in] tree compiled expression tree
 * @return 0 - false, 1 - true
 */
expublic int Bboolev (UBFH * p_ub, char *tree)
{
    API_ENTRY;
    
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    
    if (NULL==tree)
    {
         ndrx_Bset_error_fmt(BEINVAL, "tree is NULL");
         return EXFAIL;
    }
    
    return ndrx_Bboolev (p_ub, tree);
}

/**
 * This function is not tested hardly. But it should work fine.
 * @param p_ub
 * @param tree
 * @return
 */
expublic double Bfloatev (UBFH * p_ub, char *tree)
{
    API_ENTRY;
    
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    
    if (NULL==tree)
    {
         ndrx_Bset_error_fmt(BEINVAL, "tree is NULL");
         return EXFAIL;
    }
    
    return ndrx_Bfloatev (p_ub, tree);
}

expublic void Btreefree (char *tree)
{
    API_ENTRY;
    ndrx_Btreefree (tree);
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
expublic int  Bnext(UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len)
{
    char fn[] = "Bnext";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    /* Seems this caused tricks for multi threading.*/
    static __thread Bnext_state_t state;

    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }
    else if (NULL==bfldid || NULL==occ)
    {
        ndrx_Bset_error_msg(BEINVAL, "Bnext: ptr of bfldid or occ is NULL!");
        return EXFAIL;
    }
    else if (*bfldid != BFIRSTFLDID && state.p_ub != p_ub)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: Different buffer [state: %p used: %p] "
                                    "used for different state", fn,
                                    state.p_ub, p_ub);
        return EXFAIL;
    }
    else if (*bfldid != BFIRSTFLDID && state.size!=hdr->bytes_used)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: Buffer size changed [state: %d used: %d] "
                                    "from last search", fn,
                                    state.size, hdr->bytes_used);
        return EXFAIL;
    }
    else
    {
        if (*bfldid == BFIRSTFLDID)
        {
            memset(&state, 0, sizeof(state));
        }

        return ndrx_Bnext(&state, p_ub, bfldid, occ, buf, len, NULL);
    }
}

/**
 * API entry for Bproj
 * @param p_ub
 * @param fldlist
 * @return SUCCEED/FAIL
 */
expublic int Bproj (UBFH * p_ub, BFLDID * fldlist)
{
    char fn[] = "Bproj";
    int processed;
    API_ENTRY;
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }
    else
    {
        /* Call the implementation */
        return ndrx_Bproj (p_ub, fldlist, PROJ_MODE_PROJ, &processed);
    }
}

/**
 * Projection copy on 
 * @param p_ub_dst
 * @param p_ub_src
 * @param fldlist
 * @return
 */
expublic int Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src,
                                    BFLDID * fldlist)
{
    char fn[] = "Bprojcpy";
    API_ENTRY;
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", fn);
        ndrx_Bappend_error_msg("(Bprojcpy: arguments fail for src buf!)");
        return EXFAIL;
    }
    else if (EXSUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", fn);
        ndrx_Bappend_error_msg("(Bprojcpy: arguments fail for dst buf!)");
        return EXFAIL;
    }
    else
    {
        /* Call the implementation */
        return ndrx_Bprojcpy (p_ub_dst, p_ub_src, fldlist);
    }
}

/**
 * Will not implement.
 * @param
 * @param
 * @return
 */
expublic int Bindex (UBFH * p_ub, BFLDOCC occ)
{
    UBF_LOG(log_debug, "Bindex: Not implemented - ignore!");
    return EXSUCCEED;
}

/**
 * Not implemented & not planning to do so
 * @param p_ub
 * @return 0 - number of fields indexed.
 */
expublic BFLDOCC Bunindex (UBFH * p_ub)
{
    UBF_LOG(log_debug, "Bunindex: Not implemented - ignore!");
    return 0;
}

/**
 * Not using indexes (Not implemented & not supported)
 * @param p_ub
 * @return 0 - size used for index
 */
expublic long Bidxused (UBFH * p_ub)
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
expublic int Brstrindex (UBFH * p_ub, BFLDOCC occ)
{
    UBF_LOG(log_debug, "Brstrindex: Not implemented - ignore!");
    return EXSUCCEED;
}

/**
 * Will re-use _Bproj for this
 * @param p_ub
 * @param bfldid
 * @return
 */
expublic int Bdelall (UBFH *p_ub, BFLDID bfldid)
{
    int ret=EXSUCCEED;
    char fn[] = "Bdelall";
    int processed;
    API_ENTRY;
    
    UBF_LOG(log_warn, "%s: enter", fn);
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bproj (p_ub, &bfldid, PROJ_MODE_DELALL, &processed);
    }

    if (EXSUCCEED==ret && 0==processed)
    {
        /* Set error that none of fields have been deleted */
        ret=EXFAIL;
        ndrx_Bset_error_msg(BNOTPRES, "No fields have been deleted");
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
expublic int Bdelete (UBFH *p_ub, BFLDID *fldlist)
{
    int ret=EXSUCCEED;
    char fn[] = "Bdelete";
    int processed;
    API_ENTRY;

    UBF_LOG(log_warn, "%s: enter", fn);
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bproj (p_ub, fldlist, PROJ_MODE_DELETE, &processed);
    }

    if (EXSUCCEED==ret && 0==processed)
    {
        /* Set error that none of fields have been deleted */
        ret=EXFAIL;
        ndrx_Bset_error_msg(BNOTPRES, "No fields have been deleted");
    }
    
    UBF_LOG(log_warn, "%s: return %d", fn, ret);
    return ret;
}

/**
 * Returns field number out of field ID
 * @param bfldid
 * @return 
 */
expublic BFLDOCC Bfldno (BFLDID bfldid)
{
    UBF_LOG(log_debug, "Bfldno: Mask: %d", EFFECTIVE_BITS_MASK);
    return (BFLDOCC) bfldid & EFFECTIVE_BITS_MASK;
}

/**
 * returns TRUE if buffer is bisubf
 * @param p_ub - FB to test.
 * @return TRUE/FALSE
 */
expublic int Bisubf (UBFH *p_ub)
{
    int ret=EXTRUE;
    char fn[] = "Bisubf";
    API_ENTRY;
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFALSE;
        ndrx_Bunset_error();
    }
    
    return ret;
}

/**
 * Returns free buffer space that could be used by fields.
 * @param p_Fb
 * @return -1 on error or free buffer space
 */
expublic long Bunused (UBFH *p_ub)
{
    long ret=EXFAIL;
    char fn[] = "Bunused";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFALSE;
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
expublic long Bsizeof (UBFH *p_ub)
{
    long ret=EXFAIL;
    char fn[] = "Bsizeof";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;

    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFALSE;
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
expublic char * Btype (BFLDID bfldid)
{
    int type = bfldid >> EFFECTIVE_BITS;
    API_ENTRY;

    if (IS_TYPE_INVALID(type))
    {
        ndrx_Bset_error_fmt(BTYPERR, "Unknown type number %d", type);
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
expublic int Bfree (UBFH *p_ub)
{
    int ret=EXSUCCEED;
    char fn[] = "Bfree";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    API_ENTRY;
    
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        ret=EXFAIL;
    }
    else
    {
        memset(hdr->magic, 0, UBF_MAGIC_SIZE);
        NDRX_FREE(p_ub);
    }
    
    return ret;
}

/**
 * API function for Bupdate.
 * @param p_ub_dst - dest buffer to update
 * @param p_ub_src - source buffer to take values out from.
 * @return SUCEED/FAIL
 */
expublic int Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    int ret=EXSUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", __func__);
    if (EXSUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", __func__);
        ndrx_Bappend_error_msg("(Bupdate: arguments fail for src buf!)");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", __func__);
        ndrx_Bappend_error_msg("(Bupdate: arguments fail for dst buf!)");
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bupdate (p_ub_dst, p_ub_src);
    }
    UBF_LOG(log_debug, "Return %s %d", __func__, ret);
    return ret;
}

/**
 * Concat two buffers
 * @param p_ub_dst
 * @param p_ub_src
 * @return 
 */
expublic int Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src)
{
    int ret=EXSUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", __func__);
    if (EXSUCCEED!=validate_entry(p_ub_src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", __func__);
        ndrx_Bappend_error_msg("(Bconcat: arguments fail for src buf!)");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=validate_entry(p_ub_dst, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dst buf!", __func__);
        ndrx_Bappend_error_msg("(Bconcat: arguments fail for dst buf!)");
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bconcat (p_ub_dst, p_ub_src);
    }
    UBF_LOG(log_debug, "Return %s %d", __func__, ret);
    return ret;
}

/**
 * CBfind API entry
 * @return NULL/ptr to value
 */
expublic char * CBfind (UBFH * p_ub, 
                    BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype)
{
    int bfldid_type = (bfldid>>EFFECTIVE_BITS);
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d occ: %hd", __func__, bfldid, occ);

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return NULL;
    }
    
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return NULL);
    
    /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(bfldid_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", bfldid_type);
        return NULL;
    }

    /* Call the implementation */
    return ndrx_CBfind (p_ub, bfldid, occ, len, usrtype, CB_MODE_TEMPSPACE, 0);
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
expublic char * CBgetalloc (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC occ, int usrtype, BFLDLEN *extralen)
{
    char *ret=NULL;
    char fn[] = "CBgetalloc";
    int bfldid_type = (bfldid>>EFFECTIVE_BITS);
    API_ENTRY;
    UBF_LOG(log_debug, "%s: bfldid: %d occ: %hd", fn, bfldid, occ);

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "CBgetalloc: arguments fail!");
        return NULL;
    }
    
    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return NULL);
    
    /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(bfldid_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", bfldid_type);
        return NULL;
    }

    /* Call the implementation */
    ret=ndrx_CBfind (p_ub, bfldid, occ, extralen, usrtype, CB_MODE_ALLOC, 
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
expublic BFLDOCC Bfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len)
{
    char *fn = "Bfindocc";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    if (NULL==buf)
    {
         ndrx_Bset_error_fmt(BEINVAL, "buf is NULL");
         return EXFAIL;
    }
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }


    /* validate user type */

    /* Call the implementation */
    return ndrx_Bfindocc (p_ub, bfldid, buf, len);
}

/**
 * API entry for CBfindocc
 * @param p_ub
 * @param bfldid
 * @param buf
 * @param len
 * @return
 */
expublic BFLDOCC CBfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len, int usrtype)
{
    char *fn = "CBfindocc";
    int bfldid_type = (bfldid>>EFFECTIVE_BITS);
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    if (NULL==buf)
    {
         ndrx_Bset_error_fmt(BEINVAL, "buf is NULL");
         return EXFAIL;
    }
 
    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }

    /* validate user specified type */
    VALIDATE_USER_TYPE(usrtype, return EXFAIL);
    
    /* validate that type is not complex */
    if (IS_TYPE_COMPLEX(bfldid_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported bfldid type %d", bfldid_type);
        return EXFAIL;
    }

    /* Call the implementation */
    return ndrx_CBfindocc (p_ub, bfldid, buf, len, usrtype);
}

/**
 * API version of Bgetalloc
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param len
 * @return NULL (on failure)/ ptr on success.
 */
expublic char * Bgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen)
{
    char *fn = "Bgetalloc";
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, occ, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return NULL;
    }

    /* Call the implementation */
    return ndrx_Bgetalloc (p_ub, bfldid, occ, extralen);
}

/**
 * Bfindlast API entry
 * @return NULL/ptr to value
 */
expublic char * Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC *occ, BFLDLEN *len)
{
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", __func__, bfldid);

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return NULL;
    }

    /* Call the implementation */
    return ndrx_Bfindlast (p_ub, bfldid, occ, len);
}

/**
 * Bfindlast API entry
 * @return NULL/ptr to value
 */
expublic int Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len)
{
    API_ENTRY;

    UBF_LOG(log_debug, "%s: bfldid: %d", __func__, bfldid);

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, bfldid, 0, 0))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }

    /* Call the implementation */
    return ndrx_Bgetlast (p_ub, bfldid, occ, buf, len);
}


/**
 * API function for Bfprint
 * @param p_ub
 * @param outf
 * @return
 */
expublic int Bfprint (UBFH *p_ub, FILE * outf)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==outf)
    {
        ndrx_Bset_error_msg(BEINVAL, "output file cannot be NULL!");
        return EXFAIL;
    }

    return ndrx_Bfprint (p_ub, outf, NULL, NULL, 0);
}

/**
 * Print UBF buffer by using callback function for data outputting
 * @param p_ub UBF buffer to print
 * @param dataptr1 user pointer passed back to callback
 * @param p_writef callback function for data output. Note that 'buffer' argument
 *  is allocated and deallocated by Bfprintcb it self. The string is zero byte
 *  terminated. The dataptr1 passed to function is forwarded to callback func.
 *  *datalen* includes the EOS byte. if do_write is set to TRUE, the data in buffer
 *  is written to output file.
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bfprintcb (UBFH *p_ub, 
        ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==p_writef)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_writef callback cannot be NULL!");
        return EXFAIL;
    }

    return ndrx_Bfprint (p_ub, NULL, p_writef, dataptr1, 0);
}

/**
 * API function for Bprint
 * uses _Bfprint in core with stdout.
 * @param p_ub
 * @param outf
 * @return
 */
expublic int Bprint (UBFH *p_ub)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }

    return ndrx_Bfprint (p_ub, stdout, NULL, NULL, 0);
}

/**
 * Print UBF buffer to logger
 * @param lev logging level to start print at
 * @param title title of the dump
 * @param p_ub UBF buffer
 */
expublic void ndrx_tplogprintubf(int lev, char *title, UBFH *p_ub)
{
    API_ENTRY;
    ndrx_debug_t * dbg = debug_get_tp_ptr();
    
    if (dbg->level>=lev)
    {

#ifdef UBF_API_DEBUG
        UBF_header_t *buf = (UBF_header_t *)p_ub;
        UBF_LOG(lev, "buffer_type = [%x]", (unsigned)buf->buffer_type);
        UBF_LOG(lev, "version = [%x]", (unsigned)buf->buffer_type);
        UBF_LOG(lev, "magic = [%c%c%c%c]", buf->magic[0], buf->magic[1], buf->magic[2], buf->magic[3]);
        UBF_LOG(lev, "cache_long_off = [%x]", buf->cache_long_off);
        UBF_LOG(lev, "cache_char_off = [%x]", buf->cache_char_off);
        UBF_LOG(lev, "cache_float_off = [%x]", buf->cache_float_off);
        UBF_LOG(lev, "cache_double_off = [%x]", buf->cache_double_off);
        UBF_LOG(lev, "cache_string_off = [%x]", buf->cache_string_off);
        UBF_LOG(lev, "buf_len = [%x]", buf->buf_len);
        UBF_LOG(lev, "bytes_used = [%x]", buf->bytes_used);
        UBF_LOG(lev, "bfldid = [%x]", buf->bfldid);
#endif

        TP_LOG(lev, "%s", title);
        /* Do standard validation */
        if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
        {
            UBF_LOG(log_warn, "arguments fail - nothing to log");
        }
        else
        {
            /* use plugin callback */
            ndrx_Bfprint (p_ub, dbg->dbg_f_ptr, ndrx_G_plugins.p_ndrx_tplogprintubf_hook, NULL, 0);
        }
    }
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
expublic char * Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len)
{
    char *fn = "Btypcvt";
    dtype_str_t * dtype;
    API_ENTRY;


    if (NULL==from_buf)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s:from buf cannot be NULL!", fn);
        return NULL; /* <<< RETURN! */
    }

    if (IS_TYPE_INVALID(from_type))
    {
        ndrx_Bset_error_fmt(BTYPERR, "%s: Invalid from_type %d", fn, from_type);
        return NULL; /* <<< RETURN! */
    }
    
    if (IS_TYPE_COMPLEX(from_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported from_type type %d", from_type);
        return NULL;
    }
    
    if (IS_TYPE_INVALID(to_type))
    {
        ndrx_Bset_error_fmt(BTYPERR, "%s: Invalid from_type %d", fn, to_type);
        return NULL; /* <<< RETURN! */
    }
    
    if (IS_TYPE_COMPLEX(to_type))
    {
        ndrx_Bset_error_fmt(BEBADOP, "Unsupported to_type type %d", to_type);
        return NULL;
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
    return ndrx_Btypcvt(to_len, to_type, from_buf, from_type, from_len);
}

/**
 * API entry for Bextread
 * @param p_ub - bisubf buffer
 * @param inf - file to read from 
 * @return SUCCEED/FAIL
 */
expublic int Bextread (UBFH * p_ub, FILE *inf)
{
    char *fn = "Bextread";
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==inf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return EXFAIL;
    }
    return ndrx_Bextread (p_ub, inf, NULL, NULL);
}

/**
 * Read UBF buffer from printed/text format UBF.
 * @param p_ub ptr to UBF buffer
 * @param p_readf callback to read function. Function shall provide back data
 *  to ndrx_Bextread(). The reading must be feed line by line. The input line
 *  must be terminated with EOS. The buffer size which accepts the input line
 *  is set by `bufsz'. The function receives forwarded \p dataptr1 argument.
 *  Once EOF is reached, the callback shall return read of 0 bytes. Otherwise
 *  it must return number of bytes read, including EOS.
 * @param dataptr1 option user pointer forwarded to \p p_readf.
 * @return SUCCEED/FAIL
 */
expublic int Bextreadcb (UBFH * p_ub, 
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==p_readf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Callback function cannot be NULL!");
        return EXFAIL;
    }
    
    return ndrx_Bextread (p_ub, NULL, p_readf, dataptr1);
}

/**
 * API entry for Bboolpr
 * @param tree - compiled expression tree
 * @param outf - file to print to tree
 */
expublic void Bboolpr (char * tree, FILE *outf)
{
    API_ENTRY;

    /* Do standard validation */
    if (NULL==tree)
    {
        ndrx_Bset_error_msg(BEINVAL, "Evaluation tree cannot be NULL!");
        return;
    }
    /* check output file */
    if (NULL==outf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return;
    }

    ndrx_Bboolpr (tree, outf, NULL, NULL);
    /* put newline at the end. */
    fprintf(outf, "\n");
}

/**
 * API entry for Bboolpr. Print boolean expression to callback function.
 * @param[in] tree compiled boolean expression tree to print
 * @param[in] p_writef callback function to which print the buffer. The datalen
 *  includes the EOS symbol.
 * @param[in] dataptr1 user pointer to forward to \p p_writef function
 */
expublic void Bboolprcb (char * tree, 
        int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1)
{
    API_ENTRY;

    /* Do standard validation */
    if (NULL==tree)
    {
        ndrx_Bset_error_msg(BEINVAL, "Evaluation tree cannot be NULL!");
        return;
    }
    /* check output file */
    if (NULL==p_writef)
    {
        ndrx_Bset_error_msg(BEINVAL, "Input callback function p_writef "
                "cannot be NULL!");
        return;
    }

    ndrx_Bboolpr (tree, NULL, p_writef, dataptr1);
    /* put newline at the end. */
    p_writef("\n", 2, dataptr1);
}

/**
 * String macro for CBadd
 * @param p_ub
 * @param bfldid
 * @param buf
 * @return
 */
expublic int Badds (UBFH *p_ub, BFLDID bfldid, char *buf)
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
expublic int Bchgs (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf)
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
expublic int Bgets (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf)
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
expublic char * Bgetsa (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen)
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
expublic char * Bfinds (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    return CBfind(p_ub, bfldid, occ, 0, BFLD_STRING);
}

/**
 * API entry for Bread. Read binary UBF buffer form input stream
 * @param p_ub UBF buffer
 * @param inf input file stream
 * @return EXSUCCEED(0)/EXFAIL(-1)
 */
expublic int Bread (UBFH * p_ub, FILE * inf)
{
    char *fn = "Bread";
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==inf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Input file cannot be NULL!");
        return EXFAIL;
    }

    return ndrx_Bread (p_ub, inf, NULL, NULL);
}

/**
 * API entry for Bwrite. Write binary buffer to output stream.
 * @param p_ub UBF buffer to read from
 * @param outf output stream to write to
 * @return EXSUCCEED/EXFAIL, Berror set in case of problems.
 */
expublic int Bwrite (UBFH *p_ub, FILE * outf)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==outf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Output file cannot be NULL!");
        return EXFAIL;
    }

    return ndrx_Bwrite (p_ub, outf, NULL, NULL);
}

/**
 * Read UBF buffer from callback function. The format is binary, platform specific.
 * @param p_ub UBF buffer to write to
 * @param[in] p_readf is a callback function which is used to read data from.
 *  The special requirement of this callback is that, The number of bytes which
 *  must be read are specified in `bufsz', otherwise error will occur. In case of
 *  callback function error by it self, -1 can be returned in that case. `buffer'
 *  is buffer where read data must be put in. `dataptr1` is forwarded from \p
 *  dataptr1 argument.
 * @param[in] dataptr1 is value which is forwarded to \p p_writef. This is optional
 *  and may contain NULL.
 * @return EXSUCCEED/EXFAIL, Berror set in case of problems.
 */
expublic int Breadcb (UBFH * p_ub, 
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==p_readf)
    {
        ndrx_Bset_error_msg(BEINVAL, "Read callback function must not be NULL!");
        return EXFAIL;
    }

    return ndrx_Bread (p_ub, NULL, p_readf, dataptr1);
}

/**
 * Write UBF buffer to callback function
 * @param p_ub UBF buffer to read from
 * @param[in] p_writef is a callback function which receives portions of UBF buffer.
 *  once function is called, all `bufsz' bytes must be accepted. And function
 *  at success must return the number of bytes written (i.e. the same `bufsz')
 *  other cases are considered as errro. The `buffer' is pointer where buffer
 *  bytes are stored. The `dataptr1' is a value forwarded from the Bwritecb()
 *  arguments.
 * @param[in] dataptr1 is value which is forwarded to \p p_writef. This is optional
 *  and may contain NULL.
 * @return EXSUCCEED/EXFAIL, Berror set in case of problems.
 */
expublic int Bwritecb (UBFH *p_ub, 
        long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        return EXFAIL;
    }
    /* check output file */
    if (NULL==p_writef)
    {
        ndrx_Bset_error_msg(BEINVAL, "Output callback function must not be NULL!");
        return EXFAIL;
    }

    return ndrx_Bwrite (p_ub, NULL, p_writef, dataptr1);
}

/**
 * Get the length of the field
 * @param p_ub
 * @param bfldid
 * @param occ
 * @return 
 */
expublic int Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ)
{
    API_ENTRY;

    if (EXSUCCEED!=validate_entry(p_ub, bfldid,occ,0))
    {
        UBF_LOG(log_warn, "Bget: arguments fail!");
        return EXFAIL; /* <<<< RETURN HERE! */
    }

    return ndrx_Blen (p_ub, bfldid, occ);
}

/**
 * Set callback function that can be used in expressions for long value.
 * @param funcname
 * @param functionPtr
 * @return SUCCEED/FAIL
 */
expublic int Bboolsetcbf (char *funcname, 
            long (*functionPtr)(UBFH *p_ub, char *funcname))
{
    API_ENTRY;
    {
/* Lock the region */
    MUTEX_LOCK;
    {
        int ret;
        ret = ndrx_Bboolsetcbf (funcname, functionPtr);
        MUTEX_UNLOCK;
        return ret;
    }
    }
}

/**
 * VIEW - test is field is NULL
 * @param cstruct ptr to C structure in memory
 * @param cname field name
 * @param occ occurrence of the field (array index)
 * @param view view name
 * @return EXFAIL/EXFALSE/EXTRUE
 */
expublic int Bvnull(char *cstruct, char *cname, BFLDOCC occ, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvnull(cstruct, cname, occ, view);
    
out:

    return ret;
}

/**
 * Init structure element.
 * Count indicators and Length indicators (where appropriate) are set to zero.
 * @param cstruct C structure ptr in mem
 * @param cname field name
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bvselinit(char *cstruct, char *cname, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvselinit(cstruct, cname, view);
    
out:
    return ret;
}

/**
 * Initialize structure field
 * @param cstruct memory addr
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bvsinit(char *cstruct, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvsinit(cstruct, view);
out:
    return ret;
}

/**
 * Refresh view cache not supported by Enduro/X (and not needed).
 */
expublic void Bvrefresh(void)
{
    UBF_LOG(log_warn, "Bvrefresh - not supported by Enduro/X");
}

/**
 * Set view option (thread safe)
 * @param cname field name 
 * @param option option (see B_FTOS, B_STOF, B_OFF, B_BOTH)
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bvopt(char *cname, int option, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvopt(cname, option, view);
    
out:
    return ret;
}

/**
 * Convert UBF buffer to C struct
 * @param p_ub UBF buffer
 * @param cstruct ptr to mem block
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bvftos(UBFH *p_ub, char *cstruct, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_ub)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ub is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvftos(p_ub, cstruct, view);
    
out:
    return ret;
}

/**
 * Copy data from structure to UBF
 * @param p_ub ptr to UBF buffer
 * @param cstruct ptr to memory block
 * @param mode BUPDATE, BOJOIN, BJOIN, BCONCAT
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bvstof(UBFH *p_ub, char *cstruct, int mode, char *view)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;
    
    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_ub)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ub is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_Bvstof(p_ub, cstruct, mode, view);
    
out:
    return ret;
}
/**
 * Join two buffers, update only existing fields in dest, remove missing fields
 * @param dest - dest buffer (being modified)
 * @param src - src buffer (not modified)
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bjoin(UBFH *dest, UBFH *src)
{
    int ret=EXSUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", __func__);
    if (EXSUCCEED!=validate_entry(src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", __func__);
        ndrx_Bappend_error_msg("(Bjoin: arguments fail for src buf!)");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=validate_entry(dest, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dest buf!", __func__);
        ndrx_Bappend_error_msg("(Bjoin: arguments fail for dest buf!)");
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bjoin (dest, src);
    }
    UBF_LOG(log_debug, "Return %s %d", __func__, ret);
    return ret;
}

/**
 * Outer join two buffers, update existing, do not remove non-existing fields
 * @param dest - dest buffer (being modified)
 * @param src - src buffer (not modified)
 * @return EXSUCCEED/EXFAIL
 */
expublic int Bojoin(UBFH *dest, UBFH *src)
{
    int ret=EXSUCCEED;
    API_ENTRY;
    /* Do standard validation */
    UBF_LOG(log_debug, "Entering %s", __func__);
    if (EXSUCCEED!=validate_entry(src, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for src buf!", __func__);
        ndrx_Bappend_error_msg("(Bojoin: arguments fail for src buf!)");
        ret=EXFAIL;
    }
    else if (EXSUCCEED!=validate_entry(dest, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail for dest buf!", __func__);
        ndrx_Bappend_error_msg("(Bojoin: arguments fail for dest buf!)");
        ret=EXFAIL;
    }
    else
    {
        /* Call the implementation */
        ret=ndrx_Bojoin (dest, src);
    }
    UBF_LOG(log_debug, "Return %s %d", __func__, ret);
    return ret;
}

/**
 * Dynamically read the 
 * @param cstruct view object (struct instance) from where to take the value
 * @param view view name
 * @param cname field name
 * @param occ array index (zero based)
 * @param buf buffer where to store the value
 * @param len buffer on input user for strings and carray. Indicates the space.
 * On output indicates the length of data loaded, used for carray and strings.
 * optional
 * @param usrtype User field type
 * @param flags 0 or BVACCESS_NOTNULL (report view 'NULL' values as BNOTPRES)
 * @return 0 - succeed, -1 FAIL
 */
expublic int CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN *len, int usrtype, long flags)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==buf)
    {
        ndrx_Bset_error_msg(BEINVAL, "buf is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_CBvget(cstruct, view, cname, occ, buf, len, usrtype, flags);
    
out:
    return ret;
}

/**
 * Dynamically set view value. 
 * 
 * @param cstruct user view object
 * @param view view name
 * @param cname field name
 * @param occ occurrence to set. If occ+1>C_<count field>, then count field 
 * is incremented to this occ +1
 * @param buf buffer to take value from
 * @param len buffer len (mandatory for carray)
 * @param usrtype User type of data see BFLD_*
 * @return 0 - succeed, -1 fail
 */
expublic int CBvchg(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN len, int usrtype)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==buf)
    {
        ndrx_Bset_error_msg(BEINVAL, "buf is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_CBvchg(cstruct, view, cname, occ, buf, len, usrtype);
    
out:
    return ret;

}

/**
 * Size of the view
 * @param view view name
 * @return -1 on fail, or size of the view
 */
expublic long Bvsizeof(char *view)
{
    long ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_Bvsizeof(view);
    
out:
    return ret;
}



/**
 * Copy the view contents from one buffer to another
 * @param cstruct_dst Destination buffer
 * @param cstruct_src Source buffer
 * @param view View name
 * @return -1 on fail, or number of bytes copied
 */
expublic long Bvcpy(char *cstruct_dst, char *cstruct_src, char *view)
{
    long ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct_src)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct_src is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct_dst)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct_dst is NULL!");
        EXFAIL_OUT(ret);
    }
    
    
    ret = ndrx_Bvcpy(cstruct_dst, cstruct_src, view);
    
out:
    return ret;
}



/**
 * Get field occurrences number of array elements in VIEW.
 * Set by 'count' column in view file.
 * @param cstruct view object
 * @param view view name
 * @param cname filed name
 * @param maxocc returns the 'count' value of the view file
 * @param realocc returns number of non null elements in array. Scanned from
 * end of the array. Once first non NULL value is found, then this is detected
 * as real count.
 * @param dim_size number of bytes for field dimension
 * @return count set in C_<count_filed>. If count  indicator field not used,
 * then 'count' value (max value) from view is returned.
 * In case of error -1 is returned.
 */
expublic BFLDOCC Bvoccur(char *cstruct, char *view, char *cname, 
        BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size, int* fldtype)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_Bvoccur(cstruct, view, cname, maxocc, realocc, dim_size, fldtype);
    
out:
    return ret;
}

/**
 * Set occurrences (C_<field>) field
 * @param cstruct
 * @param view
 * @param cname
 * @param occ
 * @return 
 */
expublic int Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==view || EXEOS==view[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "view is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cname || EXEOS==cname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL or empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==cstruct)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_Bvsetoccur(cstruct, view, cname, occ);
    
out:
    return ret;
}

/**
 * Iterate dynamically over the view structure
 * @param state where to store the iteration state, initialised when 'view' is set
 * @param view view name, on start of iter, must be set
 * @param cname field name to return
 * @param fldtype type to returnd BFLD_* consts applies
 * @return 1 - For success (have next), 0 EOF (nothing to return), -1 FAIL
 */
expublic int Bvnext (Bvnext_state_t *state, char *view, 
        char *cname, int *fldtype, BFLDOCC *maxocc, long *dim_size)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY;

    if (NULL==state)
    {
        ndrx_Bset_error_msg(BEINVAL, "state is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==view)
    {
        if (NULL == state->v)
        {
            ndrx_Bset_error_msg(BEINVAL, "view is null but state contains NULL");
            EXFAIL_OUT(ret);
        }
        
        if (NULL == state->vel)
        {
            UBF_LOG(log_debug, "View scan early EOF");
            ret = 0;
            goto out;
        }
    }
    
    if (NULL==cname)
    {
        ndrx_Bset_error_msg(BEINVAL, "cname is NULL, no where "
                "to store field name");
        EXFAIL_OUT(ret);
    }
        
    ret=ndrx_Bvnext (state, view, cname, fldtype, maxocc, dim_size);

out:

    return ret;    
}

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
expublic int Bvcmp(char *cstruct1, char *view1, char *cstruct2, char *view2)
{
    int ret = EXSUCCEED;
    API_ENTRY;
    VIEW_ENTRY2;
    
    if (NULL==cstruct1)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct1 is NULL!");
        ret=-2;
        goto out;
    }
    
    if (NULL==view1)
    {
        ndrx_Bset_error_msg(BEINVAL, "view1 is NULL!");
        ret=-2;
        goto out;
    }
    
    if (NULL==cstruct2)
    {
        ndrx_Bset_error_msg(BEINVAL, "cstruct2 is NULL!");
        ret=-2;
        goto out;
    }
    
    if (NULL==view2)
    {
        ndrx_Bset_error_msg(BEINVAL, "view2 is NULL!");
        ret=-2;
        goto out;
    }
        
    ret=ndrx_Bvcmp(cstruct1, view1, cstruct2, view2);

out:

    return ret;    
}

/**
 * Compare buffer 1 with buffer 2
 * @param p_ubf1 UBF buf1
 * @param p_ubf2 UBF buf2
 * @return -1 ubf1 have less fields, id is less than ubf2 fld, value of ubf1 is less
 * than ubf2.
 * 1 - vice versa -1 (ubf1 have more fields, id greater or value greater)
 * 0 buffers matches
 * in case of error, `berror' will not be 0.
 */
expublic int Bcmp(UBFH *p_ubf1, UBFH *p_ubf2)
{
    int ret=EXSUCCEED;
    UBF_header_t *ubf1_h = (UBF_header_t *)p_ubf1;
    UBF_header_t *ubf2_h = (UBF_header_t *)p_ubf2;
    API_ENTRY;

    UBF_LOG(log_debug, "About to compare FB=%p to FB=%p", p_ubf1, p_ubf2);

    if (NULL==p_ubf1)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ubf1 is NULL!");
        ret=-2;
        goto out;
    }

    if (NULL==p_ubf2)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ubf2 is NULL!");
        ret=-2;
        goto out;
    }

    if (0!=strncmp(ubf1_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ubf1 magic failed!");
        ret=-2;
        goto out;
    }

    if (0!=strncmp(ubf2_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ubf2 magic failed!");
        ret=-2;
        goto out;
    }

    ret = ndrx_Bcmp(p_ubf1, p_ubf2);
    
out:
    UBF_LOG(log_debug, "return %d", ret);
    
    return ret;
}

/**
 * Test is ubf2 a subset of ubf1 (fields and values matches)
 * @param p_ubf1 UBF buffer 1
 * @param p_ubf2 UBF buffer 2
 * @return EXFAIL(sys err)/EXTRUE(is subset)/EXFALSE(not a subset, fields or
 * values does not match)
 */
expublic int Bsubset(UBFH *p_ubf1, UBFH *p_ubf2)
{
    int ret=EXSUCCEED;
    UBF_header_t *ubf1_h = (UBF_header_t *)p_ubf1;
    UBF_header_t *ubf2_h = (UBF_header_t *)p_ubf2;
    API_ENTRY;

    UBF_LOG(log_debug, "%s: About to check FB2=%p as subset of to FB1=%p", __func__,
                                    p_ubf2, p_ubf1);

    if (NULL==p_ubf1)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ubf1 is NULL!");
        EXFAIL_OUT(ret);
    }

    if (NULL==p_ubf2)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_ubf2 is NULL!");
        EXFAIL_OUT(ret);
    }

    if (0!=strncmp(ubf1_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ubf1 magic failed!");
        EXFAIL_OUT(ret);
    }

    if (0!=strncmp(ubf2_h->magic, UBF_MAGIC, UBF_MAGIC_SIZE))
    {
        ndrx_Bset_error_msg(BNOTFLD, "p_ubf2 magic failed!");
        EXFAIL_OUT(ret);
    }

    ret = ndrx_Bsubset(p_ubf1, p_ubf2);
    
out:
    UBF_LOG(log_debug, "%s: return %d", __func__, ret);
    
    return ret;
}


/**
 * Load LMBB database of fields (usable only if tables not loaded already)
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int Bflddbload(void) 
{
    int ret = EXSUCCEED;
    
    API_ENTRY;
    
    if (NULL!=ndrx_G_ubf_db)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: field db tables already loaded (%p)",
                __func__, ndrx_G_ubf_db);
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_ubfdb_Bflddbload();
out:   
    return ret;
}

#define DB_API_ENTRY if (!ndrx_G_ubf_db_triedload)\
    {\
        /* error will be set here */\
        if (EXFAIL==ndrx_ubfdb_Bflddbload())\
        {\
            EXFAIL_OUT(ret);\
        }\
    }

/**
 * Return LMDB database handler
 * @param [out] dbi_id id -> name mapping database interface
 * @param [out] dbi_nm name -> id database handler
 * @return DB Env handler or NULL (B error set)
 */
expublic EDB_env * Bfldddbgetenv (EDB_dbi **dbi_id, EDB_dbi **dbi_nm)
{
    int ret = EXSUCCEED;
    EDB_env *dbenv = NULL;
    
    API_ENTRY;
    DB_API_ENTRY;
    
    if (NULL==dbi_id)
    {
        ndrx_Bset_error_msg(BEINVAL, "dbi_id is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==dbi_nm)
    {
        ndrx_Bset_error_msg(BEINVAL, "dbi_nm is NULL!");
        EXFAIL_OUT(ret);
    }
    
    dbenv = ndrx_ubfdb_Bfldddbgetenv(dbi_id, dbi_nm);
    
out:
                        
    if (EXSUCCEED!=ret)
    {
        return NULL;
    }

    return dbenv;
}



/**
 * Add field to database
 * @param txn LMDB transaction
 * @param fldtype filed type, see BFLD_*
 * @param bfldno field number (not compiled)
 * @param fldname field name
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int Bflddbadd(EDB_txn *txn,
        short fldtype, BFLDID bfldno, char *fldname)
{
    int ret=EXSUCCEED;
    
    API_ENTRY;
    DB_API_ENTRY;
    
    if (NULL==ndrx_G_ubf_db)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: field db not loaded!",
                __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==txn)
    {
        ndrx_Bset_error_msg(BEINVAL, "txn is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (bfldno <= 0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "invalid bfldno = %d!", 
                (int)bfldno);
        EXFAIL_OUT(ret);
    }
    
    if (fldtype < BFLD_MIN || fldtype > BFLD_MAX)
    {
        ndrx_Bset_error_fmt(BEINVAL, "invalid field type: %d", (int)fldtype);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==fldname)
    {
        ndrx_Bset_error_msg(BEINVAL, "fldname is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS==fldname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "fldname is empty (EOS)!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_ubfdb_Bflddbadd(txn, fldtype, bfldno, fldname);
    
 out:   
    return ret;
}

/**
 * Delete field from UBF db
 * @param txn LMDB transaction
 * @param bfldid Field
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int Bflddbdel(EDB_txn *txn, BFLDID bfldid)
{
    int ret=EXSUCCEED;
    
    API_ENTRY;
    DB_API_ENTRY;
    
    if (NULL==ndrx_G_ubf_db)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: field db not loaded!",
                __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==txn)
    {
        ndrx_Bset_error_msg(BEINVAL, "txn is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (bfldid <= 0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "invalid bfldno = %d!", 
                (int)bfldid);
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_ubfdb_Bflddbdel(txn, bfldid);
    
 out:   
    return ret;
}

/**
 * Drop database
 * @param txn LMDB transaction
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int Bflddbdrop(EDB_txn *txn)
{
    int ret=EXSUCCEED;
    
    API_ENTRY;
    DB_API_ENTRY;
    
    if (NULL==ndrx_G_ubf_db)
    {
        ndrx_Bset_error_fmt(BEINVAL, "%s: field db not loaded!",
                __func__);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==txn)
    {
        ndrx_Bset_error_msg(BEINVAL, "txn is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_ubfdb_Bflddbdrop(txn);
    
 out:   
    return ret;
}

/**
 * Unload UBF database
 */
expublic void Bflddbunload(void)
{
    ndrx_ubfdb_Bflddbunload();
}

/**
 * Unlink database
 * @return EXSUCCEED/EXFAIL (B error set)
 */
expublic int Bflddbunlink(void)
{
    API_ENTRY;
    return ndrx_ubfdb_Bflddbunlink();
}

/**
 * Get field infos from cursor data
 * @param[in] data UBF data record
 * @param[out] p_fldtype ptr field type
 * @param[out] p_bfldno field number (not compiled
 * @param[out] p_bfldid filed id (compiled)
 * @param[out] fldname field name to store
 * @param[in] fldname_bufsz size of field name variable
 * @return 
 */
expublic int Bflddbget(EDB_val *data,
        short *p_fldtype, BFLDID *p_bfldno, BFLDID *p_bfldid, 
        char *fldname, int fldname_bufsz)
{
    API_ENTRY;
    int ret = EXSUCCEED;
    
    if (NULL==data)
    {
        ndrx_Bset_error_msg(BEINVAL, "data is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_bfldno)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_bfldno is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_bfldid)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_bfldno is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_fldtype)
    {
        ndrx_Bset_error_msg(BEINVAL, "p_fldtype is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==fldname)
    {
        ndrx_Bset_error_msg(BEINVAL, "fldname is NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (fldname_bufsz<=0)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Invalid buffer size, must be >=0, got %d",
                fldname_bufsz);
        EXFAIL_OUT(ret);
    }
    
    ret=ndrx_ubfdb_Bflddbget(data, p_fldtype, p_bfldno, p_bfldid,
            fldname, fldname_bufsz);
    
 out:   
    return ret;
}

/**
 * Resolve field name from id (compiled)
 * @param bfldid field id
 * @return field name (one slot stored in TLS) or NULL (B error set)
 */
expublic char * Bflddbname (BFLDID bfldid)
{
    int ret=EXSUCCEED;
    char *f = NULL;
    API_ENTRY;
    DB_API_ENTRY;
    
    if (0>=bfldid)
    {
        ndrx_Bset_error_msg(BEINVAL, "Invalid field id (<=0)");
        EXFAIL_OUT(ret);
    }
    
    f = ndrx_ubfdb_Bflddbname (bfldid);
    
 out:   

    if (EXSUCCEED!=ret)
    {
        return NULL;
    }
 
    return f;
}

/**
 * Resolve field id from field name
 * @param fldname field name
 * @return field id or BBADFLDID (B error set)
 */
expublic BFLDID Bflddbid (char *fldname)
{
    BFLDID ret=EXSUCCEED;
    API_ENTRY;
    DB_API_ENTRY;
    
    if (NULL==fldname || EXEOS==fldname[0])
    {
        ndrx_Bset_error_msg(BEINVAL, "Invalid field id null or empty!");
        EXFAIL_OUT(ret);
    }
    
    ret = ndrx_ubfdb_Bflddbid (fldname);
    
 out:

    return ret;
}

/**
 * Count all occurrences of the fields in the buffer
 * @param p_ub - UBF buffer
 * @return Number of all occurrences of the fields in buffer, -1 FAIL
 */
expublic BFLDOCC Bnum (UBFH *p_ub)
{
    char fn[] = "Bnum";
    UBF_header_t *hdr = (UBF_header_t *)p_ub;

    API_ENTRY;

    /* Do standard validation */
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", fn);
        return EXFAIL;
    }
    else
    {
        return ndrx_Bnum(p_ub);
    }
}

/**
 * Return number of bytes required for UBF buffer
 * @param nrfields number of fields
 * @param totsize space required for all fields (data size)
 * @return Space needed for UBF buffer or EXFAIL
 */
expublic long Bneeded(BFLDOCC nrfields, BFLDLEN totsize)
{
    long ret = EXSUCCEED;
    API_ENTRY;
    
    if (nrfields<=0)
    {
        ndrx_Bset_error_msg(BEINVAL, "Invalid nrfields (<=0)");
        EXFAIL_OUT(ret);
    }
    
    if (totsize<=0)
    {
        ndrx_Bset_error_msg(BEINVAL, "Invalid totsize (<=0)");
        EXFAIL_OUT(ret);
    }
    
    /* compute the total size */
    ret = ndrx_Bneeded(nrfields, totsize);
    
    
    if (ret > MAXUBFLEN)
    {
        ndrx_Bset_error_fmt(BEINVAL, "Buffer size estimated larger (%ld) than max (%ld)",
                ret, (long)MAXUBFLEN);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Reallocate UBF block
 * @param p_Fb
 * @param f number of fields
 * @param v field length in bytes
 * @return reallocated UBF buffer or NULL on failure
 */
expublic UBFH * Balloc (BFLDOCC f, BFLDLEN v)
{
    API_ENTRY;
    
    return ndrx_Balloc (f, v, EXFAIL);
}

/**
 * Reallocate UBF block
 * @param p_Fb
 * @param f number of fields
 * @param v field length in bytes
 * @return reallocated UBF buffer or NULL on failure
 */
expublic UBFH * Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v)
{
    API_ENTRY;
    
    if (EXSUCCEED!=validate_entry(p_ub, 0, 0, VALIDATE_MODE_NO_FLD))
    {
        UBF_LOG(log_warn, "%s: arguments fail!", __func__);
        p_ub=NULL;
    }
    
    
    return ndrx_Brealloc (p_ub, f, v, EXFAIL);
}

/**
 * Dummy functions. Enduro/X all buffers are 32bit
 * @param dest dest buffer
 * @param src source buffer
 * @return EXSUCCEED
 */
expublic int B32to16(UBFH *dest, UBFH *src)
{
    return EXSUCCEED;
}

/**
 * Dummy functions. Enduro/X all buffers are 32bit
 * @param dest dest buffer
 * @param src source buffer
 * @return EXSUCCEED
 */
expublic int B16to32(UBFH *dest, UBFH *src)
{
    return EXSUCCEED;
}

/* vim: set ts=4 sw=4 et smartindent: */
