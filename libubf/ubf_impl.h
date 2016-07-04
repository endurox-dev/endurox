/* 
** This is impelementation header. Shared between ubf.c and ubf_impl.c
**
** @file ubf_impl.h
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/* get_loc state info */
typedef struct
{
    BFLDID *last_checked;
} get_fld_loc_info_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern char * get_fld_loc(UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            dtype_str_t **fld_dtype,
                            char ** last_checked,
                            char ** last_matched,
                            int *last_occ,
                            get_fld_loc_info_t *last_start);
extern void ubf_cache_shift(UBFH *p_ub, BFLDID fldid, int size_diff);
extern void ubf_cache_dump(UBFH *p_ub, char *msg);
extern int ubf_cache_update(UBFH *p_ub);

extern int _Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN * buflen);
extern int _Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len,
                                get_fld_loc_info_t *last_start);
extern int _Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ,
                            char * buf, BFLDLEN len, get_fld_loc_info_t *last_start);
extern bool have_buffer_size(UBFH *p_ub, int add_size, bool set_err);
extern int validate_entry(UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, int mode);
extern char * _Bfind (UBFH * p_ub, BFLDID bfldid,
                                        BFLDOCC occ, BFLDLEN * len,
                                        char **p_fld);
extern BFLDOCC _Boccur (UBFH * p_ub, BFLDID bfldid);
extern int _Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);

extern char * _CBfind (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC occ, BFLDLEN * len, int usrtype, int mode,
                        int extralen);
extern char * _Bgetalloc (UBFH * p_ub,
                            BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern char * _Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                                BFLDOCC *occ,
                                                BFLDLEN * len);
extern char * _Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len);
extern int _Bfprint (UBFH *p_ub, FILE * outf);

extern int _Bnext(Bnext_state_t *state, UBFH *p_ub, BFLDID *bfldid,
                                BFLDOCC *occ, char *buf, BFLDLEN *len,
                                char **d_ptr);
extern int _Bproj (UBFH * p_ub, BFLDID * fldlist,
                                    int mode, int *processed);

extern int _Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src,
                                    BFLDID * fldlist);
extern int _Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src);
extern int _Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src);
extern BFLDOCC _Bfindocc (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len);
extern BFLDOCC _CBfindocc (UBFH *p_ub, BFLDID bfldid, char * value, BFLDLEN len, int usrtype);
extern int _Bgetlast (UBFH *p_ub, BFLDID bfldid,
                                   BFLDOCC *occ, char *buf, BFLDLEN *len);
extern int _Bextread (UBFH * p_ub, FILE *inf);
extern void _Bboolpr (char * tree, FILE *outf);
extern int _Bread  (UBFH * p_ub, FILE * inf);
extern int _Bwrite (UBFH *p_ub, FILE * outf);
extern int _Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int _Bboolsetcbf (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));
#ifdef	__cplusplus
}
#endif

#endif	/* _UBF_H */

