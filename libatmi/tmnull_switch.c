/**
 * @brief Load null driver - static version
 *
 * @file tmnull_switch.c
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
#include <tmenv.h>

#include <xa.h>

#include "atmi_tls.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate MUTEX_LOCKDECL(M_init);
/*---------------------------Prototypes---------------------------------*/


expublic int ndrx_nul_xa_open_entry_stat(char *xa_info, int rmid, long flags);
expublic int ndrx_nul_xa_close_entry_stat(char *xa_info, int rmid, long flags);
expublic int ndrx_nul_xa_start_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_end_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_rollback_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_prepare_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_commit_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
expublic int ndrx_nul_xa_forget_entry_stat(XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

expublic int ndrx_nul_xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int ndrx_nul_xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int ndrx_nul_xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
expublic int ndrx_nul_xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int ndrx_nul_xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);

struct xa_switch_t tmnull_switch = 
{ 
    .name = "tmnull_switch",
    .flags = TMNOFLAGS,
    .version = 0,
    .xa_open_entry = ndrx_nul_xa_open_entry_stat,
    .xa_close_entry = ndrx_nul_xa_close_entry_stat,
    .xa_start_entry = ndrx_nul_xa_start_entry_stat,
    .xa_end_entry = ndrx_nul_xa_end_entry_stat,
    .xa_rollback_entry = ndrx_nul_xa_rollback_entry_stat,
    .xa_prepare_entry = ndrx_nul_xa_prepare_entry_stat,
    .xa_commit_entry = ndrx_nul_xa_commit_entry_stat,
    .xa_recover_entry = ndrx_nul_xa_recover_entry_stat,
    .xa_forget_entry = ndrx_nul_xa_forget_entry_stat,
    .xa_complete_entry = ndrx_nul_xa_complete_entry_stat
};

/**
 * Open API
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    static int first = EXTRUE;
    
    /* mark that suspend no required by this resource... */
    if (first)
    {
        MUTEX_LOCK_V(M_init);
        if (first)
        {
            ndrx_xa_nosuspend(EXTRUE);
            first=EXFALSE;
        }
        MUTEX_UNLOCK_V(M_init);
    }
    
    if (G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_open_entry() - already open!");
        return XAER_RMERR;
    }
    
    G_atmi_tls->tmnull_is_open = EXTRUE;
    G_atmi_tls->tmnull_rmid = rmid;
             
    return XA_OK;
}
/**
 * Close entry
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    NDRX_LOG(log_error, "xa_close_entry() called");
    
    G_atmi_tls->tmnull_is_open = EXFALSE;
    return XA_OK;
}

/**
 * Open text file in RMID folder. Create file by TXID.
 * Check for file existance. If start & not exists - ok .
 * If exists and join - ok. Otherwise fail.
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_start_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    return XA_OK;
}

/**
 * Terminate XA access
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_end_entry() - XA not open!");
        return XAER_RMERR;
    }
    
out:
    
    return XA_OK;
}

/**
 * Rollback
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_rollback_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    return XA_OK;
}

/**
 * prepare
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_prepare_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    return XA_OK;
}

/**
 * Commit
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_commit_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    return XA_OK;
}

/**
 * Return list of trans
 * @param sw
 * @param xid
 * @param count
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_recover_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    return 0; /* 0 transactions found... */
}

/**
 * Forget tran
 * @param sw
 * @param xid
 * @param rmid
 * @param flags
 * @return 
 */
expublic int ndrx_nul_xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
   if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_forget_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "xa_forget_entry() - not implemented!!");
    return XA_OK;
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
expublic int ndrx_nul_xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
{
    if (!G_atmi_tls->tmnull_is_open)
    {
        NDRX_LOG(log_error, "xa_complete_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "xa_complete_entry() - not using!!");
    return XAER_RMERR;
}


/* Static entries */
expublic int ndrx_nul_xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return ndrx_nul_xa_open_entry(&tmnull_switch, xa_info, rmid, flags);
}
expublic int ndrx_nul_xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return ndrx_nul_xa_close_entry(&tmnull_switch, xa_info, rmid, flags);
}
expublic int ndrx_nul_xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_start_entry(&tmnull_switch, xid, rmid, flags);
}

expublic int ndrx_nul_xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_end_entry(&tmnull_switch, xid, rmid, flags);
}
expublic int ndrx_nul_xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_rollback_entry(&tmnull_switch, xid, rmid, flags);
}
expublic int ndrx_nul_xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_prepare_entry(&tmnull_switch, xid, rmid, flags);
}

expublic int ndrx_nul_xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_commit_entry(&tmnull_switch, xid, rmid, flags);
}

expublic int ndrx_nul_xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return ndrx_nul_xa_recover_entry(&tmnull_switch, xid, count, rmid, flags);
}
expublic int ndrx_nul_xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return ndrx_nul_xa_forget_entry(&tmnull_switch, xid, rmid, flags);
}
expublic int ndrx_nul_xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return ndrx_nul_xa_complete_entry(&tmnull_switch, handle, retval, rmid, flags);
}

/* vim: set ts=4 sw=4 et smartindent: */
