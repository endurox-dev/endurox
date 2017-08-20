/* 
** Typed VIEW testing
**
** @file atmiclt40.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test040.h"
#include "tperror.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/*
 * Do the test call to the server
 */
int main(int argc, char** argv) {

    long rsplen;
    int i, j;
    int ret=EXSUCCEED;
    
    struct MYVIEW1 *v1;
    struct MYVIEW3 *v3;
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to init: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==v1)
    {
        NDRX_LOG(log_error, "TESTERROR: failed to alloc VIEW buffer!");
        EXFAIL_OUT(ret);
    }

    for (j=0; j<1000; j++)
    {
        
        v1= (struct MYVIEW1 *) tpalloc("VIEW", "MYVIEW1", sizeof(struct MYVIEW1));
        
        init_MYVIEW1(v1);
        
        NDRX_DUMP(log_debug, "VIEW1 request...", v1, sizeof(struct MYVIEW1));

        if (EXSUCCEED!=tpcall("TEST40_VIEW", (char *)v1, 0, (char **)&v1, &rsplen, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to call TEST40_VIEW");
            EXFAIL_OUT(ret);
        }
        
        v3 = (struct MYVIEW3 *)v1;
        if (EXSUCCEED!=validate_MYVIEW3((v3)))
        {
            NDRX_LOG(log_error, "Failed to validate V3!");
            EXFAIL_OUT(ret);
        }
        
        tpfree((char *)v3);
    }
    
out:

    if (ret>=0)
        ret=EXSUCCEED;

    return ret;
}

