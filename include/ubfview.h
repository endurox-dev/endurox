/**
 * @brief VIEW Support structures
 *
 * @file ubfview.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#ifndef UBFVIEW_H
#define	UBFVIEW_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <utlist.h>
#include <exhash.h>
#include <fdatatype.h>
#include <view_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
/* field flags: */
#define NDRX_VIEW_FLAG_ELEMCNT_IND_C    0x00000001 /* Include elemnt cnt ind */
#define NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F  0x00000002 /* One way map only C->UBF */
#define NDRX_VIEW_FLAG_LEN_INDICATOR_L  0x00000004 /* Include str/carr len ind */
#define NDRX_VIEW_FLAG_0WAYMAP_N        0x00000008 /* 0 way map (do not copy to ubf) */
#define NDRX_VIEW_FLAG_NULLFILLER_P     0x00000010 /* last char of NULL value is filler */
#define NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S  0x00000020 /* One way map only UBF->C */

    
#define NDRX_VIEW_FIELD_SEPERATORS        " \t"
#define NDRX_VIEW_TOKEN_START            "VIEW"
#define NDRX_VIEW_TOKEN_END              "END"
#define NDRX_VIEW_EMPTY_PARAM            "-"
#define NDRX_VIEW_FLD_SIZE_MAX           65535
#define NDRX_VIEW_FLD_COUNT_MAX          65535
/* will use the same compat base */
#define NDRX_VIEW_UBF_BASE               6000
#define NDRX_VIEW_SIZE_DEFAULT_SIZE      1024


#define NDRX_VIEW_QUOTES_NONE           0
#define NDRX_VIEW_QUOTES_SINGLE         1
#define NDRX_VIEW_QUOTES_DOUBLE         2
    
#define NDRX_VIEW_COUNT_SETUP\
	if (f->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)\
	{\
	    C_count = (short *)(cstruct+f->count_fld_offset);\
	}\
	else\
	{\
            C_count_stor = f->count;\
	    C_count = &C_count_stor;\
	}

#define NDRX_VIEW_LEN_SETUP(OCC, DIMSZ)\
	if (f->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)\
	{\
	    L_length = (unsigned short *)(cstruct+f->length_fld_offset+\
		    OCC*sizeof(unsigned short));\
	}\
	else\
	{\
	    L_length_stor = DIMSZ;\
	    L_length = &L_length_stor;\
	}\

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
    int     nullval_int;
    long    nullval_long;
    float   nullval_float;
    double  nullval_double;
    
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
extern NDRX_API int ndrx_view_init(void);
extern NDRX_API int ndrx_view_chkload_directories(void);
extern NDRX_API void ndrx_view_deleteall(void);
extern NDRX_API ndrx_typedview_t * ndrx_view_get_view(char *vname);
extern NDRX_API ndrx_typedview_field_t * ndrx_view_get_field(ndrx_typedview_t *v, char *cname);
extern NDRX_API void ndrx_view_cksum_update(ndrx_typedview_t *v, char *str, int len);

extern NDRX_API int ndrx_Bvnull(char *cstruct, char *cname, BFLDOCC occ, char *view);
extern NDRX_API int ndrx_Bvnull_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f, 
        BFLDOCC occ, char *cstruct);

extern NDRX_API int ndrx_Bvselinit_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f,  
        BFLDLEN single_occ, char *cstruct);
extern NDRX_API int ndrx_Bvselinit(char *cstruct, char *cname, char *view);

extern NDRX_API int ndrx_Bvsinit_int(ndrx_typedview_t *v, char *cstruct);
extern NDRX_API int ndrx_Bvsinit(char *cstruct, char *view);


extern NDRX_API int ndrx_Bvopt_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f, int option);
extern NDRX_API int ndrx_Bvopt(char *cname, int option, char *view) ;

extern NDRX_API int ndrx_Bvftos_int(UBFH *p_ub, ndrx_typedview_t *v, char *cstruct);
extern NDRX_API int ndrx_Bvftos(UBFH *p_ub, char *cstruct, char *view);

extern NDRX_API int ndrx_Bvstof_int(UBFH *p_ub, ndrx_typedview_t *v, char *cstruct, int mode);
extern NDRX_API int ndrx_Bvstof(UBFH *p_ub, char *cstruct, int mode, char *view);

/* View dynamic data access functions: */
extern NDRX_API int ndrx_CBvget_int(char *cstruct, ndrx_typedview_t *v,
    ndrx_typedview_field_t *f, BFLDOCC occ, char *buf, BFLDLEN *len, 
                 int usrtype, long flags);
extern NDRX_API int ndrx_CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN *len, int usrtype, long flags);

extern NDRX_API int ndrx_CBvchg_int(char *cstruct, ndrx_typedview_t *v, 
        ndrx_typedview_field_t *f, BFLDOCC occ, char *buf, 
                 BFLDLEN len, int usrtype);
extern NDRX_API int ndrx_CBvchg(char *cstruct, char *view, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN len, int usrtype);

extern NDRX_API long ndrx_Bvsizeof(char *view);

extern NDRX_API long ndrx_Bvcpy(char *cstruct_dst, char *cstruct_src, char *view);

extern NDRX_API BFLDOCC ndrx_Bvoccur_int(char *cstruct, ndrx_typedview_t *v, 
        ndrx_typedview_field_t *f, BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size,
        int *fldtype);

extern NDRX_API BFLDOCC ndrx_Bvoccur(char *cstruct, char *view, char *cname, 
        BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size, int *fldtype);

extern NDRX_API int ndrx_Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ);

extern NDRX_API int ndrx_Bvnext (Bvnext_state_t *state, char *view, 
        char *cname, int *fldtype, BFLDOCC *maxocc, long *dim_size);

#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_VIEW_H */

/* vim: set ts=4 sw=4 et smartindent: */
