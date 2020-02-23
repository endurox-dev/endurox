/**
 * @brief Standard header tests
 *
 * @file test_nstd_standard.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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

#define _GNU_SOURCE
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
#include <utlist.h>
#include <nstdutil.h>
/**
 * Test NDRX_STRCPY_S macro
 */
Ensure(test_nstd_ndrx_strcpy_s)
{
    char tmp[16] = {EXEOS};
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), "HELLO");
    assert_string_equal(tmp, "HELLO");
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), " WORLD");
    assert_string_equal(tmp, "HELLO WORLD");
    
    NDRX_STRCAT_S(tmp, sizeof(tmp), " THIS");

    /* on C11 / solaris 11.4 this might be stripped to empty */
    if (EXEOS!=tmp[0])
    {
        assert_string_equal(tmp, "HELLO WORLD THI");
    }
}

/**
 * Test NDRX_ASPRINTF
 */
Ensure(test_nstd_ndrx_asprintf)
{
    char *p = (char *)123;
    long len;
    
    NDRX_ASPRINTF(&p, &len, "Hello %d %s", 1, "world");
    
    assert_not_equal(p, NULL);
    assert_not_equal(p, 123);
    assert_equal(len, 13);
    
    assert_string_equal(p, "Hello 1 world");
    
    NDRX_FREE(p);
    
}

/**
 * Test split add
 */
Ensure(test_ndrx_string_list_splitadd)
{
    string_list_t *list = NULL;
    string_list_t *el = NULL;
    int i = 0;
    
    assert_equal(ndrx_string_list_splitadd(&list, "\tHELLO: WORLD:22", ":"), EXSUCCEED);
    
    /* Check the entries in list: */
    LL_FOREACH(list, el)
    {
        i++;
        
        switch (i)
        {
            case 1:
                assert_equal(el->qname, "HELLO");
                break;
            case 2:
                assert_equal(el->qname, "WORLD");
                break;
            case 3:
                assert_equal(el->qname, "22");
                break;
            default:
                /* Too many entries! */
                NDRX_LOG(log_error, "Too many entries! [%s]", el->qname);
                assert_equal(EXFALSE, EXTRUE);
                break;
        }
    }
    
    ndrx_string_list_free(list);
}

/**
 * Standard header tests
 * @return
 */
TestSuite *ubf_nstd_standard(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_ndrx_strcpy_s);
    add_test(suite, test_nstd_ndrx_asprintf);
            
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
