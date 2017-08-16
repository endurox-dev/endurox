/* 
** FML32 Emulation via UBF library
**
** @file fml32.h
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
#ifndef FML32_H
#define	FML32_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
#include <fml.h> /* Fallback to FML */
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* capability for functions */
#define Fread32 Bread
#define Fwrite32 Bwrite
#define Fjoin32 Bjoin
#define Flen32 Blen
#define CFfind32 CBfind
#define CFfindocc32 CBfindocc
#define CFgetalloc32 CBgetalloc
#define Fboolpr32 Bboolpr
#define Fextread32 Bextread
#define Ffindocc32 Bfindocc
#define Fgetalloc32 Bgetalloc
#define Ffindlast32 Bfindlast
#define Fgetlast32 Bgetlast
#define Fprint32 Bprint
#define Ffprint32 Bfprint
#define Ftypcvt32 Btypcvt
#define Fadds32 Badds
#define Fchgs32 Bchgs
#define Fgets32 Bgets
#define Fgetsa32 Bgetsa
#define Ffinds32 Bfinds
#define CFadd32 CBadd
#define CFchg32 CBchg
#define CFget32 CBget
#define Fdel32 Bdel
#define Fpres32 Bpres
#define Fproj32 Bproj
#define Fprojcpy32 Bprojcpy
#define Fldid32 Bfldid
#define Fname32 Bfname
#define Fcpy32 Bcpy
#define Fchg32 Bchg
#define Finit32 Binit
#define Fnext32 Bnext
#define Fget32 Bget
#define Fboolco32 Bboolco
#define Ffind32 Bfind
#define Fboolev32 Bboolev
#define Ffloatev32 Bfloatev
#define Fadd32 Badd
#define F_error32 B_error
#define Fstrerror32 Bstrerror
#define Fmkfldid32 Bmkfldid
#define Foccur32 Boccur
#define Fused32 Bused
#define Fldtype32 Bfldtype
#define Fdelall32 Bdelall
#define Fdelete32 Bdelete
#define Fldno32 Bfldno
#define Funused32 Bunused
#define Fsizeof32 Bsizeof
#define Ftype32 Btype
#define Ffree32 Bfree
#define Falloc32 Balloc
#define Frealloc32 Brealloc
#define Fupdate32 Bupdate
#define Fconcat32 Bconcat
#define Ftreefree32 Btreefree
#define Findex32 Bindex
#define Funindex32 Bunindex
#define Fidxused32 Bidxused
#define Frstrindex32 Brstrindex
#define Ferror32 Berror
#define Fielded32 Bisubf
    
/* VIEW related */
#define Fvnull32 Bvnull
#define Fvselinit32 Bvselinit
#define Fvsinit32 Bvsinit
#define Fvrefresh32 Bvrefresh
#define Fvopt32 Bvopt
#define Fvftos32 Bvftos
#define Bvstof32 Bvstof
/* VIEW related, END */

#ifndef FBFR32
#define FBFR32 UBFH
#endif

#define FLDLEN32 BFLDLEN
#define FLDOCC32 BFLDOCC
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* FML32_H */

