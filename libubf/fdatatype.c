/**
 * @brief UBF library
 *   Date type functions. Sizes, put/read data
 *   Enduro Execution Library
 *
 * @file fdatatype.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ubf_int.h>
#include <ubf.h>
#include <fdatatype.h>

#include "ndebug.h"
#include "ferror.h"
#include "atmi_tls.h"
#include <ubf_tls.h>
#include <ubf_impl.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#if EX_ALIGNMENT_BYTES == 8

#define ALIGNED_SIZE(DSIZE) \
    (sizeof(BFLDID)*2 + DSIZE) + DEFAULT_ALIGN - (sizeof(BFLDID)*2 + DSIZE) % DEFAULT_ALIGN;

#else

#define ALIGNED_SIZE(DSIZE) \
    (sizeof(BFLDID) + DSIZE) + DEFAULT_ALIGN - (sizeof(BFLDID) + DSIZE) % DEFAULT_ALIGN;

#endif
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/* Get field sizes */
exprivate int get_fb_dftl_size(dtype_str_t *t, char *fb, int *payload_size);
exprivate int get_fb_string_size(dtype_str_t *t, char *fb, int *payload_size);
exprivate int get_fb_carray_size(dtype_str_t *t, char *fb, int *payload_size);

/* Function for putting data into buffer */
exprivate int put_data_dflt(dtype_str_t *t, char *fb, BFLDID bfldid, char *data, int len);
exprivate int put_data_string(dtype_str_t *t, char *fb, BFLDID bfldid, char *data, int len);
exprivate int put_data_carray(dtype_str_t *t, char *fb, BFLDID bfldid, char *data, int len);

exprivate int get_d_size_dftl (struct dtype_str *t, char *data, int len, int *payload_size);
exprivate int get_d_size_string (struct dtype_str *t, char *data, int len, int *payload_size);
exprivate int get_d_size_carray (struct dtype_str *t, char *data, int len, int *payload_size);

exprivate int get_data_dflt (struct dtype_str *t, char *fb, char *buf, int *len);
exprivate int get_data_str (struct dtype_str *t, char *fb, char *buf, int *len);
exprivate int get_data_carr (struct dtype_str *t, char *fb, char *buf, int *len);

exprivate int g_dflt_empty(struct dtype_ext1* t);
exprivate int g_str_empty(struct dtype_ext1* t);
exprivate int g_carr_empty(struct dtype_ext1* t);

exprivate int put_empty_dftl(struct dtype_ext1* t, char *fb, BFLDID bfldid);
exprivate int put_empty_str(struct dtype_ext1* t, char *fb, BFLDID bfldid);
exprivate int put_empty_carr(struct dtype_ext1* t, char *fb, BFLDID bfldid);

exprivate void dump_short (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_long (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_char (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_float (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_double (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_string (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_carray (struct dtype_ext1 *t, char *text, char *data, int *len);
exprivate void dump_int (struct dtype_ext1 *t, char *text, char *data, int *len);

exprivate char *tbuf_short (struct dtype_ext1 *t, int len);
exprivate char *tbuf_long (struct dtype_ext1 *t, int len);
exprivate char *tbuf_char (struct dtype_ext1 *t, int len);
exprivate char *tbuf_float (struct dtype_ext1 *t, int len);
exprivate char *tbuf_double (struct dtype_ext1 *t, int len);
exprivate char *tbuf_string (struct dtype_ext1 *t, int len);
exprivate char *tbuf_carray (struct dtype_ext1 *t, int len);
exprivate char *tbuf_int (struct dtype_ext1 *t, int len);

exprivate char *tallocdlft (struct dtype_ext1 *t, int *len);

/**
 * Functions to compare field values.
 */
exprivate int cmp_short (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_long (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_int (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_char (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_float (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_double (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_string (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
exprivate int cmp_carray (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);

/* for sparc we set to 8 */
#define DEFAULT_ALIGN       EX_ALIGNMENT_BYTES
/**
 * We operate with 32 bit align.
 */
expublic dtype_str_t G_dtype_str_map[] =
{
/* str type   typeid       default size align elm fb size       put data in fb   get data size   get data from fb */
{"short",	BFLD_SHORT,  BFLD_SHORT_SIZE,  4, get_fb_dftl_size,  put_data_dflt,  get_d_size_dftl, get_data_dflt},     /* 0 */
{"long",	BFLD_LONG,   BFLD_LONG_SIZE,   8, get_fb_dftl_size,  put_data_dflt,  get_d_size_dftl, get_data_dflt},     /* 1 */
{"char",	BFLD_CHAR,   BFLD_CHAR_SIZE,   4, get_fb_dftl_size,  put_data_dflt,  get_d_size_dftl, get_data_dflt},     /* 2 */
{"float",	BFLD_FLOAT,  BFLD_FLOAT_SIZE,  4, get_fb_dftl_size,  put_data_dflt,  get_d_size_dftl, get_data_dflt},     /* 3 */
{"double",	BFLD_DOUBLE, BFLD_DOUBLE_SIZE, 8, get_fb_dftl_size,  put_data_dflt,  get_d_size_dftl, get_data_dflt},     /* 4 */
{"string",	BFLD_STRING, BFLD_STRING_SIZE, 4, get_fb_string_size, put_data_string,  get_d_size_string, get_data_str}, /* 5 */
{"carray",	BFLD_CARRAY, BFLD_CARRAY_SIZE, 4, get_fb_carray_size, put_data_carray, get_d_size_carray, get_data_carr}, /* 6 */
{"int",		BFLD_INT,    BFLD_INT_SIZE,    4, get_fb_dftl_size, put_data_dflt, get_d_size_dftl, get_data_dflt},	  /* 7 */
{""}
};


#define DAO EX_ALIGNMENT_BYTES
#define DAC EX_ALIGNMENT_BYTES+4 /* for carray having some extra... */

/**
 * We operate with 32 bit align.
 */
expublic dtype_ext1_t G_dtype_ext1_map[] =
{
/* type,    get default,  put empty deflt, dump,    datoff, tmpbuf fn, alloc buf */
{BFLD_SHORT, g_dflt_empty, put_empty_dftl, dump_short, DAO, tbuf_short, tallocdlft, cmp_short},     /* 0 */
{BFLD_LONG,  g_dflt_empty, put_empty_dftl, dump_long,  DAO, tbuf_long,  tallocdlft, cmp_long},      /* 1 */
{BFLD_CHAR,  g_dflt_empty, put_empty_dftl, dump_char,  DAO, tbuf_char,  tallocdlft, cmp_char},      /* 2 */
{BFLD_FLOAT, g_dflt_empty, put_empty_dftl, dump_float, DAO, tbuf_float, tallocdlft, cmp_float},     /* 3 */
{BFLD_DOUBLE,g_dflt_empty, put_empty_dftl, dump_double,DAO, tbuf_double,tallocdlft, cmp_double},    /* 4 */
{BFLD_STRING,g_str_empty,  put_empty_str,  dump_string,DAO, tbuf_string,tallocdlft, cmp_string},    /* 5 */
{BFLD_CARRAY,g_carr_empty, put_empty_carr, dump_carray,DAC, tbuf_carray,tallocdlft, cmp_carray},      /* 6 */
{BFLD_INT,  g_dflt_empty, put_empty_dftl,  dump_int,   DAO, tbuf_int,   tallocdlft, cmp_int},       /* 1 */
{-1}
};
/*********************** Basic data type operations ***************************/

/******************************************************************************/
/* Bellow functions returns data block size in bisubf buffer. This data includes
 * data length it self + service information (BFLDID size, etc..) */
/******************************************************************************/

/**
 * Get element size
 * @param t
 * @param data
 * @return
 */
exprivate int get_fb_dftl_size(dtype_str_t *t, char *fb, int *payload_size)
{
    if (NULL!=payload_size)
        *payload_size = t->size;

    /* e.g. sizeof(BFLDID) + 1 (char) + 4 - ( 4 + 1) % 4 =
     * 5 + 4- 5 % 4 = 5 + 4 - 1 = 8*/
/*    return (sizeof(BFLDID) + t->size) + DEFAULT_ALIGN - (sizeof(BFLDID) + t->size) % DEFAULT_ALIGN; */
    return ALIGNED_SIZE(t->size);
}

/**
 * Returns the field size used by string
 * @param t - record descriptor
 * @param data - FB
 * @return length in bytes
 */
exprivate int get_fb_string_size(dtype_str_t *t, char *fb, int *payload_size)
{
    UBF_string_t *str = (UBF_string_t *)fb;
    int data_size = strlen(str->str) + 1;
    /* int aligned; 
    int tmp;*/
    if (NULL!=payload_size)
        *payload_size = data_size;

    /*
    aligned = sizeof(BFLDID) + data_size;
    tmp=aligned%DEFAULT_ALIGN;
    aligned = aligned+ (tmp>0?DEFAULT_ALIGN-tmp:0);
    */
    
    
    
    /* Including EOS (+1) */
    /* return aligned; */
    return ALIGNED_SIZE(data_size);
}
/**
 * Get data size which are placed into bisubf buffer.
 * @param t
 * @param data
 * @return
 */
exprivate int get_fb_carray_size(dtype_str_t *t, char *fb, int *payload_size)
{
    UBF_carray_t *carr = (UBF_carray_t *)fb;
    /* int aligned;
    int tmp; */
    if (NULL!=payload_size)
        *payload_size = carr->dlen;

    /*
    aligned = (sizeof (BFLDID) + sizeof (BFLDLEN) + carr->dlen);
    tmp=aligned%DEFAULT_ALIGN;
    aligned = aligned+ (tmp>0?DEFAULT_ALIGN-tmp:0);
    */
    
    return ALIGNED_SIZE((sizeof (BFLDLEN) + carr->dlen));
}

/******************************************************************************/
/* Put data into bisubf buffer. This initializes all service information
 * and it copies data itself into buffer */
/******************************************************************************/
/**
 * Put data into buffer - default function usable for fixed length data types
 * NOTE: May require split-up for endiannes.
 * @param t data type
 * @param fb pointer to bisubf buffer
 * @param bfldid field ID
 * @param data pointer to data
 * @param len buffer size???
 * @return
 */
exprivate int put_data_dflt(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    UBF_generic_t *dflt = (void *)fb;
    /* int align; */
    dflt->bfldid = bfldid;
    memcpy(dflt->d, data, t->size);
    /* align=t->aligned_size-t->size; */

    /* Reset to 0 aligned memory 
     * TODO, do we need this reset?

    if (align > 0)
        memset(dflt->d+t->size, 0, align);
     */
    return EXSUCCEED;
}

/**
 * Put some stuffy string in!
 * Return last pointer!?!?
 * As from spec, we are not interested in length!
 */
exprivate int put_data_string(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    UBF_string_t *str = (void *)fb;
    /* int tlen = strlen(data)+1; */
    /*int align = tlen % DEFAULT_ALIGN;  This assumes that fieldid is aligned already */
    
    strcpy(str->str, data);
    str->bfldid=bfldid;

    /*
    if (align>0)
    {
        align=DEFAULT_ALIGN-align;
        memset(str->str + tlen, 0, align);
    }
    */
    return EXSUCCEED;
}

exprivate int put_data_carray(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len)
{
    UBF_carray_t *carr = (UBF_carray_t *)fb;
    /* int align; */
    carr->bfldid = bfldid;
    carr->dlen = len;

    if (len>0) /* Do not play with memory if length is 0 */
    {
        /*align = len % DEFAULT_ALIGN;  This assumes that fieldid & dlen is aligned already */
        memcpy(carr->carr, data, len);
        /*
        if (align>0)
        {
            align=DEFAULT_ALIGN-align;
            memset(carr->carr + len, 0, align);
        }
         * */
    }
    
    return EXSUCCEED;
}

/******************************************************************************/
/* This function data size including service information, that could cost
 * if added to bisubf buffer. I.e. this returns target size of the data if put
 * into the buffer */
/******************************************************************************/

/**
 * Get data size for elements from table
 * @param t
 * @param data
 * @param len
 * @return
 */
exprivate int get_d_size_dftl (struct dtype_str *t, char *data, 
        int len, int *payload_size)
{
    if (NULL!=payload_size)
        *payload_size = t->size;
    return /* t->aligned_size + sizeof(BFLDID); */ ALIGNED_SIZE(t->size);
}

/**
 * Return the size of new string data
 * @param t
 * @param data
 * @param len
 * @return
 */
exprivate int get_d_size_string (struct dtype_str *t, char *data, 
        int len, int *payload_size)
{
    int str_data_len = strlen(data)+1;
/*    int aligned;
    int tmp; */
    /* Counting: BFLDID, STRLEN + EOS */
    if (NULL!=payload_size)
        *payload_size = str_data_len;

    /*
    aligned = (sizeof(BFLDID)+str_data_len);
    tmp=aligned%DEFAULT_ALIGN;
    aligned = aligned+ (tmp>0?DEFAULT_ALIGN-tmp:0);
    */
    
    return ALIGNED_SIZE(str_data_len);
}

/**
 * Return data size of character array element. 
 * @param t
 * @param data
 * @param len
 * @return
 */
exprivate int get_d_size_carray (struct dtype_str *t, char *data, 
        int len, int *payload_size)
{
    /* Counting: BFLDID, DLEN + data len*/
    /*
    int aligned;
    int tmp;*/
    if (NULL!=payload_size)
        *payload_size=len;

    /*
    aligned = (sizeof(BFLDID)+sizeof(BFLDLEN)+len);
    tmp=aligned%DEFAULT_ALIGN;
    aligned = aligned+ (tmp>0?DEFAULT_ALIGN-tmp:0);
    */
    
    return ALIGNED_SIZE((sizeof(BFLDLEN)+len));
}

/******************************************************************************/
/* These functions returns the data from bisubf buffer. Buffer lenght is
 * checked. Functions also return size of data copied into destination buffer */
/******************************************************************************/

/**
 * Read data (default)
 * NOTE: May require split-up for endiannes.
 * @param t descriptor table
 * @param fb address in FB
 * @param buf address to output buffer
 * @param len field length
 * @return
 */
exprivate int get_data_dflt (struct dtype_str *t, char *fb, char *buf, int *len)
{
    UBF_generic_t *dflt = (void *)fb;
    int ret=EXSUCCEED;
    
    if (NULL!=len && *len < t->size)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", t->size, *len);
        EXFAIL_OUT(ret);
    }

    memcpy(buf, dflt->d, t->size);

    /* Return the size to caller */
    if (NULL!=len)
    {
        *len = t->size;
    }
    
out:
    return ret;
}

/**
 * Get String data
 * @param t
 * @param fb
 * @param buf
 * @param len
 * @return
 */
exprivate int get_data_str (struct dtype_str *t, char *fb, char *buf, int *len)
{
    UBF_string_t *str = (UBF_string_t *)fb;
    int data_len = strlen(str->str)+1;
    int ret=EXSUCCEED;

    if (NULL!=len && *len < data_len)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", data_len, *len);
        EXFAIL_OUT(ret);
    }
    else
    {
        strcpy(buf, str->str);
        ret = EXSUCCEED;
    }
    
    /* Return the size to caller */
    if (NULL!=len)
    {
        *len = data_len;
    }
    
out:
    return ret;
}

/**
 * Return character array data
 * @param t
 * @param fb
 * @param buf
 * @param len
 * @return
 */
exprivate int get_data_carr (struct dtype_str *t, char *fb, char *buf, int *len)
{
    UBF_carray_t *carr = (UBF_carray_t *)fb;
    int ret=EXSUCCEED;

    if (NULL!=len && *len>0 && *len < carr->dlen)
    {
        /* Set error, that string buffer too short */
        ndrx_Bset_error_fmt(BNOSPACE, "output buffer too short. Data len %d in buf, "
                                "output: %d", carr->dlen, *len);
        EXFAIL_OUT(ret);
    }
    else
    {
        /* copy the data */
        memcpy(buf, carr->carr, carr->dlen);
        *len=carr->dlen;
    }
out:
    return ret;
}
/*********************** Extended 1 data type operations **********************/

/******************************************************************************/
/* Bellow functions return empty element data size (including service info)   */
/******************************************************************************/

/**
 * Get default empty element size
 * @param t
 * @return
 */
exprivate int g_dflt_empty(struct dtype_ext1* t)
{
    /*
    int len = G_dtype_str_map[t->fld_type].aligned_size + sizeof(BFLDID);
    return len;
    */
    return ALIGNED_SIZE(G_dtype_str_map[t->fld_type].size);
}

/**
 * Return empty string size
 * @param t
 * @return
 */
exprivate int g_str_empty(struct dtype_ext1* t)
{
    /*
    int len;
    len = sizeof(BFLDID)+4; /* actually we need +1, but +3 for align
    return len;
    */
    return ALIGNED_SIZE(1); /* empty string eos */
}
/**
 * Return empty character array size
 * @param t
 * @return
 */
exprivate int g_carr_empty(struct dtype_ext1* t)
{
    /*
    int len;
    len = sizeof(BFLDID)+sizeof(BFLDLEN);  This is already aligned 
    return len;
     * */
    return ALIGNED_SIZE(sizeof(BFLDLEN)); /* empty string eos */
}

/******************************************************************************/
/* Bellow functions puts empty data into bisubf buffer                       */
/******************************************************************************/


/**
 * Put empty default data type (fill with 0 all element)
 * This also applies on chars and doubles (which on some platforms could be wrong)
 * This also uses alignment.
 * @param t
 * @param fb
 * @return
 */
exprivate int put_empty_dftl(struct dtype_ext1* t, char *fb, BFLDID bfldid)
{
    int ret=EXSUCCEED;
    int elem_size = G_dtype_str_map[t->fld_type].size;
    
    UBF_generic_t *fld = (UBF_generic_t *)fb;
    fld->bfldid = bfldid;
    memset(fld->d, 0, elem_size); /* Set to default 0 */

    return ret;
}

/**
 * Put empty string element
 * @param t
 * @param fb
 * @return
 */
exprivate int put_empty_str(struct dtype_ext1* t, char *fb, BFLDID bfldid)
{
    int ret=EXSUCCEED;

    UBF_string_t *fld = (UBF_string_t *)fb;
    fld->bfldid = bfldid;
    /* Uses alignmnet */
    fld->str[0] = EXEOS;
    /*
    memset(fld->str, 0, DEFAULT_ALIGN);
    */
    return ret;
}

/**
 * Put empty character array element
 * Function is already aligned.
 * @param t
 * @param fb
 * @return
 */
exprivate int put_empty_carr(struct dtype_ext1* t, char *fb, BFLDID bfldid)
{
    int ret=EXSUCCEED;

    UBF_carray_t *fld = (UBF_carray_t *)fb;
    fld->bfldid = bfldid;
    fld->dlen = 0;
    /*fld->carr - intentionally not setting, because there is no data!*/
    return ret;
}


/******************************************************************************/
/* Bellow functions dump the value depending on it's type                     */
/* this is part of dtype_ext1->p_dump_data                                    */
/******************************************************************************/

exprivate void dump_short (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        short s = (short)*data;
        UBF_LOG(log_debug, "%s:\n[%hd]", text, s);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_long (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        long *l = (long *)data;
        UBF_LOG(log_debug, "%s:\n[%ld]", text, *l);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_char (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        char *c = (char *)data;
        UBF_LOG(log_debug, "%s:\n[%c]", text, *c);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_float (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        float *f = (float *)data;
        UBF_LOG(log_debug, "%s:\n[%.6f]", text, *f);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_double (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        double *d = (double *)data;
        UBF_LOG(log_debug, "%s:\n[%.13f]", text, *d);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_string (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        UBF_LOG(log_debug, "%s:\n[%s]", text, data);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

exprivate void dump_carray (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data && NULL!=len)
    {
        UBF_DUMP(log_debug, text, data, *len);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null data or len]", text);
    }
}

exprivate void dump_int (struct dtype_ext1 *t, char *text, char *data, int *len)
{
    if (NULL!=data)
    {
        int *l = (int *)data;
        UBF_LOG(log_debug, "%s:\n[%d]", text, *l);
    }
    else
    {
        UBF_LOG(log_debug, "%s:\n[null]", text);
    }
}

/******************************************************************************/
/* Bellow functions returns temporary buffer space that could be used by      */
/* calls like CBfind.                                                         */
/* this is part of dtype_ext1->p_tbuf                                         */
/******************************************************************************/

exprivate char *tbuf_short (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;
    return (char *)&G_ubf_tls->tbuf_s;
}

exprivate char *tbuf_long (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;
    return (char *)&G_ubf_tls->tbuf_l;
}

exprivate char *tbuf_char (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;
    return (char *)&G_ubf_tls->tbuf_c;
}

exprivate char *tbuf_float (struct dtype_ext1 *t, int len)
{
   UBF_TLS_ENTRY;
   return (char *)&G_ubf_tls->tbuf_f;
}

exprivate char *tbuf_double (struct dtype_ext1 *t, int len)
{
   UBF_TLS_ENTRY;
   return (char *)&G_ubf_tls->tbuf_d;
}

/**
 * We will re-alloc the buffer each time
 * @param t - ptr to ext1 struct
 * @param len - potential data size
 * @return
 */
exprivate char *tbuf_string (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;

    if (G_ubf_tls->str_dat_len!=len)
    {
        /* Free up the memory */
        if (NULL!=G_ubf_tls->str_buf_ptr)
        {
            NDRX_FREE(G_ubf_tls->str_buf_ptr);
        }

        G_ubf_tls->str_buf_ptr=NDRX_MALLOC(len);

        if (NULL==G_ubf_tls->str_buf_ptr)
        {
            /* set error */
            ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate string tmp "
                    "space for %d bytes", len);
        }
        else
        {
            UBF_LOG(log_debug, "tbuf_string: allocated %d bytes", len);
        }
    }
    else
    {
        UBF_LOG(log_debug, "tbuf_string: re-using existing space", len);
    }
    
    return G_ubf_tls->str_buf_ptr;
}

/**
 * We will re-alloc the buffer each time
 * @param t - ptr to ext1 struct
 * @param len - potential data size
 * @return
 */
exprivate char *tbuf_carray (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;

    if (G_ubf_tls->carray_dat_len!=len)
    {
        /* Free up the memory */
        if (NULL!=G_ubf_tls->carray_buf_ptr)
        {
            NDRX_FREE(G_ubf_tls->carray_buf_ptr);
        }
        
        G_ubf_tls->carray_buf_ptr=NDRX_MALLOC(len);

        if (NULL==G_ubf_tls->carray_buf_ptr)
        {
            /* set error */
            ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate carray tmp space "
                    "for %d bytes", len);
        }
        else
        {
            UBF_LOG(log_debug, "tbuf_carray: allocated %d bytes", len);
        }
    }
    else
    {
        UBF_LOG(log_debug, "tbuf_carray: re-using existing space", len);
    }

    return G_ubf_tls->carray_buf_ptr;
}

exprivate char *tbuf_int (struct dtype_ext1 *t, int len)
{
    UBF_TLS_ENTRY;
    return (char *)&G_ubf_tls->tbuf_i;
}

/******************************************************************************/
/* Bellow functions are designed for buffer allocation for conversation space.*/
/* User is responsible to free it up                                          */
/******************************************************************************/

/**
 * Function currently common for all data types.
 * This basically just allocates the memory. It is caller's responsiblity to
 * to set correct amount to use. It is suggested that amount should be over
 * CF_TEMP_BUF_MAX define.
 * @param t
 * @param len - buffer lenght to allocate.
 * @return NULL/ptr to allocated mem
 */
exprivate char *tallocdlft (struct dtype_ext1 *t, int *len)
{
    char *ret=NULL;
    int alloc_size = *len;
    ret=NDRX_MALLOC(alloc_size);

    if (NULL==ret)
    {
        ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate %d bytes for user", alloc_size);
    }
    else
    {
        *len = alloc_size;
    }

    return ret;
}

/******************************************************************************/
/* Bellow functions are designed to compare two values of the data type       */
/* part of dtype_ext1->p_cmp                                                  */
/******************************************************************************/

exprivate int cmp_short (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2,
    long mode)
{
    short *s1 = (short *)val1;
    short *s2 = (short *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        return  *s1 - *s2;
    }
    else
    {
        return (*s1==*s2);
    }
}

exprivate int cmp_long (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2,
    long mode)
{
    long *l1 = (long *)val1;
    long *l2 = (long *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        long lres = *l1 - *l2;
        
        if (lres > 0)
            return 1;
        else if (lres < 0)
            return -1;
        else
            return 0;
    }
    else
    {
        return (*l1==*l2);
    }
}

exprivate int cmp_int (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2,
    long mode)
{
    int *i1 = (int *)val1;
    int *i2 = (int *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        return (*i1 - *i2);
    }
    else
    {
        return (*i1==*i2);
    }
}

exprivate int cmp_char (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    unsigned char *c1 = (unsigned char *)val1;
    unsigned char *c2 = (unsigned char *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        return (*c1 - *c2);
    }
    else
    {
        return (*c1==*c2);
    }
}

exprivate int cmp_float (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    float *f1 = (float *)val1;
    float *f2 = (float *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        float f = *f1-*f2;
        
        if (fabs(f) < FLOAT_EQUAL )
        {
            return 0;
        }
        else if (f > 0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
        
    }
    else
    {
        return (fabs(*f1-*f2)<FLOAT_EQUAL);
    }
    
}

exprivate int cmp_double (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    double *f1 = (double *)val1;
    double *f2 = (double *)val2;
    
    if (mode & UBF_CMP_MODE_STD)
    {
        double f = *f1-*f2;
        
        if (fabs(f) < FLOAT_EQUAL )
        {
            return 0;
        }
        else if (f > 0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
        
    }
    else
    {
        return (fabs(*f1-*f2)<DOUBLE_EQUAL);
    }
}

/**
 * NOTE: val1 - assuming is user API input!
 * Last expression is being cached for better performance.
 * @param t
 * @param val1 
 * @param len1 
 * @param val2 - user input, may contain regular expression to match on buffer
 * @param len2 - if !=0, then using regexp
 * @return
 */
exprivate int cmp_string (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    int ret=EXSUCCEED;
    char *fn = "cmp_string";

    if (mode & UBF_CMP_MODE_STD)
    {
        return strcmp(val1, val2);
    }
    else if (0==len2)
    {
        ret=(0==strcmp(val1, val2));
    }
    else
    {
        static regex_t re;
        static char *cashed_string=NULL;
        int tmp_len;
        char *tmp_regex=NULL;
        int err;

        if (NULL==cashed_string || 0!=strcmp(val2, cashed_string))
        {
            if (NULL!=cashed_string)
            {
                UBF_LOG(log_debug, "Freeing-up previously allocated "
                                                    "resources");
                NDRX_FREE(cashed_string);
                regfree(&re);
            }

            tmp_len= strlen(val2)+1; /* + EOS*/
            cashed_string = NDRX_MALLOC(tmp_len); /* +EOS */
            tmp_regex = NDRX_MALLOC(tmp_len+2); /* ^$ */

            if (NULL==cashed_string)
            {
                ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate %d bytes", tmp_len);
                ret=EXFAIL;
            }

            if (NULL==tmp_regex)
            {
                ndrx_Bset_error_fmt(BMALLOC, "Failed to allocate %d bytes", tmp_len);
                ret=EXFAIL;
            }
            else
            {
                strcpy(tmp_regex+1, val2);
                tmp_regex[0] = '^';
                tmp_len = strlen(tmp_regex);
                tmp_regex[tmp_len] = '$';
                tmp_regex[tmp_len+1] = '\0'; /* put ending EOS */
            }

            if (EXSUCCEED==ret)
            {
                UBF_LOG(log_debug, "%s:Compiling regex [%s]", fn, tmp_regex);
            }

            if (EXSUCCEED==ret && EXSUCCEED!=(err=regcomp(&re, tmp_regex, 
                    REG_EXTENDED | REG_NOSUB)))
            {
                ndrx_report_regexp_error("regcomp", err, &re);
                ret=EXFAIL;
            }
            else if (EXSUCCEED==ret)
            {
                strcpy(cashed_string, val2);
                UBF_LOG(log_debug, "%s:REGEX: Compiled OK", fn);
            }

            /* Free up allocated cache */
            if (EXFAIL==ret)
            {
                if (NULL!=cashed_string)
                {
                    NDRX_FREE(cashed_string); /* free up cached string for next time */
                    cashed_string=NULL;
                }
            }

            /* Free up temporary regexp expression */
            if (NULL!=tmp_regex)
            {
                NDRX_FREE(tmp_regex);
            }
        }

        if (EXSUCCEED==ret && EXSUCCEED==regexec(&re, val1, (size_t) 0, NULL, 0))
        {
            UBF_LOG(log_debug, "%s:REGEX: Matched", fn);
            ret=EXTRUE;
        }
        else
        {
            UBF_LOG(log_debug, "%s:REGEX: NOT Matched", fn);
        }   
    }

    return ret;
}

exprivate int cmp_carray (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode)
{
    
    if (mode & UBF_CMP_MODE_STD)
    {
        long ldiff = len1 - len2;
        
        
        if ( ldiff > 0)
        {
            return 1;
        }
        else if ( ldiff < 0)
        {
            return -1;
        }
        
        return memcmp(val1, val2, len1);
    }
    else
    {
        if (len1!=len2)
            return EXFALSE;
        
        return (0==memcmp(val1, val2, len1));
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
