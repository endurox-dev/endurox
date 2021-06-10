/**
 * @brief sys_fsync unit tests
 *
 * @file test_nstd_fsync.c
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
#include <cgreen/cgreen.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <string.h>
#include <ndebug.h>
#include <exbase64.h>
#include <nstdutil.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Basic fsync tests
 */
Ensure(test_nstd_fsync)
{
    long flags=0;
    FILE *f=NULL;
    char filename[]="/tmp/ubf-test-XXXXXX";
    char buf[128];
    assert_not_equal(mkstemp(filename), EXFAIL);
    assert_not_equal((f=fopen(filename, "r")), NULL);
    
    /* should parse OK */
    NDRX_STRCPY_SAFE(buf, "fsync,nosync");
    assert_equal(ndrx_fsync_parse(buf, &flags), EXFAIL);
    
    /* parse fail */
    NDRX_STRCPY_SAFE(buf, "fdatasync,nosync");
    assert_equal(ndrx_fsync_parse(buf, &flags), EXFAIL);

    assert_equal(ndrx_fsync_fsync(f, flags), EXSUCCEED);
    assert_equal(ndrx_fsync_fsync(NULL, flags), EXFAIL);

    NDRX_STRCPY_SAFE(buf, "fsync,dsync,fdatasync");
    assert_equal(ndrx_fsync_parse(buf, &flags), EXSUCCEED);
    assert_equal(!!(flags & NDRX_FSYNC_FSYNC), EXTRUE);
    assert_equal(!!(flags & NDRX_FSYNC_FDATASYNC), EXTRUE);
    assert_equal(!!(flags & NDRX_FSYNC_DSYNC), EXTRUE);

    flags=0;
    NDRX_STRCPY_SAFE(buf, "fsync,dsync,fdatasync");
    assert_equal(ndrx_fsync_parse(buf, &flags), EXSUCCEED);
    assert_equal(!!(flags & NDRX_FSYNC_FSYNC), EXTRUE);
    assert_equal(!!(flags & NDRX_FSYNC_FDATASYNC), EXTRUE);
    assert_equal(!!(flags & NDRX_FSYNC_DSYNC), EXTRUE);


    assert_equal(ndrx_fsync_fsync(f, flags), EXSUCCEED);
    assert_equal(ndrx_fsync_fsync(NULL, flags), EXFAIL);
    
    /* clean-up... */
    fclose(f);
    unlink(filename);
}

/**
 * Basic directory-sync tests
 */
Ensure(test_nstd_dsync)
{
    assert_equal(ndrx_fsync_dsync("/tmp/", NDRX_FSYNC_DSYNC), EXSUCCEED);
    assert_equal(ndrx_fsync_dsync("/no/such/directory/endurox", NDRX_FSYNC_DSYNC), EXFAIL);
    assert_equal(ndrx_fsync_dsync("/no/such/directory/endurox", 0), EXSUCCEED);
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_fsync(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_fsync);
    add_test(suite, test_nstd_dsync);

    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
