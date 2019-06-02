/**
 * @brief This is impelementation header. Shared between ubf.c and ubf_impl.c
 *
 * @file ubf_impl.h
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



#ifndef _UBF_H
#define	_UBF_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
#include <ubf_int.h>
#include <fdatatype.h>
#include <ndrstandard.h>
#include <exdb.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define UBF_BINARY_SEARCH_OK(bfldid) ((bfldid>>EFFECTIVE_BITS) < BFLD_STRING)
    
    
#define UBF_BINSRCH_GET_LAST_NONE       0x00
#define UBF_BINSRCH_GET_LAST            0x01
#define UBF_BINSRCH_GET_LAST_CHG        0x02 /* for change              */
    
#define UBF_CMP_MODE_STD          0x00000001 /* standard compare mode   */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
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
        int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);

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

#ifdef	__cplusplus
}
#endif

#endif	/* _UBF_H */

/* vim: set ts=4 sw=4 et smartindent: */
