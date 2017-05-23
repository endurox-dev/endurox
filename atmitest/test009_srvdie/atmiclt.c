/* 
** Basic test client
**
** @file atmiclt.c
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    long rsplen;
    int ret=SUCCEED;
    int tpcall_err;
    
    if (FAIL == tpcall("TESTSVFN", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "TESTSVFN failed: %s", tpstrerror(tperrno));
        
        /* Check the code TOUT & SVCERR is OK*/
        tpcall_err = tperrno;
        if (tpcall_err==TPENOENT)
        {
            NDRX_LOG(log_error, "OK-TPESVCERR");
        }
        else if (tpcall_err==TPETIME)
        {
            NDRX_LOG(log_error, "OK-TPETIME");
        }
        else
        {
            NDRX_LOG(log_error, "TESTERROR: UNEXPECTED ERROR");
        }
        
        goto out;
    }

out:
    /* Terminate the session */
    tpterm();

    return ret;
}

