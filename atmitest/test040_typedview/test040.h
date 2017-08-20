/* 
** Typed VIEW tests
**
** @file test040.h
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
#ifndef TEST040_H
#define	TEST040_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "t40.h"
#include "t40_2.h"
    
#define TEST_AS_STRING(FLD, OCC, VAL)\
    assert_equal(CBget(p_ub, FLD, OCC, tmp, 0L, BFLD_STRING),  EXSUCCEED);\
    assert_string_equal(VAL, tmp);

#define TEST_AS_STRING2(FLD, OCC, VAL)\
    assert_equal(CBget(p_ub, FLD, OCC, tmp, 0L, BFLD_STRING),  EXSUCCEED);\
    assert_equal(strcmp(VAL, tmp), 0);

#define TEST_AS_DOUBLE(FLD, OCC, VAL)\
    assert_equal(CBget(p_ub, FLD, OCC, (char *)&dtemp, 0L, BFLD_DOUBLE),  EXSUCCEED);\
    assert_double_equal(VAL, dtemp);
    
#define GET_CARRAY_DOUBLE_TEST_LEN(FLD, OCC, LEN)\
    len=sizeof(tmp);\
    assert_equal(Bget(p_ub, FLD, OCC, tmp, &len),  EXSUCCEED);\
    assert_equal(len, LEN);


extern void init_MYVIEW1(struct MYVIEW1 *v);
extern void init_MYVIEW3(struct MYVIEW3 *v);

extern int validate_MYVIEW1(struct MYVIEW1 *v);

#ifdef	__cplusplus
}
#endif

#endif	/* TEST040_H */

