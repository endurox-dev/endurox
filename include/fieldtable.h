/**
 * @brief UBF library, field table handling routines (i.e. .fd files)
 *
 * @file fieldtable.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#ifndef FIELDTABLE_H
#define	FIELDTABLE_H

#ifdef	__cplusplus
extern "C" {
#endif
/*---------------------------Includes-----------------------------------*/
#include <fdatatype.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*
 * Hash table entries for data type descriptors
 * It keeps the double linked list in case
 */
typedef struct ft_node ft_node_t;

struct ft_node
{
    UBF_field_def_t def;
    /* part of the hash */
    ft_node_t *next;
};
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API UBF_field_def_t * ndrx_fldnmhash_get(char *key);
extern NDRX_API UBF_field_def_t * _bfldidhash_get(BFLDID id);
extern NDRX_API int ndrx_prepare_type_tables(void);
/* Internal version of Bfname (for debug, etc..) */
extern NDRX_API char * ndrx_Bfname_int (BFLDID bfldid);
extern NDRX_API BFLDID ndrx_Bfldid_int (char *fldnm);

extern NDRX_API int _ubf_loader_init(void);

extern NDRX_API int ndrx_ubf_load_def_file(FILE *fp,
                int (*put_text_line) (char *text), /* callback for putting text line */
                int (*put_def_line) (UBF_field_def_t *def),  /* callback for writting defintion */
                int (*put_got_base_line) (char *base), /* Callback for base line */
                char *fname,
                int check_dup
            );
#ifdef	__cplusplus
}
#endif

#endif	/* FIELDTABLE_H */

/* vim: set ts=4 sw=4 et smartindent: */
