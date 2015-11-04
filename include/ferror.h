/* 
** UBF library - header file for UBF error handling.
**
** @file ferror.h
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
public void _Fset_error(int error_code);
public void _Fset_error_msg(int error_code, char *msg);
public void _Fset_error_fmt(int error_code, const char *fmt, ...);
/* Is error already set?  */
public int _Fis_error(void);
public void _Bappend_error_msg(char *msg);

public void _Bunset_error(void);

/* This is not related with UBF error, we will handle it here anyway
 * because file is part of error handling
 */
public void report_regexp_error(char *fun_nm, int err, regex_t *rp);
#ifdef	__cplusplus
}
#endif

#endif	/* FERROR_H */

