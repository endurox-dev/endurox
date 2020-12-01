/**
 * Print tests for view
 * @file test_printv.c
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
#include <unistd.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"
#include "atmi_int.h"

/**
 * Test - test plus command
 */
Ensure(test_bvextread_plus)
{
    struct UBTESTVIEW2 v;
    
    /* Check with sub-fields */
    char *test_plus[]= {
        "tshort1\t1\n",
	"tlong1\t2\n",
	"+ tshort1\t2\n",
	"+ tlong1\t99999\n",
        NULL
    };
    
    memset(&v, 0, sizeof(v));
    
    /* load field table */
    load_field_table();

    open_test_temp("w");
    write_to_temp(test_plus);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bvextread((char *)&v, "UBTESTVIEW2", M_test_temp_file), EXSUCCEED);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    
    /* check the view contents  for those fields shall be 2 & 3*/
    assert_equal(v.tshort1, 2);
    assert_equal(v.tlong1, 99999);
}


/**
 * Test - test minus command
 */
Ensure(test_bvextread_minus)
{
    struct UBTESTVIEW2 v;
    
    /* Check with sub-fields */
    char *test_plus[]= {
        "tshort1\t8888\n",
	"tlong1\t2\n",
	"- tshort1\t\n",
	"- tlong1\t\n",
        NULL
    };
    
    memset(&v, 0, sizeof(v));
    
    /* load field table */
    load_field_table();

    open_test_temp("w");
    write_to_temp(test_plus);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bvextread((char *)&v, "UBTESTVIEW2", M_test_temp_file), EXSUCCEED);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    
    /* check the view contents  for those fields shall be 2 & 3*/
    assert_equal(v.tshort1, 2000);
    assert_equal(v.tlong1, 5);
}


/**
 * Test - test assign command
 */
Ensure(test_bvextread_assign)
{
    struct UBTESTVIEW2 v;
    
    /* Check with sub-fields */
    char *test_plus[]= {
        "tshort1\t8888\n",
	"tlong1\t7771\n",
	"= tshort1\ttlong1\n",
        NULL
    };
    
    memset(&v, 0, sizeof(v));
    
    /* load field table */
    load_field_table();

    open_test_temp("w");
    write_to_temp(test_plus);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bvextread((char *)&v, "UBTESTVIEW2", M_test_temp_file), EXSUCCEED);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    
    /* check the view contents  for those fields shall be 2 & 3*/
    assert_equal(v.tshort1, 7771);
    
}


/**
 * Test Bvfprint
 */
Ensure(test_bvfprint)
{
    FILE *f;
    int fstdout;
    char filename[]="/tmp/view-test-XXXXXX";
    
    struct UBTESTVIEW1 v1;
    struct UBTESTVIEW1 v2;

    assert_not_equal(mkstemp(filename), EXFAIL);
    
    memset(&v1, 0, sizeof(v1));
    memset(&v2, 0, sizeof(v2));
    
    extest_init_UBTESTVIEW1(&v1);
    /*
    Bvprint((char *)&v1, "UBTESTVIEW1");
    
    printf("EOF\n");
    */
    
    /* nothing much to test here... */
    /*close(1);  close stdout */
    assert_not_equal((f=fopen(filename, "w")), NULL);
    /* fstdout = dup2(fileno(f), 1); make file appear as stdout */
    assert_equal(Bvfprint((char *)&v1, "UBTESTVIEW1", f), NULL);
    fclose(f);

    /* OK, if we have that output, try to extread it! */
    assert_not_equal((f=fopen(filename, "r")), NULL);

    /*
    Bvprint((char *)&v2, "UBTESTVIEW1");
    */
    Bvprint((char *)&v1, "UBTESTVIEW1");
    
    
    assert_equal(Bvextread((char *)&v2, "UBTESTVIEW1", f), EXSUCCEED);
    /* compare read buffer */
    assert_equal(Bvcmp((char *)&v1, "UBTESTVIEW1", (char *)&v2, "UBTESTVIEW1"), 0);
    /* Remove test file */
    assert_equal(unlink(filename), EXSUCCEED);
}

/**
 * Test bvprint
 * There is special note for this!
 * If running in single test mode, then STDOUT will be lost!
 */
Ensure(test_bvprint)
{
    FILE *f;
    int fstdout;
    char filename[]="/tmp/view-test-XXXXXX";
    struct UBTESTVIEW1 v1;
    struct UBTESTVIEW1 v2;

    
    memset(&v1, 0, sizeof(v1));
    memset(&v2, 0, sizeof(v2));
    extest_init_UBTESTVIEW1(&v1);
    
    assert_not_equal(mkstemp(filename), EXFAIL);
    
    /* nothing much to test here... */
    close(1); /* close stdout */
    assert_not_equal((f=fopen(filename, "w")), NULL);
    fstdout = dup2(fileno(f), 1); /* make file appear as stdout */
    assert_equal(Bvprint((char *)&v1, "UBTESTVIEW1"), NULL);
    fclose(f);

    /* OK, if we have that output, try to extread it! */
    assert_not_equal((f=fopen(filename, "r")), NULL);

    assert_equal(Bvextread((char *)&v2, "UBTESTVIEW1", f), EXSUCCEED);
    /* compare read buffer */
    assert_equal(Bvcmp((char *)&v1, "UBTESTVIEW1", (char *)&v2, "UBTESTVIEW1"), 0);
    /* Remove test file */
    assert_equal(unlink(filename), EXSUCCEED);
}


/**
 * Test data holder for bfprintcb_data
 */
typedef struct bfprintcb_data bfprintcb_data_t;
struct bfprintcb_data
{
    int nrlines;
    int curline;
    char lines[1024][100];
};

/**
 * Write callback, this will fill in passed array
 * @param buffer, we may realloc
 * @param datalen output data len
 * @param dataptr1 custom pointer
 * @param do_write shall the Enduro/X wirte to output log..
 * @param outf output stream currencly used
 * @param fid field it processing
 * @return 
 */
exprivate int test_bfprintcb_writef(char **buffer, long datalen, void *dataptr1, 
        int *do_write, FILE *outf, BFLDID fid)
{
    bfprintcb_data_t *data = (bfprintcb_data_t *)dataptr1;
    
    assert_equal(strlen(*buffer)+1, datalen);
    
    NDRX_STRCPY_SAFE(data->lines[data->nrlines], *buffer);
    data->nrlines++;
    return EXSUCCEED;
}

/**
 * Return the buffer line
 * @param buffer buffer to put in the result. Note that is should go line by line
 * @param bufsz buffer size
 * @param dataptr1 user data ptr
 * @return number of bytes written to buffer
 */
exprivate long bextreadcb_readf(char *buffer, long bufsz, void *dataptr1)
{
    bfprintcb_data_t *data = (bfprintcb_data_t *)dataptr1;
    
    if (data->curline < data->nrlines)
    {
        NDRX_STRCPY_SAFE_DST(buffer, data->lines[data->curline], bufsz);
        data->curline++;
        
        return strlen(buffer)+1;
        
    }
    else
    {
        return 0;
    }
}

/**
 * Bvfprintcb testing (i.e. callback testing...)
 * @return
 */
Ensure(test_bvfprintcb)
{
    bfprintcb_data_t data;
    int line_counter=0;

    struct UBTESTVIEW1 v1;
    struct UBTESTVIEW1 v2;

    
    memset(&v1, 0, sizeof(v1));
    memset(&v2, 0, sizeof(v2));
    extest_init_UBTESTVIEW1(&v1);
    
    memset(&data, 0, sizeof(data));
    
    assert_equal(Bvfprintcb((char *)&v1, "UBTESTVIEW1", 
        test_bfprintcb_writef, (void *)&data), EXSUCCEED);
    
    UBF_LOG(log_error, "Bvfprintcb: %s", Bstrerror(Berror));
    
    /* ok now read back from callback.. */
    assert_equal(Bvextreadcb((char *)&v2, "UBTESTVIEW1", bextreadcb_readf, (char *)&data), EXSUCCEED);
    
    assert_equal(Bvcmp((char *)&v1, "UBTESTVIEW1", (char *)&v2, "UBTESTVIEW1"), 0);
    
}


TestSuite *ubf_printv_tests(void)
{
    TestSuite *suite = create_test_suite();

    std_basic_setup();
    
    add_test(suite, test_bvprint);
    add_test(suite, test_bvfprint);
    add_test(suite, test_bvfprintcb);
    add_test(suite, test_bvextread_plus);
    add_test(suite, test_bvextread_minus);
    add_test(suite, test_bvextread_assign);
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
