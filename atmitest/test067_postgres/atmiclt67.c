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
#include <ecpglib.h>
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
expublic long sql_run(char **list, int ret_col_row_1, long *ret_val)
{
    PGconn * conn = ECPGget_PGconn(NULL);
    long ret = EXSUCCEED;
    char *command, *codes;
    int i;
    PGresult *res = NULL;
    ExecStatusType estat;
    
    /* get connection object */
    if (NULL==conn)
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get connection!");
        EXFAIL_OUT(ret);
    }
    
    /* process the commands */
    for (i=0; NULL!=list[i]; i+=2)
    {
        PQclear(res);
        
        command = list[i];
        codes = list[i+1];
        
        NDRX_LOG(log_debug, "Command [%s] codes [%s]", command, codes);
        
        res = PQexec(conn, command);
        
        estat =PQresultStatus(res);
        if (PGRES_COMMAND_OK != estat && PGRES_TUPLES_OK != estat) 
        {
            char *state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
            char *ok = "0000";
            if (NULL==state)
            {
                state = ok;
            }
            
            if (0==strstr(codes,state))
            {
                NDRX_LOG(log_error, "TESTERROR: Statement [%s] failed with [%s] accepted [%s]",
                        command, state, codes);
                EXFAIL_OUT(ret);
            }
        }
        
    }
    
    if (ret_col_row_1)
    {
        if (PGRES_TUPLES_OK != estat)
        {
            NDRX_LOG(log_error, "TESTERROR: Requested result, but none provided!");
            EXFAIL_OUT(ret);
        }
        else
        {
            *ret_val = atol(PQgetvalue(res, 0, 0));
            
            NDRX_LOG(log_info, "Value extracted: %ld", *ret_val);
        }
    }
    
out:
    PQclear(res);

    return ret;
}

/**
 * Create tables
 * @return  EXSUCCEED/EXFAIL
 */
expublic int sql_mktab(void)
{

    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "drop table extest;", "0000;42P01"
            ,"CREATE TABLE extest(userid integer UNIQUE NOT NULL);", "0000"
            ,NULL, NULL};
    return sql_run(commands, EXFALSE, NULL);
}

/**
 * Delete from table
 * @return EXSUCCEED/EXFAIL
 */
expublic int sql_delete(void)
{
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "delete from extest;", "0000"
            ,NULL, NULL};
    
    return sql_run(commands, EXFALSE, NULL);
}

/**
 * Count records added to table
 * @return count/EXFAIL
 */
expublic long sql_count(void)
{
    long ret_val = EXSUCCEED;
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "select count(*) from extest;", "0000"
            ,NULL, NULL};
    
    if (EXSUCCEED!=sql_run(commands, EXTRUE, &ret_val))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get count!");
        ret_val = EXFAIL;
        goto out;
    }
    
out:
    return ret_val;
}

/**
 * Seems like if we try to insert the same unique value in prepared transaction
 * it will lock up and wait for prep transaction result thus to have some
 * more data, call sql_insert2
 * @return 
 */
expublic int sql_insert(void)
{
    /* run some insert */
    
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "insert into extest(userid) values ((select COALESCE(max(userid), 1)+1 from extest));", "0000"
            ,NULL, NULL};
    
    return sql_run(commands, EXFALSE, NULL);
}


/**
 * In transaction inserts / for suspended
 * @return 
 */
expublic int sql_insert2(void)
{
    /* run some insert */
    
    char *commands[]   = {
            /* Command code       Accepted SQL states*/
            "insert into extest(userid) values (999990);", "0000"
            ,"insert into extest(userid) values (999991);", "0000"
            ,"insert into extest(userid) values (999992);", "0000"
            ,NULL, NULL};
    
    return sql_run(commands, EXFALSE, NULL);
}

/**
 * 1. Do the test call to the server
 * Also we need some test cases from shell processing with stalled commits.
 * Thus needs some parameters to be passed to executable.
 * 
 * Additional tests:
 * 2. postgresql prepare fails / commit shall fail / transaction aborted result
 * 3. run some inserts commit but leave prepared in db, test the commit/rollback
 *  command line commands.
 */
int main(int argc, char** argv)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    long i;
    int ret=EXSUCCEED;
    TPTRANID t;
    
    /* open connection */
    if (EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (argc > 1)
    {
        NDRX_LOG(log_debug, "Got test case: [%s]", argv[1]);
        if (0==strcmp("endfail", argv[1]))
        {
            sql_mktab();
            
            for (i=0; i<100; i++)
            {
                if (EXSUCCEED!=tpbegin(60, 0))
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", 
                            tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }

                if (EXSUCCEED!=sql_insert())
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to insert: %s");
                    EXFAIL_OUT(ret);
                }

                if (EXSUCCEED==tpcommit(0))
                {
                    NDRX_LOG(log_error, "TESTERROR: Commit must fail!");
                    EXFAIL_OUT(ret);
                }

                if (tperrno!=TPEABORT)
                {
                    NDRX_LOG(log_error, "TESTERROR: Expected TPEABORT got: %d!", 
                            tperrno);
                    EXFAIL_OUT(ret);
                }
            }
            
            ret = EXSUCCEED;
            goto out;
        } /* end fail test */
        else if (0==strcmp("doinsert", argv[1]))
        {
            sql_mktab();

            /* run some inserts... say 50 */
            
            if (EXSUCCEED!=tpbegin(60, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }

            for (i=0; i<50; i++)
            {

                if (EXSUCCEED!=sql_insert())
                {
                    NDRX_LOG(log_error, "TESTERROR: Failed to insert: %s");
                    EXFAIL_OUT(ret);
                }
                
            }
            
            if (EXSUCCEED==tpcommit(0))
            {
                NDRX_LOG(log_error, "TESTERROR: Commit must fail!");
                EXFAIL_OUT(ret);
            }
            
            goto out;
            
        }
        else if (0==strcmp("ck50", argv[1]))
        {
         
            if (50!=(ret=(int)sql_count()))
            {
                NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 50", ret);
                EXFAIL_OUT(ret);
            }
            ret = EXSUCCEED;
            goto out;
            
        }
        else if (0==strcmp("ck0", argv[1]))
        {
            if (0!=(ret=(int)sql_count()))
            {
                NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 0", ret);
                EXFAIL_OUT(ret);
            }
            ret = EXSUCCEED;
            goto out;
        }
        else if (0==strcmp("testq", argv[1]))
        {
            ret = q_run(&p_ub);
            goto out;
        }
        else if (0==strcmp("tout", argv[1]))
        {
            if (EXSUCCEED!=tpbegin(60, 0))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED == tpcall("TOUTSV", (char *)p_ub, 0L, (char **)&p_ub, 
                    &rsplen,TPTRANSUSPEND))
            {
                NDRX_LOG(log_error, "TESTERROR: expected error got OK");
                EXFAIL_OUT(ret);
            }
            
            if (TPETIME!=tperrno)
            {
                NDRX_LOG(log_error, "TESTERROR: expected TPETIME, got %d",
                        tperrno);
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED==tpcommit(0))
            {
                NDRX_LOG(log_error, "TESTERROR: Commit must fail!");
                EXFAIL_OUT(ret);
            }
            
            if (TPEABORT!=tperrno)
            {
                NDRX_LOG(log_error, "TESTERROR: Commit not TPEABORT got %d!", tperrno);
                EXFAIL_OUT(ret);
            }
            ret = EXSUCCEED;
            goto out;
        }
        
    }
    
    sql_mktab();
    
    /**************************************************************************/
    NDRX_LOG(log_debug, "Test commit");
    /**************************************************************************/
    if (EXSUCCEED!=tpbegin(60, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
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
            NDRX_LOG(log_error, "TESTERROR: TESTSV failed: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    
    /* test that we have 0 records here... */
    if (EXSUCCEED!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 0", ret);
        EXFAIL_OUT(ret);
    }
        
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to commit: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (900!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 900", ret);
        EXFAIL_OUT(ret);
    }
    
    ret = EXSUCCEED;
    
    
    /**************************************************************************/
    NDRX_LOG(log_debug, "Test suspend");
    /**************************************************************************/
    
    sql_delete();
    
    if (EXSUCCEED!=tpbegin(60, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<1000; i++)
    {
        sql_insert();
    }
    
    if (1000!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 1000", ret);
        EXFAIL_OUT(ret);
    }
    
    /* suspend transaction, shall get 0 */
    
    if (EXSUCCEED!=tpsuspend(&t, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to suspend: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 0", ret);
        EXFAIL_OUT(ret);
    }
    
    /* start another transaction */
    NDRX_LOG(log_debug, "running tran within");
    
    if (EXSUCCEED!=tpbegin(60, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    sql_insert2();
    
    if (3!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 1000", ret);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to commit: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "continue with org tran...");
    if (EXSUCCEED!=tpresume(&t, 0L))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to resume: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* as this is new branch tran, we get 0 */
    if (3!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 3", ret);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to commit: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* as this is new branch tran, we get 0 */
    if (1003!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 1000", ret);
        EXFAIL_OUT(ret);
    }
    
    /**************************************************************************/
    NDRX_LOG(log_debug, "Test abort");
    /**************************************************************************/
    
    sql_delete();
    
    if (EXSUCCEED!=tpbegin(60, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<499; i++)
    {
        sql_insert();
    }
    
    /* as this is new branch tran, we get 0 */
    if (499!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 499", ret);
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=tpabort(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to abort: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 0", ret);
        EXFAIL_OUT(ret);
    }
    
    
    /**************************************************************************/
    NDRX_LOG(log_debug, "Test timeout");
    /**************************************************************************/
    
    sql_delete();
    
    if (EXSUCCEED!=tpbegin(5, 0))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to begin: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<10; i++)
    {
        sql_insert();
    }
    
    /* as this is new branch tran, we get 0 */
    if (10!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 10", ret);
        EXFAIL_OUT(ret);
    }
    
    /* tmsrv must rollback in this time */
    sleep(7);
    
    if (EXSUCCEED==tpcommit(0))
    {
        NDRX_LOG(log_error, "TESTERROR: Commit OK, must fail!");
        EXFAIL_OUT(ret);
    }
    
    if (TPEABORT!=tperrno)
    {
        NDRX_LOG(log_error, "TESTERROR: Transaction must be aborted!");
        EXFAIL_OUT(ret);
    }

    /* as this is new branch tran, we get 0 */
    if (0!=(ret=(int)sql_count()))
    {
        NDRX_LOG(log_error, "TESTERROR: Got count: %d, expected 0", ret);
        EXFAIL_OUT(ret);
    }
    
    /* this shall work out fine, as preared was aborted... */
    for (i=0; i<10; i++)
    {
        sql_insert();
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

