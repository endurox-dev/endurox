/* 
** Typed string support
**
** @file typed_string.h
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
#ifndef __TYPED_STRING_H
#define	__TYPED_STRING_H

#ifdef	__cplusplus
extern "C" {
#endif


/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define STRING_MAGIC_SIZE   4  
#define STRING_MAGIC        "STR1"  
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/* STRING Header record */
struct STRING_header
{
    unsigned char       buffer_type;
    unsigned char       version;
    char                magic[STRING_MAGIC_SIZE];
    BFLDLEN             buf_len;
    /* Actual data goes here... */
    char str[1];
};

typedef struct STRING_header STRING_header_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif	/* __TYPED_STRING_H */

