/**
 * @brief This is transactional client
 *
 * @file atmiclt71_txn.c
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
#include "test71.h"


#include <dlfcn.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------Externs------------------------------------*/
extern int __write_to_tx_file(char *buf);
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
    int ret = EXSUCCEED;
    int i;
    char buf[64];
    
    if (EXSUCCEED!=tpinit(NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: failed to init: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: failed to tpopen: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    for (i=0; i<33; i++)
    {
        if (EXSUCCEED!=tpbegin(10, 0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to begin: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        snprintf(buf, sizeof(buf), "CLT %d\n", i);
        NDRX_LOG(log_debug, "TRA: %s", buf);
        
        if (EXSUCCEED!=__write_to_tx_file(buf)) /* symbol from xa switch lib */
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to write to transaction: %s", 
                     Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=tpcommit(0))
        {
            NDRX_LOG(log_error, "TESTERROR: failed to commit: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
    }
    
out:
    /* close anyway.. */
    tpabort(0);
    tpclose();
    tpterm();

    return ret;
        
}

#ifdef __cplusplus
} // extern "C"
#endif

/* vim: set ts=4 sw=4 et smartindent: */
