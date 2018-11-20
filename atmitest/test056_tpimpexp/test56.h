/* 
 * tpimport()/tpexport() function tests - common header
 *
 * @file test56.h
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
#ifndef TEST56_H
#define TEST56_H

#ifdef  __cplusplus
extern "C" {
#endif


#define CARR_BUFFSIZE       NDRX_MSGSIZEMAX
#define CARR_BUFFSIZE_B64   (4 * (CARR_BUFFSIZE) / 3)


extern NDRX_API int test_impexp_string();
extern NDRX_API int test_impexp_ubf();
extern NDRX_API int test_impexp_view();
extern NDRX_API int test_impexp_json();
extern NDRX_API int test_impexp_carray();

#ifdef  __cplusplus
}
#endif

#endif  /* TEST56_H */

