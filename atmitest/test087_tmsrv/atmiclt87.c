/**
 * @brief TMSRV State Driving verification - client
 *
 * @file atmiclt87.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    int ret = 0;
    char *odata=NULL;
    long olen;
    
    if (0!=tpopen())
    {
        fprintf(stderr, "Failed to topopen: %s\n", tpstrerror(tperrno));
        ret=-1;
        goto out;
    }
    
    if (0!=tpbegin(60, 0))
    {
        fprintf(stderr, "Failed to tpbegin: %s\n", tpstrerror(tperrno));
        ret=-1;
        goto out;
    }
    
    if (0!=tpcall("TESTSV1", NULL, 0, &odata, &olen, 0))
    {
        fprintf(stderr, "Failed to tpcall: %s\n", tpstrerror(tperrno));
    }
    
    if (argc>1 && argv[1][0]=='A')
    {
        if (0!=tpabort(0))
        {
            fprintf(stdout, "TPABORT: %s\n", tpstrerror(tperrno));
            ret=-1;
            goto out;
        }
        else
        {
            fprintf(stdout, "TPABORT OK\n");
        }
    }
    else
    {
        if (0!=tpcommit(0))
        {
            fprintf(stderr, "TPCOMMIT: %s\n", tpstrerror(tperrno));
            ret=-1;
            goto out;
        }
        else
        {
            fprintf(stdout, "TPCOMMIT OK\n");
        }
    }
    
out:
    tpterm();
    tpclose();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
