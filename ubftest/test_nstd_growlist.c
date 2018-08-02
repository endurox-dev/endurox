/* 
 * @brief Growlist testing from Enduro/X Standard Library
 *
 * @file test_nstd_growlist.c
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

#include <stdio.h>
#include <stdlib.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include <ndebug.h>
#include <exbase64.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"
#include <nstdutil.h>

/**
 * Basic grow list testing
 */
Ensure(test_nstd_growlist)
{
    ndrx_growlist_t list;
    long i;
    long *data;
    
    ndrx_growlist_init(&list, 11, sizeof(void *));
    
    for (i=0; i<100; i++)
    {
        assert_equal(EXSUCCEED, ndrx_growlist_add(&list, (void *)&i, i));
    }
    
    for (i=0; i<100; i++)
    {
        data = (long*)((char *)(list.mem) + i * sizeof(void *));
        
        assert_equal( i, *data);
    }
    
    assert_equal(list.maxindexused, 99);
    
    i = 777;
    assert_equal(EXSUCCEED, ndrx_growlist_add(&list, (void *)&i, 999));
    
    assert_equal(list.maxindexused, 999);
    
    data = (long*)((char *)(list.mem) + 999 * sizeof(void *));
        
    assert_equal( i, *data);
    
    i=1001;
    assert_equal(EXSUCCEED, ndrx_growlist_append(&list, (void *)&i));
    
    assert_equal(*NDRX_GROWLIST_ACCESS(&list, 1000, long), (long)1001);
    
    data = (long*)((char *)(list.mem) + 1000 * list.size);
        
    assert_equal( i, *data);
    
    
    ndrx_growlist_free(&list);
    
}

/**
 * Grow list testing
 * @return
 */
TestSuite *ubf_nstd_growlist(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_growlist);
            
    return suite;
}
