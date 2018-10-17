/* 
** tpimport()/tpexport() function tests - common header
**
** @file testtest056_tpimpexp.h
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
#ifndef TEST56_H
#define TEST56_H

#ifdef  __cplusplus
extern "C" {
#endif


#define VALUE_EXPECTED "Hello EnduroX"

extern NDRX_API int test_impexp_string();
extern NDRX_API int test_impexp_ubf();
extern NDRX_API int test_impexp_view();
extern NDRX_API int test_impexp_json();
extern NDRX_API int test_impexp_carray();

#ifdef  __cplusplus
}
#endif

#endif  /* TEST56_H */

