/**
 * @brief Multi-buffer tests
 *
 * @file atmiunit0_mbuf.c
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
/*#include "ubfunit1.h"*/
#include "ndebug.h"
#include "xatmi.h"
#include "atmi_int.h"
#include <fdatatype.h>

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
 * Test the serialization engine with call info, recursion of UBFS
 * and PTRs
 */
Ensure(test_mbuf)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    UBFH *p_ub2 = (UBFH *)tpalloc("UBF", NULL, 1024);
    UBFH *p_ub3 = (UBFH *)tpalloc("UBF", NULL, 1024);
    UBFH *p_ub4 = (UBFH *)tpalloc("UBF", NULL, 1024);
    
    UBFH *p_ub5 = NULL;
       
    UBFH *p_callinfo = (UBFH *)tpalloc("UBF", NULL, 1024);
    BVIEWFLD vf;
    char buf[8096];
    long buf_len;
    long olen;
    assert_not_equal(p_ub, NULL);
    assert_not_equal(p_ub2, NULL);
    assert_not_equal(p_ub3, NULL);
    assert_not_equal(p_ub4, NULL);
    assert_not_equal(p_callinfo, NULL);
    
    
    /* load CI fields */
    assert_equal(Bchg(p_callinfo, T_STRING_10_FLD, 0, "HELLO CALL INFO", 0L), EXSUCCEED);
    assert_equal(CBchg(p_callinfo, T_LONG_FLD, 0, "255266", 0L, BFLD_STRING), EXSUCCEED);
    
    assert_equal(tpsetcallinfo((char *)p_ub, p_callinfo, 0), EXSUCCEED);
    
    /* load some-sub-sub fields */
    assert_equal(Bchg(p_ub4, T_STRING_FLD, 0, "HELLO 4", 0L), EXSUCCEED);
    assert_equal(CBchg(p_ub4, T_SHORT_FLD, 0, "777", 0L, BFLD_STRING), EXSUCCEED);
    
    
    /* Load sub-ubf to 3 */
    assert_equal(Bchg(p_ub3, T_UBF_FLD, 0, (char *)p_ub4, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub4, T_STRING_2_FLD, 0, "HELLO 4 PTR", 0L), EXSUCCEED);
    assert_equal(CBchg(p_ub4, T_SHORT_FLD, 0, "999", 0L, BFLD_STRING), EXSUCCEED);
    
    /* set PTR */
    assert_equal(Bchg(p_ub3, T_PTR_FLD, 0, (char *)&p_ub4, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub2, T_PTR_FLD, 3, (char *)&p_ub3, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_PTR_FLD, 3, (char *)&p_ub2, 0L), EXSUCCEED);
    
    /* load some buffer at tome level too... */
    assert_equal(CBchg(p_ub, T_SHORT_3_FLD, 3, "22", 0L, BFLD_STRING), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_UBF_2_FLD, 3, (char *)p_ub4, 0L), EXSUCCEED);
    
    /* have some view as terminator */
    memset(&vf, 0, sizeof(vf));
    assert_equal(Bchg(p_ub, T_VIEW_FLD, 3, (char *)&vf, 0L), EXSUCCEED);
    
    /* OK drive out */
    buf_len=sizeof(buf);
    
    /* print the source buf */
    fprintf(stdout, "Source buf:\n");
    Bprint(p_ub);
    
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 0), EXSUCCEED);
    
    /* OK dump the output */
    ndrx_mbuf_tlv_debug(buf, buf_len);
    
    
    olen=0;
    assert_equal(ndrx_mbuf_prepare_incoming (buf, buf_len, (char **)&p_ub5, &olen, 0, 0), EXSUCCEED);
    
    /* print the buffer, shall be the same as first one... */
    fprintf(stdout, "Parsed buf:\n");
    Bprint(p_ub5);
    
    /* TODO: Free up.. */
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *atmiunit0_mbuf(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_mbuf);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
