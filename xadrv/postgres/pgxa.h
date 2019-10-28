/**
 * @brief Added for compatiblity
 *
 * @file pgxa.h
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

#ifndef PGXA_H
#define	PGXA_H

#ifdef	__cplusplus
extern "C" {
#endif

/*---------------------------Includes-----------------------------------*/
#include <xa.h>
#include <libpq-fe.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
    
#define NDRX_PG_XID_LEN     200 /**< XID Length as defined by PREPARE TRANSACTION */
#define NDRX_PG_STMTBUFSZ   1024 /**< Statement buffer length */
    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
    
/**
 * PG Database connection details
 */
struct ndrx_pgconnect
{
    int c;  /**< Compatiblity flag, ECPG_COMPAT_PGSQL | ECPG_COMPAT_INFORMIX | ECPG_COMPAT_INFORMIX_SE*/
    char user[65];  /**< user name according to NAMEDATALEN */
    char password[101]; /**< plain password for connect */
    char url[255+25+64+1]; /**< comain max, + protocol + database name + eos */
    
};
typedef struct ndrx_pgconnect ndrx_pgconnect_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
extern int ndrx_pg_xa_cfgparse(char *buffer, ndrx_pgconnect_t *conndata);
extern int ndrx_pg_xid_to_db(XID *xid, char *buf, int bufsz);
extern int ndrx_pg_db_to_xid(char *buf, XID *xid);

extern PGconn * ndrx_pg_connect(ndrx_pgconnect_t *conndata, char *connname);
extern int ndrx_pg_disconnect(PGconn *conn, char *connname);


#ifdef	__cplusplus
}
#endif

#endif	/* PGXA_H */

/* vim: set ts=4 sw=4 et smartindent: */
