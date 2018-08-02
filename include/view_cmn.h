/* 
 * @brief Common view structures
 *
 * @file view_cmn.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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

#ifndef VIEW_CMN_H
#define	VIEW_CMN_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/*
 * In-memory structure offsets
 */ 
typedef struct
{
    char   *cname;
    long    offset;
    long    fldsize;
    long    count_fld_offset; /* optional, if not used then -1*/
    long    length_fld_offset; /* optional, if not used then -1*/
} ndrx_view_offsets_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API int ndrx_view_update_offsets(char *vname, ndrx_view_offsets_t *p);
extern NDRX_API int ndrx_view_update_object(char *vname, long ssize);
extern NDRX_API int ndrx_view_load_file(char *fname, int is_compiled);
extern NDRX_API int ndrx_view_plot_object(FILE *f);
extern NDRX_API void ndrx_view_loader_configure(int no_ubf_proc);
extern NDRX_API void ndrx_view_deleteall(void);

#ifdef	__cplusplus
}
#endif

#endif	/* VIEW_CMN_H */

