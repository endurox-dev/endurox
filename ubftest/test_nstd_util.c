/**
 * @brief Standard utility library tests
 *
 * @file test_nstd_util.c
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
#include "test.fd.h"
#include "ubfunit1.h"
#include "xatmi.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define OFSZ(s,e)   EXOFFSET(s,e), EXELEM_SIZE(s,e)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/**
 * Argument testing struct
 */
struct ndrx_args_test
{
    int int1;
    int int2;
    int int3;
    
    int bool1;
    int bool2;
    int bool3;
    int bool4;
};

/**
 * Argument testing type
 */
typedef struct ndrx_args_test ndrx_args_test_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Mapping structure for arguments testing
 */
static ndrx_args_loader_t M_args_test_map[] = 
{
    {OFSZ(ndrx_args_test_t,int1), "int1", NDRX_ARGS_INT, 0, 0, -10, 10},
    {OFSZ(ndrx_args_test_t,int2), "int2", NDRX_ARGS_INT, 0, 0, 0, INT_MAX},
    {OFSZ(ndrx_args_test_t,int3), "int3", NDRX_ARGS_INT, 0, 0, 0, 0},
    
    {OFSZ(ndrx_args_test_t,bool1), "bool1", NDRX_ARGS_BOOL, 0, 0, 0, 0},
    {OFSZ(ndrx_args_test_t,bool2), "bool2", NDRX_ARGS_BOOL, 0, 0, 0, 0},
    {OFSZ(ndrx_args_test_t,bool3), "bool3", NDRX_ARGS_BOOL, 0, 0, 0, 0},
    {OFSZ(ndrx_args_test_t,bool4), "bool4", NDRX_ARGS_BOOL, 0, 0, 0, 0},
    
    {EXFAIL}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * Check that file exists
 */
Ensure(test_nstd_file_exists)
{
    assert_equal(ndrx_file_exists("ubftab_test/test1.fd"), EXTRUE);
    assert_equal(ndrx_file_exists("ubftab_test/hello/test2.fd"), EXFALSE);
    assert_equal(ndrx_file_exists("ubftab_test"), EXTRUE);
    assert_equal(ndrx_file_exists("ubftab_test_non_exist"), EXFALSE);
}

/**
 * Enduro/X version of strsep...
 */
Ensure(test_nstd_strsep)
{
    char test_str[] = "HELLO//WORLD";
    char *p;
    
    p = test_str;
    
    assert_string_equal(ndrx_strsep(&p, "/"), "HELLO");
    
    assert_string_equal(ndrx_strsep(&p, "/"), "");
    assert_string_equal(ndrx_strsep(&p, "/"), "WORLD");
    assert_true(NULL==ndrx_strsep(&p, "/"));
    
}

/**
 * Test argument setter and getter
 */
Ensure(test_nstd_args)
{
    ndrx_args_test_t test_val;
    char val[128];
    char errbuf[256];
    
    test_val.int1 = 2;
    test_val.int2 = 3;
    test_val.int3 = 4;
    
    test_val.bool1 = EXTRUE;
    test_val.bool2 = EXFALSE;
    test_val.bool3 = EXTRUE;
    test_val.bool4 = EXFALSE;
    
    
    /* read the current values ...*/
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int1", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "2");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int2", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "3");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int3", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "4");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool1", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "Y");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool2", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "N");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool3", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "Y");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool4", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "N");
    
    /* change some values, and test the buffer... */
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "int2", "5555", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "int1", "-12344", errbuf, sizeof(errbuf)), EXFAIL);
    assert_string_not_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "int1", "12344", errbuf, sizeof(errbuf)), EXFAIL);
    assert_string_not_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "int1", "-2", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "bool1", "X", errbuf, sizeof(errbuf)), EXFAIL);
    assert_string_not_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "bool1", "n", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");\

    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "bool2", "Y", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "bool3", "N", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "bool4", "y", errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(errbuf, "");
    
    errbuf[0] = EXEOS;
    assert_equal(ndrx_args_loader_set(M_args_test_map, &test_val, 
        "not_exists", "y", errbuf, sizeof(errbuf)), EXFAIL);
    assert_string_not_equal(errbuf, "");
    
    
    /* read values again and test... */
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int1", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "-2");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int2", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "5555");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "int3", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "4");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool1", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "N");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool2", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "Y");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool3", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "N");
    
    assert_equal(ndrx_args_loader_get(M_args_test_map, &test_val, 
        "bool4", val, sizeof(val), errbuf, sizeof(errbuf)), EXSUCCEED);
    assert_string_equal(val, "Y");
    
}

/**
 * Test environment substitute routines
 * Bug #452
 */
Ensure(test_nstd_env_subs)
{
    char testbuf[32+1];
    
    /* value is changed : (value longer than env var)*/
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG_MEGA_LONG_TESTZZZ", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "DB1_JDBC/DEBUG_MEGA_LONG_TESTZZZ");
    
    /* value longer and no space */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG_MEGA_LONG_TESTZZZZ", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    
    /* value shorter than env var */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "DB1_JDBC/DEBUG");
    
    /* value equals in len.. */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "AAAAAAAAAAAAAAAA", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "DB1_JDBC/AAAAAAAAAAAAAAAA");
    
    /* value longer than var, two vars */
    NDRX_STRCPY_SAFE(testbuf, "A${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "THIS_HELLO", EXTRUE);
    setenv("NDRX_TC", "THIS_WORLD", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "ATHIS_HELLOBTHIS_WORLDZ");
    
    /* value shorter */
    NDRX_STRCPY_SAFE(testbuf, "A${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "AHELLOBWORLDZ");
    
    /* check escapes.. */
    NDRX_STRCPY_SAFE(testbuf, "A\\${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "A${NDRX_TX}BWORLDZ");
    
    NDRX_STRCPY_SAFE(testbuf, "A\\\\${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_env_subs_len(testbuf, sizeof(testbuf));
    assert_string_equal(testbuf, "A\\HELLOBWORLDZ");
    
    
}

/**
 * Just get the env.
 * @param data1
 * @param data2
 * @param data3
 * @param data4
 * @param symbol
 * @param outbuf
 * @param outbufsz
 * @return 
 */
int test_get_env(void *data1, void *data2, void *data3, void *data4,
            char *symbol, char *outbuf, long outbufsz)
{
    NDRX_STRCPY_SAFE_DST(outbuf, getenv(symbol), outbufsz);
    
    return EXSUCCEED;
}
/**
 * Test environment substitute routines, context
 * Bug #452
 */
Ensure(test_nstd_env_subs_ctx)
{
    char testbuf[32+1];
    
    /* value is changed : (value longer than env var)*/
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG_MEGA_LONG_TESTZZZ", EXTRUE);
    
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    
    assert_string_equal(testbuf, "DB1_JDBC/DEBUG_MEGA_LONG_TESTZZZ");
    
    /* value longer and no space */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG_MEGA_LONG_TESTZZZZ", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    
    /* value shorter than env var */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "DEBUG", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "DB1_JDBC/DEBUG");
    
    /* value equals in len.. */
    NDRX_STRCPY_SAFE(testbuf, "DB1_JDBC/${NDRX_TESTXXXX}");
    setenv("NDRX_TESTXXXX", "AAAAAAAAAAAAAAAA", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "DB1_JDBC/AAAAAAAAAAAAAAAA");
    
    /* value longer than var, two vars */
    NDRX_STRCPY_SAFE(testbuf, "A${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "THIS_HELLO", EXTRUE);
    setenv("NDRX_TC", "THIS_WORLD", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "ATHIS_HELLOBTHIS_WORLDZ");
    
    /* value shorter */
    NDRX_STRCPY_SAFE(testbuf, "A${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "AHELLOBWORLDZ");
    
    /* check escapes.. */
    NDRX_STRCPY_SAFE(testbuf, "A\\${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "A${NDRX_TX}BWORLDZ");
    
    NDRX_STRCPY_SAFE(testbuf, "A\\\\${NDRX_TX}B${NDRX_TC}Z");
    setenv("NDRX_TX", "HELLO", EXTRUE);
    setenv("NDRX_TC", "WORLD", EXTRUE);
    ndrx_str_subs_context(testbuf, sizeof(testbuf), '{', '}', NULL, NULL, NULL, NULL,
                    test_get_env);
    assert_string_equal(testbuf, "A\\HELLOBWORLDZ");   
}

/**
 * Check the random file name generator
 */
Ensure(test_nstd_ndrx_mkstemps)
{
    FILE *f;
    char templ[128];
    char tmp[128];
    
    NDRX_STRCPY_SAFE(templ, "XXXXXX");
    assert_equal(ndrx_mkstemps(templ, 1, 0), NULL);
    assert_equal(errno, EINVAL);
    
    assert_not_equal((f=ndrx_mkstemps(templ, 0, 0)), NULL);
    
    assert_string_not_equal(templ, "XXXXXX");
    ndrx_file_exists(templ);
    /* try write */
    assert_equal(fprintf(f, "HELLO TEST"), 10);
    fclose(f);
    assert_equal(unlink(templ), EXSUCCEED);
    
    /* try file file with some extension */
    NDRX_STRCPY_SAFE(templ, "temp_compile_XXXXXX.c");
    NDRX_LOG(log_info, "0 Got name: [%s]", templ);
    assert_not_equal((f=ndrx_mkstemps(templ, 2, 0)), NULL);
    
    NDRX_LOG(log_info, "Got name: [%s]", templ);
    
    /* file open ok */
    assert_string_not_equal(templ, "temp_compile_XXXXXX.c");
    assert_equal(strlen(templ), strlen("temp_compile_XXXXXX.c"));
    /* check the positions... that other parts are OK */
    NDRX_LOG(log_debug, "Got templ: [%s]", templ);
    NDRX_STRNCPY_EOS(tmp, templ, 13, sizeof(tmp));
    assert_string_equal(tmp, "temp_compile_");
    
    
    NDRX_LOG(log_debug, "Got templ+12: [%s]", templ+19);
    
    NDRX_STRNCPY_EOS(tmp, templ+19, 2, sizeof(tmp));
    assert_string_equal(tmp, ".c");
    fclose(f);
    assert_equal(unlink(templ), EXSUCCEED);
    
    /* check file exists error */
    assert_equal((f=ndrx_mkstemps("README", 0, NDRX_STDF_TEST)), NULL);
    assert_equal(errno, EEXIST);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *ubf_nstd_util(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_nstd_strsep);
    add_test(suite, test_nstd_args);
    add_test(suite, test_nstd_env_subs);
    add_test(suite, test_nstd_env_subs_ctx);
    add_test(suite, test_nstd_file_exists);
    add_test(suite, test_nstd_ndrx_mkstemps);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
