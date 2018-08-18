/**
 * @brief UBF library internals
 *
 * @file ubf_int.h
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define UBFFLDMAX	64

/* Print some debug out there! */
#define UBFDEBUG(x)	do { fprintf(stderr, x); } while(0);
#define FLDTBLDIR	"FLDTBLDIR"
#define FIELDTBLS	"FIELDTBLS"
    
#define CONF_VIEWFILES  "VIEWFILES"         /* List of view files to load      */
#define CONF_VIEWDIR    "VIEWDIR"           /* Folders with view files stored, ':' - sep   */
    
#define UBFDEBUGLEV "UBF_E_"

#define UBF_MAGIC   "UBF1"
#define UBF_MAGIC_SIZE   4
#define CF_TEMP_BUF_MAX 64
#define EXTREAD_BUFFSIZE    16384

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

/* #define UBF_API_DEBUG   1 *//* Provide lots of debugs from UBF API? */

#define VALIDATE_MODE_NO_FLD    0x1

#define IS_TYPE_INVALID(T) (T<BFLD_MIN || T>BFLD_MAX)

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
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

typedef short _UBF_SHORT;
typedef unsigned int _UBF_INT;
typedef double UBF_DOUBLE;
typedef float UBF_FLOAT;
typedef long UBF_LONG;
typedef char UBF_CHAR;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API void ndrx_build_printable_string(char *out, int out_len, char *in, 
        int in_len);


#ifdef	__cplusplus
}
#endif

#endif	/* __UBF_INT_H */
/* vim: set ts=4 sw=4 et smartindent: */
