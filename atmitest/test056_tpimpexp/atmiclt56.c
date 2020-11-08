/**
 * @brief tpimport()/tpexport() function tests - client
 *
 * @file atmiclt56.c
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
#include <ubfutil.h>
#include "test56.h"
#include "t56.h"
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

    int ret = EXSUCCEED;
    
    if (EXSUCCEED!=test_impexp_testemb())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to test embedded buffers!!!");
        EXFAIL_OUT(ret);
    }
    
    if ( EXSUCCEED!=test_impexp_string() )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import/export STRING!!!!");
        EXFAIL_OUT(ret);
    }
    
    if ( EXSUCCEED!=test_impexp_ubf() )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import/export UBF!!!!");
        EXFAIL_OUT(ret);
    }

    if ( EXSUCCEED!=test_impexp_view() )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import/export VIEW!!!!");
        EXFAIL_OUT(ret);
    }

    if ( EXSUCCEED!=test_impexp_carray() )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import/export CARRAY!!!!");
        EXFAIL_OUT(ret);
    }
    if ( EXSUCCEED!=test_impexp_json() )
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to import/export JSON !!!!");
        EXFAIL_OUT(ret);
    }

out:

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
