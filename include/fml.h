/**
 * @brief FML emulation via UBF library
 *
 * @file fml.h
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


/* fixes for Support #519 shared section, include once: */
#ifndef FML32_H
    
    
#define FVIEWNAMESIZE NDRX_VIEW_NAME_LEN
#define FVIEWFLD    BVIEWFLD

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
#define FLD_PTR         BFLD_PTR    /**< pointer to a buffer            */
#define FLD_FML         BFLD_UBF    /**< embedded FML buffer          */
#define FLD_VIEW        BFLD_VIEW   /**< embedded VIEW buffer         */

#define BADFLDID BBADFLDID
#define FIRSTFLDID BFIRSTFLDID

#define FSTDXINT  16
    
/* compatibility for error codes */
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
    
#define F_FTOS B_FTOS
#define F_STOF B_STOF
#define F_OFF B_OFF
#define F_BOTH B_BOTH
    
#define Fnext_state Bnext_state 
#define Fnext_state_t Bnext_state_t

/* Added for compatibility */
#define F32to16 B32to16
#define F16to32 B16to32

/* end of shared once */
#endif

/* capability for functions 16 bit compat: */
#define MAXFBLEN MAXUBFLEN
#define Fbfr Ubfh
#define FBFR UBFH
#define Ferror Berror
#define FLDID BFLDID
#define FLDLEN BFLDLEN
#define FLDOCC BFLDOCC

#define Fread Bread
#define Fwrite Bwrite
#define Fjoin Bjoin
#define Fojoin Bojoin
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
#define Fcmp Bcmp
#define Fsubset Bsubset
#define Fnum Bnum 
#define Fneeded Bneeded

/* VIEW related */
#define Fvnull Bvnull
#define Fvselinit Bvselinit
#define Fvsinit Bvsinit
#define Fvrefresh Bvrefresh
#define Fvopt Bvopt
#define Fvftos Bvftos
#define Bvstof Bvstof
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

/* vim: set ts=4 sw=4 et smartindent: */
