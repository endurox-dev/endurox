/**
 * @brief UBF library internals
 *
 * @file ubf_int.h
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
#ifndef __UBF_INT_H
#define	__UBF_INT_H

#ifdef	__cplusplus
extern "C" {
#endif


/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
#include <string.h>
#include <stdio.h>
#include <exhash.h>
#include <ndrstandard.h>
#include <ndrx_config.h>
#include <fdatatype.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define UBF_BINARY_SEARCH_OK(bfldid) ((bfldid>>EFFECTIVE_BITS) < BFLD_STRING)
    
    
#define UBF_BINSRCH_GET_LAST_NONE       0x00
#define UBF_BINSRCH_GET_LAST            0x01
#define UBF_BINSRCH_GET_LAST_CHG        0x02 /**< for change              */
    
#define UBF_CMP_MODE_STD          0x00000001 /**< standard compare mode   */

/* Print some debug out there! */
#define UBFDEBUG(x)	do { fprintf(stderr, x); } while(0);
#define FLDTBLDIR	"FLDTBLDIR"
#define FIELDTBLS	"FIELDTBLS"
    
#define CONF_VIEWFILES  "VIEWFILES"         /* List of view files to load      */
#define CONF_VIEWDIR    "VIEWDIR"           /* Folders with view files stored, ':' - sep   */
    
#define UBFDEBUGLEV "UBF_E_"

/* #define UBF_16 */

#ifdef UBF_16

/* Currently we run in 16 bit mode. */
#define EFFECTIVE_BITS  ((sizeof(_UBF_INT)-2)*8-3)
/* This is ((2^((sizeof(_UBF_INT)-2)*8-3))-1)
 * 2^(2*8-3) - 1 = > 8191 = 0x1fff
 */
#define EFFECTIVE_BITS_MASK  0x1fff

#else

/* 32 bit mode, effective bits - 25. */
/* Max number: */
#define EFFECTIVE_BITS  25
/* Effective bits mask */
#define EFFECTIVE_BITS_MASK  0x1FFFFFF

#endif

#define BFLD_SHORT_SIZE	sizeof(unsigned short)
#define BFLD_LONG_SIZE	sizeof(long)
#define BFLD_CHAR_SIZE	sizeof(char)
#define BFLD_FLOAT_SIZE	sizeof(float)
#define BFLD_DOUBLE_SIZE	sizeof(double)
#define BFLD_STRING_SIZE	0
#define BFLD_CARRAY_SIZE	0
#define BFLD_INT_SIZE		sizeof(int)
#define BFLD_PTR_SIZE		sizeof(char *)
#define BFLD_UBF_SIZE		sizeof(UBF_ubf_t)
#define BFLD_VIEW_SIZE		sizeof(UBF_view_t)


/* #define UBF_API_DEBUG   1 *//* Provide lots of debugs from UBF API? */
/* #define BIN_SEARCH_DEBUG        1*/
    
#define VALIDATE_MODE_NO_FLD    0x1

#define IS_TYPE_INVALID(T) (T<BFLD_MIN || T>BFLD_MAX)
    
/**
 * Check is this complex type, no conversions, etc..
 * @param T type to check
 */
#define IS_TYPE_COMPLEX(T) (BFLD_PTR==T||BFLD_UBF==T||BFLD_VIEW==T)

#define CHECK_ALIGN(p, p_ub, hdr) (((long)p) > ((long)p_ub+hdr->bytes_used))

/*
 * Projection copy internal mode
 */
#define PROJ_MODE_PROJ        0
#define PROJ_MODE_DELETE      1
#define PROJ_MODE_DELALL      2

/**
 * Conversation buffer allocation mode
 */
#define CB_MODE_DEFAULT        0
#define CB_MODE_TEMPSPACE      1
#define CB_MODE_ALLOC          2

#define DOUBLE_EQUAL        0.000001
#define FLOAT_EQUAL         0.00001

#define DOUBLE_RESOLUTION	6
#define FLOAT_RESOLUTION	5
    
    
#define UBF_EOF(HDR, FIELD) ((char *)FIELD >= (((char *)HDR) + HDR->bytes_used))

/**
 * UBF buffer alloc
 */
#define NDRX_USYSBUF_MALLOC_WERR_OUT(__buf, __p_bufsz, __ret)  \
{\
    int __buf_size__ = NDRX_MSGSIZEMAX;\
    __buf = NDRX_FPMALLOC(__buf_size__, NDRX_FPSYSBUF);\
    if (NULL==__buf)\
    {\
        int err = errno;\
        ndrx_Bset_error_fmt(BMALLOC, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        NDRX_LOG(log_error, "%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        userlog("%s: failed to allocate sysbuf: %s", __func__, strerror(errno));\
        errno = err;\
        EXFAIL_OUT(__ret);\
    }\
    __p_bufsz = __buf_size__;\
}

/* for sparc we set to 8 */
#define DEFAULT_ALIGN       EX_ALIGNMENT_BYTES
    
    
#if EX_ALIGNMENT_BYTES == 8

#define ALIGNED_SIZE(DSIZE) \
    ((sizeof(BFLDID)*2 + DSIZE + DEFAULT_ALIGN - 1) / DEFAULT_ALIGN * DEFAULT_ALIGN)
#else

#define ALIGNED_SIZE(DSIZE) \
    ((sizeof(BFLDID) + DSIZE + DEFAULT_ALIGN -1 ) / DEFAULT_ALIGN * DEFAULT_ALIGN)
#endif
   
/**
 * Get aligned size with the data
 * @param TNAME type name this shall include standard field id with 8 alignment if needed
 */
#define ALIGNED_SIZE_T(TNAME, DSIZE) \
    ((sizeof(TNAME) + DSIZE + DEFAULT_ALIGN -1 ) / DEFAULT_ALIGN * DEFAULT_ALIGN)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API void ndrx_build_printable_string(char *out, int out_len, char *in, 
        int in_len);


extern char * get_fld_loc_binary_search(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype, int get_last, int *last_occ, 
                            char ** last_checked, char ** last_match);
extern char * get_fld_loc(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype,
                            char ** last_checked,
                            char ** last_matched,
                            int *last_occ,
                            Bfld_loc_info_t *last_start);
extern void ubf_cache_shift(UBFH *p_ub, BFLDID fldid, int size_diff);
extern void ubf_cache_dump(UBFH *p_ub, char *msg);
extern int ubf_cache_update(UBFH *p_ub);

extern int ndrx_Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN * buflen);
extern int ndrx_Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len,
                                Bfld_loc_info_t *last_start, 
				Bfld_loc_info_t *next_fld);
extern int ndrx_Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len, Bfld_loc_info_t *last_start, 
                            int upd_only);
extern int have_buffer_size(UBFH *p_ub, int add_size, int set_err);
extern int validate_entry(UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, int mode);
extern char * ndrx_Bfind (UBFH * p_ub, BFLDID bfldid,
                                        BFLDOCC occ, BFLDLEN * len,
                                        char **p_fld);
extern BFLDOCC ndrx_Boccur (UBFH * p_ub, BFLDID bfldid);
extern int _Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);

extern char * ndrx_CBfind (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC occ, BFLDLEN * len, int usrtype, int mode,
                        int extralen);
extern char * ndrx_Bgetalloc (UBFH * p_ub,
                            BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern char * ndrx_Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                                BFLDOCC *occ,
                                                BFLDLEN * len);
extern char * ndrx_Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len);

extern int ndrx_Bfprint (UBFH *p_ub, FILE * outf, 
        ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1);

extern int ndrx_Bnext(Bnext_state_t *state, UBFH *p_ub, BFLDID *bfldid,
                                BFLDOCC *occ, char *buf, BFLDLEN *len,
                                char **d_ptr);
extern int ndrx_Bproj (UBFH * p_ub, BFLDID * fldlist,
                                    int mode, int *processed);

extern int ndrx_Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src,
                                    BFLDID * fldlist);
extern int ndrx_Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src);
extern int ndrx_Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src);
extern BFLDOCC ndrx_Bfindocc (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len);
extern BFLDOCC ndrx_CBfindocc (UBFH *p_ub, BFLDID bfldid, char * value, BFLDLEN len, int usrtype);
extern int ndrx_Bgetlast (UBFH *p_ub, BFLDID bfldid,
                                   BFLDOCC *occ, char *buf, BFLDLEN *len);
extern int ndrx_Bextread (UBFH * p_ub, FILE *inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern void ndrx_Bboolpr (char * tree, FILE *outf, 
        int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);
extern int ndrx_Bread  (UBFH * p_ub, FILE * inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern int ndrx_Bwrite (UBFH *p_ub, FILE * outf,
        long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern int ndrx_Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int ndrx_Bboolsetcbf (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));

extern int ndrx_Bcmp(UBFH *p_ubf1, UBFH *p_ubf2);
extern int ndrx_Bsubset(UBFH *p_ubf1, UBFH *p_ubf2);

extern BFLDOCC ndrx_Bnum(UBFH *p_ub);

extern int ndrx_Bjoin(UBFH *dest, UBFH *src);
extern int ndrx_Bojoin(UBFH *dest, UBFH *src);

extern UBFH * ndrx_Balloc (BFLDOCC f, BFLDLEN v, long len_set);
extern UBFH * ndrx_Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v, long len_set);

extern long ndrx_Bneeded(BFLDOCC nrfields, BFLDLEN totsize);

/* FLD_PTR support: */
extern NDRX_API int ndrx_put_data_ptr(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len);
extern NDRX_API int ndrx_get_data_ptr (struct dtype_str *t, char *fb, char *buf, int *len);
extern NDRX_API int ndrx_cmp_ptr (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
extern NDRX_API void ndrx_dump_ptr (struct dtype_ext1 *t, char *text, char *data, int *len);

/* FLD_UBF support: */

extern NDRX_API int ndrx_get_fb_ubf_size(dtype_str_t *t, char *fb, int *payload_size);
extern NDRX_API int ndrx_put_data_ubf(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len);
extern NDRX_API int ndrx_get_d_size_ubf (struct dtype_str *t, char *data, 
        int len, int *payload_size);
extern NDRX_API int ndrx_get_data_ubf (struct dtype_str *t, char *fb, char *buf, int *len);
extern NDRX_API int ndrx_g_ubf_empty(struct dtype_ext1* t);
extern NDRX_API int ndrx_put_empty_ubf(struct dtype_ext1* t, char *fb, BFLDID bfldid);
extern NDRX_API void ndrx_dump_ubf(struct dtype_ext1 *t, char *text, char *data, int *len);
extern NDRX_API int ndrx_cmp_ubf (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
extern NDRX_API int ndrx_cmp_view (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
extern NDRX_API char *ndrx_talloc_view (struct dtype_ext1 *t, int *len);

#ifdef	__cplusplus
}
#endif

#endif	/* __UBF_INT_H */
/* vim: set ts=4 sw=4 et smartindent: */
