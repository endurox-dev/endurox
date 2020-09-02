/**
 * @brief UBF field type convertion functions
 *
 * @file cf.c
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
#include "cf.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CARR_TEMP_BUF \
        char tmp[CF_TEMP_BUF_MAX+1]; \
        int cpy_len = in_len > CF_TEMP_BUF_MAX?CF_TEMP_BUF_MAX: in_len;\
        UBF_LOG(log_debug, "[%10.10s]", input_buf);\
        memcpy(tmp, input_buf, cpy_len); \
        tmp[cpy_len] = EXEOS

/*
 * This generally checks fixed data type lengths, i.e. if converting out
 * then if specified shorter that standard data type, then rejected!
 */
#define CHECK_OUTPUT_BUF_SIZE \
    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len)\
    {\
        if (to->size > *out_len)\
        {\
            ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified :%d", to->size, *out_len);\
            return NULL; \
        }\
    }

/*
 * This converts simple data type to string
 */
#define CONV_TO_STRING(X, C) \
    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len)\
    {\
        char tmp[CF_TEMP_BUF_MAX+1];\
        sprintf(tmp, X, (C)*ptr);\
        len = strlen(tmp)+1; /* Including EOS! */\
        if (*out_len<len)\
        {\
            ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified: %d", len, *out_len);\
            return NULL;\
        }\
        else\
        {\
            strcpy(output_buf, tmp);\
        }\
    }\
    else\
    {\
        /* In case if converting in, we have space for trailing EOS! */\
        sprintf(output_buf, X, (C)*ptr);\
        if (NULL!=out_len) /* In case if we really need it! */\
            len = strlen(output_buf)+1;\
    }\
    if (NULL!=out_len)\
        *out_len = len;\

/*
 * Convert simple data type to character array
 * This is the same as above, but strncpy used instead of strcpy!
 */
#define CONV_TO_CARRAY(X, C)\
if (CNV_DIR_OUT==cnv_dir)\
    {\
        char tmp[CF_TEMP_BUF_MAX+1];\
        sprintf(tmp, X, (C)*ptr);\
        len = strlen(tmp); /* NOT Including EOS! */\
        if (NULL!=out_len && *out_len < len)\
        {\
            ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified: %d", len, *out_len);\
            return NULL;\
        }\
        else\
        {\
            NDRX_STRNCPY(output_buf, tmp, len);\
        }\
    }\
    else\
    {\
        /* In case if converting in, we have space for trailing EOS! */\
        sprintf(output_buf, X, (C)*ptr);\
        if (NULL!=out_len) /* In case if we really need it! */\
            len = strlen(output_buf);\
    }\
    if (NULL!=out_len)\
        *out_len = len;

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* Functions from short to something */
exprivate char * conv_short_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_short_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Functions from long to something */
exprivate char * conv_long_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_long_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Functions from char to something */
exprivate char * conv_char_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_char_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Functions from float to something */
exprivate char * conv_float_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_float_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Functions from double to something */
exprivate char * conv_double_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_double_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Functions from string to something */
exprivate char * conv_string_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_string_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

exprivate char * conv_carr_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_carr_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Int type operations */
exprivate char * conv_int_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_int_ptr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* ptr type operations */
exprivate char * conv_ptr_short(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_long(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_char(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_float(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_double(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_string(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_carr(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);
exprivate char * conv_ptr_int(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);

/* Special conversation parts */
exprivate char * conv_same(struct conv_type *t, int cnv_dir, char *input_buf, int in_len, char *output_buf , int *out_len);


expublic conv_type_t G_conv_fn_short[] =
{
    {BFLD_SHORT, BFLD_SHORT, conv_same},
    {BFLD_SHORT, BFLD_LONG, conv_short_long},
    {BFLD_SHORT, BFLD_CHAR, conv_short_char},
    {BFLD_SHORT, BFLD_FLOAT, conv_short_float},
    {BFLD_SHORT, BFLD_DOUBLE, conv_short_double},
    {BFLD_SHORT, BFLD_STRING, conv_short_string},
    {BFLD_SHORT, BFLD_CARRAY, conv_short_carr},
    {BFLD_SHORT, BFLD_INT, conv_short_int},
    {BFLD_SHORT, BFLD_RFU0, NULL},
    {BFLD_SHORT, BFLD_PTR, conv_short_ptr}
};
expublic conv_type_t G_conv_fn_long[] =
{
    {BFLD_LONG, BFLD_SHORT, conv_long_short},
    {BFLD_LONG, BFLD_LONG, conv_same},
    {BFLD_LONG, BFLD_CHAR, conv_long_char},
    {BFLD_LONG, BFLD_FLOAT, conv_long_float},
    {BFLD_LONG, BFLD_DOUBLE, conv_long_double},
    {BFLD_LONG, BFLD_STRING, conv_long_string},
    {BFLD_LONG, BFLD_CARRAY, conv_long_carr},
    {BFLD_LONG, BFLD_INT, conv_long_int},
    {BFLD_LONG, BFLD_RFU0, NULL},
    {BFLD_LONG, BFLD_PTR, conv_long_ptr}
};

expublic conv_type_t G_conv_fn_char[] =
{

    {BFLD_CHAR, BFLD_SHORT, conv_char_short},
    {BFLD_CHAR, BFLD_LONG, conv_char_long},
    {BFLD_CHAR, BFLD_CHAR, conv_same},
    {BFLD_CHAR, BFLD_FLOAT, conv_char_float},
    {BFLD_CHAR, BFLD_DOUBLE, conv_char_double},
    {BFLD_CHAR, BFLD_STRING, conv_char_string},
    {BFLD_CHAR, BFLD_CARRAY, conv_char_carr},
    {BFLD_CHAR, BFLD_INT, conv_char_int},
    {BFLD_CHAR, BFLD_RFU0, NULL},
    {BFLD_CHAR, BFLD_PTR, conv_char_ptr}
};

expublic conv_type_t G_conv_fn_float[] =
{
    {BFLD_FLOAT, BFLD_SHORT, conv_float_short},
    {BFLD_FLOAT, BFLD_LONG, conv_float_long},
    {BFLD_FLOAT, BFLD_CHAR, conv_float_char},
    {BFLD_FLOAT, BFLD_FLOAT, conv_same},
    {BFLD_FLOAT, BFLD_DOUBLE, conv_float_double},
    {BFLD_FLOAT, BFLD_STRING, conv_float_string},
    {BFLD_FLOAT, BFLD_CARRAY, conv_float_carr},
    {BFLD_FLOAT, BFLD_INT, conv_float_int},
    {BFLD_FLOAT, BFLD_RFU0, NULL},
    {BFLD_FLOAT, BFLD_PTR, conv_float_ptr}
};

expublic conv_type_t G_conv_fn_double[] =
{
    {BFLD_DOUBLE, BFLD_SHORT, conv_double_short},
    {BFLD_DOUBLE, BFLD_LONG, conv_double_long},
    {BFLD_DOUBLE, BFLD_CHAR, conv_double_char},
    {BFLD_DOUBLE, BFLD_FLOAT, conv_double_float},
    {BFLD_DOUBLE, BFLD_DOUBLE, conv_same},
    {BFLD_DOUBLE, BFLD_STRING, conv_double_string},
    {BFLD_DOUBLE, BFLD_CARRAY, conv_double_carr},
    {BFLD_DOUBLE, BFLD_INT, conv_double_int},
    {BFLD_DOUBLE, BFLD_RFU0, NULL},
    {BFLD_DOUBLE, BFLD_PTR, conv_double_ptr}
};

expublic conv_type_t G_conv_fn_string[] =
{
    {BFLD_STRING, BFLD_SHORT, conv_string_short},
    {BFLD_STRING, BFLD_LONG, conv_string_long},
    {BFLD_STRING, BFLD_CHAR, conv_string_char},
    {BFLD_STRING, BFLD_FLOAT, conv_string_float},
    {BFLD_STRING, BFLD_DOUBLE, conv_string_double},
    {BFLD_STRING, BFLD_STRING, conv_same},
    {BFLD_STRING, BFLD_CARRAY, conv_string_carr},
    {BFLD_STRING, BFLD_INT, conv_string_int},
    {BFLD_STRING, BFLD_RFU0, NULL},
    {BFLD_STRING, BFLD_PTR, conv_string_ptr}
};

expublic conv_type_t G_conv_fn_carr[] =
{
    {BFLD_CARRAY, BFLD_SHORT, conv_carr_short},
    {BFLD_CARRAY, BFLD_LONG, conv_carr_long},
    {BFLD_CARRAY, BFLD_CHAR, conv_carr_char},
    {BFLD_CARRAY, BFLD_FLOAT, conv_carr_float},
    {BFLD_CARRAY, BFLD_DOUBLE, conv_carr_double},
    {BFLD_CARRAY, BFLD_STRING, conv_carr_string},
    {BFLD_CARRAY, BFLD_CARRAY, conv_same},
    {BFLD_CARRAY, BFLD_INT, conv_carr_int},
    {BFLD_CARRAY, BFLD_RFU0, NULL},
    {BFLD_CARRAY, BFLD_PTR, conv_carr_ptr}
};

expublic conv_type_t G_conv_fn_int[] =
{
    {BFLD_INT, BFLD_SHORT, conv_int_short},
    {BFLD_INT, BFLD_LONG, conv_int_long},
    {BFLD_INT, BFLD_CHAR, conv_int_char},
    {BFLD_INT, BFLD_FLOAT, conv_int_float},
    {BFLD_INT, BFLD_DOUBLE, conv_int_double},
    {BFLD_INT, BFLD_STRING, conv_int_string},
    {BFLD_INT, BFLD_CARRAY, conv_int_carr},
    {BFLD_INT, BFLD_INT, conv_same},
    {BFLD_INT, BFLD_RFU0, NULL},
    {BFLD_INT, BFLD_PTR, conv_int_ptr}
};

expublic conv_type_t G_conv_fn_ptr[] =
{
    {BFLD_PTR, BFLD_SHORT, conv_ptr_short},
    {BFLD_PTR, BFLD_LONG, conv_ptr_long},
    {BFLD_PTR, BFLD_CHAR, conv_ptr_char},
    {BFLD_PTR, BFLD_FLOAT, conv_ptr_float},
    {BFLD_PTR, BFLD_DOUBLE, conv_ptr_double},
    {BFLD_PTR, BFLD_STRING, conv_ptr_string},
    {BFLD_PTR, BFLD_CARRAY, conv_ptr_carr},
    {BFLD_PTR, BFLD_INT, conv_ptr_carr},
    {BFLD_PTR, BFLD_RFU0, NULL},
    {BFLD_PTR, BFLD_INT, conv_same},
};

/**
 * Get conversation buffer to use
 * @param in_from_type from data type
 * @param in_to_type to data type
 * @param in_temp_buf static temp buffer
 * @param in_data data buffer (pointer to data)
 * @param in_len input data length used for carray
 * @param out_alloc_buf pointer to dynamically allocated buffer
 * @param mode - see CB_MODE_*
 * @param extra_len - extra len to allocate for buffer (usable only in CB_MODE_ALLOC mode)
 * @return
 */
expublic char * ndrx_ubf_get_cbuf(int in_from_type, int in_to_type,
                        char *in_temp_buf, char *in_data, int in_len,
                        char **out_alloc_buf,
                        int *alloc_size,
                        int mode,
                        int extra_len)
{
    char *ret=NULL;
    dtype_ext1_t *dtype_ext1 = &G_dtype_ext1_map[in_to_type];
    dtype_str_t *dtype = &G_dtype_str_map[in_to_type];

    /*
     * We allocated extra +1 for EOS,  no matter of fact is it needed or not.
     * for simplicity.
     */
    /* Fixes Bgetalloc() 05/05/2016 mvitolin */
    if ((BFLD_CARRAY==in_from_type || BFLD_STRING==in_from_type) &&
            (BFLD_CARRAY==in_to_type || BFLD_STRING==in_to_type))
    {
        UBF_LOG(log_debug, "Conv: carray/string->carray/string - allocating buf, "
                                        "size: %d", in_len+1);

        if (CB_MODE_DEFAULT==mode)
        {
            /* 18/04/2013, seems fails on RPI.
             * Fix the issue with invalid len for strings passed in.
             */
            if (BFLD_STRING==in_from_type)
            {
                in_len = strlen(in_data);
            }
            /* if target is string, then we need extra */
            if (NULL==(*out_alloc_buf = NDRX_MALLOC(in_len+1)))
            {
                ndrx_Bset_error(BMALLOC);
                return NULL; /* <<<< RETURN!!! */
            }
            ret=*out_alloc_buf;
            /* Set the output size! */
            *alloc_size=in_len+1;
        }
        else if (CB_MODE_TEMPSPACE==mode)
        {
            /* Using temporary buffer space */
            if (NULL==(ret = dtype_ext1->p_tbuf(dtype_ext1, in_len+1)))
            {
                return NULL; /* <<<< RETURN!!! */
            }
            /* Set the output size! */
            *alloc_size=in_len+1;
        }
        else if (CB_MODE_ALLOC==mode)
        {
            int tmp_len = in_len+1+extra_len;
            /* Using temporary buffer space */
            if (NULL==(ret = dtype_ext1->p_talloc(dtype_ext1, &tmp_len)))
            {
                return NULL; /* <<<< RETURN!!! */
            }
            /* Set the output size! */
            *alloc_size=tmp_len;
            *out_alloc_buf=ret;
        }
    }
    else
    {
        UBF_LOG(log_debug, "Conv: using temp buf mode: %d", mode);
        if (CB_MODE_DEFAULT==mode)
        {
            /* Not sure do we need to return alloc_size here? */
            ret=in_temp_buf;
            *alloc_size=CB_MODE_TEMPSPACE;
        }
        else if (CB_MODE_TEMPSPACE==mode)
        {
            /* Using temporary buffer space */
            if (NULL==(ret = dtype_ext1->p_tbuf(dtype_ext1, CF_TEMP_BUF_MAX)))
            {
                return NULL; /* <<<< RETURN!!! */
            }
            /* Set the output size! */
            /* Bug #544 - previously was  dtype->size which could be 0 */
            *alloc_size=CF_TEMP_BUF_MAX;
        }
        else if (CB_MODE_ALLOC==mode)
        {
            int tmp_len;
            
            /* Using temporary buffer space -> in case of PTR/UBF/VIEW use
             * originally estimated size. There is no other use there excep
             * Bgetalloc()
             */
            if (BFLD_UBF==in_from_type || BFLD_VIEW==in_from_type)
            {
                tmp_len = in_len+extra_len;
            }
            else
            {
                tmp_len = CF_TEMP_BUF_MAX+extra_len;
            }
            
            if (NULL==(ret = dtype_ext1->p_talloc(dtype_ext1, &tmp_len)))
            {
                return NULL; /* <<<< RETURN!!! */
            }
            /* Set the output size! */
            *alloc_size=tmp_len;
            *out_alloc_buf=ret;
        }
    }
    
    return ret;
}

/**
 * Convert data types,
 * @param t
 * @param ibuf
 * @param ilen
 * @param olen
 * @return
 */
expublic char * ndrx_ubf_convert(int from_type, int cnv_dir, char *input_buf, int in_len,
                        int to_type, char *output_buf , int *out_len)
{
    conv_type_t* conv_tab = NULL;
    conv_type_t *conv_entry = NULL;
    char *ret=NULL;

    switch (from_type)
    {
        case BFLD_SHORT:
            conv_tab = (conv_type_t*)&G_conv_fn_short;
            break;
        case BFLD_LONG:
            conv_tab = (conv_type_t*)&G_conv_fn_long;
            break;
        case BFLD_CHAR:
            conv_tab = (conv_type_t*)&G_conv_fn_char;
            break;
        case BFLD_FLOAT:
            conv_tab = (conv_type_t*)&G_conv_fn_float;
            break;
        case BFLD_DOUBLE:
            conv_tab = (conv_type_t*)&G_conv_fn_double;
            break;
        case BFLD_STRING:
            conv_tab = (conv_type_t*)&G_conv_fn_string;
            break;
        case BFLD_CARRAY:
            conv_tab = (conv_type_t*)&G_conv_fn_carr;
            break;
	case BFLD_INT:
            conv_tab = (conv_type_t*)&G_conv_fn_int;
            break;
        case BFLD_PTR:
            conv_tab = (conv_type_t*)&G_conv_fn_ptr;
            break;
        default:
            /* ERR - invalid type */
            ret=NULL;
            ndrx_Bset_error_fmt(BTYPERR, "Bad from type %d", from_type);
            break;
    }

    if (NULL!=conv_tab)
    {
        UBF_LOG(log_debug, "Converting from %d to %d",
                                        from_type, to_type);
        conv_entry = &conv_tab[to_type];
        ret=conv_entry->conv_fn(conv_entry, cnv_dir, input_buf, in_len, output_buf, out_len);
    }

    return ret;
}

/**
 * Returns the same data, but copied into different buffer.
 * Does buffer length check.
 */
exprivate char * conv_same(struct conv_type *t, int cnv_dir, char *input_buf, 
			  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *dtype = &G_dtype_str_map[t->to_type];
    int real_data;
    int size = dtype->p_get_data_size(dtype, input_buf, in_len, &real_data);
    
    /* Check buffer sizes */

    if (NULL!=out_len && real_data > *out_len)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified :%d", 
			    real_data, *out_len);
        return NULL;
    }

    if (NULL!=out_len)
        *out_len = real_data;

    /* Copy off the buffer data */
    memcpy(output_buf, input_buf, real_data);

    return output_buf;
}

/********************************* SHORT ***************************************/
exprivate char * conv_short_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    long *l = (long *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (long)*ptr;

    return output_buf;
}

exprivate char * conv_short_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *c = (char) *ptr;

    return output_buf;
}

exprivate char * conv_short_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    float *f = (float *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float) *ptr;

    return output_buf;
}

exprivate char * conv_short_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    double *d = (double *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double) *ptr;

    return output_buf;
}

exprivate char * conv_short_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    int len;

    CONV_TO_STRING("%hd", short);

    return output_buf;
}

exprivate char * conv_short_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    int len;

    CONV_TO_CARRAY("%hd", short);

    return output_buf;
}

exprivate char * conv_short_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    int *l = (int *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (int)*ptr;

    return output_buf;
}

exprivate char * conv_short_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    short *ptr = (short *)input_buf;
    void** p = (void **)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void*)(long)*ptr;

    return output_buf;
}

/********************************* LONG ****************************************/
exprivate char * conv_long_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    short *l = (short *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (short)*ptr;

    return output_buf;
}

exprivate char * conv_long_char(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *c = (char) *ptr;

    return output_buf;
}

exprivate char * conv_long_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    float *f = (float *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float) *ptr;

    return output_buf;
}

exprivate char * conv_long_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    double *d = (double *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double) *ptr;

    return output_buf;
}

exprivate char * conv_long_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    int len;

    CONV_TO_STRING("%ld", long);

    return output_buf;
}

exprivate char * conv_long_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    long len;
    
    CONV_TO_CARRAY("%ld", long);

    return output_buf;
}

exprivate char * conv_long_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    int *i = (int *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = (int)*ptr;

    return output_buf;
}

exprivate char * conv_long_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    long *ptr = (long *)input_buf;
    void **p = (void **)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void *)(long)*ptr;

    return output_buf;
}


/********************************* BFLD_CHAR ***********************************/
exprivate char * conv_char_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    short *l = (short *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (short)*ptr;

    return output_buf;
}

exprivate char * conv_char_long(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    long *l = (long *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (long) *ptr;

    return output_buf;
}

exprivate char * conv_char_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    float *f = (float *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float) *ptr;

    return output_buf;
}

exprivate char * conv_char_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    double *d = (double *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double) *ptr;

    return output_buf;
}

exprivate char * conv_char_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    int len;

    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len &&  *out_len<2)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "data size: 2 specified :%d", len, *out_len);
        return NULL; /* <<< RETURN */
    }
    else
    {
        output_buf[0] = ptr[0];
        output_buf[1] = EXEOS;

        if (NULL!=out_len)
            *out_len = 2;

    }

    return output_buf;
}

exprivate char * conv_char_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    int len;
    
    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len &&  *out_len<1)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "data size: 1 specified :%d", len, *out_len);
        return NULL; /* <<< RETURN */
    }
    else
    {
        if (NULL!=out_len)
            *out_len = 1;
        output_buf[0] = ptr[0];
    }

    return output_buf;
}

exprivate char * conv_char_int(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    int *i = (int *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = (int) *ptr;

    return output_buf;
}

exprivate char * conv_char_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    unsigned char *ptr = (unsigned char *)input_buf;
    void **p = (void **)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void *)(long) *ptr;

    return output_buf;
}

/********************************* BFLD_FLOAT ***********************************/
exprivate char * conv_float_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    short *l = (short *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (short)*ptr;

    return output_buf;
}

exprivate char * conv_float_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    
    *c = (short) *ptr;

    return output_buf;
}

exprivate char * conv_float_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    double *d = (double *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double) *ptr;

    return output_buf;
}

exprivate char * conv_float_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    long *d = (long *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (long) *ptr;

    return output_buf;
}

exprivate char * conv_float_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    int len;
    char fmt[]="%.0lf";
    fmt[2]+=FLOAT_RESOLUTION;

    CONV_TO_STRING(fmt, float);

    return output_buf;
}

exprivate char * conv_float_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    int len;
    char fmt[]="%.0lf";
    fmt[2]+=FLOAT_RESOLUTION;
    
    CONV_TO_CARRAY(fmt, float);

    return output_buf;
}

exprivate char * conv_float_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    int *i = (int *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = (int)*ptr;

    return output_buf;
}

exprivate char * conv_float_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *ptr = (float *)input_buf;
    void **p = (void **)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void*)(long)*ptr;

    return output_buf;
}

/********************************* BFLD_DOUBLE ***********************************/
exprivate char * conv_double_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    short *l = (short *)output_buf;


    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (short)*ptr;

    return output_buf;
}

exprivate char * conv_double_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *c = (char) *ptr;

    return output_buf;
}

exprivate char * conv_double_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    float *f = (float *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float) *ptr;

    return output_buf;
}

exprivate char * conv_double_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    long *d = (long *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (long) *ptr;

    return output_buf;
}

exprivate char * conv_double_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				   int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    int len;
    char fmt[]="%.0lf";
    fmt[2]+=DOUBLE_RESOLUTION;

    CONV_TO_STRING(fmt, double);

    return output_buf;
}

exprivate char * conv_double_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    int len;
    char fmt[]="%.0lf";
    fmt[2]+=DOUBLE_RESOLUTION;
    
    CONV_TO_CARRAY(fmt, double);

    return output_buf;
}

exprivate char * conv_double_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    int *i = (int *)output_buf;


    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = (int)*ptr;

    return output_buf;
}

exprivate char * conv_double_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    double *ptr = (double *)input_buf;
    void **p = (void **)output_buf;


    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void *)(long)*ptr;

    return output_buf;
}


/********************************* BFLD_STRING ***********************************/
exprivate char * conv_string_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    short *s = (short *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *s = atoi(ptr); /* Bug #210 */

    return output_buf;
}

exprivate char * conv_string_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

     *c = ptr[0];

    return output_buf;
}

exprivate char * conv_string_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    float *f = (float *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = atof(ptr);

    return output_buf;
}

exprivate char * conv_string_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    long *d = (long *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = atol(ptr);

    return output_buf;
}

exprivate char * conv_string_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				   int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    double *d = (double *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = atof(ptr);

    return output_buf;
}

exprivate char * conv_string_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int input_strlen = strlen(input_buf);

    /* Check output size! Check only outgoing conv. Because
     * on incoming dest buffer should be fine!*/
    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len && *out_len > 0 && *out_len < input_strlen)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified :%d", input_strlen, *out_len);
        return NULL; /*<<<< RETURN!*/
    }

    NDRX_STRNCPY(output_buf, input_buf, input_strlen);

    if (NULL!=out_len)
        *out_len = input_strlen;

    return output_buf;
}

exprivate char * conv_string_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    int *i = (int *)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = atoi(ptr);

    return output_buf;
}

exprivate char * conv_string_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    void **p = (void **)output_buf;

    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    /* LP64 */
    sscanf (ptr, "0x%lx", (long *)p);
    
    return output_buf;
}

/********************************* BFLD_CARRAY ***********************************/
exprivate char * conv_carr_short(struct conv_type *t, int cnv_dir, char *input_buf,
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    short *s = (short *)output_buf;
    CARR_TEMP_BUF;
    
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *s = atoi(tmp); /* Bug #210 */

    return output_buf;
}

exprivate char * conv_carr_char(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

     *c = ptr[0];

    return output_buf;
}

exprivate char * conv_carr_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    float *f = (float *)output_buf;
    CARR_TEMP_BUF;
    CHECK_OUTPUT_BUF_SIZE;
    
    if (NULL!=out_len)
        *out_len = to->size;

    *f = atof(tmp);

    return output_buf;
}

exprivate char * conv_carr_long(struct conv_type *t, int cnv_dir, char *input_buf, 
			       int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    long *d = (long *)output_buf;
    CARR_TEMP_BUF;
    CHECK_OUTPUT_BUF_SIZE;
    
    if (NULL!=out_len)
        *out_len = to->size;

    *d = atol(tmp);

    return output_buf;
}

exprivate char * conv_carr_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    double *d = (double *)output_buf;
    CARR_TEMP_BUF;
    CHECK_OUTPUT_BUF_SIZE;
    
    if (NULL!=out_len)
        *out_len = to->size;

    *d = atof(tmp);

    return output_buf;
}

exprivate char * conv_carr_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int input_carrlen = in_len;

    /* Check output size! */
     /* Now we include EOS */
    if (CNV_DIR_OUT==cnv_dir && NULL!=out_len && *out_len > 0 && *out_len < input_carrlen+1)
    {
        ndrx_Bset_error_fmt(BNOSPACE, "data size: %d specified :%d", input_carrlen+1, *out_len);
        return NULL; /*<<<< RETURN!*/
    }

    NDRX_STRNCPY_SRC(output_buf, input_buf, input_carrlen);
    output_buf[input_carrlen] = EXEOS;

    if (NULL!=out_len)
        *out_len = input_carrlen+1; /* We have to count in EOS! */

    return output_buf;
}

exprivate char * conv_carr_int(struct conv_type *t, int cnv_dir, char *input_buf,
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    int *s = (int *)output_buf;
    CARR_TEMP_BUF;
    
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *s = atoi(tmp);

    return output_buf;
}

exprivate char * conv_carr_ptr(struct conv_type *t, int cnv_dir, char *input_buf,
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    char *ptr = (char *)input_buf;
    void **p = (void **)output_buf;
    CARR_TEMP_BUF;
    
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    /* LP64 */
    sscanf(tmp, "0x%lx", (long*)p);

    return output_buf;
}


/********************************* BFLD_INT ***********************************/

exprivate char * conv_int_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    short *l = (short *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (short)*ptr;

    return output_buf;
}

exprivate char * conv_int_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    long *l = (long *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (long)*ptr;

    return output_buf;
}

exprivate char * conv_int_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *c = (char) *ptr;

    return output_buf;
}

exprivate char * conv_int_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    float *f = (float *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float) *ptr;

    return output_buf;
}

exprivate char * conv_int_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    double *d = (double *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double) *ptr;

    return output_buf;
}

exprivate char * conv_int_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    int len;

    CONV_TO_STRING("%d", int);

    return output_buf;
}

exprivate char * conv_int_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    int len;

    CONV_TO_CARRAY("%d", int);

    return output_buf;
}

exprivate char * conv_int_ptr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    int *ptr = (int *)input_buf;
    void **p = (void *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *p = (void *)(long)*ptr;

    return output_buf;
}

/********************************* BFLD_PTR ***********************************/


exprivate char * conv_ptr_short(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void *)input_buf;
    short *s = (short *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *s = (short)(long)*ptr;

    return output_buf;
}

exprivate char * conv_ptr_long(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    long *l = (long *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *l = (long)*ptr;

    return output_buf;
}

exprivate char * conv_ptr_char(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    unsigned char *c = (unsigned char *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *c = (char)(long)*ptr;

    return output_buf;
}

exprivate char * conv_ptr_float(struct conv_type *t, int cnv_dir, char *input_buf, 
				 int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    float *f = (float *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *f = (float)(long) *ptr;

    return output_buf;
}

exprivate char * conv_ptr_double(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    double *d = (double *)output_buf;

    /* Check the data type len for output */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *d = (double)(long)*ptr;

    return output_buf;
}

exprivate char * conv_ptr_string(struct conv_type *t, int cnv_dir, char *input_buf, 
				  int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    int len;

    /* assume working on LP64 */
    CONV_TO_STRING("0x%lx", long);

    return output_buf;
}

exprivate char * conv_ptr_carr(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    int len;

    /* assume working on LP64 */
    CONV_TO_CARRAY("0x%lx", long);

    return output_buf;
}

exprivate char * conv_ptr_int(struct conv_type *t, int cnv_dir, char *input_buf, 
				int in_len, char *output_buf , int *out_len)
{
    dtype_str_t *to = &G_dtype_str_map[t->to_type];
    void **ptr = (void **)input_buf;
    int *i = (int *)output_buf;

    /* Check the data type lengths */
    CHECK_OUTPUT_BUF_SIZE;

    if (NULL!=out_len)
        *out_len = to->size;

    *i = (int)(long)*ptr;

    return output_buf;
}

/* vim: set ts=4 sw=4 et smartindent: */
