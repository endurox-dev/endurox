/* 
** UBF API library
**
** @file ubf.h
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
#ifndef UBF_H
#define UBF_H

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define UBF_VERSION     1
#define UBF_EXTENDED
#define MAXUBFLEN	0xffffffff		/* Maximum UBFH length */
#define BF_LENGTH        32
/* UFB field types, suggest the c data types */
    
#define BFLD_MIN        0
#define BFLD_SHORT	0
#define BFLD_LONG	1
#define BFLD_CHAR	2
#define BFLD_FLOAT	3
#define BFLD_DOUBLE	4
#define BFLD_STRING	5
#define BFLD_CARRAY	6
#define BFLD_MAX    6

/* invalid field id - returned from functions where field id not found */
#define BBADFLDID (BFLDID)0
/* define an invalid field id used for first call to Bnext */
#define BFIRSTFLDID (BFLDID)0

#define BMINVAL             0 /* min error */
#define BERFU0              1
#define BALIGNERR           2
#define BNOTFLD             3
#define BNOSPACE            4
#define BNOTPRES            5
#define BBADFLD             6
#define BTYPERR             7
#define BEUNIX              8
#define BBADNAME            9
#define BMALLOC             10
#define BSYNTAX             11
#define BFTOPEN             12
#define BFTSYNTAX           13
#define BEINVAL             14
#define BERFU1              15
#define BBADTBL             16
#define BBADVIEW            17
#define BVFSYNTAX           18
#define BVFOPEN             19
#define BBADACM             20
#define BNOCNAME            21
#define BEBADOP             22
#define BMAXVAL             22 /* max error */

/* Configuration: */
#define CONF_NDRX_UBFMAXFLDS     "NDRX_UBFMAXFLDS"

#define Berror	(*ndrx_Bget_Ferror_addr())
#define BFLDID32 BFLDID
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
typedef int BFLDID;
typedef int BFLDLEN;
typedef int BFLDOCC;

typedef struct Ubfh UBFH;

/* Bnext state struct */
struct Bnext_state
{
    BFLDID *p_cur_bfldid;
    BFLDOCC cur_occ;
    UBFH *p_ub;
    size_t size;
};
typedef struct Bnext_state Bnext_state_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int * ndrx_Bget_Ferror_addr (void);
extern NDRX_API int Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int CBadd (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int CBchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype);
extern NDRX_API int CBget (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype);
extern NDRX_API int Bdel (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bproj (UBFH * p_ub, BFLDID * fldlist);
extern NDRX_API int Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist);
extern NDRX_API BFLDID Bfldid (char *fldnm);
extern NDRX_API char * Bfname (BFLDID bfldid);
extern NDRX_API int Bcpy (UBFH * p_ub_dst, UBFH * p_ub_src);
extern NDRX_API int Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len);
extern NDRX_API int Binit (UBFH * p_ub, BFLDLEN len);
extern NDRX_API int Bnext (UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen);
extern NDRX_API char * Bboolco (char * expr);
extern NDRX_API char * Bfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len);
extern NDRX_API int Bboolev (UBFH * p_ub, char *tree);
extern NDRX_API double Bfloatev (UBFH * p_ub, char *tree);
extern NDRX_API int Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len);
extern NDRX_API void B_error (char *str);
extern NDRX_API char * Bstrerror (int err);
extern NDRX_API BFLDID Bmkfldid (int fldtype, BFLDID bfldid);
extern NDRX_API BFLDOCC Boccur (UBFH * p_ub, BFLDID bfldid);
extern NDRX_API long Bused (UBFH *p_ub);
extern NDRX_API int Bfldtype (BFLDID bfldid);
extern NDRX_API int Bdelall (UBFH *p_ub, BFLDID bfldid);
extern NDRX_API int Bdelete (UBFH *p_ub, BFLDID *fldlist);
extern NDRX_API BFLDOCC Bfldno (BFLDID bfldid);
extern NDRX_API long Bunused (UBFH *p_ub);
extern NDRX_API long Bsizeof (UBFH *p_ub);
extern NDRX_API char * Btype (BFLDID bfldid);
extern NDRX_API int Bfree (UBFH *p_ub);
extern NDRX_API UBFH * Balloc (BFLDOCC f, BFLDLEN v);
extern NDRX_API UBFH * Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v);
extern NDRX_API int Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API int Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src);
extern NDRX_API char * CBfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype);
extern NDRX_API char * CBgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen);
extern NDRX_API BFLDOCC CBfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len, int usrtype);
extern NDRX_API BFLDOCC Bfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len);
extern NDRX_API char * Bgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC *occ, BFLDLEN *len);
extern NDRX_API int Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len);
extern NDRX_API int Bprint (UBFH *p_ub);
extern NDRX_API int Bfprint (UBFH *p_ub, FILE * outf);
extern NDRX_API char * Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len);
extern NDRX_API int Bextread (UBFH * p_ub, FILE *inf);
extern NDRX_API void Bboolpr (char * tree, FILE *outf);
extern NDRX_API int Bread (UBFH * p_ub, FILE * inf);
extern NDRX_API int Bwrite (UBFH *p_ub, FILE * outf);
extern NDRX_API void Btreefree (char *tree);
/* Callback function that can be used from expressions */
extern NDRX_API int Bboolsetcbf (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));
extern NDRX_API int Badds (UBFH *p_ub, BFLDID bfldid, char *buf);
extern NDRX_API int Bchgs (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API int Bgets (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern NDRX_API char * Bgetsa (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern NDRX_API char * Bfinds (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern NDRX_API int Bisubf (UBFH *p_ub);
extern NDRX_API int Bindex (UBFH * p_ub, BFLDOCC occ);
extern NDRX_API BFLDOCC Bunindex (UBFH * p_ub);
extern NDRX_API long Bidxused (UBFH * p_ub);
extern NDRX_API int Brstrindex (UBFH * p_ub, BFLDOCC occ);

/* VIEW related */
extern NDRX_API int Bvnull(char *cstruct, char *cname, BFLDOCC occ, char *view);
extern NDRX_API int Bvselinit(char *cstruct, char *cname, char *view);
extern NDRX_API int Bvsinit(char *cstruct, char *view);
extern NDRX_API void Bvrefresh(void);

/* VIEW related, END */

/* ATMI library TLS: */
extern NDRX_API void * ndrx_ubf_tls_get(void);
extern NDRX_API int ndrx_ubf_tls_set(void *data);
extern NDRX_API void ndrx_ubf_tls_free(void *data);
extern NDRX_API void * ndrx_ubf_tls_new(int auto_destroy, int auto_set);

#if defined(__cplusplus)
}
#endif

#endif
