/**
 * @brief Sample application for testing SQL codes provided for Posgres
 *  for certain cases
 *
 * @file codetest.c
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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#include <userlog.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <exparson.h>

#include <ecpglib.h>


/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
typedef struct {
    
    char *field;
    int id;
    
} details_t;
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/


exprivate details_t M_tests [] = {
    
    {"PG_DIAG_SEVERITY", PG_DIAG_SEVERITY},
    {"PG_DIAG_SQLSTATE", PG_DIAG_SQLSTATE},
    {"PG_DIAG_MESSAGE_PRIMARY", PG_DIAG_MESSAGE_PRIMARY},
    {"PG_DIAG_MESSAGE_DETAIL", PG_DIAG_MESSAGE_DETAIL},
    {"PG_DIAG_MESSAGE_HINT", PG_DIAG_MESSAGE_HINT},
    {"PG_DIAG_STATEMENT_POSITION", PG_DIAG_STATEMENT_POSITION},
    {"PG_DIAG_CONTEXT", PG_DIAG_CONTEXT},
    {"PG_DIAG_SOURCE_FILE", PG_DIAG_SOURCE_FILE},
    {"PG_DIAG_SOURCE_LINE", PG_DIAG_SOURCE_LINE},
    {"PG_DIAG_SOURCE_FUNCTION", PG_DIAG_SOURCE_FUNCTION},
    
    {NULL}
    
};

/*------------------------------Prototypes------------------------------------*/

/**
 * Entry point for test application
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char **argv)
{
    char stmt[1024];
    PGresult *res = NULL;
    PGconn * conn = NULL;
    int ret;
    details_t *det;
    
    char *tests_nok[] = {
        "BEGIN TRANSACTION;"
        ,"PREPARE TRANSACTION 'EHLO';"
        ,"ROLLBACK PREPARED 'EHLO';"
        ,"COMMIT PREPARED 'EHLO';"
        ,"drop table ndrx_test_account;"
        ,"drop table ndrx_test_account;"
        ,"CREATE TABLE ndrx_test_account(userid integer UNIQUE NOT NULL);"
        ,"select count(*) from ndrx_test_account"
        ,"BEGIN TRANSACTION;"
        ,"insert into ndrx_test_account(userid) values (1);"
        ,"PREPARE TRANSACTION 'TEST1';"
        ,"ROLLBACK PREPARED 'TEST1';"
        ,"COMMIT PREPARED 'TEST1';"
        ,"BEGIN TRANSACTION;"
        ,"insert into ndrx_test_account(userid) values (1);"
        ,"PREPARE TRANSACTION 'TEST1';"
        ,"COMMIT PREPARED 'TEST1';"
        ,"ROLLBACK PREPARED 'TEST1';"
        ,"select count(*) from ndrx_test_account;"
        ,NULL
    };

    int i;
    
    sqlca.sqlcode = 0;
    
    
    if (!ECPGconnect (__LINE__, 0, "unix:postgresql://localhost/template1", "postgres", 
            "", "local", EXFALSE))
    {
        NDRX_LOG(log_error, "ECPGconnect failed, code %ld state: [%s]: %s", 
                (long)sqlca.sqlcode, sqlca.sqlstate, sqlca.sqlerrm.sqlerrmc);
        
        return EXFAIL;
    }
    
    conn = ECPGget_PGconn("local");
    if (NULL==conn)
    {
        NDRX_LOG(log_error, "Postgres error: failed to get PQ connection!");
        return EXFAIL;
    }
    
    while (tests_nok[i])
    {
        NDRX_LOG(log_info, "Exec: [%s]", tests_nok[i]);

        res = PQexec(conn, tests_nok[i]);

        ret=(int)PQresultStatus(res);
        det = M_tests;

        NDRX_LOG(log_error, "PQresultStatus: %d (conn err: %s)", ret, PQerrorMessage(conn));
        while (det->field)
        {
            NDRX_LOG(log_error, "%s: %s", det->field, PQresultErrorField(res, det->id));
            det++;
        }
        i++;
        
        PQclear(res);
    }
    /*
     WE GET:

N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:plugins_load:inbase.c:0180:No plugins defined by NDRX_PLUGINS env variable
N:NDRX:5:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:cconfig_load:config.c:0429:CC tag set to: []
N:NDRX:5:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:x_inicfg_new:inicfg.c:0114:_ndrx_inicfg_new: load_global_env: 1
N:NDRX:5:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:ig_load_pass:config.c:0396:_ndrx_cconfig_load_pass: ret: 0 is_internal: 1 G_tried_to_load: 1
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0141:Exec: [BEGIN TRANSACTION;]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334264:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0141:Exec: [PREPARE TRANSACTION 'EHLO';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334265:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0141:Exec: [ROLLBACK PREPARED 'EHLO';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334266:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0141:Exec: [COMMIT PREPARED 'EHLO';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0148:PQresultStatus: 7 (conn err: ERROR:  prepared transaction with identifier "EHLO" does not exist
)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SEVERITY: ERROR
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SQLSTATE: 42704
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: prepared transaction with identifier "EHLO" does not exist
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: twophase.c
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: 539
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: LockGXact
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334267:main        :estest.c:0141:Exec: [drop table ndrx_test_account;]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334269:main        :estest.c:0141:Exec: [CREATE TABLE ndrx_test_account(userid integer UNIQUE NOT NULL);]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334283:main        :estest.c:0141:Exec: [BEGIN TRANSACTION;]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0141:Exec: [insert into ndrx_test_account(userid) values (1);]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334284:main        :estest.c:0141:Exec: [PREPARE TRANSACTION 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334285:main        :estest.c:0141:Exec: [ROLLBACK PREPARED 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0141:Exec: [COMMIT PREPARED 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0148:PQresultStatus: 7 (conn err: ERROR:  prepared transaction with identifier "TEST1" does not exist
)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SEVERITY: ERROR
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SQLSTATE: 42704
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: prepared transaction with identifier "TEST1" does not exist
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: twophase.c
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: 539
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: LockGXact
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0141:Exec: [BEGIN TRANSACTION;]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334286:main        :estest.c:0141:Exec: [insert into ndrx_test_account(userid) values (1);]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0141:Exec: [PREPARE TRANSACTION 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334287:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0141:Exec: [COMMIT PREPARED 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0148:PQresultStatus: 1 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334288:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0141:Exec: [ROLLBACK PREPARED 'TEST1';]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0148:PQresultStatus: 7 (conn err: ERROR:  prepared transaction with identifier "TEST1" does not exist
)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SEVERITY: ERROR
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SQLSTATE: 42704
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: prepared transaction with identifier "TEST1" does not exist
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: twophase.c
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: 539
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: LockGXact
N:NDRX:4:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0141:Exec: [select count(*) from ndrx_test_account;]
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0148:PQresultStatus: 2 (conn err: )
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SEVERITY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SQLSTATE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_PRIMARY: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_DETAIL: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_MESSAGE_HINT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_STATEMENT_POSITION: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_CONTEXT: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FILE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_LINE: (null)
N:NDRX:2:d5d3db3a: 3461:7f311b089800:000:20190623:124334289:main        :estest.c:0151:PG_DIAG_SOURCE_FUNCTION: (null)

     */
    
    
    /* try to do some work */
    
    
    return 0;
}

/* vim: set ts=4 sw=4 et smartindent: */
