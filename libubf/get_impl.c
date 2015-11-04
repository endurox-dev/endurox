/* 
** UBF library
** Get functions lives here
**
** @file get_impl.c
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

#include <stdio.h>
#include <stdlib.h>

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
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * Internal version of Bgetalloc
 * uses Bfind in core.
 * @param p_ub
 * @param bfldid
 * @param occ
 * @param extralen - extra size to allocate/data len returned
 * @return
 */
public char * _Bgetalloc (UBFH * p_ub, BFLDID bfldid,
                            BFLDOCC occ, BFLDLEN *extralen)
{
    int data_type = (bfldid>>EFFECTIVE_BITS);
    BFLDLEN tmp_len = 0;
    char *cvn_buf;
    char *fb_data;
    char *alloc_buf=NULL;
    int alloc_size = 0;
    char fn[]="_Bgetalloc";
    char *ret=NULL;
    char *p_fld=NULL;
    dtype_str_t *dtype_str = &G_dtype_str_map[data_type];
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1 = &G_dtype_ext1_map[data_type];
    #endif
/*******************************************************************************/

    fb_data=_Bfind (p_ub, bfldid, occ, &tmp_len, &p_fld);

    if (NULL!=fb_data)
    {
        /* Allocate the buffer */
        if (NULL==(ret=get_cbuf(data_type,data_type,
                        NULL, fb_data, tmp_len,
                        &alloc_buf,
                        &alloc_size,
                        CB_MODE_ALLOC,
                        extralen!=NULL?(*extralen):0)))
        {
            UBF_LOG(log_error, "%s: get_cbuf failed!", fn);
        }
        else
        {
            /* put data into allocated buffer */
            if (SUCCEED!=dtype_str->p_get_data(dtype_str, p_fld, ret, &tmp_len))
            {
                free(ret); /* free up allocated memory */
                ret=NULL;
            }
            else
            {
                /* return the len if needed. */
                if (NULL!=extralen)
                    *extralen = tmp_len;
            }
        }
    }
    else
    {
        ret=NULL;
    }
/***************************************** DEBUG *******************************/
    /* Dump here happens two times?
     * One time data is dumpet at end of _Bfind
     * second time it is dumped here. But OK it this way we see that data is
     * successfully pushed into return buffer.
     */
    #ifdef UBF_API_DEBUG
    if (NULL!=ret)
    {
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Bgetalloc got data", ret, &tmp_len);
    }
    #endif
/*******************************************************************************/

    return ret;
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
public int _Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN * len)
{
    int ret=SUCCEED;
    dtype_str_t *dtype;
    char *p;
    char *last_checked=NULL;
    int last_occ=-1;
    char fnname[] = "_Bget";
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif
/*******************************************************************************/

    UBF_LOG(log_debug, "%s: bfldid: %x, occ: %d", fnname, bfldid, occ);

    if (NULL!=(p=get_fld_loc(p_ub, bfldid, occ, &dtype, &last_checked, NULL, &last_occ, NULL)))
    {
        if (NULL!=buf)
        {
            ret=dtype->p_get_data(dtype, (char *)p, buf, len);
        }
        else
        {
            UBF_LOG(log_debug, "%s: buf=NULL, not returning data!", fnname);
        }
    }
    else
    {
        _Fset_error(BNOTPRES);
        ret=FAIL;
    }
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (SUCCEED==ret)
    {
        __dbg_dtype_ext1 = &G_dtype_ext1_map[bfldid>>EFFECTIVE_BITS];
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Bget got data", buf, len);
    }
    #endif
/*******************************************************************************/
    UBF_LOG(log_debug, "%s: ret: %d", fnname, ret);

    return ret;
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
public int _Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len)
{
    int ret=SUCCEED;
    dtype_str_t *dtype;
    char *p;
    char *last_checked = NULL;
    char *last_match = NULL;
    int last_occ=-1;
    char fnname[] = "_Bgetlast";
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    dtype_ext1_t *__dbg_dtype_ext1;
    #endif
/*******************************************************************************/

    UBF_LOG(log_debug, "%s: bfldid: %x", fnname);

    get_fld_loc(p_ub, bfldid, -2, &dtype, &last_checked, &last_match, &last_occ, NULL);

    if (FAIL!=last_occ && !_Fis_error()) /* Exclude cases when error have been raised! */
    {
        /* Have to get data type again - because in last mode it is not avaialble  */
        dtype = &G_dtype_str_map[bfldid>>EFFECTIVE_BITS];
        if (NULL!=buf)
        {
            ret=dtype->p_get_data(dtype, (char *)last_match, buf, len);
        }
        else
        {
            UBF_LOG(log_debug, "%s: buf=NULL, not returning data!", fnname);
        }

        if (NULL!=occ)
        {
            *occ = last_occ;
            UBF_LOG(log_debug, "%s: Got occ %d!", fnname, *occ);
        }
        else
        {
            UBF_LOG(log_debug, "%s: occ=NULL, not returning occ!", fnname);
        }
    }
    else
    {
        _Fset_error(BNOTPRES); /* IF error have been set this will not override! */
        ret=FAIL;
    }
/***************************************** DEBUG *******************************/
    #ifdef UBF_API_DEBUG
    if (SUCCEED==ret && NULL!=buf)
    {
        __dbg_dtype_ext1 = &G_dtype_ext1_map[bfldid>>EFFECTIVE_BITS];
        __dbg_dtype_ext1->p_dump_data(__dbg_dtype_ext1, "_Bget got data", buf, len);
    }
    #endif
/*******************************************************************************/
    UBF_LOG(log_debug, "%s: ret: %d", fnname, ret);

    return ret;
}

