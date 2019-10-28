/**
 * @brief Postgres ECPG Connection
 *
 * @file ecpg.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>

#include "atmi_shm.h"
#include "utlist.h"

#include <xa.h>
#include <ecpglib.h>
#include <pgxa.h>
#include <thlock.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform connect
 * @param conndata parsed connection data
 * @param connname connection name
 * @return NULL (connection failed) or connection object
 */
expublic PGconn * ndrx_pg_connect(ndrx_pgconnect_t *conndata, char *connname)
{
    PGconn *ret = NULL;
    
    NDRX_LOG(log_debug, "Establishing ECPG connection: [%s]", conndata);
    
    /* OK, try to open, with out autocommit please!
     */
    if (!ECPGconnect (__LINE__, conndata->c, conndata->url, conndata->user, 
            conndata->password, connname, EXFALSE))
    {
        NDRX_LOG(log_error, "ECPGconnect failed, code %ld state: [%s]: %s", 
                (long)sqlca.sqlcode, sqlca.sqlstate, sqlca.sqlerrm.sqlerrmc);
        ret = NULL;
        goto out;
    }
    
    ret = ECPGget_PGconn(connname);
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Postgres error: failed to get PQ connection!");
        ret = NULL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * disconnect from postgres
 * @param conn current connection object
 * @param connname connection name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_pg_disconnect(PGconn *conn, char *connname)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "Closing ECPG connection: [%s]", connname);
    
    if (!ECPGdisconnect(__LINE__, connname))
    {
        NDRX_LOG(log_error, "ECPGdisconnect failed: %s", 
                PQerrorMessage(conn));
        EXFAIL_OUT(ret);
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
