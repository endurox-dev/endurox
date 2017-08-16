/* 
** FML emulation via UBF library
**
** @file fml.h
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

#ifndef FML_H
#define	FML_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ubf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define MAXFBLEN MAXUBFLEN
#define F_LENGTH BF_LENGTH

#define FLD_MIN BFLD_MIN
#define FLD_SHORT BFLD_SHORT
#define FLD_LONG BFLD_LONG
#define FLD_CHAR BFLD_CHAR
#define FLD_FLOAT BFLD_FLOAT
#define FLD_DOUBLE BFLD_DOUBLE
#define FLD_STRING BFLD_STRING
#define FLD_CARRAY BFLD_CARRAY
#define FLD_MAX BFLD_MAX
#define FLD_PTR         9

#define BADFLDID BBADFLDID
#define FIRSTFLDID BFIRSTFLDID

#define FSTDXINT  16
    
/* capabiltiy for error codes */
#define FMINVAL BMINVAL
#define FALIGNERR BALIGNERR
#define FNOTFLD BNOTFLD
#define FNOSPACE BNOSPACE
#define FNOTPRES BNOTPRES
#define FBADFLD BBADFLD
#define FTYPERR BTYPERR
#define FEUNIX BEUNIX
#define FBADNAME BBADNAME
#define FMALLOC BMALLOC
#define FSYNTAX BSYNTAX
#define FFTOPEN BFTOPEN
#define FFTSYNTAX BFTSYNTAX
#define FEINVAL BEINVAL
#define FBADTBL BBADTBL
#define FBADVIEW BBADVIEW
#define FVFSYNTAX BVFSYNTAX
#define FVFOPEN BVFOPEN
#define FBADACM BBADACM
#define FNOCNAME BNOCNAME
#define FEBADOP BEBADOP
#define FMAXVAL  BMAXVAL
    
#define Ferror Berror
#define FLDID32 BFLDID32

#define FLDID BFLDID
#define FLDLEN BFLDLEN
#define FLDOCC BFLDOCC

#define Fbfr Ubfh
#define FBFR UBFH
    
#define F_FTOS B_FTOS
#define F_STOF B_STOF
#define F_OFF B_OFF
#define F_BOTH B_BOTH
    
#define Fnext_state Bnext_state 
#define Fnext_state_t Bnext_state_t
    
/* capability for functions */
#define Fread Bread
#define Fwrite Bwrite
#define Fjoin Bjoin
#define Flen Blen
#define CFfind CBfind
#define CFfindocc CBfindocc
#define CFgetalloc CBgetalloc
#define Fboolpr Bboolpr
#define Fextread Bextread
#define Ffindocc Bfindocc
#define Fgetalloc Bgetalloc
#define Ffindlast Bfindlast
#define Fgetlast Bgetlast
#define Fprint Bprint
#define Ffprint Bfprint
#define Ftypcvt Btypcvt
#define Fadds Badds
#define Fchgs Bchgs
#define Fgets Bgets
#define Fgetsa Bgetsa
#define Ffinds Bfinds
#define CFadd CBadd
#define CFchg CBchg
#define CFget CBget
#define Fdel Bdel
#define Fpres Bpres
#define Fproj Bproj
#define Fprojcpy Bprojcpy
#define Fldid Bfldid
#define Fname Bfname
#define Fcpy Bcpy
#define Fchg Bchg
#define Finit Binit
#define Fnext Bnext
#define Fget Bget
#define Fboolco Bboolco
#define Ffind Bfind
#define Fboolev Bboolev
#define Ffloatev Bfloatev
#define Fadd Badd
#define F_error B_error
#define Fstrerror Bstrerror
#define Fmkfldid Bmkfldid
#define Foccur Boccur
#define Fused Bused
#define Fldtype Bfldtype
#define Fdelall Bdelall
#define Fdelete Bdelete
#define Fldno Bfldno
#define Funused Bunused
#define Fsizeof Bsizeof
#define Ftype Btype
#define Ffree Bfree
#define Falloc Balloc
#define Frealloc Brealloc
#define Fupdate Bupdate
#define Fconcat Bconcat
#define Ftreefree Btreefree
#define Findex Bindex
#define Funindex Bunindex
#define Fidxused Bidxused
#define Frstrindex Brstrindex
#define Ferror Berror
#define Fielded Bisubf
    
/* VIEW related */
#define Fvnull Bvnull
#define Fvselinit Bvselinit
#define Fvsinit Bvsinit
#define Fvrefresh Bvrefresh
#define Fvopt Bvopt
#define Fvftos Bvftos
/* VIEW related, END */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* FML_H */

