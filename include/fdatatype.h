/* 
** Data type definitions headers/functions externs
**
** @file fdatatype.h
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

#ifndef FDATATYPE_H
#define	FDATATYPE_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
#include <ubf_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_UBF_TYPE_LEN             32 /* Storage for field type */

/* Language mode... */    
#define HDR_MIN_LANG            0
#define HDR_C_LANG              0         /* Default goes to C                */
#define HDR_GO_LANG             1         /* Golang                           */
#define HDR_MAX_LANG            1

    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
struct dtype_str {
    char *fldname;
    _UBF_SHORT fld_type;
    size_t size;
    int aligned_size;
    int (*p_next) (struct dtype_str*, char *fb, int *payload_size); /* Field size used */
    int (*p_put_data) (struct dtype_str *t, char *fb, BFLDID bfldid, char *data, int len);
    int (*p_get_data_size) (struct dtype_str *t, char *data, int len, int *payload_size);
    int (*p_get_data) (struct dtype_str *t, char *fb, char *buf, int *len);
};

typedef struct dtype_str dtype_str_t;

/**
 * Logically this extension of the dtype_str
 */
struct dtype_ext1 {
    _UBF_SHORT fld_type;
    int (*p_empty_sz) (struct dtype_ext1* t); /* size in bytes of empty entry of the data type */
    /**
     * This function puts in memory block pointe by fb an empty instance
     * of the data type element.
     */
    int (*p_put_empty) (struct dtype_ext1* t, char *fb, BFLDID bfldid);
    void (*p_dump_data) (struct dtype_ext1 *t, char *text, char *data, int *len);
    int hdr_size; /* header size (bytes afer which data starts in FB */
    /* Get temporary buffer space */
    char *(*p_tbuf) (struct dtype_ext1 *t, int len);
    /* allocate conversation buffer for user. */
    char *(*p_talloc) (struct dtype_ext1 *t, int *len);
    /* Fn to compare two values of data type */
    int (*p_cmp) (struct dtype_ext1 *t, char *val1, BFLDLEN len1, char *val2, 
        BFLDLEN len2, long mode);
};

typedef struct dtype_ext1 dtype_ext1_t;

/* UBF Header record */
struct UBF_header
{
    unsigned char       buffer_type;
    unsigned char       version;
    char                magic[UBF_MAGIC_SIZE];
    
    /* cache offsets: 
    TODO: 
    BFLDLEN      cache_short_off; - short is in start anyway.
    */
    BFLDLEN      cache_long_off;
    BFLDLEN      cache_char_off;
    BFLDLEN      cache_float_off;
    BFLDLEN      cache_double_off;
    BFLDLEN      cache_string_off;
    BFLDLEN      cache_carray_off;
    
    BFLDLEN      buf_len; /* includes header */
    _UBF_INT     opts;
    BFLDLEN      bytes_used;
    BFLDID       bfldid;
};

typedef struct UBF_header UBF_header_t;


/*
 * Struct for string
 */
struct UBF_string
{
    BFLDID   bfldid;
    char str[1];
};
typedef struct UBF_string UBF_string_t;

/*
 * Struct for string
 */
struct UBF_carray
{
    BFLDID   bfldid;
    BFLDLEN  dlen; /* Data len */
    /* If len is 0, then this already part of next structure?!?! */
    char carr[1];
};
typedef struct UBF_carray UBF_carray_t;

/*
 * Struct for char
 */
struct UBF_char
{
    BFLDID   bfldid;
    UBF_CHAR cval;
};
typedef struct UBF_char UBF_char_t;


/*
 * Struct for double
 */
struct UBF_double
{
    BFLDID   bfldid;
    UBF_DOUBLE dval;
};
typedef struct UBF_double UBF_double_t;

/*
 * Struct for float
 */
struct UBF_float
{
    BFLDID   bfldid;
    UBF_FLOAT dval;
};
typedef struct UBF_float UBF_float_t;

/*
 * Struct for long
 */
struct UBF_long
{
    BFLDID   bfldid;
    UBF_LONG lval;
};
typedef struct UBF_long UBF_long_t;


/*
 * Generic (for system defined type processing)
 */
struct UBF_generic
{
    BFLDID bfldid;
    char d[1];
};
typedef struct UBF_generic UBF_generic_t;


/**
 * field definition found in .fd files
 */
typedef struct UBF_field_def UBF_field_def_t;
struct UBF_field_def
{
    BFLDID bfldid;
    _UBF_SHORT fldtype;
    char fldname[UBFFLDMAX+1];
    
    EX_hash_handle hh; /* makes this structure hashable */
    UBF_field_def_t *next; /* next with the same hash ID! */
};

/*---------------------------Globals------------------------------------*/
extern NDRX_API dtype_str_t G_dtype_str_map[];
extern NDRX_API dtype_ext1_t G_dtype_ext1_map[];
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* FDATATYPE_H */

