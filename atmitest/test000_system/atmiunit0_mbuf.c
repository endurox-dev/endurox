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
#include <nstdutil.h>
#include <typed_buf.h>

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
    int i;
    
    for (i=0; i<40; i++)
    {
        ndrx_growlist_t list;
        UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
        UBFH *p_ub2 = (UBFH *)tpalloc("UBF", NULL, 1024);
        UBFH *p_ub3 = (UBFH *)tpalloc("UBF", NULL, 1024);
        UBFH *p_ub4 = (UBFH *)tpalloc("UBF", NULL, 1024);

        UBFH *p_ub5 = NULL;

        /* RCV ptrs: */
        UBFH *p_ub5_2 = NULL;
        UBFH *p_ub5_3 = NULL;
        UBFH *p_ub5_4 = NULL;

        UBFH *p_callinfo = (UBFH *)tpalloc("UBF", NULL, 1024);
        UBFH *p_callinfo5 = NULL;
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
        assert_equal(Bchg(p_ub3, T_STRING_5_FLD, 0, "This is string 5 ub3", 0L), EXSUCCEED);

        assert_equal(Bchg(p_ub4, T_STRING_2_FLD, 0, "HELLO 4 PTR", 0L), EXSUCCEED);
        assert_equal(CBchg(p_ub4, T_SHORT_FLD, 0, "999", 0L, BFLD_STRING), EXSUCCEED);

        /* set PTR */
        assert_equal(Bchg(p_ub3, T_PTR_2_FLD, 0, (char *)&p_ub4, 0L), EXSUCCEED);
        assert_equal(Bchg(p_ub3, T_PTR_3_FLD, 0, (char *)&p_ub4, 0L), EXSUCCEED);
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

        /* print the source buf 
        fprintf(stdout, "Source buf:\n");
        Bprint(p_ub);*/

        assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 0), EXSUCCEED);

        /* OK dump the output */
        ndrx_mbuf_tlv_debug(buf, buf_len);

        olen=0;
        assert_equal(ndrx_mbuf_prepare_incoming (buf, buf_len, (char **)&p_ub5, &olen, 0, 0), EXSUCCEED);

        /* print the buffer, shall be the same as first one... 
        fprintf(stdout, "Parsed buf:\n");
        Bprint(p_ub5);
        */
        
        /* Check the occurrences */
        assert_equal(Bget(p_ub5, T_PTR_FLD, 0, (char *)&p_ub5_2, 0L), EXSUCCEED);
        assert_equal(p_ub5_2, NULL);

        assert_equal(Bget(p_ub5, T_PTR_FLD, 1, (char *)&p_ub5_2, 0L), EXSUCCEED);
        assert_equal(p_ub5_2, NULL);

        assert_equal(Bget(p_ub5, T_PTR_FLD, 2, (char *)&p_ub5_2, 0L), EXSUCCEED);
        assert_equal(p_ub5_2, NULL);

        assert_equal(Bget(p_ub5, T_PTR_FLD, 3, (char *)&p_ub5_2, 0L), EXSUCCEED);
        assert_not_equal(p_ub5_2, NULL);


        /* ptr T_PTR_FLD 3 pos again in buf 2 */

        assert_equal(Bget(p_ub5_2, T_PTR_FLD, 0, (char *)&p_ub5_3, 0L), EXSUCCEED);
        assert_equal(p_ub5_3, NULL);

        assert_equal(Bget(p_ub5_2, T_PTR_FLD, 1, (char *)&p_ub5_3, 0L), EXSUCCEED);
        assert_equal(p_ub5_3, NULL);

        assert_equal(Bget(p_ub5_2, T_PTR_FLD, 2, (char *)&p_ub5_3, 0L), EXSUCCEED);
        assert_equal(p_ub5_3, NULL);

        assert_equal(Bget(p_ub5_2, T_PTR_FLD, 4, (char *)&p_ub5_3, 0L), EXFAIL);
        assert_equal(p_ub5_3, NULL);

        assert_equal(Bget(p_ub5_2, T_PTR_FLD, 3, (char *)&p_ub5_3, 0L), EXSUCCEED);
        assert_not_equal(p_ub5_3, NULL);

        /* ptr T_PTR_FLD 4 pos again in buf 3 */
        assert_equal(Bget(p_ub5_3, T_PTR_2_FLD, 0, (char *)&p_ub5_4, 0L), EXSUCCEED);
        assert_not_equal(p_ub5_4, NULL);

        /* check the contents of ptr5 buf */
        assert_equal(Bcmp(p_ub5_4, p_ub4), 0);

        /* clean up 3 & compare */

        Bprint(p_ub5_3);

        assert_equal(Bdel(p_ub3, T_PTR_2_FLD, 0), EXSUCCEED);
        Bprint(p_ub5_3);
        assert_equal(Bdel(p_ub5_3, T_PTR_2_FLD, 0), EXSUCCEED);

        assert_equal(Bdel(p_ub3, T_PTR_3_FLD, 0), EXSUCCEED);
        assert_equal(Bdel(p_ub5_3, T_PTR_3_FLD, 0), EXSUCCEED);

        assert_equal(Bcmp(p_ub5_3, p_ub3), 0);

        /* cleanup level 2 & compare */

        assert_equal(Bdel(p_ub2, T_PTR_FLD, 3), EXSUCCEED);
        assert_equal(Bdel(p_ub5_2, T_PTR_FLD, 3), EXSUCCEED);
        assert_equal(Bcmp(p_ub5_2, p_ub2), 0);


        /* cleanup level 1 & compare */

        assert_equal(Bdel(p_ub, T_PTR_FLD, 3), EXSUCCEED);
        assert_equal(Bdel(p_ub5, T_PTR_FLD, 3), EXSUCCEED);
        assert_equal(Bcmp(p_ub5, p_ub), 0);
        
        /* Test call infos for buffer 1 */
        assert_equal(tpgetcallinfo((char *)p_ub5, (UBFH **)&p_callinfo5, 0), EXSUCCEED);
        assert_equal(Bcmp(p_callinfo, p_callinfo5), 0);

        /* free up buffers... */
        tpfree((char *)p_ub);
        tpfree((char *)p_ub2);
        tpfree((char *)p_ub3);
        tpfree((char *)p_ub4);
        tpfree((char *)p_ub5);
        tpfree((char *)p_ub5_2);
        tpfree((char *)p_ub5_3);
        tpfree((char *)p_ub5_4);
        tpfree((char *)p_callinfo);
        tpfree((char *)p_callinfo5);
        
        /* check that all buffers are free */
        assert_equal(ndrx_buffer_list(&list), EXSUCCEED);
        assert_equal(list.maxindexused, -1);
    }
    
}

Ensure(test_mbuf_nocallinfo)
{
    UBFH *p_ub = NULL;
    UBFH *p_ci = NULL;
    UBFH *p_ub_rcv = NULL;
    char buf[8096];
    long buf_len;
    long olen;
    ndrx_growlist_t list;
    
    p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    assert_not_equal(p_ub, NULL);
    assert_equal(Bchg(p_ub, T_STRING_10_FLD, 0, "HELLO MAIN", 0L), EXSUCCEED);
    
    p_ci = (UBFH *)tpalloc("UBF", NULL, 1024);
    assert_not_equal(p_ci, NULL);
    assert_equal(Bchg(p_ci, T_STRING_10_FLD, 0, "HELLO HEADER", 0L), EXSUCCEED);
    
    assert_equal(tpsetcallinfo((char *)p_ub, p_ci, 0), EXSUCCEED);
    
    buf_len=sizeof(buf);
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 
            NDRX_MBUF_FLAG_NOCALLINFO), EXSUCCEED);

    /* OK dump the output */
    ndrx_mbuf_tlv_debug(buf, buf_len);

    olen=0;
    assert_equal(ndrx_mbuf_prepare_incoming (buf, buf_len, (char **)&p_ub_rcv, &olen, 0, 0), EXSUCCEED);
    
    /* There shall call infos header associated */
    tpfree((char *)p_ci);
    p_ci=NULL;
    assert_equal(tpgetcallinfo((char *)p_ub_rcv, (UBFH **)&p_ci, 0), EXFAIL);
    assert_equal(tperrno, TPESYSTEM);
    
    tpfree((char *)p_ub_rcv);
    tpfree((char *)p_ub);
    
    /* validate that no buffers left */
    assert_equal(ndrx_buffer_list(&list), EXSUCCEED);
    assert_equal(list.maxindexused, -1);
    
}

/**
 * Test that output buffer is filled gradually
 */
Ensure(test_mbuf_buf_full_ubf)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = NULL;
    UBFH *p_ub2 = NULL;
    char buf[4096];
    char blob[5000];
    long buf_len;
    int i;
    int fail1, fail2;
    
    for (i=0; i<10000; i++)
    {
        p_ub = (UBFH *)tpalloc("UBF", NULL, 10000);
        
        if (NULL==p_ub)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to alloc: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        p_ub2 = (UBFH *)tpalloc("UBF", NULL, 10000);
        
        if (NULL==p_ub2)
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to alloc 2: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bchg(p_ub, T_CARRAY_2_FLD, 0, blob, i+1))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to add blob: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bchg(p_ub2, T_CARRAY_2_FLD, 0, blob, i+1))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to add blob: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* set buffer 1 ptr */
        if (EXSUCCEED!=Bchg(p_ub, T_PTR_FLD, 0, (char *)&p_ub2, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to add blob PTR: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        fail1=EXFALSE;
        
        /* convert out... 2x fails first ... */
        buf_len=sizeof(buf);
        ret=ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 0);
        
        if (EXFAIL==ret)
        {
            /* had some OK loops */
            if (tperrno==TPEINVAL && i> 1000)
            {
                ret=EXSUCCEED;
                fail1=EXTRUE;
            }
            else
            {
                NDRX_LOG(log_error, "Failed to prep outgoing loop %d: %s",
                        i, tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }
        
        /* check 2nd buffer, shall not fail if first fails (as 2x less usage) */
        fail2=EXFALSE;
        buf_len=sizeof(buf);
        ret=ndrx_mbuf_prepare_outgoing ((char *)p_ub2, 0, buf, &buf_len, 0, 0);
        
        if (EXFAIL==ret)
        {
            /* had some OK loops */
            if (tperrno==TPEINVAL && i> 1000)
            {
                ret=EXSUCCEED;
                fail2=EXTRUE;
            }
            else
            {
                NDRX_LOG(log_error, "Failed to prep outgoing loop %d: %s",
                        i, tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
        }
        
        if (!fail1 && fail2)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected fail1 && !fail2");
            EXFAIL_OUT(ret);
        }
        
        if (fail1 && fail2)
        {
            break;
        }
    
        tpfree((char *)p_ub);
        tpfree((char *)p_ub2);
        p_ub = NULL;
        p_ub2 = NULL;
    }
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=p_ub2)
    {
        tpfree((char *)p_ub2);
    }

    assert_equal(ret, EXSUCCEED);
    assert_equal(fail1, EXTRUE);
    assert_equal(fail2, EXTRUE);
    
}


/**
 * Test that output buffer is filled gradually
 */
Ensure(test_mbuf_buf_full_any)
{
    int ret=EXSUCCEED;
    char buf[4096];
    long buf_len;
    int j, i, k;
    char *p_buf=NULL;
    char *btypes[] = {"STRING", "CARRAY", "JSON"};
    int btypes_ok[3];
    
    memset(btypes_ok, 0, sizeof(btypes_ok));
    
    for (i=0; i<N_DIM(btypes); i++)
    {
        for (j=0; j<10000; j++)
        {
            p_buf = tpalloc(btypes[i], NULL, 10000);

            if (NULL==p_buf)
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to alloc %s: %s", 
                        btypes[i], tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
            
            memset(p_buf, 0, sizeof(j));
            
            for (k=0; k<j; k++)
            {
                p_buf[k]=' '; /* fill space, OK for all buffers we test */
            }

            buf_len=sizeof(buf);
            ret=ndrx_mbuf_prepare_outgoing (p_buf, j, buf, &buf_len, 0, 0);

            if (EXFAIL==ret)
            {
                /* had some OK loops */
                if (tperrno==TPEINVAL)
                {
                    ret=EXSUCCEED;
                    btypes_ok[i]=EXTRUE;
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to prep outgoing loop %d: %s",
                            j, tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }
            }
            
            tpfree(p_buf);
            p_buf=NULL;

        }
    }
    
out:
    
    if (NULL!=p_buf)
    {
        tpfree(p_buf);
    }
    assert_equal(ret, EXSUCCEED);
    assert_equal(btypes_ok[0], EXTRUE);
    assert_equal(btypes_ok[1], EXTRUE);
    assert_equal(btypes_ok[2], EXTRUE);
    
}

/**
 * Check that we use ilen for primary buffers
 */
Ensure(test_mbuf_buf_carray_ilen)
{
    int ret=EXSUCCEED;
    char buf[4096];
    long buf_len;
    char *p_buf=NULL;
    
    p_buf = tpalloc("CARRAY", NULL, 50000);
    
    if (NULL==p_buf)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to alloc CARRAY: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    buf_len=sizeof(buf);
    ret=ndrx_mbuf_prepare_outgoing (p_buf, 1000, buf, &buf_len, 0, 0);
    
    assert_equal(ret, EXSUCCEED);
    /* with TLV header we shall be there */
    assert_equal(!! (buf_len < 1100), EXTRUE);
    
out:

    if (NULL!=p_buf)
    {
        tpfree(p_buf);
    }
}

/**
 * Check the ptr re-use works
 * i.e. same pointer is not serialize twice
 */
Ensure(test_mbuf_reuse)
{
    char buf[8096];
    long buf_len;
    char *carray = tpalloc("CARRAY", NULL, 4096);
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 8186);
    ndrx_growlist_t list;
    
    assert_not_equal(carray, NULL);
    assert_not_equal(p_ub, NULL);
    
    
    assert_equal(Bchg(p_ub, T_PTR_FLD, 0, (char *)&carray, 0L), EXSUCCEED);
    assert_equal(Bchg(p_ub, T_PTR_2_FLD, 0, (char *)&carray, 0L), EXSUCCEED);
    
    buf_len = sizeof(buf);
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 0), EXSUCCEED);
    
    /* same space shall be enough */
    assert_equal(ndrx_mbuf_prepare_outgoing ((char *)p_ub, 0, buf, &buf_len, 0, 0), EXSUCCEED);
    
    /* have some 1024 for hdr, the carray 4096 stays as one */
    assert_equal(!!(buf_len < 4096+1024), EXTRUE);
    assert_equal(!!(buf_len > 4096), EXTRUE);
    
    tpfree(carray);
    tpfree((char *)p_ub);
    
    /* check that all buffers are free */
    assert_equal(ndrx_buffer_list(&list), EXSUCCEED);
    assert_equal(list.maxindexused, -1);
    
}

/**
 * Standard library tests
 * @return
 */
TestSuite *atmiunit0_mbuf(void)
{
    TestSuite *suite = create_test_suite();

    add_test(suite, test_mbuf);
    add_test(suite, test_mbuf_reuse);
    add_test(suite, test_mbuf_nocallinfo);
    add_test(suite, test_mbuf_buf_full_ubf);
    add_test(suite, test_mbuf_buf_full_any);
    add_test(suite, test_mbuf_buf_carray_ilen);
    
    return suite;
}

/* vim: set ts=4 sw=4 et smartindent: */
