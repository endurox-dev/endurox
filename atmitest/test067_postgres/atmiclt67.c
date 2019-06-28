/**
 * @brief PostgreSQL PQ TMSRV driver tests / branch transactions - client
 *  Perform local calls with help of PQ commands.
 *  the server process shall run the code with Embedded SQL
 *
 * @file atmiclt67.c
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
#include "test67.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * RUn list of SQLs
 * @param [in] array of string with statements
 * @param [in] should we return from last stmt first value
 * @param [out] return value of the first col/row
 * @return EXSUCCEED/EXFAIL and return value 
 */
long sql_run(char **list, int ret_col_row_1, long *ret_val)
{
    PGconn * conn = ECPGget_PGconn("");
    long ret = EXSUCCEED;
    char *command, char *codes;
    int i;
    
    /* get connection object */
    if (NULL==conn)
    {
        NDRX_LOG(log_error, "Failed to get connection!");
        EXFAIL_OUT(ret);
    }
    
    /* process the commands */
    for (i=0; NULL!=list[i]; i+=2)
    {
        command = list[i];
        codes = list[i+1];
        
        NDRX_LOG(log_debug, "Command [%s] codes [%s]", command, codes);
        
        
    }
    
    
out:
    return ret;
}

int sql_prepare(void)
{

    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "drop table extest;", "0000;42P01"
            ,"CREATE TABLE extest(userid integer UNIQUE NOT NULL);", "0000"
            ,NULL, NULL};
    /* TODO: EXEC */
}

int sql_delete(void)
{
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "delete from extest;", "0000"
            ,NULL, NULL};
}


long sql_count(void)
{
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "select count(*) from extest;", "0000"
            ,NULL, NULL};
}

long sql_insert(void)
{
    /* run some insert */
    
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "insert into extest(userid) values ((select COALESCE(max(userid), 1)+1 from extest));", "0000"
            ,NULL, NULL};
    
}

/**
 * Do the test call to the server
 * Also we need some test cases from shell processing with stalled commits.
 * Thus needs some parameters to be passed to executable.
 */
int main(int argc, char** argv)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    long i;
    int ret=EXSUCCEED;
    
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to open: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpbegin(60, 0))
    {
        NDRX_LOG(log_error, "Failed to begin: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<900; i++)
    {
        if (EXFAIL==Bchg(p_ub, T_LONG_FLD, 0, (char *)&i, 0))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }    

        if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
        {
            NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "Failed to commit: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:
    
    if (EXSUCCEED!=ret)
    {
        tpabort(0);
    }

    tpclose();
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

