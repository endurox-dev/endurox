/**
 * @brief UBF library
 *   This file implements find functions.
 *
 * @file find_impl.c
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
#include <stdio.h>
#include <stdlib.h>
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

#include "ubf_tls.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Find the entry into buffer.
 * Return raw pointer directly
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param p_len
 * @param p_fld - pointer to start of the actual field (optional)
 * @return
 */
expublic char * ndrx_Bfind (UBFH * p_ub, BFLDID bfldid,
                                        BFLDOCC occ, BFLDLEN * len,
                                        char **p_fld)
{
    dtype_str_t *dtype;
    char *last_checked=NULL;
    int last_occ;
    char *ret = NULL;
    int data_len = 0;
    dtype_ext1_t *dtype_ext1;
    char fn[] = "_Bfind";
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif
/*******************************************************************************/

    UBF_LOG(log_debug, "Bfind: bfldid: %d occ: %hd", bfldid, occ);

    if (UBF_BINARY_SEARCH_OK(bfldid))
    {
        ret=get_fld_loc_binary_search(p_ub, bfldid, occ, &dtype, 
                UBF_BINSRCH_GET_LAST_NONE, NULL, NULL, NULL);
    }
    else
    {
        ret=get_fld_loc(p_ub, bfldid, occ,
                                &dtype,
                                &last_checked,
                                NULL,
                                &last_occ,
                                NULL);
    }

    /* Get the data size of Bfind */
    if (NULL!=ret)
    {
        int dlen;

        /* Save the pointer to the field */
        if (NULL!=p_fld)
        {
            *p_fld=ret;
        }

        /* Get the payload size */
        if (NULL!=len)
        {
           /* *len = data_len;*/
           dtype->p_next(dtype, (char *)ret, len);
        }

        dtype_ext1 = &G_dtype_ext1_map[dtype->fld_type];
        dlen = dtype_ext1->hdr_size;
        
        if (NULL!=dtype_ext1->p_prep_ubfp)
        {
            /* translate to value */
            ret=dtype_ext1->p_prep_ubfp(dtype_ext1, 
                    &G_ubf_tls->ndrx_Bfind_tls_stor, ret);
        }
        else
        {
            /* Move us to start of the data. */
            ret+=dlen;
        }
    }
    else
    {
        UBF_LOG(log_error, "%s: Field not present!", fn);
        ndrx_Bset_error(BNOTPRES);
    }

    UBF_LOG(log_debug, "Bfind: return: %p", ret);
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (NULL!=ret)
    {
        __dbg_dtype_ext1 = &G_dtype_ext1_map[bfldid>>EFFECTIVE_BITS];
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Bfind got data", ret, len);
    }
    #endif
/*******************************************************************************/
    return ret;
}

/**
 * Internal version of CBFind.
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param buf
 * @param type
 * @return
 */
expublic char * ndrx_CBfind (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC occ, BFLDLEN * len, int usrtype,
                        int mode, int extralen)
{
    int from_type = (bfldid>>EFFECTIVE_BITS);
    BFLDLEN tmp_len = 0;
    char *cvn_buf;
    char *fb_data;
    char *alloc_buf=NULL;
    int alloc_size = 0;
    char *ret=NULL;
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1 = &G_dtype_ext1_map[usrtype];
    #endif
/*******************************************************************************/

    fb_data=ndrx_Bfind (p_ub, bfldid, occ, &tmp_len, NULL);

    if (NULL!=fb_data)
    {
        /* Allocate the buffer */
        if (NULL==(cvn_buf=ndrx_ubf_get_cbuf(from_type, usrtype,
                        NULL, fb_data, tmp_len,
                        &alloc_buf,
                        &alloc_size,
                        mode,
                        extralen)))
        {
            UBF_LOG(log_error, "%s: get_cbuf failed!", __func__);
            ndrx_Bset_error_fmt(BEUNIX, "%s: get_cbuf failed!", __func__);
            /* Error should be already set */
            return NULL; /* <<<< RETURN!!!! */
        }
        
        /* Bug #330*/
        if (NULL!=len)
        {
            *len = alloc_size;
        }
        
        ret = ndrx_ubf_convert(from_type, CNV_DIR_OUT, fb_data, tmp_len,
                                    usrtype, cvn_buf, len);
        if (NULL==ret)
        {
            UBF_LOG(log_error, "%s: failed to convert data!", __func__);
            ndrx_Bset_error_fmt(BEUNIX, "%s: failed to convert data!", __func__);
            /* Error should be provided by conversation function */
            ret=NULL;
        }
    }
    else
    {
        UBF_LOG(log_error, "%s: Field not present!", __func__);
        ret=NULL;
    }
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (NULL!=ret)
    {
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_CBfind got data", ret, len);
    }
    #endif
/*******************************************************************************/
    return ret;
}

/**
 * Internal Bfindocc implementation.
 * @param p_ub
 * @param bfldid
 * @param value
 * @param len
 * @return
 */
expublic BFLDOCC ndrx_Bfindocc (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len)
{
    dtype_str_t *dtype=NULL;
    dtype_ext1_t *dtype_ext1;
    UBF_header_t *hdr = (UBF_header_t *)p_ub;
    char *last_checked=NULL;
    int last_occ;
    BFLDOCC ret = EXFAIL;
    char *fn = "_Bfindocc";
    BFLDLEN dlen;
    char *p_fld;
    char *p_dat;
    BFLDLEN step;
    int iocc=0;
    BFLDID *p_bfldid;
    int cmp_ret;
    
    UBF_LOG(log_debug, "%s: bfldid: %d", fn, bfldid);

    /* find first occurrance */
    p_fld=get_fld_loc(p_ub, bfldid, 0,
                            &dtype,
                            &last_checked,
                            NULL,
                            &last_occ,
                            NULL);
    
    /* loop over the data */
    while (NULL!=p_fld)
    {
        dtype_ext1 = &G_dtype_ext1_map[dtype->fld_type];
        dlen = dtype_ext1->hdr_size;
        /* Find next occ */
        p_dat = p_fld+dlen;
        step = dtype->p_next(dtype, p_fld, &dlen);
        /* Now do compare */
        cmp_ret=dtype_ext1->p_cmp(dtype_ext1, p_dat, dlen, buf, len, 0L);
        if (EXTRUE==cmp_ret)
        {
            UBF_LOG(log_debug, "%s: Found occurrence: %d", fn, iocc);
            ret=iocc;
            break; /* <<< BREAKE on Found. */
        }
        else if (EXFALSE==cmp_ret)
        {
            p_fld+=step;
            /* Align error */
            if (CHECK_ALIGN(p_fld, p_ub, hdr))
            {
                ndrx_Bset_error_fmt(BALIGNERR, "%s: Pointing to non UBF area: %p",
                                            fn, p_fld);
                break; /* <<<< BREAK!!! */
            }
            p_bfldid = (BFLDID *)p_fld;

            if (*p_bfldid!=bfldid)
            {
                /* Next field is not ours! */
                break; /* <<< BREAK!!! */
            }
            else
            {
                iocc++;
            }
        } /* if/else */
        else if (EXFAIL==cmp_ret)
        {
            /* Regexp failed or malloc problems: error should be already set! */
            break; /* <<< BREAK!!! */
        }
    }/* while */

    if (!ndrx_Bis_error() && ret==EXFAIL )
    {
        /* The we do not have a result */
        ndrx_Bset_error_fmt(BNOTPRES, "%s: Occurrance of field %d not found last occ: %hd",
                                    fn, bfldid, iocc);
    }
    
    UBF_LOG(log_debug, "%s: return %d", fn, ret);

    return ret;
}

/**
 * Internal version of CBfindocc.
 * will re-use _Bfindocc for searching the occurrance but in extra will conver the
 * data type
 * @param p_ub - ptr to FB
 * @param bfldid - field ID to search for
 * @param value - value to search for
 * @param len - len for carray
 * @param usrtype - user data type specified
 * @return -1 (FAIL)/>=0 occurrance
 */
expublic BFLDOCC ndrx_CBfindocc (UBFH *p_ub, BFLDID bfldid, char * value, BFLDLEN len, int usrtype)
{
    int ret=EXFAIL;
    int cvn_len=0;
    char *cvn_buf;
    char tmp_buf[CF_TEMP_BUF_MAX];
    int to_type = (bfldid>>EFFECTIVE_BITS);
    /* Buffer management */
    char *alloc_buf = NULL;
    char *p;
    char *fn = "_CBfindocc";
    /* if types are the same then do direct call */
    if (usrtype==to_type)
    {
        UBF_LOG(log_debug, "%s: the same types - direct call!", fn);
        return ndrx_Bfindocc(p_ub, bfldid, value, len); /* <<<< RETURN!!! */
    }

    /* if types are not the same then go the long way... */

    /* Allocate the buffer dynamically or statically */
    if (NULL==(p=ndrx_ubf_get_cbuf(usrtype, to_type, tmp_buf,value, len, 
            &alloc_buf, &cvn_len, CB_MODE_DEFAULT, 0)))
    {
        UBF_LOG(log_error, "%s: Malloc failed!", fn);
        return EXFAIL; /* <<<< RETURN!!!! */
    }

    /* Convert the value */
    cvn_buf = ndrx_ubf_convert(usrtype, CNV_DIR_IN, value, len,
                        to_type, p, &cvn_len);

    if (NULL!=cvn_buf)
    {   /* In case of string, we do not want regexp processing, so do not pass the len */
        ret=ndrx_Bfindocc(p_ub, bfldid, cvn_buf, (BFLD_STRING!=to_type?cvn_len:0));
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
    
    UBF_LOG(log_debug, "%s: return %d", fn, ret);

    return ret;
}

/**
 * Backend impelmentation for Bfindlast
 * @param p_ub
 * @param bfldid
 * @param occ - returning occ number.
 * @param len
 * @return 
 */
expublic char * ndrx_Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                                BFLDOCC *occ,
                                                BFLDLEN * len)
{
    
    int data_type = (bfldid>>EFFECTIVE_BITS);
    dtype_str_t *dtype;
    char *last_checked=NULL;
    char *last_match=NULL;
    int last_occ;
    char *ret = NULL;
    dtype_ext1_t *dtype_ext1;
    char fn[] = "_Bfindlast";

    UBF_LOG(log_debug, "%s: bfldid: %d occ: %hd", fn, bfldid, occ);

    get_fld_loc(p_ub, bfldid, -2,
                            &dtype,
                            &last_checked,
                            &last_match,
                            &last_occ,
                            NULL);

    dtype = &G_dtype_str_map[data_type];
    /* Get the data size of Bfind */
    if (EXFAIL!=last_occ && !ndrx_Bis_error())
    {
        int dlen;

        /* Return the occurrence found. */
        if (NULL!=occ)
            *occ = last_occ;

        ret = last_match;
        /* Get the payload size */
        if (NULL!=len)
        {
           dtype->p_next(dtype, (char *)ret, len);
        }

        dtype_ext1 = &G_dtype_ext1_map[data_type];
        dlen = dtype_ext1->hdr_size;
        /* Move us to start of the data. */
        ret+=dlen;
    }
    else
    {
        /* set the error that field is not found */
        ndrx_Bset_error(BNOTPRES);
    }

    UBF_LOG(log_debug, "%s: return: %p occ: %d", fn, ret, last_occ);
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (NULL!=ret)
    {
        dtype_ext1->p_dump_data(dtype_ext1, "_Bfindlast got data", ret, len);
    }
    #endif
/*******************************************************************************/
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
