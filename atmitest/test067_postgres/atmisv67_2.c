/**
 * @brief PQ Versoin of test server
 *
 * @file atmisv67_2.c
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
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>
#include <string.h>
#include <unistd.h>
#include "test67.h"
#include <libpq-fe.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Shared test function add some data to DB
 */
int shared_svc_func(TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char testbuf[1024];
    UBFH *p_ub = (UBFH *)p_svc->data;
    PGresult* res;
    PGconn *conn;
    const char *const values[] = {testbuf};

    tplogprintubf(log_debug, "Got UBF", p_ub);
    
    if (EXFAIL==CBget(p_ub, T_LONG_FLD, 0, testbuf, 0, BFLD_STRING))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_LONG_FLD: %s", 
                 Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    conn = tpgetconn();
    
    if (NULL==conn)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get connection: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    res = PQexecParams(conn, 
               "INSERT INTO EXTEST(userid) VALUES ($1)",
               1,
               NULL,
               values,
               NULL,
               NULL,
               0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        NDRX_LOG(log_error, "SELECT failed: %s", PQerrorMessage(conn));
        PQclear(res);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Standard service entry.
 * transaction shall be open by caller.
 */
void TESTSV (TPSVCINFO *p_svc)
{
    int ret;
    ret = shared_svc_func(p_svc);
    
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                p_svc->data,
                0L,
                0L);
}

/**
 * Timeout service
 */
void TOUTSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;

    sleep(15);
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Failure service
 */
void FAILSV (TPSVCINFO *p_svc)
{
    int ret;
    
    /* do some insert */
    ret = shared_svc_func(p_svc);
    
    /* fail the insert */
    tpreturn(  TPFAIL,
                0L,
                p_svc->data,
                0L,
                0L);
}

/**
 * Do initialisation
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    /*
     * This opens connection to database
     */
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to tpopen: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=tpadvertise("TESTSV", TESTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TESTSV: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("TOUTSV", TOUTSV))
    {
        NDRX_LOG(log_error, "Failed to initialise TOUTSV: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise("FAILSV", FAILSV))
    {
        NDRX_LOG(log_error, "Failed to initialise FAILSV: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    

out:
    return ret;
}

/**
 * Do de-initialisation
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
    tpclose();
}

/* vim: set ts=4 sw=4 et smartindent: */
