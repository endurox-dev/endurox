/* 
** Simple thread server caller.
**
** @file atmiclt18.c
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
#include <math.h>
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <ntimer.h>
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

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 9218);
    long rsplen;
    int i;
    int ret=SUCCEED;
    double d;
    int cd_got;
    int cd[3];
    
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 1", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 2", 0);
    Badd(p_ub, T_STRING_FLD, "THIS IS TEST FIELD 3", 0);
    
    cd[0] = tpacall("TESTSV", (char *)p_ub, 0L, 0L);
    cd[1] = tpacall("TESTSV", (char *)p_ub, 0L, 0L);
    cd[2] = tpacall("TESTSV", (char *)p_ub, 0L, 0L);
    
    /* wait for tout */
    sleep(10);
    
    for (i = 0; i<3; i++)
    {
        cd_got = cd[i];
        if (SUCCEED==(ret = tpgetrply(&cd_got, (char **)&p_ub, &rsplen, TPNOBLOCK)))
        {
            NDRX_LOG(log_error, "TESTERROR The call was ok to server "
                    "- must be tout!");
            ret=FAIL;
            goto out;
        }

        ret = SUCCEED;
        if (cd_got != cd[i])
        {
            NDRX_LOG(log_error, "TESTERROR cd %d <> go_cd %d", cd, cd_got);
            ret=FAIL;
            goto out;
        }

        if (tperrno!=TPETIME)
        {
            NDRX_LOG(log_error, "Te error should be time-out!");
            ret=FAIL;
            goto out;
        }
    }
    
    /* the the case when we get reply back, this should not be a timeout. */
    cd[0] = tpacall("ECHO", (char *)p_ub, 0L, 0L);
    sleep(1);
    if (SUCCEED!=(ret = tpgetrply(&cd_got, (char **)&p_ub, &rsplen, TPNOBLOCK | TPGETANY)))
    {
        NDRX_LOG(log_error, "TESTERROR Normal reply fails!!!");
        ret=FAIL;
        goto out;
    }
    
    sleep(10);
    if (SUCCEED!=(ret = tpgetrply(&cd_got, (char **)&p_ub, &rsplen, TPNOBLOCK | TPGETANY)))
    {
        NDRX_LOG(log_error, "TESTERROR Normal reply fails!!!");
        ret=FAIL;
        goto out;
    }
    
    if (cd_got!=cd[0])
    {
        NDRX_LOG(log_error, "TESTERROR Got invalid response! "
                "cd[0] = %d got_cd = %d", cd[0], cd_got);
        ret=FAIL;
        goto out;
    }
    
    tpfree((char *)p_ub);
    
out:
    tpterm();
    NDRX_LOG(log_error, "RESULT: %d", ret);
    return ret;
}

