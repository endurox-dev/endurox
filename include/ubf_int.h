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
#include <nstdutil.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define UBF_BINARY_SEARCH_OK(bfldid) ((bfldid>>EFFECTIVE_BITS) < BFLD_STRING || (bfldid>>EFFECTIVE_BITS)==BFLD_PTR)
    
    
#define UBF_BINSRCH_GET_LAST_NONE       0x00
#define UBF_BINSRCH_GET_LAST            0x01
#define UBF_BINSRCH_GET_LAST_CHG        0x02 /**< for change              */
    
#define UBF_CMP_MODE_STD          0x00000001 /**< standard compare mode   */

/* Print some debug out there! */
#define UBFDEBUG(x)	do { fprintf(stderr, x); } while(0);
    
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
/** strip field id & align off */
#define BFLD_VIEW_SIZE		(sizeof(UBF_view_t) - EX_ALIGNMENT_BYTES)


/* #define UBF_API_DEBUG   1 *//* Provide lots of debugs from UBF API? */
/* #define BIN_SEARCH_DEBUG        1*/
    
#define VALIDATE_MODE_NO_FLD    0x1

#define IS_TYPE_INVALID(T) (T<BFLD_MIN || T>BFLD_MAX)
    
/**
 * Check is this complex type, no conversions, etc..
 * @param T type to check
 */
#define IS_TYPE_COMPLEX(T) (BFLD_UBF==T||BFLD_VIEW==T)

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

    
#define VIEW_ENTRY if (EXSUCCEED!=ndrx_view_init()) {EXFAIL_OUT(ret);}
#define VIEW_ENTRY2 if (EXSUCCEED!=ndrx_view_init()) {ret=-2; goto out;}
#define VIEW_ENTRY3 if (EXSUCCEED!=ndrx_view_init()) {return EXFAIL;}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/    

/**
 * Recursive field id with dot syntax
 */
typedef struct
{
    ndrx_growlist_t fldidocc;   /**< array of: fldid1,fldocc1,fldid2,occ2,-1  */
    char *cname;        /**< In case if leaf view, have a malloced field name */
    BFLDOCC cname_occ;  /**< occurrence for view field                        */
    
    /* last UBF name        */
    char *fldnm;
    
    /* last field cached...*/
    BFLDID bfldid;
    BFLDOCC occ;
    int nrflds;
    
} ndrx_ubf_rfldid_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
    
extern NDRX_API int ndrx_get_num_from_hex(char c);
extern NDRX_API int ndrx_normalize_string(char *str, int *out_len);

extern NDRX_API void ndrx_build_printable_string(char *out, int out_len, char *in, 
        int in_len);

extern NDRX_API int ndrx_get_nonprintable_char_tmpspace(char *str, int len);

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
        ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1, int level);

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
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1,
        int level, char **p_readbuf_buffered);
extern void ndrx_Bboolpr (char * tree, FILE *outf, 
        int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);
extern int ndrx_Bread  (UBFH * p_ub, FILE * inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern int ndrx_Bwrite (UBFH *p_ub, FILE * outf,
        long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern int ndrx_Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int ndrx_Bboolsetcbf2 (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));

extern int ndrx_Bcmp(UBFH *p_ubf1, UBFH *p_ubf2);
extern int ndrx_Bsubset(UBFH *p_ubf1, UBFH *p_ubf2, int level);

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


/* FLD_VIEW support: */

extern NDRX_API int ndrx_get_fb_view_size(dtype_str_t *t, char *fb, int *payload_size);
extern NDRX_API int ndrx_put_data_view(dtype_str_t *t, char *fb, BFLDID bfldid, 
        char *data, int len);
extern NDRX_API int ndrx_get_d_size_view (struct dtype_str *t, char *data, 
        int len, int *payload_size);
extern NDRX_API int ndrx_get_data_view (struct dtype_str *t, char *fb, char *buf, int *len);
extern NDRX_API int ndrx_g_view_empty(struct dtype_ext1* t);
extern NDRX_API int ndrx_put_empty_view(struct dtype_ext1* t, char *fb, BFLDID bfldid);
extern NDRX_API void ndrx_dump_view(struct dtype_ext1 *t, char *text, char *data, int *len);
extern NDRX_API int ndrx_cmp_view (struct dtype_ext1 *t, char *val1, BFLDLEN len1, 
        char *val2, BFLDLEN len2, long mode);
extern NDRX_API char *ndrx_talloc_view (struct dtype_ext1 *t, int *len);

extern NDRX_API char* ndrx_prep_viewp (struct dtype_ext1 *t, 
        ndrx_ubf_tls_bufval_t *storage, char *data);


/* Recursive API: */
extern NDRX_API int ndrx_Bgetr (UBFH * p_ub, BFLDID *fldidocc,
                            char * buf, BFLDLEN * buflen);


extern NDRX_API int ndrx_Bpresr (UBFH *p_ub, BFLDID *fldidocc);

extern NDRX_API int ndrx_CBgetr (UBFH * p_ub, BFLDID *fldidocc,
                            char * buf, BFLDLEN * len, int usrtype);
extern NDRX_API char* ndrx_Bfindr (UBFH *p_ub, BFLDID *fldidocc, BFLDLEN *p_len);

extern NDRX_API char* ndrx_CBfindr (UBFH *p_ub, BFLDID *fldidocc, BFLDLEN *p_len, int usrtype);

extern NDRX_API int ndrx_Bpresr (UBFH *p_ub, BFLDID *fldidocc);

extern NDRX_API int ndrx_Bvnullr(UBFH *p_ub, BFLDID *fldidocc, char *cname, BFLDOCC occ);
extern NDRX_API int ndrx_CBvgetr(UBFH *p_ub, BFLDID *fldidocc, char *cname, BFLDOCC occ, 
             char *buf, BFLDLEN *len, int usrtype, long flags);

extern NDRX_API void ndrx_ubf_rfldid_free(ndrx_ubf_rfldid_t *rfldid);
extern NDRX_API int ndrx_ubf_rfldid_parse(char *rfldidstr, ndrx_ubf_rfldid_t *rfldid);

#ifdef	__cplusplus
}
#endif

#endif	/* __UBF_INT_H */
/* vim: set ts=4 sw=4 et smartindent: */
