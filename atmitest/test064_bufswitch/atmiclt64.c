/**
 * @brief Buffer type switch + null - client
 *
 * @file atmiclt64.c
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
#include "test64.h"
#include "t64.h"
#include <tpadm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Universal service cross-tester
 * @param svc service to call
 * @param input_type input buffer to allocate (use "NULL", for none)
 * @param intput_sub buffer sub-type
 * @param output_type output buffer to expect (use "NULL", for none)
 * @param output_sub output buffer sub-type, expected
 * @param flags tpcall flags
 * @param tpcallerr expected tpcall error code
 * @param validate shall we perform data validation?
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tester(char *svc, char *input_type, char *input_sub, 
        char *output_type, char *output_sub, long flags, int tpcallerr, int validate)
{
    char *data = NULL;
    long len = 0;
    int tpcallerrgot = 0;
    int ret = EXSUCCEED;
    char btype[16]="";
    char stype[16]="";
    
    NDRX_LOG(log_debug, "svc: [%s] input_type: [%s] input_sub: [%s] output_type: "
            "[%s] output_sub: [%s] flags: %ld tpcallerr: %d validate: %d",
            svc, input_type, input_sub, output_type, output_sub, flags, 
            tpcallerr, validate);
    
    if (0!=strcmp("NULL", input_type) &&
            NULL==(data = tpalloc(input_type, input_sub, 1024)))
    {
        NDRX_LOG(log_error, "Failed to alloc buffer!");
        EXFAIL_OUT(ret);
    }
    
    if (0==strcmp(input_type, "STRING"))
    {
        strcpy(data, "HELLO CLIENT");
    }
    else if (0==strcmp(input_type, "JSON"))
    {
        strcpy(data, "[]");
    }
    else if (0==strcmp(input_type, "CARRAY"))
    {
        strcpy(data, "HELLO CARRAY CLIENT");
        len = strlen(data);
    }
    else if (0==strcmp(input_type, "VIEW"))
    {
        struct MYVIEW1 *vv = (struct MYVIEW1 *)data;
        strcpy(vv->tstring3[2], "HELLO VIEW");
    }
    else if (0==strcmp(input_type, "UBF"))
    {
        if (EXSUCCEED!=Bchg((UBFH *)data, T_STRING_9_FLD, 4, "HELLO UBF CLIENT", 0L))
        {
            NDRX_LOG(log_error, "Failed to add T_STRING_9_FLD[4]: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(input_type, "NULL"))
    {
        /* nothing todo */
    }
    else
    {
        NDRX_LOG(log_error, "Unsupported buffer type [%s]", input_type);
        EXFAIL_OUT(ret);
    }
    
    /* call server process!!! */
    
    if (EXSUCCEED!=tpcall(svc, data, len, &data, &len, flags))
    {
        NDRX_LOG(log_error, "Failed to call %s: %s", svc, tpstrerror(tperrno));
        
        /* is it failure? */
        tpcallerrgot = tperrno;
    }
    
    if (tpcallerr!=tpcallerrgot)
    {
        NDRX_LOG(log_error, "TESTERROR! Expected %d error, but got: %d", 
                tpcallerr, tpcallerrgot);
        EXFAIL_OUT(ret);
    }
    
    /* validate response!!!!! */
    
    /* get buffer types.. first... */
    
    NDRX_LOG(log_debug, "Response ptr: %p", data);
    
    if (EXFAIL==tptypes(data, btype, stype))
    {
        NDRX_LOG(log_error, "TESTERROR! Failed to read buffer %p type: %s",
                data, tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(output_type, btype))
    {
        NDRX_LOG(log_error, "TESTERROR! Expected response type [%s] but got [%s]", 
                output_type, btype);
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(output_sub, stype))
    {
        NDRX_LOG(log_error, "TESTERROR! Expected response sub-type [%s] but got [%s]", 
                output_sub, stype);
        EXFAIL_OUT(ret);
    }
    
    if (!validate)
    {
        NDRX_LOG(log_debug, "No validate");
        goto out;
    }
    
    /* validate some values... */
    
    if (0==strcmp(output_type, "STRING"))
    {
        if (0!=strcmp(data, "WORLD"))
        {
            NDRX_LOG(log_error, "Test error, expected: [WORLD] got [%s]",
                    data);
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(output_type, "JSON"))
    {
        if (0!=strcmp(data, "{}"))
        {
            NDRX_LOG(log_error, "Test error, expected: [{}] got [%s]",
                    data);
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(output_type, "CARRAY"))
    {
        if (0!=strncmp(data, "SPACE", 5))
        {
            NDRX_LOG(log_error, "Test error, expected: [SPACE] got [%c%c%c%c%c]",
                    data[0],data[1],data[2],data[3],data[4]);
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(output_type, "VIEW"))
    {
        struct MYVIEW2 *v = (struct MYVIEW2 *)data;
        
        if (0!=strcmp(v->tstring1, "TEST 55"))
        {
            NDRX_LOG(log_error, "Test error, expected: [TEST 55] got [%s]",
                    v->tstring1);
            EXFAIL_OUT(ret);
        }
        
    }
    else if (0==strcmp(output_type, "UBF"))
    {
        char tmp[128];
        
        if (EXSUCCEED!=Bget((UBFH *)data, T_STRING_FLD, 1, tmp, 0L))
        {
            NDRX_LOG(log_error, "TESTERROR ! Failed to get data!");
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp(tmp, "HELLO WORLD"))
        {
            NDRX_LOG(log_error, "TESTERROR, expected: [HELLO WORLD] got [%s]",
                    tmp);
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strcmp(output_type, "NULL"))
    {
        
    }
    else
    {
        NDRX_LOG(log_error, "Unsupported buffer type [%s]", input_type);
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (data!=NULL)
    {
        tpfree(data);
    }

    return ret;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int i;
    int ret=EXSUCCEED;
    char *bufs[] = {"NULL", "JSON", "STRING", "CARRAY", "VIEW", "UBF", ""};
    char **p;
    char subtyp[16];
    char osubtyp[16];
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    ndrx_growlist_t list;
    if (argc < 2)
    {
        NDRX_LOG(log_error, "Usage: %s NULL|JSON|STRING|"
                "CARRAY|VIEW|UBF", argv[0]);
        EXFAIL_OUT(ret);
    }

    /* test case by case 
     * crossvalidate all types
     */
    snprintf(svcnm, sizeof(svcnm), "%sRSP", argv[1]);
    
    if (0==strcmp(argv[1], "VIEW"))
    {
        NDRX_STRCPY_SAFE(osubtyp, "MYVIEW2");
    }
    else
    {
        osubtyp[0]=EXEOS;
    }
    
    for (i=0; i<1000; i++)
    {
        p = bufs;
        
        while (*p[0]!=EXEOS)
        {
            int ret_err = 0;
            if (0==strcmp(*p, "VIEW"))
            {
                NDRX_STRCPY_SAFE(subtyp, "MYVIEW1");
            }
            else
            {
                NDRX_STRCPY_SAFE(subtyp, "");
            }
            
            /* call with buffer switch - normal call */
            if (EXSUCCEED!=tester(svcnm, *p, subtyp, argv[1], osubtyp, 0L, 0, EXTRUE))
            {
                NDRX_LOG(log_error, "1 fail");
                EXFAIL_OUT(ret);
            }
            
            /* deny buffer type switch */
            if (0!=strcmp(argv[1], *p) ||
                    /* for views */
                    0==strcmp(*p, "VIEW")
                    )
            {
                ret_err = TPEOTYPE;
            }
            
            if (EXSUCCEED!=tester(svcnm, *p, subtyp, *p, subtyp, TPNOCHANGE, 
                    ret_err, EXFALSE))
            {
                NDRX_LOG(log_error, "2 fail");
                EXFAIL_OUT(ret);
            }
            
            p++;
        }
    }
    
out:
    
    /* Count the buffers left in the system (allocated)
     * they must be 0.
     */
    
    if (EXSUCCEED==ndrx_buffer_list(&list))
    {
        NDRX_LOG(log_debug, "buffers left: %d", list.maxindexused);
        
        if (list.maxindexused > -1)
        {
            NDRX_LOG(log_error, "TESTERROR! Buffers in system: %d", 
                    list.maxindexused+1);
            ret=EXFAIL;
        }
    }
    else
    {
        ret=EXFAIL;
    }
    
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

/* vim: set ts=4 sw=4 et smartindent: */
