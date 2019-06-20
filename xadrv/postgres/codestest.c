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
        "PREPARE TRANSACTION 'EHLO';",
        "ROLLBACK PREPARED 'EHLO';",
        "COMMIT PREPARED 'EHLO';",
        NULL
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

        NDRX_LOG(log_error, "PQresultStatus: %d", ret);
        while (det->field)
        {
            NDRX_LOG(log_error, "%s: %s", det->field, PQresultErrorField(res, det->id));
            det++;
        }
        i++;
    }
    /*
     WE GET:
Exec: [PREPARE TRANSACTION 'EHLO';]
PQresultStatus: 1
PG_DIAG_SEVERITY: (null)
PG_DIAG_SQLSTATE: (null)
PG_DIAG_MESSAGE_PRIMARY: (null)
PG_DIAG_MESSAGE_DETAIL: (null)
PG_DIAG_MESSAGE_HINT: (null)
PG_DIAG_STATEMENT_POSITION: (null)
PG_DIAG_CONTEXT: (null)
PG_DIAG_SOURCE_FILE: (null)
PG_DIAG_SOURCE_LINE: (null)
PG_DIAG_SOURCE_FUNCTION: (null)
Exec: [ROLLBACK PREPARED 'EHLO';]
PQresultStatus: 7
PG_DIAG_SEVERITY: ERROR
PG_DIAG_SQLSTATE: 42704
PG_DIAG_MESSAGE_PRIMARY: prepared transaction with identifier "EHLO" does not exist
PG_DIAG_MESSAGE_DETAIL: (null)
PG_DIAG_MESSAGE_HINT: (null)
PG_DIAG_STATEMENT_POSITION: (null)
PG_DIAG_CONTEXT: (null)
PG_DIAG_SOURCE_FILE: twophase.c
PG_DIAG_SOURCE_LINE: 539
PG_DIAG_SOURCE_FUNCTION: LockGXact
Exec: [COMMIT PREPARED 'EHLO';]
PQresultStatus: 7
PG_DIAG_SEVERITY: ERROR
PG_DIAG_SQLSTATE: 42704
PG_DIAG_MESSAGE_PRIMARY: prepared transaction with identifier "EHLO" does not exist
PG_DIAG_MESSAGE_DETAIL: (null)
PG_DIAG_MESSAGE_HINT: (null)
PG_DIAG_STATEMENT_POSITION: (null)
PG_DIAG_CONTEXT: (null)
PG_DIAG_SOURCE_FILE: twophase.c
PG_DIAG_SOURCE_LINE: 539
PG_DIAG_SOURCE_FUNCTION: LockGXact
     */
    
    
    
    /* try to do some work */
    
    
    return 0;
}

/* vim: set ts=4 sw=4 et smartindent: */
