/**
 * @brief FLD_PTR type support
 *
 * @file fld_ptr.c
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
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <regex.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ndebug.h>
#include <ferror.h>
#include <errno.h>
#include <ubf_tls.h>
#include <fdatatype.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Put data, we use data ptr directly
 * @param t field type descriptor
 * @param fb UBF buffer pointer
 * @param bfldid field id to install
 * @param data data ptr (use just ptr)
 * @param len data len, not used
 */
expublic int ndrx_put_data_ptr(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    UBF_ptr_t *ptr = (UBF_ptr_t *)fb;
    /* int align; */
    ptr->bfldid = bfldid;
    ptr->ptr = data;
    
    return EXSUCCEED;
}

/**
 * Return character array data
 * @param t field type descriptor
 * @param fb UBF position of data
 * @param buf output buffer
 * @param len optional data len
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_get_data_ptr (struct dtype_str *t, char *fb, char *buf, int *len)
{
    UBF_ptr_t *ptr = (UBF_ptr_t *)fb;
    int ret=EXSUCCEED;

    if (NULL!=len && *len>0 && *len < t->size)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", t->size, *len);
        EXFAIL_OUT(ret);
    }
    else
    {
        /* copy the data */
        memcpy(buf, &(ptr->ptr), t->size);
        /* Bug #495 */
        if (NULL!=len)
        {
            *len=t->size;
        }
    }
out:
    return ret;
}

/**
 * Compare two pointers
 * @param t type descriptor
 * @param val1 value 1 compare
 * @param len1 not used
 * @param val2 value 2 to compare
 * @param len2 not used
 * @param mode compare mode standard (UBF_CMP_MODE_STD) or true/false
 * @return returns -1 (ptr1 < ptr2), 0 (ptr1==ptr2), 1 (ptr1 > ptr2)
 */
expublic int ndrx_cmp_ptr (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    if (mode & UBF_CMP_MODE_STD)
    {
        return memcmp(&val1, &val2, sizeof(UBF_ptr_t));
    }
    else
    {
        return (0==memcmp(&val1, &val2, sizeof(UBF_ptr_t)));
    }
}

/**
 * Dump to log the field value
 * @param t field type descr
 * @param text debug msg
 * @param data data to print
 * @param len not used
 */
expublic void ndrx_dump_ptr (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        void **ptr = (void **)data;
        UBF_LOG(log_debug, "%s:\n[%p]", text, *ptr);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
