/**
 *
 * @file test_print.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
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
#include <unistd.h>
#include "test.fd.h"
#include "ubfunit1.h"
#include "ndebug.h"

/**
 * Reference output that should be printed to log file.
 */
char *ref_print[]= {
"T_SHORT_FLD\t88\n",
"T_SHORT_FLD\t-1\n",
"T_SHORT_2_FLD\t0\n",
"T_SHORT_2_FLD\t212\n",
"T_LONG_FLD\t-1021\n",
"T_LONG_FLD\t-2\n",
"T_LONG_FLD\t0\n",
"T_LONG_FLD\t0\n",
"T_LONG_FLD\t-4\n",
"T_LONG_2_FLD\t0\n",
"T_LONG_2_FLD\t0\n",
"T_LONG_2_FLD\t212\n",
"T_CHAR_FLD\tc\n",
"T_CHAR_FLD\t.\n",
"T_CHAR_2_FLD\t\n",
"T_CHAR_2_FLD\t\n",
"T_CHAR_2_FLD\t\n",
"T_CHAR_2_FLD\tb\n",
"T_FLOAT_FLD\t17.31000\n",
"T_FLOAT_FLD\t1.31000\n",
"T_FLOAT_2_FLD\t0.00000\n",
"T_FLOAT_2_FLD\t0.00000\n",
"T_FLOAT_2_FLD\t0.00000\n",
"T_FLOAT_2_FLD\t0.00000\n",
"T_FLOAT_2_FLD\t1227.00000\n",
"T_DOUBLE_FLD\t12312.111100\n",
"T_DOUBLE_FLD\t112.110000\n",
"T_DOUBLE_2_FLD\t0.000000\n",
"T_DOUBLE_2_FLD\t0.000000\n",
"T_DOUBLE_2_FLD\t0.000000\n",
"T_DOUBLE_2_FLD\t0.000000\n",
"T_DOUBLE_2_FLD\t0.000000\n",
"T_DOUBLE_2_FLD\t1232.100000\n",
"T_STRING_FLD\tTEST STR VAL\n",
"T_STRING_FLD\tTEST STRING ARRAY2\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\t\n",
"T_STRING_2_FLD\tMEGA STR\\\\ING \\0a \\09 \\0d\n",
"T_CARRAY_FLD\t\n",
"T_CARRAY_FLD\tY\\01\\02\\03AY1 TEST \\\\STRING DATA\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\n",
"T_CARRAY_2_FLD\t\\00\\01\\02\\03AY1 TEST \\\\STRING DATA\n",
NULL
};

/**
 * Prepare test data for print test.
 * @param p_ub
 */
void load_print_test_data(UBFH *p_ub)
{
    short s = 88;
    long l = -1021;
    char c = 'c';
    float f = 17.31;
    double d = 12312.1111;
    char carr[] = "CARRAY1 TEST \\STRING DATA";
    char string2[] = "MEGA STR\\ING \n \t \r";
    carr[0] = 0;
    carr[1] = 1;
    carr[2] = 2;
    carr[3] = 3;
    BFLDLEN len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 0, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 0, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 0, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 0, (char *)"TEST STR VAL", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 0, (char *)carr, len), EXSUCCEED);

    /* Make second copy of field data (another for not equal test)*/
    s = -1;
    l = -2;
    c = '.';
    f = 1.31;
    d = 112.11;
    carr[0] = 'Y';
    len = strlen(carr);

    assert_equal(Bchg(p_ub, T_SHORT_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_FLD, 1, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_FLD, 1, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_FLD, 1, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_FLD, 1, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_FLD, 1, (char *)"TEST STRING ARRAY2", 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_FLD, 1, (char *)carr, len), EXSUCCEED);

    l = -4;
    assert_equal(Bchg(p_ub, T_LONG_FLD, 4, (char *)&l, 0), EXSUCCEED);

    s = 212;
    l = 212;
    c = 'b';
    f = 1227;
    d = 1232.1;
    carr[0] = 0;
    assert_equal(Bchg(p_ub, T_SHORT_2_FLD, 1, (char *)&s, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_LONG_2_FLD, 2, (char *)&l, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CHAR_2_FLD, 3, (char *)&c, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_FLOAT_2_FLD, 4, (char *)&f, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_DOUBLE_2_FLD, 5, (char *)&d, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_STRING_2_FLD, 6, (char *)string2, 0), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_CARRAY_2_FLD, 7, (char *)carr, len), EXSUCCEED);
}


/**
 * Bfprint testing.
 * @return
 */
Ensure(test_bfprint)
{
    char fb[2024];
    UBFH *p_ub = (UBFH *)fb;
    BFLDLEN len=0;
    FILE *f=NULL;
    char filename[]="/tmp/ubf-test-XXXXXX";
    char readbuf[1024];
    int line_counter=0;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    load_print_test_data(p_ub);
    load_field_table();
    assert_not_equal(mkstemp(filename), EXFAIL);
    assert_not_equal((f=fopen(filename, "w")), NULL);
    assert_equal(Bfprint(p_ub, f), EXSUCCEED);
    fclose(f);

    /* Re-open file in read mode end re-compare the buffer. */
    assert_not_equal((f=fopen(filename, "r")), NULL);

    /* compare the buffers */
    while(NULL!=fgets(readbuf, sizeof(readbuf), f))
    {
        if (NULL==ref_print[line_counter])
        {
            /* reached end of our knowledge of the test data */
            assert_equal_with_message(0, 1, "output file too big!");
            break;
        }
        assert_string_equal(ref_print[line_counter], readbuf);
        line_counter++;
    }
    fclose(f);

    /* remove the file */
    assert_equal(unlink(filename), EXSUCCEED);

    /* cannot print on null file */
    assert_equal(Bfprint(p_ub, NULL), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
}

/**
 * Test data holder for bfprintcb_data
 */
typedef struct bfprintcb_data bfprintcb_data_t;
struct bfprintcb_data
{
    int nrlines;
    char lines[1024][100];
};

/**
 * Write callback, this will fill in passed array
 * @param buffer
 * @param datalen
 * @param dataptr1
 * @return 
 */
exprivate int test_bfprintcb_writef(char *buffer, long datalen, void *dataptr1)
{
    bfprintcb_data_t *data = (bfprintcb_data_t *)dataptr1;
    
    assert_equal(strlen(buffer)+1, datalen);
    
    NDRX_STRCPY_SAFE(data->lines[data->nrlines], buffer);
    data->nrlines++;
    return EXSUCCEED;
}

/**
 * Bfprintcb testing (i.e. callback testing...)
 * @return
 */
Ensure(test_bfprintcb)
{
    char fb[2024];
    UBFH *p_ub = (UBFH *)fb;
    bfprintcb_data_t data;
    int line_counter=0;
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);

    memset(&data, 0, sizeof(data));
    load_print_test_data(p_ub);
    load_field_table();
    
    assert_equal(Bfprintcb(p_ub, test_bfprintcb_writef, (void *)&data), EXSUCCEED);
    UBF_LOG(log_error, "Bfprintcb: %s", Bstrerror(Berror));
    
    /* compare the buffers */
    for (line_counter=0; line_counter<N_DIM(ref_print)-1; line_counter++)
    {
        assert_string_equal(data.lines[line_counter], ref_print[line_counter]);
        line_counter++;
    }
    
    assert_equal(data.nrlines, N_DIM(ref_print)-1);
    
    /* cannot print on null file */
    assert_equal(Bfprintcb(p_ub, NULL, NULL), EXFAIL);
    assert_equal(Berror, BEINVAL);
    
}

/**
 * Test bprint
 * There is special note for this!
 * If running in single test mode, then STDOUT will be lost!
 */
Ensure(test_bprint)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;
    BFLDLEN len=0;
    FILE *f;
    int fstdout;
    char filename[]="/tmp/ubf-test-XXXXXX";

    assert_not_equal(mkstemp(filename), EXFAIL);
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    load_print_test_data(p_ub);
    /* nothing much to test here... */
    close(1); /* close stdout */
    assert_not_equal((f=fopen(filename, "w")), NULL);
    fstdout = dup2(fileno(f), 1); /* make file appear as stdout */
    assert_equal(Bprint(p_ub), NULL);
    fclose(f);

    /* OK, if we have that output, try to extread it! */
    assert_not_equal((f=fopen(filename, "r")), NULL);

    assert_equal(Bextread(p_ub2, f), EXSUCCEED);
    /* compare read buffer */
    assert_equal(Bcmp(p_ub, p_ub2), 0);
    /* Remove test file */
    assert_equal(unlink(filename), EXSUCCEED);
}

/**
 * Test function for Bextread, using field IDs (not present in table)
 */
Ensure(test_bextread_bfldid)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;

    BFLDLEN len=0;
    FILE *f;
    char filename[]="/tmp/ubf-test-XXXXXX";

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    load_print_test_data(p_ub);
    set_up_dummy_data(p_ub);
    

    assert_not_equal(mkstemp(filename), EXFAIL);

    assert_not_equal((f=fopen(filename, "w")), NULL);
    assert_equal(Bfprint(p_ub, f), EXSUCCEED);
    fclose(f);

    /* read stuff form file */
    assert_not_equal((f=fopen(filename, "r")), NULL);
    assert_equal(Bextread(p_ub2, f), EXSUCCEED);
    fclose(f);
    
    /* compare readed buffer */
    assert_equal(Bcmp(p_ub, p_ub2), 0);
    /* Remove test file */
    assert_equal(unlink(filename), EXSUCCEED);
}

/**
 * Test function for Bextread, using field names
 */
Ensure(test_bextread_fldnm)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;

    BFLDLEN len=0;
    FILE *f;
    char filename[]="/tmp/ubf-test-XXXXXX";

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);

    load_print_test_data(p_ub);
    set_up_dummy_data(p_ub);
    load_field_table();

    assert_not_equal(mkstemp(filename), EXFAIL);
    assert_not_equal((f=fopen(filename, "w")), NULL);
    assert_equal(Bfprint(p_ub, f), EXSUCCEED);
    fclose(f);

    /* read stuff form file */
    assert_not_equal((f=fopen(filename, "r")), NULL);
    assert_equal(Bextread(p_ub2, f), EXSUCCEED);
    fclose(f);

    /* compare readed buffer */
    assert_equal(Bcmp(p_ub, p_ub2), 0);
    /* Remove test file */
    assert_equal(unlink(filename), EXSUCCEED);
}
/**
 * Testing extread for errors
 */
Ensure(test_bextread_chk_errors)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;

    char *missing_new_line_at_end[]= {
        "T_SHORT_FLD\t88\n",
        "T_SHORT_FLD\t-1\n",
        "T_SHORT_2_FLD\t0\n",
        "T_SHORT_2_FLD\t212\n",
        "T_LONG_FLD\t-1021\n",
        "T_LONG_FLD\t-2\n",
        "T_LONG_FLD\t0\n",
        "T_LONG_FLD\t0", /* <<< error line */
        NULL
    };

    char *no_field_name[]= {
        "T_SHORT_FLD\t-1\n",
        "\t0\n", /* <<< error line */
        "T_SHORT_2_FLD\t212\n",
        "T_LONG_FLD\t-1021\n",
        "T_LONG_FLD\t-2\n",
        "T_LONG_FLD\t0\n",
        "T_LONG_FLD\t0\n",
        NULL
    };
    
    char *no_value_seperator[]= {
        "T_SHORT_FLD\t-1\n",
        "T_SHORT_2_FLD\t212\n",
        "T_LONG_FLD\t-1021\n",
        "T_LONG_FLD -2\n",/* <<< error line */
        "T_LONG_FLD\t0\n",
        "T_LONG_FLD\t0\n",
        NULL
    };

    char *prefix_error_hex[]= {
        "T_SHORT_FLD\t-1\n",
        "T_SHORT_2_FLD\t212\n",
        "T_LONG_FLD\t-1021\n",
        "T_LONG_FLD\t0\n",
        "T_LONG_FLD\t0\n",
        "T_STRING_FLD\t\\\n", /* <<< error on this line, strange prefixing.*/
        NULL
    };

    char *invalid_hex_number[]= {
        "T_SHORT_FLD\t-1\n",
        "T_SHORT_2_FLD\t212\n",
        "T_LONG_FLD\t-1021\n",
        "T_LONG_FLD\t0\n",
        "T_LONG_FLD\t0\n",
        "T_STRING_FLD\tabc\\yu123\n", /* <<< error on this line, strange prefixing.*/
        NULL
    };

    char *empty_line_error[]= {
        "\n", /* <<< fail on empty line */
        NULL
    };

    char *invalid_field_id[]= {
        "T_SHORT_FLD\t-1\n",
        "IVALID_FIELD_NAME\t212\n", /* <<< error on this line */
        NULL
    };

    char *invalid_field_id_syntax[]= {
        "T_SHORT_FLD\t-1\n",
        "((BFLDID32)4294967295)\t212\n", /* <<< error on this line */
        NULL
    };
        
    /* load field table */
    load_field_table();

    /*--------------------------------------------------------*/
    /* test the newline is missing at the end */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(missing_new_line_at_end);
    close_test_temp();

    /* file not opened - unix error - might cause double free!?!? */
    /*M_test_temp_file=NULL;
    assert_equal(Bextread(p_ub, M_test_temp_file), FAIL);
    assert_equal(Berror, BEUNIX);
     */

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BSYNTAX);
    close_test_temp();

    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* test the newline is missing at the end */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(no_field_name);
    close_test_temp();
    
    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BSYNTAX);
    close_test_temp();

    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* test the newline is missing at the end */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(no_value_seperator);
    close_test_temp();

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BSYNTAX);
    close_test_temp();

    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* test bad prefixing */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(prefix_error_hex);
    close_test_temp();

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BSYNTAX);
    close_test_temp();

    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* invalid hex number provided */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(invalid_hex_number);
    close_test_temp();

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BSYNTAX);
    close_test_temp();

    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* Empty line also is not supported */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(empty_line_error);
    close_test_temp();

    /* syntax error should fail */
    /* to be backwards compatible we just ignore this stuff... */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXSUCCEED);
    /* assert_equal(Berror, BSYNTAX); */
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* Field id not found */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(invalid_field_id);
    close_test_temp();

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BBADNAME);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    /*--------------------------------------------------------*/
    /* Invalid bfldid syntax */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(invalid_field_id_syntax);
    close_test_temp();

    /* syntax error should fail */
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BBADFLD);
    close_test_temp();
    /* now open the file */
    remove_test_temp();

    
}

/**
 * Testing extread for errors
 */
Ensure(test_bextread_comments)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    short s;
    long l;

    char *comment_test[]= {
        "T_SHORT_FLD\t88\n",
        "#T_SHORT_2_FLD\t212\n",
        "#T_STRING_FLD\t-1021\n",
        "#T_STRING_FLD\t-2\n",
        "T_LONG_FLD\t-1\n",
        NULL
    };
    
    /* load field table */
    load_field_table();

    /*--------------------------------------------------------*/
    /* Testing comment. */
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(comment_test);
    close_test_temp();
    
    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXSUCCEED);

    /* Ensure that fields are missing */
    assert_equal(Bpres(p_ub, T_SHORT_2_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 0), EXFALSE);
    assert_equal(Bpres(p_ub, T_STRING_FLD, 1), EXFALSE);

    /* Ensure that we have what we expect */
    assert_equal(Bget(p_ub, T_SHORT_FLD, 0, (char *)&s, 0), EXSUCCEED);
    assert_equal(s, 88);
    assert_equal(Bget(p_ub, T_LONG_FLD, 0, (char *)&l, 0), EXSUCCEED);
    assert_equal(l, -1);
    close_test_temp();
    
    /* now open the file */
    remove_test_temp();
}

/**
 * Test - flag for buffer delete
 */
Ensure(test_bextread_minus)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;
    short s;
    long l;

    char *test_minus[]= {
        "T_SHORT_FLD\t123\n",
        "T_DOUBLE_FLD\t0.1\n",
        "T_CARRAY_FLD\tABCDE\n",
        "T_STRING_FLD\tTEST_STRING\n",
        "T_FLOAT_FLD\t1\n",
        "- T_CARRAY_FLD\tABCDE\n",
        "- T_STRING_FLD\tTEST_STRING\n",
        NULL
    };

    /* load field table */
    load_field_table();
    
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(test_minus);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();

    /* now open the file */
    remove_test_temp();

    /* Load reference data into buffer 2 */
    assert_equal(CBchg(p_ub2, T_SHORT_FLD, 0, "123", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_DOUBLE_FLD, 0, "0.1", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_FLOAT_FLD, 0, "1", 0, BFLD_STRING), EXSUCCEED);

    /* Compare buffers now should be equal */
    assert_equal(Bcmp(p_ub, p_ub2), 0);
}

/**
 * Test - flag for buffer delete
 */
Ensure(test_bextread_plus)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;
    short s;
    long l;
    
    char *test_plus[]= {
        "T_SHORT_FLD\t999\n",
        "T_DOUBLE_FLD\t888\n",
        "T_FLOAT_FLD\t777.11\n",
        "T_STRING_FLD\tABC\n",
        "+ T_SHORT_FLD\t123\n",
        "+ T_DOUBLE_FLD\t0.1\n",
        "+ T_FLOAT_FLD\t1\n",
        "+ T_STRING_FLD\tCDE\n",
        NULL
    };

    /* load field table */
    load_field_table();

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(test_plus);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
    
    /* Load reference data into buffer 2 */
    assert_equal(CBchg(p_ub2, T_SHORT_FLD, 0, "123", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_DOUBLE_FLD, 0, "0.1", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_FLOAT_FLD, 0, "1", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_STRING_FLD, 0, "CDE", 0, BFLD_STRING), EXSUCCEED);

    /* Compare buffers now should be equal */
    assert_equal(Bcmp(p_ub, p_ub2), 0);
}


/**
 * Test - Test buffer EQ op
 */
Ensure(test_bextread_eq)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    char fb2[2048];
    UBFH *p_ub2 = (UBFH *)fb2;
    short s;
    long l;

    char *test_eq[]= {
        "T_SHORT_FLD\t999\n",
        "T_LONG_FLD\t124545\n",
        "T_CHAR_FLD\ta\n",
        "T_DOUBLE_FLD\t888\n",
        "T_FLOAT_FLD\t777.11\n",
        "T_STRING_FLD\tABC\n",
        "T_CARRAY_FLD\tEFGH\n",
        /* Value will be copied to FLD2 */
        "= T_SHORT_2_FLD\tT_SHORT_FLD\n",
        "= T_LONG_2_FLD\tT_LONG_FLD\n",
        "= T_CHAR_2_FLD\tT_CHAR_FLD\n",
        "= T_DOUBLE_2_FLD\tT_DOUBLE_FLD\n",
        "= T_FLOAT_2_FLD\tT_FLOAT_FLD\n",
        "= T_STRING_2_FLD\tT_FLOAT_FLD\n",
        "= T_CARRAY_2_FLD\tT_CARRAY_FLD\n",
        NULL
    };

    /* load field table */
    load_field_table();

    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    assert_equal(Binit(p_ub2, sizeof(fb2)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(test_eq);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXSUCCEED);
    close_test_temp();
    /* now open the file */
    remove_test_temp();

    /* Load reference data into buffer 2 */
    assert_equal(CBchg(p_ub2, T_SHORT_FLD, 0, "t999", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_LONG_FLD, 0, "124545", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_CHAR_FLD, 0, "a", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_FLOAT_FLD, 0, "777.11", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_DOUBLE_FLD, 0, "888", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_STRING_FLD, 0, "ABC", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_CARRAY_FLD, 0, "EFGH", 0, BFLD_STRING), EXSUCCEED);

    assert_equal(CBchg(p_ub2, T_SHORT_2_FLD, 0, "t999", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_LONG_2_FLD, 0, "124545", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_CHAR_2_FLD, 0, "a", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_FLOAT_2_FLD, 0, "777.11", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_DOUBLE_2_FLD, 0, "888", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_STRING_2_FLD, 0, "ABC", 0, BFLD_STRING), EXSUCCEED);
    assert_equal(CBchg(p_ub2, T_CARRAY_2_FLD, 0, "EFGH", 0, BFLD_STRING), EXSUCCEED);

    /* Compare buffers now should be equal  - ignore for now...
    assert_equal(memcmp(p_ub, p_ub2, sizeof(fb)), NULL);
    */
}

/**
 * Test - Test buffer EQ failure
 */
Ensure(test_bextread_eq_err)
{
    char fb[2048];
    UBFH *p_ub = (UBFH *)fb;
    short s;
    long l;

    char *test_eq_err[]= {
        "T_SHORT_FLD\t999\n",
        "T_LONG_FLD\t124545\n",
        "T_CHAR_FLD\ta\n",
        "T_DOUBLE_FLD\t888\n",
        "T_FLOAT_FLD\t777.11\n",
        "T_STRING_FLD\tABC\n",
        "T_CARRAY_FLD\tEFGH\n",
        /* Value will be copied to FLD2 */
        "= T_SHORT_2_FLD\tT_SHORT_FLD\n",
        "= T_LONG_2_FLD\tT_LONG_FLD\n",
        "= T_CHAR_2_FLD\tT_CHAR_FLD\n",
        "= T_DOUBLE_2_FLD\tT_DOUBLE_FLD\n",
        "= T_FLOAT_2_FLD\tT_FLOAT_FLD\n",
        "= T_STRING_2_FLD\tFIELD_NOT_PRESENT\n", /* <<< error on this lines */
        "= T_CARRAY_2_FLD\tT_CARRAY_FLD\n",
        NULL
    };
    /* load field table */
    load_field_table();
    assert_equal(Binit(p_ub, sizeof(fb)), EXSUCCEED);
    open_test_temp("w");
    write_to_temp(test_eq_err);
    close_test_temp();

    open_test_temp_for_read("r");
    assert_equal(Bextread(p_ub, M_test_temp_file), EXFAIL);
    assert_equal(Berror, BBADNAME);
    close_test_temp();
    /* now open the file */
    remove_test_temp();
}

TestSuite *ubf_print_tests(void)
{
    TestSuite *suite = create_test_suite();

    
    add_test(suite, test_bfprintcb);
     
    add_test(suite, test_bprint);
    add_test(suite, test_bfprint);
    
    add_test(suite, test_bextread_bfldid);
    add_test(suite, test_bextread_fldnm);
    add_test(suite, test_bextread_chk_errors);
    add_test(suite, test_bextread_comments);
    add_test(suite, test_bextread_minus);
    add_test(suite, test_bextread_plus);
    add_test(suite, test_bextread_eq);
    add_test(suite, test_bextread_eq_err);
    


    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
