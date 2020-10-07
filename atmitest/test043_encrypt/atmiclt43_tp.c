/**
 * @brief Test tpencrypt()/tpdecrypt() functionality
 *
 * @file atmiclt43_tp.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test43.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Pass on CLI encrypted string value
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    char buf1[1024];
    char buf2[1064];
    char buf3[1024];
    long len1;
    long len2;
    long len3;
#define EXPECTED_VAL    "hello Test 2"
    
    if (argc<2)
    {
        fprintf(stderr, "usage: %s <encrypted_data_block>\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    /* check that we can decrypt */
    
    /* no space for EOS! */
    len1 = strlen(EXPECTED_VAL);
    if (EXSUCCEED==tpdecrypt(argv[1], 0, buf1, &len1, TPEX_STRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected error due to size issues!");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "error: %d: %s", tperrno, tpstrerror(tperrno));
    
    if (tperrno!=TPELIMIT)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPELIMIT got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (len1!= strlen(EXPECTED_VAL)+1)
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid clr value estimate: %d vs %d!", 
                len1, strlen(EXPECTED_VAL)+1);
        EXFAIL_OUT(ret);
    }
    
    /* got space OK */
    len1 = strlen(EXPECTED_VAL)+1;
    if (EXSUCCEED!=tpdecrypt(argv[1], 0, buf1, &len1, TPEX_STRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected OK, got: %d: %s", 
                tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* check the value */
    NDRX_LOG(log_debug, "[%s] vs [%s]", EXPECTED_VAL, buf1);
    
    if (0!=strcmp(EXPECTED_VAL, buf1))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid decrypted value: [%s] vs [%s]", 
                EXPECTED_VAL, buf1);
        EXFAIL_OUT(ret);
    }
    
    /* now encrypt the same data.. */
    len1=sizeof(buf1);
    
    if (EXSUCCEED!=tpencrypt(EXPECTED_VAL, 0, buf1, &len1, TPEX_STRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected OK, got: %d: %s", 
                tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(argv[1], buf1))
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid encrypted value: [%s] vs [%s]", 
                argv[1], buf1);
        EXFAIL_OUT(ret);
    }
    
    if (strlen(argv[1])+1!=len1)
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid output len of string enc: %d vs %ld", 
                strlen(argv[1])+1, len1);
        EXFAIL_OUT(ret);
    }
    
    /* now work with raw data block... */
    memset(buf1, '9', sizeof(buf1));
    memset(buf2, '9', sizeof(buf2));
    
    len1=sizeof(buf1);
    len2=sizeof(buf2)-100;
    
    if (EXSUCCEED==tpencrypt(buf1, len1, buf2, &len2, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpencrypt(bin) Expected fail, got OK");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPELIMIT)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPELIMIT got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (len2<len1)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected output data size be >= to orgdata (%ld vs %dl)!",
                len2, len1);
        EXFAIL_OUT(ret);
    }
    
    /*len2=sizeof(buf2); -> use estimated space... */
    if (EXSUCCEED!=tpencrypt(buf1, len1, buf2, &len2, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected OK tpencrypt(bin), got: %d: %s", 
                tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* now compare output data, must not match... */
    if (0==memcmp(buf1, buf2, sizeof(buf1)))
    {
        NDRX_LOG(log_error, "TESTERROR: Encrypted data blocks does not match org!");
        EXFAIL_OUT(ret);
    }
    
    /* len2 shall be bit smaller than org size, but equal or larger the initial data */
    
    if (len2>=sizeof(buf2) || len2<len1)
    {
        NDRX_LOG(log_error, "TESTERROR: Invalid sizes after encrypt: %ld, %z %ld",
                len2, sizeof(buf2), len1);
        EXFAIL_OUT(ret);
    }
    /* decrypt data block */
    len3=sizeof(buf3);
    if (EXSUCCEED!=tpdecrypt(buf2, len2, buf3, &len3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt(bin) Expected OK, got: %d: %s", 
                tperrno, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* now compare output data, must not match... */
    if (0!=memcmp(buf1, buf3, sizeof(buf3)))
    {
        NDRX_LOG(log_error, "TESTERROR: Encrypted data blocks does match org!");
        EXFAIL_OUT(ret);
    }
    
    /* dec size shall be original */
    if (len3!=len1)
    {
        NDRX_LOG(log_error, "TESTERROR: Decrypted size does not match org [%ld] vs [%ld]",
                len3, len1);
        EXFAIL_OUT(ret);
    }
    
    /* check invalid arguments */
    
    NDRX_LOG(log_info, "Checking tpdecrypt invalid values");
    len3=sizeof(buf3);
    if (EXSUCCEED==tpdecrypt(NULL, len2, buf3, &len3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    len3=sizeof(buf3);
    if (EXSUCCEED==tpdecrypt(buf2, 0, buf3, &len3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    len3=sizeof(buf3);
    if (EXSUCCEED==tpdecrypt(buf2, len2, NULL, &len3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED==tpdecrypt(buf2, len2, buf3, NULL, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    len3=0;
    if (EXSUCCEED==tpdecrypt(buf2, len2, buf3, &len3, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpdecrypt() expected TPELIMIT");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPELIMIT)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPELIMIT got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Checking tpencrypt invalid values");
    
    
    /* now work with raw data block... */
    memset(buf1, '9', sizeof(buf1));
    memset(buf2, '9', sizeof(buf2));
    
    len1=sizeof(buf1);
    len2=sizeof(buf2);
    
    if (EXSUCCEED==tpencrypt(NULL, len1, buf2, &len2, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpencrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED==tpencrypt(buf1, 0, buf2, &len2, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpencrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    len1=sizeof(buf1);
    if (EXSUCCEED==tpencrypt(buf1, len1, NULL, &len2, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpencrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    
    len1=sizeof(buf1);
    if (EXSUCCEED==tpencrypt(buf1, len1, buf2, NULL, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: tpencrypt() expected inval");
        EXFAIL_OUT(ret);
    }
    
    if (tperrno!=TPEINVAL)
    {
        NDRX_LOG(log_error, "TESTERROR: Expected TPEINVAL got %d!", tperrno);
        EXFAIL_OUT(ret);
    }
    
    
out:
    
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
