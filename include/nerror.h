/* 
 * @brief NSTD library - header file for error handling.
 *
 * @file nerror.h
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
#ifndef NERROR_H
#define	NERROR_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_ERROR_LEN   1024

#define NMINVAL             0 /* min error */
#define NEINVALINI          1 /* Invalid INI file */
#define NEMALLOC            2 /* Malloc failed */
#define NEUNIX              3 /* Unix error occurred */
#define NEINVAL             4 /* Invalid value passed to function */
#define NESYSTEM            5 /* System failure */
#define NEMANDATORY         6 /* Mandatory field is missing */
#define NEFORMAT            7 /* Format error */
#define NETOUT              8 /* Time-out condition */
#define NENOCONN            9 /* Connection not found */
#define NELIMIT            10 /* Limit reached */
#define NEPLUGIN           11 /* Plugin error */
#define NENOSPACE          12 /* No space */
#define NEINVALKEY         13 /* Invalid key (probably) */
#define NMAXVAL            13 /* max error */

#define Nerror  (*_Nget_Nerror_addr())

   
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern NDRX_API char * ndrx_Necodestr(int err);
extern NDRX_API void _Nset_error(int error_code);
extern NDRX_API char * Nstrerror (int err);
extern NDRX_API void _Nset_error_msg(int error_code, char *msg);
extern NDRX_API void _Nset_error_fmt(int error_code, const char *fmt, ...);
/* Is error already set?  */
extern NDRX_API int _Nis_error(void);
extern NDRX_API void _Nappend_error_msg(char *msg);

extern NDRX_API void _Nunset_error(void);

extern NDRX_API int * _Nget_Nerror_addr (void);

#ifdef	__cplusplus
}
#endif

#endif	/* NERROR_H */

