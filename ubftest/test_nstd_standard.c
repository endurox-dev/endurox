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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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


/*
 * Ensure(test_nstd_ndrx_strcpy_s)
 * - NDRX_STRCPY_SAFE
 * - NDRX_STRCPY_SAFE_DST
 * - NDRX_STRNCPY
 * - NDRX_STRNCPY_EOS
 * - NDRX_STRNCPY_SRC
 * - NDRX_STRCPY_LAST_SAFE
 */

/**
 * Copy to test dest string with using target size as sizeof(DST)
 */
Ensure(test_nstd_NDRX_STRCPY_SAFE)
{
    char dst[6];
    
    /* check normal copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_SAFE(dst, "ABCD");
    assert_string_equal(dst, "ABCD");
    
    /* check full copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_SAFE(dst, "ABCDE");
    assert_string_equal(dst, "ABCDE");
    
    /* check truncate copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_SAFE(dst, "HELLO WORLD");
    assert_string_equal(dst, "HELLO");
    
}

/**
 * The dest buffer space is given then parameter..
 * used for pointer buffers
 */
Ensure(test_nstd_NDRX_STRCPY_SAFE_DST)
{
    char dst[6];
    char *p = dst;
    
    /* check normal copy */
    memset(p, 1, sizeof(dst));
    NDRX_STRCPY_SAFE_DST(p, "ABCD", 6);
    assert_string_equal(p, "ABCD");
    assert_string_equal(dst, "ABCD");
    
    /* check full copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_SAFE_DST(dst, "ABCDE", 6);
    assert_string_equal(p, "ABCDE");
    assert_string_equal(dst, "ABCDE");
    
    /* check truncate copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_SAFE_DST(dst, "HELLO WORLD", 6);
    assert_string_equal(p, "HELLO");
    assert_string_equal(dst, "HELLO");
}

/**
 * This is same as strncpy, but instead it does not fill the non-copied
 * left overs with zeros..
 * Generally this is not safe, but faster version.
 */
Ensure(test_nstd_NDRX_STRNCPY)
{
    char dst[7];
    
    /* zero is copied from src.. */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY(dst, "ABCD", 6);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 'D');
    assert_equal(dst[4], 0);
    assert_equal(dst[5], 1);
    assert_equal(dst[6], 1);
    
    /* full copied, no zero space */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY(dst, "ABCDEF", 6);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 'D');
    assert_equal(dst[4], 'E');
    assert_equal(dst[5], 'F');
    assert_equal(dst[6], 1);
    
    /* check truncate copy */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY(dst, "ABCDEF", 3);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 1);
    assert_equal(dst[4], 1);
    assert_equal(dst[5], 1);
    assert_equal(dst[6], 1);
}

/**
 * This is same as NDRX_STRNCPY, but ensure that string is terminated.
 * Thus we give dest buffer size + how much to copy.
 */
Ensure(test_nstd_NDRX_STRNCPY_EOS)
{
    char dst[7];
    
    /* zero is copied from src.. */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY_EOS(dst, "ABCD", 4, 6);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 'D');
    assert_equal(dst[4], 0);
    assert_equal(dst[5], 1);
    assert_equal(dst[6], 1);
    
    /* try copy som more */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY_EOS(dst, "ABCDE", 6, 6);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 'D');
    assert_equal(dst[4], 'E');
    assert_equal(dst[5], 0);
    assert_equal(dst[6], 1);
    
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY_EOS(dst, "ABCDEG", 6, 4);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 0);
    assert_equal(dst[4], 1);
    assert_equal(dst[5], 1);
    assert_equal(dst[6], 1);
    
    /* just full */
    memset(dst, 1, sizeof(dst));
    NDRX_STRNCPY_EOS(dst, "ABCDEG", 3, 4);
    assert_equal(dst[0], 'A');
    assert_equal(dst[1], 'B');
    assert_equal(dst[2], 'C');
    assert_equal(dst[3], 0);
    assert_equal(dst[4], 1);
    assert_equal(dst[5], 1);
    assert_equal(dst[6], 1);
}

/**
 * Do not check the dest buffer, but check source indead.
 * This will ensure that strlen does not go into unknown memories
 */
Ensure(test_nstd_NDRX_STRNCPY_SRC)
{
    char src[6]={1, 2, 3, 4, 5, 6};
    char dst[6]={6, 7, 8, 9, 10, 11};
    char result[6]={1, 2, 3, 9, 10, 11};
    
    NDRX_STRNCPY_SRC(dst, src, 3);
    assert_equal(memcmp(dst, result, 6), 0);
}

/**
 * Copy last bytes from dest to source.
 * This is SAFE, thus target buffer must be static sized.
 * EOS is placed in result string.
 */
Ensure(test_nstd_NDRX_STRCPY_LAST_SAFE)
{
    char dst[7];
    
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_LAST_SAFE(dst, "ABCFFFFFABC", 3);
    assert_string_equal(dst, "ABC");
    
    /* last 10, trunc to 6 for EOS */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_LAST_SAFE(dst, "ABCFFFFFABC", 10);
    assert_string_equal(dst, "BCFFFF");
    
    /* full size */
    memset(dst, 1, sizeof(dst));
    NDRX_STRCPY_LAST_SAFE(dst, "ABCDEF", 6);
    assert_string_equal(dst, "ABCDEF");
}


exprivate void chk_token(char *str, char **tokens, int num_tokens)
{
    char *tok;
    int i=0;
    
    for (tok = ndrx_strtokblk ( str, " \t", "'\""), i=0; NULL!=tok; tok = ndrx_strtokblk (NULL, " \t,\n", "'\""), i++)
    {
        assert_string_equal(tok, tokens[i]);
    }
    assert_equal(i, num_tokens);
}

/**
 * Check the ndrx_strtokblk engine
 */
Ensure(test_nstd_strtokblk)
{
    do {
    char test1[]="HELLO WORLD";
    chk_token(test1, (char*[]){ "HELLO", "WORLD"}, 2);
    }while(0);
    
    do {
        char test1[]="HELLO' WORLD '1 OK";
        chk_token(test1, (char*[]){ "HELLO WORLD 1", "OK"}, 2);
    }while(0);
    
    
    do {
        char test1[]="-e\t\"HELLO";
        chk_token(test1, (char*[]){ "-e", "HELLO"}, 2);
    }while(0);
    
    do {
        char test1[]="\"THIS IS SIGNER\\\\\\\"QUOTE 'RIGHT?'\"";
        chk_token(test1, (char*[]){"THIS IS SIGNER\\\"QUOTE 'RIGHT?'"}, 1);
    }while(0);
    
    do {
        char test1[]="-e\\' -z\\\"";
        chk_token(test1, (char*[]){"-e'", "-z\""}, 2);
    }while(0);
    
    do {
        char test1[]="test string '\t inside double\"OK \"?'";
        chk_token(test1, (char*[]){"test", "string", "\t inside double\"OK \"?"}, 3);
    }while(0);
    
    do {
        char test1[]="\\X \"\\'\" ";
        chk_token(test1, (char*[]){"\\X", "\\'"}, 2);
    }while(0);
    
    do {
        char test1[]="\"\\'";
        chk_token(test1, (char*[]){"\\'"}, 1);
    }while(0);
    
    
    do {
        char test1[]="'\\\"'";
        chk_token(test1, (char*[]){"\\\""}, 1);
    }while(0);
    
    
    do {
        char test1[]="arg1  arg2";
        chk_token(test1, (char*[]){"arg1", "", "arg2"}, 3);
    }while(0);
    
    do {
        char test1[]="some \\\\ arg";
        chk_token(test1, (char*[]){"some", "\\", "arg"}, 3);
    }while(0);
    
    /* nothing to escape... */
    do {
        char test1[]="some \\ arg";
        chk_token(test1, (char*[]){"some", "", "arg"}, 3);
    }while(0);
    
    
    do {
        char test1[]="some '\\\\' arg";
        chk_token(test1, (char*[]){"some", "\\", "arg"}, 3);
    }while(0);
    
    do {
        char test1[]="some,arg,1";
        chk_token(test1, (char*[]){"some", "\\", "arg"}, 3);
    }while(0);
    
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
    add_test(suite, test_nstd_NDRX_STRCPY_SAFE);
    add_test(suite, test_nstd_NDRX_STRCPY_SAFE_DST);
    add_test(suite, test_nstd_NDRX_STRNCPY);
    add_test(suite, test_nstd_NDRX_STRNCPY_EOS);
    add_test(suite, test_nstd_NDRX_STRNCPY_SRC);
    add_test(suite, test_nstd_NDRX_STRCPY_LAST_SAFE);
    add_test(suite, test_nstd_strtokblk);
    
    return suite;
}
/* vim: set ts=4 sw=4 et smartindent: */
