/**
 * @brief Posgres C XA Switch emulation
 *
 * @file pgswitch.c
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

#include <ndrstandard.h>
#include <ndebug.h>
#include <atmi.h>

#include "atmi_shm.h"

#include <xa.h>
#include <ecpglib.h>
#include <pgxa.h>
#include <thlock.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CONN_CLOSED 0   /**< The interface is not open  */
#define CONN_OPEN   1   /**< The interface is open      */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic __thread char ndrx_G_PG_conname[65]={EXEOS}; /**< connection name    */
/*---------------------------Statics------------------------------------*/

MUTEX_LOCKDECL(M_conndata_lock);
exprivate ndrx_pgconnect_t M_conndata; /**< parsed connection data            */
exprivate int M_conndata_ok = EXFALSE; /**< Is connection parsed ok & cached? */
exprivate __thread PGconn * M_conn = NULL;   /**< Actual connection           */ 
exprivate __thread int M_status = CONN_CLOSED;  /**< thread based status      */

/*---------------------------Prototypes---------------------------------*/

exprivate int xa_open_entry_stat(char *xa_info, int rmid, long flags);
exprivate int xa_close_entry_stat(char *xa_info, int rmid, long flags);
exprivate int xa_start_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_end_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_commit_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
exprivate int xa_forget_entry_stat(XID *xid, int rmid, long flags);
exprivate int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

exprivate int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
exprivate int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
exprivate int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
exprivate int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
exprivate int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);

struct xa_switch_t ndrxpgsw = 
{ 
    .name = "ndrxpgsw",
    .flags = TMNOMIGRATE,
    .version = 0,
    .xa_open_entry = xa_open_entry_stat,
    .xa_close_entry = xa_close_entry_stat,
    .xa_start_entry = xa_start_entry_stat,
    .xa_end_entry = xa_end_entry_stat,
    .xa_rollback_entry = xa_rollback_entry_stat,
    .xa_prepare_entry = xa_prepare_entry_stat,
    .xa_commit_entry = xa_commit_entry_stat,
    .xa_recover_entry = xa_recover_entry_stat,
    .xa_forget_entry = xa_forget_entry_stat,
    .xa_complete_entry = xa_complete_entry_stat
};

/**
 * API entry of loading the driver
 * @param symbol
 * @param descr
 * @return XA switch or null
 */
struct xa_switch_t *ndrx_get_xa_switch(void)
{
    /* configure other flags */
    
    /* prase the config and share it between threads */
    
    ndrx_xa_nostartxid(EXTRUE);
    return &ndrxpgsw;
}

/**
 * Open API.
 * This is called per thread.
 * The config is written in JSON.
 * Syntax:
 * { "url":"unix:postgresql://sql.mydomain.com:5432/mydb", "user":"test", "password":"test1", "compat":"INFORMIX|INFORMIX_SE|PGSQL"}
 * @param switch 
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = XA_OK;
    static int conn_counter = 0;
    int connid;
    
    if (CONN_OPEN==M_status)
    {
        NDRX_LOG(log_error, "Connection is already open");
        ret=XAER_PROTO;
        goto out;
    }
    
    /* try parse config */
    if (!M_conndata_ok)
    {
        MUTEX_LOCK_V(M_conndata_lock);
        
        if (!M_conndata_ok)
        {
            if (EXSUCCEED!=ndrx_pg_xa_cfgparse(xa_info, &M_conndata))
            {
                NDRX_LOG(log_error, "Failed to parse Open string!");
                MUTEX_UNLOCK_V(M_conndata_lock);
                ret = XAER_INVAL;
                goto out;
            }
            
            M_conndata_ok = EXTRUE;
            MUTEX_UNLOCK_V(M_conndata_lock);
        }
    }
    
    /* generate connection name. The format is following:
     * YYYYMMDD-HHMISSFFF-%d 
     */
    if (EXEOS==ndrx_G_PG_conname[0])
    {
        long date;
        long time;
        long usec;
        
        /* use the same lock for connection naming.. */
        MUTEX_LOCK_V(M_conndata_lock);
        connid = conn_counter;
        
        conn_counter++;
        
        if (conn_counter > 16000)
        {
            conn_counter = 0;
        }
        
        MUTEX_UNLOCK_V(M_conndata_lock);
        
        ndrx_get_dt_local(&date, &time, &usec);
        
        snprintf(ndrx_G_PG_conname, sizeof(conn_counter), "%ld-%ld%ld-%d",
                date, time, (long)(usec / 1000), connid);
    }
    
    NDRX_LOG(log_debug, "Connection name: [%s]", ndrx_G_PG_conname);
    
    /* OK, try to open, with out autocommit please!
     */
    if (!ECPGconnect (__LINE__, M_conndata.c, M_conndata.url, M_conndata.user, 
            M_conndata.password, ndrx_G_PG_conname, EXFALSE))
    {
        NDRX_LOG(log_error, "ECPGconnect failed: %s");
        ret = XAER_RMERR;
        goto out;
    }
    
    M_conn = ECPGget_PGconn(ndrx_G_PG_conname);
    if (NULL==M_conn)
    {
        NDRX_LOG(log_error, "Postgres error: failed to get PQ connection!");
        ret = XAER_RMERR;
        goto out;
    }
    
    M_status = CONN_OPEN;
    NDRX_LOG(log_info, "Connection [%s] is open %p", ndrx_G_PG_conname, M_conn);
    
out:
    return EXFAIL;
}

/**
 * Close entry.
 * @param sw xa switch
 * @param xa_info close string
 * @param rmid RM id
 * @param flags flags
 * @return xa err
 */
exprivate int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    int ret = XA_OK;
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Already closed");
        goto out;
    }
    
    if (!ECPGdisconnect(__LINE__, ndrx_G_PG_conname))
    {
        NDRX_LOG(log_error, "ECPGdisconnect failed: %s", 
                PQerrorMessage(M_conn));
        return XAER_RMERR;
    }
    
    M_conn = NULL;
    M_status = CONN_CLOSED;
    
    NDRX_LOG(log_info, "Connection closed");
    
out:
    return ret;
}

/**
 * Just start the transaction.
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    int ret = XA_OK;
    PGresult *res = NULL;
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Not open");
        ret = XAER_PROTO;
        goto out;
    }
    
    if (TMNOFLAGS != flags)
    {
        NDRX_LOG(log_error, "Flags not TMNOFLAGS (%ld), passed to start_entry", flags);
        ret = XAER_INVAL;
        goto out;
    }

    /* start PG transaction */
    res = PQexec(M_conn, "BEGIN");
    if (PGRES_COMMAND_OK != PQresultStatus(res))
    {
        NDRX_LOG(log_error, "Failed to begin transaction: %s", PQerrorMessage(M_conn));
        ret = XAER_RMERR;
        goto out;
    }
    
out:
    
    PQclear(res);

    return ret;
}

/**
 * Terminate transaction in progress
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    int ret = XA_OK;
    
    /* TODO: Check flags */
    
    /* TODO: Check is XA Open */
    
    /* TODO: Nothing more to check, as process will get prepare entry */
    
    return EXFAIL;
}

/**
 * We rollback only prepared transaction.
 * Thus every transaction will get prepare call
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    return EXFAIL;
}

/**
 * prepare
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
     /*
     * TODO:
     * PQtransactionStatus(con->connection);
     * - Transaction must be open
     */
    
    /* well we do not do anything here, because our exit from transaction is
     * to prepare it and that is done by Enduro/X
     */
    
     
    return EXFAIL;
}

/**
 * Commit
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    return EXFAIL;
}

/**
 * Return list of trans.
 * Java returns full list, but our buffer is limited.
 * Thus we load the `count' number of items, but we will return the actual
 * number of java items
 * @param sw xa switch
 * @param xid buffer where to load xids
 * @param count number of xid buffer size
 * @param rmid resourcemanager id
 * @param flags flags
 * @return XA_OK, XERR
 */
exprivate int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    return EXFAIL;
}

/**
 * Forget transaction
 * @param sw xa switch
 * @param xid XID
 * @param rmid RM ID
 * @param flags flags
 * @return XA_OK, XERR
 */
exprivate int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    return EXFAIL;
}

/**
 * CURRENTLY NOT USED!!!
 * @param sw
 * @param handle
 * @param retval
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
{
    return EXFAIL;
}

/* Static entries */
exprivate int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxpgsw, xa_info, rmid, flags);
}
exprivate int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxpgsw, xa_info, rmid, flags);
}
exprivate int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxpgsw, xid, rmid, flags);
}

exprivate int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxpgsw, xid, rmid, flags);
}
exprivate int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxpgsw, xid, rmid, flags);
}
exprivate int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxpgsw, xid, rmid, flags);
}

exprivate int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxpgsw, xid, rmid, flags);
}

exprivate int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxpgsw, xid, count, rmid, flags);
}
exprivate int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxpgsw, xid, rmid, flags);
}
exprivate int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxpgsw, handle, retval, rmid, flags);
}


/* vim: set ts=4 sw=4 et smartindent: */
