/* 
 * @brief UBF library - header file for UBF error handling.
 *
 * @file ferror.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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
#ifndef FERROR_H
#define	FERROR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <regex.h>
#include <ndrstandard.h>
#include <ubf_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_ERROR_LEN   1024
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API void ndrx_Bset_error(int error_code);
extern NDRX_API void ndrx_Bset_error_msg(int error_code, char *msg);
extern NDRX_API void ndrx_Bset_error_fmt(int error_code, const char *fmt, ...);
/* Is error already set?  */
extern NDRX_API int ndrx_Bis_error(void);
extern NDRX_API void ndrx_Bappend_error_msg(char *msg);

extern NDRX_API void ndrx_Bunset_error(void);

/* This is not related with UBF error, we will handle it here anyway
 * because file is part of error handling
 */
extern NDRX_API void ndrx_report_regexp_error(char *fun_nm, int err, regex_t *rp);
#ifdef	__cplusplus
}
#endif

#endif	/* FERROR_H */

