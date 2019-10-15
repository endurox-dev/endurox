/**
 * @brief Postgres C XA Switch emulation
 *
 * @file pgswitch.c
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
#include <pgxa.h>
#include <thlock.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CONN_CLOSED 0   /**< The interface is not open  */
#define CONN_OPEN   1   /**< The interface is open      */
#define TRAN_NOT_FOUND  "42704" /**< SQL State for transaction not found */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/* we need a list of XIDs for keeping the recovery BTID copies: */

/**
 * List of xid left for fetch
 */
typedef struct ndrx_xid_list ndrx_xid_list_t;
struct ndrx_xid_list
{
    XID xid;

    ndrx_xid_list_t *next, *prev;
};


/*---------------------------Globals------------------------------------*/
expublic __thread char ndrx_G_PG_conname[65]={EXEOS}; /**< connection name    */
/*---------------------------Statics------------------------------------*/

MUTEX_LOCKDECL(M_conndata_lock);
exprivate ndrx_pgconnect_t M_conndata; /**< parsed connection data            */
exprivate int M_conndata_ok = EXFALSE; /**< Is connection parsed ok & cached? */

/* threaded data: */
exprivate __thread PGconn * M_conn = NULL;   /**< Actual connection           */ 
exprivate __thread int M_status = CONN_CLOSED;  /**< thread based status      */
exprivate __thread ndrx_xid_list_t *M_list = NULL; /**< XID list of cur recov */

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

exprivate int xa_rollback_local(XID *xid, long flags);

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
 * Add entry to list
 * @param xid to put on list
 * @return EXSUCCEED/EXFAIL(OOM)
 */
exprivate int xid_list_add(XID *xid)
{
    int ret = EXSUCCEED;
    ndrx_xid_list_t *el;
    
    el = NDRX_CALLOC(1, sizeof(ndrx_xid_list_t));
    
    if (NULL==el)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to calloc: %d bytes: %s", 
                sizeof(ndrx_xid_list_t), strerror(err));
        userlog("Failed to calloc: %d bytes: %s", 
                sizeof(ndrx_xid_list_t), strerror(err));
        EXFAIL_OUT(ret);
    }
    
    memcpy((char *)&(el->xid), (char *)xid, sizeof(XID));
    
    
    DL_APPEND(M_list, el);
    
out:
    return ret;
}

/**
 * Fetch the next xid
 * @param xid
 * @return EXTRUE - have next / EXFALSE - we have EOF
 */
exprivate int xid_list_get_next(XID *xid)
{
    ndrx_xid_list_t *tmp;
    if (NULL!=M_list)
    {
        memcpy((char *)xid, (char *)&M_list->xid, sizeof(XID));
        
        tmp = M_list;
        DL_DELETE(M_list, M_list);
        NDRX_FREE((char *)tmp);
        return EXTRUE;
    }
    
    return EXFALSE;
}

/**
 * Remove all from XID list
 */
exprivate void xid_list_free(void)
{
    ndrx_xid_list_t *el, *elt;
    
    DL_FOREACH_SAFE(M_list,el,elt)
    {
        DL_DELETE(M_list,el);
        NDRX_FREE(el);
    }
    
}

/**
 * Get connection handler callback
 * i.e. backend of tpconnect() for C PG/ECPG driver
 * @return ptr to connection (for current thread)
 */
exprivate void *ndrx_pg_getconn(void)
{
    return (void *)M_conn;
}

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
    ndrx_xa_setloctxabort(xa_rollback_local);
    ndrx_xa_setgetconnn(ndrx_pg_getconn);
    
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
        
        snprintf(ndrx_G_PG_conname, sizeof(ndrx_G_PG_conname), "%ld-%ld%ld-%d",
                date, time, (long)(usec / 1000), connid);
    }
    
    NDRX_LOG(log_debug, "Connection name: [%s]", ndrx_G_PG_conname);
    
    M_conn = ndrx_pg_connect(&M_conndata, ndrx_G_PG_conname);
    
    if (NULL==M_conn)
    {
        NDRX_LOG(log_error, "Postgres error: failed to get PQ connection!");
        ret = XAER_RMERR;
        goto out;
    }
    
    M_status = CONN_OPEN;
    NDRX_LOG(log_info, "Connection [%s] is open %p", ndrx_G_PG_conname, M_conn);
    
out:
    return ret;
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

    if (EXSUCCEED!=ndrx_pg_disconnect(M_conn, ndrx_G_PG_conname))
    {
        NDRX_LOG(log_error, "ndrx_pg_disconnect failed: %s", 
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
    long accepted_flags = TMSUCCESS|TMFAIL;
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Not open");
        ret = XAER_PROTO;
        goto out;
    }
    
    /* check the flags */
    if ( (flags | accepted_flags) != accepted_flags)
    {
        NDRX_LOG(log_error, "Accepted flags are: TMSUCCESS|TMFAIL, but got %ld",
                flags);
        ret = XAER_INVAL;
        goto out;
    }

    
    NDRX_LOG(log_debug, "END OK");
out:
    
    return ret;
}

/**
 * Common transaction management routine
 * @param sw XA Switch
 * @param sql_cmd SQL Command to execute
 * @param dbg_msg debug message related with command
 * @param xid transaction XID
 * @param rmid resource manager ID
 * @param flags currently not flags are supported
 * @param[in] is_prep is this prepare statement call
 * @return XA error code or XA_OK
 */
exprivate int xa_tran_entry(struct xa_switch_t *sw, char *sql_cmd, char *dbg_msg,
        XID *xid, int rmid, long flags, int is_prep)
{
    int ret = XA_OK;
    char stmt[1024];
    char pgxid[NDRX_PG_STMTBUFSZ];
    PGresult *res = NULL;
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Not open");
        ret = XAER_PROTO;
        goto out;
    }
    
    if (TMNOFLAGS != flags)
    {
        NDRX_LOG(log_error, "Flags not TMNOFLAGS (%ld), passed to %s", 
                flags, dbg_msg);
        ret = XAER_INVAL;
        goto out;
    }
    
    if (EXSUCCEED!=ndrx_pg_xid_to_db(xid, pgxid, sizeof(pgxid)))
    {
        NDRX_DUMP(log_error, "Failed to convert XID to pg string", xid, sizeof(*xid));
        ret = XAER_INVAL;
        goto out;
    }
    
    snprintf(stmt, sizeof(stmt), "%s '%s';", sql_cmd, pgxid);
    
    NDRX_LOG(log_info, "Exec: [%s]", stmt);
    
    res = PQexec(M_conn, stmt);
    if (PGRES_COMMAND_OK != PQresultStatus(res)) 
    {
        char *state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        
        if (0==strcmp(TRAN_NOT_FOUND, state))
        {
            NDRX_LOG(log_info, "Transaction not found (probably read-only branch)");
        }
        else
        {
            NDRX_LOG(log_error, "SQL STATE %s: Failed to %s transaction by [%s]: %s",
                    state, dbg_msg, stmt, PQerrorMessage(M_conn));

            if (is_prep)
            {
                NDRX_LOG(log_error, "Work is rolled back automatically by PG");
                ret = XA_RBROLLBACK;
            }
        }
    }
    
    NDRX_LOG(log_debug, "%s OK", dbg_msg);
out:
    
    PQclear(res);

    return ret;
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
    return xa_tran_entry(sw, "ROLLBACK PREPARED", "ROLLBACK", 
            xid, rmid, flags, EXFALSE);
}

/**
 * Prepare transaction. Any error from statement exec makes the current
 * job to be rolled back.
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    return xa_tran_entry(sw, "PREPARE TRANSACTION", "PREPARE", 
            xid, rmid, flags, EXTRUE);
}

/**
 * If XID is not found then we shall assume that transaction prepare failed
 * for some reason, and we shall return XA_HEURRB.
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
exprivate int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    return xa_tran_entry(sw, "COMMIT PREPARED", "COMMIT", 
            xid, rmid, flags, EXFALSE);
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
exprivate int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, 
        int rmid, long flags)
{
    int ret = XA_OK;
    long accepted_flags = TMSTARTRSCAN|TMENDRSCAN|TMNOFLAGS;
    PGresult   *res = NULL;
    int i;
    int nrtx;
    
    /* check the flags */
    if ( (flags | accepted_flags) != accepted_flags)
    {
        NDRX_LOG(log_error, "Accepted flags are: TMSTARTRSCAN|TMENDRSCAN|TMNOFLAGS, but got %ld",
                flags);
        ret = XAER_INVAL;
        goto out;
    }
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Not open");
        ret = XAER_PROTO;
        goto out;
    }
    
    
    if (flags & TMSTARTRSCAN)
    {
        /* remove any old scans... */
        xid_list_free();
        
        /* Start a transaction block */
        res = PQexec(M_conn, "BEGIN");
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            NDRX_LOG(log_error, "BEGIN command failed: %s", 
                    PQerrorMessage(M_conn));
            PQclear(res);
            ret = XAER_RMERR;
            goto out;
        }
        
        PQclear(res);
        
        
        /* as local transaction processor at tmsrv will scan the XIDs
         * and perform commit/abort/(forget), that cannot be done in transaction
         * thus we need to scan and fill local linked list of XIDs fetched
         */

        /*
         * Fetch rows from pg_database, the system catalog of databases
         */
        res = PQexec(M_conn, "DECLARE ndrx_pq_list_xids "
                 "CURSOR  FOR SELECT gid FROM pg_prepared_xacts ORDER BY prepared;");

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            NDRX_LOG(log_error, "DECLARE CURSOR failed: %s", PQerrorMessage(M_conn));
            PQclear(res);
            ret = XAER_RMERR;
            goto out;
        }

        PQclear(res);

        res = PQexec(M_conn, "FETCH ALL in ndrx_pq_list_xids;");
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            NDRX_LOG(log_error, "FETCH ALL failed: %s", PQerrorMessage(M_conn));
            PQclear(res);
            ret = XAER_RMERR;
            goto out;
        }

        /* Read xids into linked list? */
        nrtx = PQntuples(res);

        NDRX_LOG(log_info, "Recovered %d transactions", nrtx);
        for (i = 0; i < nrtx; i++)
        {
            char *btid = PQgetvalue(res, i, 0);
            XID xid_fetch;
            
            NDRX_LOG(log_debug, "Got BTID: [%s] - try parse", btid);
            if (EXSUCCEED!=ndrx_pg_db_to_xid(btid, &xid_fetch))
            {
                continue;
            }
            
            /* Add to DL */
            if (EXSUCCEED!=xid_list_add(&xid_fetch))
            {
                NDRX_LOG(log_error, "Failed to add BTID to list!");
                PQclear(res);
                EXFAIL_OUT(ret);
            }
        }
        PQclear(res);
        
        /* close the scan */
        res = PQexec(M_conn, "CLOSE ndrx_pq_list_xids;");
        PQclear(res);

        /* end the transaction */
        res = PQexec(M_conn, "END;");
        PQclear(res);
        
    } /* TMSTARTRSCAN */
    
    /* load transactions into list (as much as we have...) */
    nrtx = 0;
    
    for (i=0; i<count; i++)
    {
        if (EXTRUE==xid_list_get_next(&xid[i]))
        {
            nrtx++;
        }
        else
        {
            break;
        }
    }
    
    ret = nrtx;
    
    if (TMENDRSCAN & flags)
    {
        xid_list_free();
    }
    
out:    
    
    NDRX_LOG(log_info, "Returning %d", ret);
    return ret;
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
    return xa_tran_entry(sw, "ROLLBACK PREPARED", "FORGET", 
            xid, rmid, flags, EXFALSE);
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

/**
 * Abort local transaction, with out XID
 * Use for xa_end so that we do not enter in prepared state if we know that
 * we are doing abort
 * @param xid transaction xid, not used
 * @param flags flags, not used
 * @return XA_OK, XE_ERR
 */
exprivate int xa_rollback_local(XID *xid, long flags)
{
    int ret = XA_OK;
    char stmt[1024];
    char pgxid[NDRX_PG_STMTBUFSZ];
    PGresult *res = NULL;
    
    if (CONN_OPEN!=M_status)
    {
        NDRX_LOG(log_debug, "XA Not open");
        ret = XAER_PROTO;
        goto out;
    }
    
    if (TMNOFLAGS != flags)
    {
        NDRX_LOG(log_error, "Flags not TMNOFLAGS (%ld)", 
                flags);
        ret = XAER_INVAL;
        goto out;
    }
    
    NDRX_STRCPY_SAFE(stmt, "ROLLBACK");
    
    NDRX_LOG(log_info, "Exec: [%s]", stmt);
    
    res = PQexec(M_conn, stmt);
    
    if (PGRES_COMMAND_OK != PQresultStatus(res)) 
    {
        char *state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        
        if (0==strcmp(TRAN_NOT_FOUND, state))
        {
            NDRX_LOG(log_info, "Transaction not found");
        }
        else
        {
            ret = XAER_RMERR;
        }
    }
    
    NDRX_LOG(log_debug, "%s OK", stmt);
    
out:
    
    PQclear(res);

    return ret;
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
