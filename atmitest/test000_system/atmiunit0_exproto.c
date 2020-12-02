/**
 * @brief Protocol converter unit tests
 *
 * @file atmiunit0_exproto.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include "test.fd.h"
#include "ndebug.h"
#include "xatmi.h"
#include "atmi_int.h"
#include <fdatatype.h>
#include <nstdutil.h>
#include <typed_buf.h>
#include <extest.h>
#include <atmi_int.h>
#include <exproto.h>

/**
 * Basic preparation before the test
 */
exprivate void basic_setup(void)
{
    
}

exprivate void basic_teardown(void)
{
    
}

/**
 * Convert UBF call two way
 */
Ensure(test_proto_ubfcall)
{
    /* allocate FB & load data, convert out... (i.e.) alloc the tpcall struct */
    char buf[2048];
    char proto_out[2048];
    long proto_len;
    UBFH *p_ub;
    tp_command_call_t *call = (tp_command_call_t *)buf;
    
    /* reset call header */
    memset(call, 0, sizeof(*call));
    
    /* init the fb */
    p_ub = (UBFH *)call->data;
    
    assert_equal(Binit(p_ub, 1024), EXSUCCEED);
    
    /* Load some buffer fields (standard mode currently */
    extest_ubf_set_up_dummy_data(p_ub, 0);
    
    call->data_len=Bused(p_ub);
    
    proto_len = 0;
    
    /* try to serialize */
    assert_equal(exproto_ex2proto(buf, sizeof(*call)+call->data_len, 
	proto_out, &proto_len, sizeof(proto_out)), EXSUCCEED);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *atmiunit0_exproto(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_proto_ubfcall);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
