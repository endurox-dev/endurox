/* 
** UBF API library
**
** @file ubf.h
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
#ifndef UBF_H
#define UBF_H

#if defined(__cplusplus)
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
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
#define BFLD_MAX        6

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
#define BERFU2              16
#define BERFU3              17
#define BERFU4              18
#define BERFU5              19
#define BERFU6              20
#define BERFU7              21
#define BERFU8              22
#define BMAXVAL             22 /* max error */

/* Configuration: */
#define CONF_NDRX_UBFMAXFLDS     "NDRX_UBFMAXFLDS"

#define Berror	(*_Bget_Ferror_addr())
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
extern int * _Bget_Ferror_addr (void);
extern int Blen (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int CBadd (UBFH *p_ub, BFLDID bfldid, char * buf, BFLDLEN len, int usrtype);
extern int CBchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len, int usrtype);
extern int CBget (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf, BFLDLEN *len, int usrtype);
extern int Bdel (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ);
extern int Bpres (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int Bproj (UBFH * p_ub, BFLDID * fldlist);
extern int Bprojcpy (UBFH * p_ub_dst, UBFH * p_ub_src, BFLDID * fldlist);
extern BFLDID Bfldid (char *fldnm);
extern char * Bfname (BFLDID bfldid);
extern int Bcpy (UBFH * p_ub_dst, UBFH * p_ub_src);
extern int Bchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len);
extern int Binit (UBFH * p_ub, BFLDLEN len);
extern int Bnext (UBFH *p_ub, BFLDID *bfldid, BFLDOCC *occ, char *buf, BFLDLEN *len);
extern int Bget (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN * buflen);
extern char * Bboolco (char * expr);
extern char * Bfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * p_len);
extern int Bboolev (UBFH * p_ub, char *tree);
extern double Bfloatev (UBFH * p_ub, char *tree);
extern int Badd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len);
extern void B_error (char *str);
extern char * Bstrerror (int err);
extern BFLDID Bmkfldid (int fldtype, BFLDID bfldid);
extern BFLDOCC Boccur (UBFH * p_ub, BFLDID bfldid);
extern long Bused (UBFH *p_ub);
extern int Bfldtype (BFLDID bfldid);
extern int Bdelall (UBFH *p_ub, BFLDID bfldid);
extern int Bdelete (UBFH *p_ub, BFLDID *fldlist);
extern BFLDOCC Bfldno (BFLDID bfldid);
extern long Bunused (UBFH *p_ub);
extern long Bsizeof (UBFH *p_ub);
extern char * Btype (BFLDID bfldid);
extern int Bfree (UBFH *p_ub);
extern UBFH * Balloc (BFLDOCC f, BFLDLEN v);
extern UBFH * Brealloc (UBFH *p_ub, BFLDOCC f, BFLDLEN v);
extern int Bupdate (UBFH *p_ub_dst, UBFH *p_ub_src);
extern int Bconcat (UBFH *p_ub_dst, UBFH *p_ub_src);
extern char * CBfind (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN * len, int usrtype);
extern char * CBgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, int usrtype, BFLDLEN *extralen);
extern BFLDOCC CBfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len, int usrtype);
extern BFLDOCC Bfindocc (UBFH *p_ub, BFLDID bfldid,
                                        char * buf, BFLDLEN len);
extern char * Bgetalloc (UBFH * p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern char * Bfindlast (UBFH * p_ub, BFLDID bfldid,
                                    BFLDOCC *occ, BFLDLEN *len);
extern int Bgetlast (UBFH *p_ub, BFLDID bfldid,
                        BFLDOCC *occ, char *buf, BFLDLEN *len);
extern int Bprint (UBFH *p_ub);
extern int Bfprint (UBFH *p_ub, FILE * outf);
extern char * Btypcvt (BFLDLEN * to_len, int to_type,
                    char *from_buf, int from_type, BFLDLEN from_len);
extern int Bextread (UBFH * p_ub, FILE *inf);
extern void Bboolpr (char * tree, FILE *outf);
extern int Bread (UBFH * p_ub, FILE * inf);
extern int Bwrite (UBFH *p_ub, FILE * outf);
extern void Btreefree (char *tree);
/* Callback function that can be used from expressions */
extern int Bboolsetcbf (char *funcname, long (*functionPtr)(UBFH *p_ub, char *funcname));
extern int Badds (UBFH *p_ub, BFLDID bfldid, char *buf);
extern int Bchgs (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern int Bgets (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char *buf);
extern char * Bgetsa (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, BFLDLEN *extralen);
extern char * Bfinds (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ);
extern int Bisubf (UBFH *p_ub);
extern int Bindex (UBFH * p_ub, BFLDOCC occ);
extern BFLDOCC Bunindex (UBFH * p_ub);
extern long Bidxused (UBFH * p_ub);
extern int Brstrindex (UBFH * p_ub, BFLDOCC occ);

#if defined(__cplusplus)
}
#endif

#endif
