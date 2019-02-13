/**
 * @brief Buffer type switch + null - client
 *
 * @file atmiclt64.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tester(char *svc, char *input_type, char *input_sub, 
        char *output_type, char *output_sub)
{
    char *data = NULL;
    long len = 0;
    int ret = EXSUCCEED;
    
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
    else
    {
        NDRX_LOG(log_error, "Unsupported buffer type [%s]", input_type);
        EXFAIL_OUT(ret);
    }
    
    /* TODO: call server process!!! */
    
    
    /* TODO: validate response!!!!! */
        
out:
    return ret;
}

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    long rsplen;
    int i;
    int ret=EXSUCCEED;
            
    if (argc < 2)
    {
        NDRX_LOG(log_error, "Usage: %s NULLREQ|NULLRSP|JSONRSP|STRINGRSP|"
                "CARRAYRSP|VIEWRSP|UBFRSP", argv[0]);
        EXFAIL_OUT(ret);
    }

    /* test case by case */
    
    if (0==strcmp(argv[1], "NULLREQ"))
    {
        /* call with NULL, respond with string */
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

