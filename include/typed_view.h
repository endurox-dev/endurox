/* 
** VIEW Support structures
**
** @file typed_view.h
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

#ifndef TYPED_VIEW_H
#define	TYPED_VIEW_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <utlist.h>
#include <exhash.h>
#include <fdatatype.h>
#include <view_cmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_VIEW_CNAME_LEN             256  /* Max c field len in struct   */
#define NDRX_VIEW_FLAGS_LEN             16   /* Max flags                   */
#define NDRX_VIEW_NULL_LEN              2660 /* Max len of the null value   */
#define NDRX_VIEW_NAME_LEN   XATMI_SUBTYPE_LEN   /* Max len of view name        */
#define NDRX_VIEW_COMPFLAGS_LEN         128  /* Compiled flags len          */
    
/* field flags: */
#define NDRX_VIEW_FLAG_ELEMCNT_IND_C    0x00000001 /* Include elemnt cnt ind */
#define NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F  0x00000002 /* One way map only C->UBF */
#define NDRX_VIEW_FLAG_LEN_INDICATOR_L  0x00000004 /* Include str/carr len ind */
#define NDRX_VIEW_FLAG_0WAYMAP_N        0x00000008 /* 0 way map (do not copy to ubf) */
#define NDRX_VIEW_FLAG_NULLFILLER_P     0x00000010 /* last char of NULL value is filler */
#define NDRX_VIEW_FLAG_1WAYMAP_C2UBF_S  0x00000020 /* One way map only UBF->C */

    
#define NDRX_VIEW_FIELD_SEPERATORS        " \t"
#define NDRX_VIEW_TOKEN_START            "VIEW"
#define NDRX_VIEW_TOKEN_END              "END"
#define NDRX_VIEW_FLD_SIZE_MAX           65535
#define NDRX_VIEW_FLD_COUNT_MAX          65535
/* will use the same compat base */
#define NDRX_VIEW_UBF_BASE               6000
#define NDRX_VIEW_SIZE_DEFAULT_SIZE      1024

    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * View field object. It can be part of linked list
 * each field may be compiled and contain the field offsets in memory.
 */
typedef struct ndrx_typedview_field ndrx_typedview_field_t;
struct ndrx_typedview_field
{
    char type_name[NDRX_UBF_TYPE_LEN+1]; /* UBF Type name */
    short typecode;                     /* Resolved type code */
    short typecode_full;                /* this is full code (i.e. may include int) */
    char cname[NDRX_VIEW_CNAME_LEN+1];  /* field name in struct */
    char fbname[UBFFLDMAX+1];           /* fielded buffer field to project to */
    BFLDID ubfid;                       /* resolved field id (resolved load time) */
    int count;                          /* array size, 1 - no array, >1 -> array */
    char flagsstr[NDRX_VIEW_FLAGS_LEN+1]; /* string flags */
    long flags;     /* binary flags */
    int size;      /* size of the field, NOTE currently decimal is not supported */
    char nullval[NDRX_VIEW_NULL_LEN+1];       /*Null value */
    char nullval_bin[NDRX_VIEW_NULL_LEN+1];   /*Null value, binary version  */
    int  nullval_bin_len;                     /* length of NULL value... */
    int  nullval_none;                        /* no NULL value */
    int  nullval_default;                     /* defaults "-" is used for nullval */
    int  nullval_quotes;                      /* is quotes used? */
    
    /* type compare values, parsed numerics: */
    short   nullval_short;
    short   nullval_int;
    short   nullval_long;
    short   nullval_float;
    short   nullval_double;
    
    /*
     * char, just use nullval_bin[0]
     * string & carray -> Compare on the fly...
     */
    
    /* type compare values, parsed numerics, end */
    
    /* Compiled meta-data section: */
    int compdataloaded;                   /* Is compiled data loaded? */
    char compflags[NDRX_VIEW_COMPFLAGS_LEN];
    long offset; /* Compiled offset */
    long fldsize; /* element size in bytes */
    long count_fld_offset;
    long length_fld_offset;

    /* Linked list */
    ndrx_typedview_field_t *next, *prev;
    
    EX_hash_handle hh;         /* makes this structure hashable */  
};

/**
 *  View object will have following attributes:
 * - File name
 * - View name
 * List of fields and their attributes
 * hashed by view name
 */
typedef struct ndrx_typedview ndrx_typedview_t;
struct ndrx_typedview
{
    char vname[NDRX_VIEW_NAME_LEN];
    char filename[PATH_MAX+1];
    uint32_t cksum;                 /* 32bit checksum */
    long ssize;                     /* structure size */
    
    ndrx_typedview_field_t *fields;
    ndrx_typedview_field_t *fields_h; /* hash access for fast NULL ops */

    EX_hash_handle hh;         /* makes this structure hashable */    
};

/*---------------------------Globals------------------------------------*/
extern ndrx_typedview_t *ndrx_G_view_hash;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API ndrx_typedview_t * ndrx_view_get_handle(void);
extern NDRX_API int ndrx_view_load_directory(char *dir);
extern NDRX_API int ndrx_view_load_directories(void);
extern NDRX_API void ndrx_view_deleteall(void);
extern NDRX_API ndrx_typedview_t * ndrx_view_get_view(char *vname);
extern NDRX_API void ndrx_view_cksum_update(ndrx_typedview_t *v, char *str, int len);

extern NDRX_API int VIEW_test(typed_buffer_descr_t *descr, char *buf, BFLDLEN len, char *expr);
extern NDRX_API void VIEW_tpfree(typed_buffer_descr_t *descr, char *buf);
extern NDRX_API int VIEW_prepare_incoming (typed_buffer_descr_t *descr, char *rcv_data, 
                        long rcv_len, char **odata, long *olen, long flags);
extern NDRX_API int VIEW_prepare_outgoing (typed_buffer_descr_t *descr, char *idata, long ilen, 
                    char *obuf, long *olen, long flags);
extern NDRX_API char * VIEW_tpalloc (typed_buffer_descr_t *descr, char *subtype, long len);
extern NDRX_API char * VIEW_tprealloc(typed_buffer_descr_t *descr, char *cur_ptr, long len);



#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_VIEW_H */

