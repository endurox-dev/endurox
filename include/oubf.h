/**
 * @brief UBF Object API header (auto-generated)
 *
 * @file oubf.h
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
#ifndef __OUBF_H
#define __OUBF_H

#ifdef  __cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <stdint.h>
#include <ubf.h>
#include <atmi.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OBerror(P_CTXT)   (*O_Bget_Ferror_addr(P_CTXT))
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int * Ondrx_Bget_Ferror_addr(TPCONTEXT_T *p_ctxt);
extern NDRX_API int * O_Bget_Ferror_addr(TPCONTEXT_T *p_ctxt);
extern NDRX_API int OBlen(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int OCBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int OCBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int OCBget(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype);
extern NDRX_API int OBdel(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int OBpres(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int OBproj(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID * fldlist);
extern NDRX_API int OBprojcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist);
extern NDRX_API BFLDID OBfldid(TPCONTEXT_T *p_ctxt, char *fldnm);
extern NDRX_API char * OBfname(TPCONTEXT_T *p_ctxt, BFLDID bfldid);
extern NDRX_API int OBcpy(TPCONTEXT_T *p_ctxt, UBFH * p_ub_dst, UBFH * p_ub_src);
extern NDRX_API int OBchg(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len);
extern NDRX_API int OBinit(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDLEN len);
extern NDRX_API int OBnext(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int OBget(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen);
extern NDRX_API char * OBboolco(TPCONTEXT_T *p_ctxt, char * expr);
extern NDRX_API char * OBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len);
extern NDRX_API int OBboolev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree);
extern NDRX_API double OBfloatev(TPCONTEXT_T *p_ctxt, UBFH * p_ub, char *tree);
extern NDRX_API int OBadd(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len);
extern NDRX_API int OBaddfast(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len, Bfld_loc_info_t *next_fld);
extern NDRX_API char * OBecodestr(TPCONTEXT_T *p_ctxt, int err);
extern NDRX_API void OB_error(TPCONTEXT_T *p_ctxt, char *str);
extern NDRX_API char * OBstrerror(TPCONTEXT_T *p_ctxt, int err);
extern NDRX_API BFLDID OBmkfldid(TPCONTEXT_T *p_ctxt, int fldtype, BFLDID bfldid);
extern NDRX_API BFLDOCC OBoccur(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid);
extern NDRX_API long OBused(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API int OBfldtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid);
extern NDRX_API int OBdelall(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid);
extern NDRX_API int OBdelete(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID *fldlist);
extern NDRX_API BFLDOCC OBfldno(TPCONTEXT_T *p_ctxt, BFLDID bfldid);
extern NDRX_API long OBunused(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API long OBsizeof(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API char * OBtype(TPCONTEXT_T *p_ctxt, BFLDID bfldid);
extern NDRX_API int OBfree(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API UBFH * OBalloc(TPCONTEXT_T *p_ctxt, BFLDOCC f, BFLDLEN v);
extern NDRX_API UBFH * OBrealloc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDOCC f, BFLDLEN v);
extern NDRX_API int OBupdate(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API int OBconcat(TPCONTEXT_T *p_ctxt, UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API char * OCBfind(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype);
extern NDRX_API char * OCBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen);
extern NDRX_API BFLDOCC OCBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len, int usrtype);
extern NDRX_API BFLDOCC OBfindocc(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,char * buf, BFLDLEN len);
extern NDRX_API char * OBgetalloc(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * OBfindlast(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDID bfldid,BFLDOCC *occ, BFLDLEN *len);
extern NDRX_API int OBgetlast(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid,BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int OBprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API int OBfprint(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf);
extern NDRX_API int OBfprintcb(TPCONTEXT_T *p_ctxt, UBFH *p_ub, int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);
extern NDRX_API char * OBtypcvt(TPCONTEXT_T *p_ctxt, BFLDLEN * to_len, int to_type,char *from_buf, int from_type, BFLDLEN from_len);
extern NDRX_API int OBextread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE *inf);
extern NDRX_API int OBextreadcb(TPCONTEXT_T *p_ctxt, UBFH * p_ub, long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern NDRX_API void OBboolpr(TPCONTEXT_T *p_ctxt, char * tree, FILE *outf);
extern NDRX_API void OBboolprcb(TPCONTEXT_T *p_ctxt, char * tree, int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1);
extern NDRX_API int OBread(TPCONTEXT_T *p_ctxt, UBFH * p_ub, FILE * inf);
extern NDRX_API int OBwrite(TPCONTEXT_T *p_ctxt, UBFH *p_ub, FILE * outf);
extern NDRX_API int OBwritecb(TPCONTEXT_T *p_ctxt, UBFH *p_ub, long (*p_writef)(char *buffer, long bufsz, void *dataptr1), void *dataptr1);
extern NDRX_API void OBtreefree(TPCONTEXT_T *p_ctxt, char *tree);
extern NDRX_API int OBboolsetcbf(TPCONTEXT_T *p_ctxt, char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));
extern NDRX_API int OBadds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, char *buf);
extern NDRX_API int OBchgs(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API int OBgets(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API char * OBgetsa(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * OBfinds(TPCONTEXT_T *p_ctxt, UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int OBisubf(TPCONTEXT_T *p_ctxt, UBFH *p_ub);
extern NDRX_API int OBindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ);
extern NDRX_API BFLDOCC OBunindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub);
extern NDRX_API long OBidxused(TPCONTEXT_T *p_ctxt, UBFH * p_ub);
extern NDRX_API int OBrstrindex(TPCONTEXT_T *p_ctxt, UBFH * p_ub, BFLDOCC occ);
extern NDRX_API int OBjoin(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src);
extern NDRX_API int OBojoin(TPCONTEXT_T *p_ctxt, UBFH *dest, UBFH *src);
extern NDRX_API int OBcmp(TPCONTEXT_T *p_ctxt, UBFH *p_ubf1, UBFH *p_ubf2);
extern NDRX_API int OBsubset(TPCONTEXT_T *p_ctxt, UBFH *p_ubf1, UBFH *p_ubf2);
extern NDRX_API int OBvnull(TPCONTEXT_T *p_ctxt, char *cstruct, char *cname, BFLDOCC occ, char *view);
extern NDRX_API int OBvselinit(TPCONTEXT_T *p_ctxt, char *cstruct, char *cname, char *view);
extern NDRX_API int OBvsinit(TPCONTEXT_T *p_ctxt, char *cstruct, char *view);
extern NDRX_API void OBvrefresh(TPCONTEXT_T *p_ctxt);
extern NDRX_API int OBvopt(TPCONTEXT_T *p_ctxt, char *cname, int option, char *view);
extern NDRX_API int OBvftos(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cstruct, char *view);
extern NDRX_API int OBvstof(TPCONTEXT_T *p_ctxt, UBFH *p_ub, char *cstruct, int mode, char *view);
extern NDRX_API int OCBvget(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype, long flags);
extern NDRX_API int OCBvchg(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ, char *buf, BFLDLEN len, int usrtype);
extern NDRX_API long OBvsizeof(TPCONTEXT_T *p_ctxt, char *view);
extern NDRX_API long OBvcpy(TPCONTEXT_T *p_ctxt, char *cstruct_dst, char *cstruct_src, char *view);
extern NDRX_API BFLDOCC OBvoccur(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC *maxocc, BFLDOCC *realocc, long *dim_size, int* fldtype);
extern NDRX_API int OBvsetoccur(TPCONTEXT_T *p_ctxt, char *cstruct, char *view, char *cname, BFLDOCC occ);
extern NDRX_API int OBvnext(TPCONTEXT_T *p_ctxt, Bvnext_state_t *state, char *view, char *cname, int *fldtype, BFLDOCC *maxocc, long *dim_size);
extern NDRX_API void * Ondrx_ubf_tls_get(TPCONTEXT_T *p_ctxt);
extern NDRX_API int Ondrx_ubf_tls_set(TPCONTEXT_T *p_ctxt, void *data);
extern NDRX_API void Ondrx_ubf_tls_free(TPCONTEXT_T *p_ctxt, void *data);
extern NDRX_API void * Ondrx_ubf_tls_new(TPCONTEXT_T *p_ctxt, int auto_destroy, int auto_set);
extern NDRX_API EDB_env * OBfldddbgetenv(TPCONTEXT_T *p_ctxt, EDB_dbi **dbi_id, EDB_dbi **dbi_nm);
extern NDRX_API int OBflddbload(TPCONTEXT_T *p_ctxt);
extern NDRX_API BFLDID OBflddbid(TPCONTEXT_T *p_ctxt, char *fldname);
extern NDRX_API char * OBflddbname(TPCONTEXT_T *p_ctxt, BFLDID bfldid);
extern NDRX_API int OBflddbget(TPCONTEXT_T *p_ctxt, EDB_val *data, short *p_fldtype,BFLDID *p_bfldno, BFLDID *p_bfldid, char *fldname, int fldname_bufsz);
extern NDRX_API int OBflddbunlink(TPCONTEXT_T *p_ctxt);
extern NDRX_API void OBflddbunload(TPCONTEXT_T *p_ctxt);
extern NDRX_API int OBflddbdrop(TPCONTEXT_T *p_ctxt, EDB_txn *txn);
extern NDRX_API int OBflddbdel(TPCONTEXT_T *p_ctxt, EDB_txn *txn, BFLDID bfldid);
extern NDRX_API int OBflddbadd(TPCONTEXT_T *p_ctxt, EDB_txn *txn, short fldtype, BFLDID bfldno, char *fldname);
#endif  /* __OUBF_H */

/* vim: set ts=4 sw=4 et smartindent: */
