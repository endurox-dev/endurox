/* 
** VIEW Support structures
**
** @file typed_view.h
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

#ifndef TYPED_VIEW_H
#define	TYPED_VIEW_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <utlist.h>
#include <exhash.h>
#include <fdatatype.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_VIEW_CNAME_LEN             256  /* Max c field len in struct   */
#define NDRX_VIEW_FLAGS_LEN             16   /* Max flags                   */
#define NDRX_VIEW_NULL_LEN              2660 /* Max len of the null value   */
#define NDRX_VIEW_NAME_LEN              128  /* Max len of the name         */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * View field object. It can be part of linked list
 * each field may be compiled and contain the field offsets in memory.
 */
typedef struct ndrx_typedview_field ndrx_typedview_field_t;
struct ndrx_typedview_field
{
    char type_name[NDRX_UBF_TYPE_LEN+1]; /* UBF Type name */
    char cname[NDRX_VIEW_CNAME_LEN+1];  /* field name in struct */
    int count;                          /* array size, 1 - no array, >1 -> array */
    char flagsstr[NDRX_VIEW_FLAGS_LEN+1]; /* string flags */
    long flags;     /* binary flags */
    long size;      /* size of the field, NOTE currently decimal is not supported */
    char nullval[NDRX_VIEW_NULL_LEN+1];   /*Null value */
    /* Linked list */
    atmi_svc_list_t *next;
};

/**
 *  View object will have following attributes:
 * - File name
 * - View name
 * List of fields and their attributes
 * hashed by view name
 */
typedef struct ndrx_typedview ndrx_typedview_t;
struct ndrx_typedview
{
    char vname[NDRX_VIEW_NAME_LEN];
    char filename[PATH_MAX+1];
    
    ndrx_typedview_field_t *fields;

    EX_hash_handle hh;         /* makes this structure hashable */    
};

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


#ifdef	__cplusplus
}
#endif

#endif	/* TYPED_VIEW_H */

