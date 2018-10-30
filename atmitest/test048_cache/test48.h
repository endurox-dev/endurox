/**
 * @brief TP Cache tests - common header
 *
 * @file test48.h
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
#ifndef TEST48_H
#define TEST48_H

#ifdef  __cplusplus
extern "C" {
#endif


#define VALUE_EXPECTED "Hello EnduroX"
#define VALUE_EXPECTED_RET "This is response from server!"
    
#define TSTAMP_BUFSZ        32              /* buffer size for tstamp testing */
    
    
    
extern void test048_stamp_get(char *buf, int bufsz);
extern int test048_stamp_isequal(char *stamp1, char *stamp2);


#ifdef  __cplusplus
}
#endif

#endif  /* TEST48_H */

/* vim: set ts=4 sw=4 et smartindent: */
