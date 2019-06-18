/**
 * @brief XID Convert from/to DB, using Java format so that we can share the
 *  C based tmsrv with Java code for faster operations.
 *
 * @file pgxid.c
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
#include <pgxa.h>
#include <exbase64.h>

#include "xa.h"

/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

/**
 * Convert XID to Posgres in PG JDBC Format style
 * @param[in] xid C XID
 * @param[out] buf output buffer 
 * @param[in] output buffer size
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_pg_xid_to_db(XID *xid, char *buf, int bufsz)
{
    int ret = EXSUCCEED;
    size_t outsz = 0;
    size_t curpos;
    snprintf(buf, bufsz, "%ld", xid->formatID);
    
    NDRX_STRCAT_S(buf, bufsz, "_");
    
    curpos = strlen(buf);
            
    if (NULL==ndrx_base64_encode(xid->data,
                    xid->gtrid_length,
                    &outsz,
                    buf + curpos))
    {
        NDRX_LOG(log_error, "Failed to encode gtrid!");
        EXFAIL_OUT(ret);
    }
    
    /* *(buf + curpos + outsz) = EXEOS; */
    
    NDRX_STRCAT_S(buf, bufsz, "_");
    
    
    curpos = strlen(buf);
    
    /* build up the bqual part */
    
    if (NULL==ndrx_base64_encode(xid->data + xid->gtrid_length,
                    xid->bqual_length,
                    &outsz,
                    buf + curpos))
    {
        NDRX_LOG(log_error, "Failed to encode gtrid!");
        EXFAIL_OUT(ret);
    }
    
    /* *(buf + curpos + outsz) = EXEOS; */
    
    NDRX_LOG(log_debug, "Got PG XID: [%s]", buf);
    
    
out:
    return ret;
}

/**
 * Convert Database TX ID to XID
 * @param[in] buf db string.
 * @param[out] xid XID 
 * @return EXSUCCEED/EXFAIL (invalid format)
 */
expublic int ndrx_pg_db_to_xid(char *buf, XID *xid)
{
    int ret = EXSUCCEED;
    char *tok, *saveptr1;
    int cnt=0;
    char tmp[201];
    size_t len;
    
    NDRX_STRCPY_SAFE(tmp, buf);
    
    NDRX_LOG(log_debug, "About to process PG xid: [%s]", tmp);
    
    tok=strtok_r (tmp, "_", &saveptr1);
    while( tok != NULL ) 
    {
        NDRX_LOG(log_debug, "Got token: [%s]", tok);
        switch (cnt)
        {
            case 0:
                /* format id */
                xid->formatID = atol(tok);
                break;
            case 1:
                
                /* gtrid */
                len = MAXGTRIDSIZE;
                if (NULL==ndrx_base64_decode(tok, strlen(tok), &len, xid->data))
                {
                    NDRX_LOG(log_error, "Failed to decode gtrid!");
                    EXFAIL_OUT(ret);
                }
                xid->gtrid_length = len;
                
                break;
            case 2:
                /* bqual */
                len = MAXBQUALSIZE;
                if (NULL==ndrx_base64_decode(tok, strlen(tok), 
                        &len, xid->data+xid->gtrid_length))
                {
                    NDRX_LOG(log_error, "Failed to decode bqual!");
                    EXFAIL_OUT(ret);
                }
                xid->bqual_length = len;
                
                break;
            default:
                /* Invalid format ID! */
                NDRX_LOG(log_error, "Invalid PG XID, token nr: %d", cnt);
                EXFAIL_OUT(ret);
                break;
        }
        cnt++;
        tok=strtok_r (NULL,";", &saveptr1);
    }
    
    NDRX_DUMP(log_debug, "Got XID from PG", xid, sizeof(*xid));

out:

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
