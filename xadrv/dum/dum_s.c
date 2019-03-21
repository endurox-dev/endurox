/**
 * @brief Load dum drivers - static version
 *
 * @file dum_s.c
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
#include <atmi_int.h>
#include <sys_mqueue.h>

#include "atmi_shm.h"

#include <xa.h>
#include "dum_common.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate __thread int M_is_open = EXFALSE;
exprivate __thread int M_rmid = EXFAIL;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


expublic int xa_open_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags);
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_end_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags);
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags);

expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags);
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags);
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags);
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags);

struct xa_switch_t ndrxdumssw = 
{ 
    .name = "ndrxdumssw",
    .flags = TMNOFLAGS,
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
 * Open API
 * @param sw
 * @param xa_info
 * @param rmid
 * @param flags
 * @return 
 */
expublic int xa_open_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    if (M_is_open)
    {
        NDRX_LOG(log_error, "xa_open_entry() - already open!");
        return XAER_RMERR;
    }
    M_is_open = EXTRUE;
    M_rmid = rmid;
             
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
expublic int xa_close_entry(struct xa_switch_t *sw, char *xa_info, int rmid, long flags)
{
    NDRX_LOG(log_error, "xa_close_entry() called");
    
    M_is_open = EXFALSE;
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
expublic int xa_start_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_end_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_rollback_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_prepare_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_commit_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_recover_entry(struct xa_switch_t *sw, XID *xid, long count, int rmid, long flags)
{
    if (!M_is_open)
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
expublic int xa_forget_entry(struct xa_switch_t *sw, XID *xid, int rmid, long flags)
{
   if (!M_is_open)
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
expublic int xa_complete_entry(struct xa_switch_t *sw, int *handle, int *retval, int rmid, long flags)
{
    if (!M_is_open)
    {
        NDRX_LOG(log_error, "xa_complete_entry() - XA not open!");
        return XAER_RMERR;
    }
    
    NDRX_LOG(log_error, "xa_complete_entry() - not using!!");
    return XAER_RMERR;
}


/* Static entries */
expublic int xa_open_entry_stat( char *xa_info, int rmid, long flags)
{
    return xa_open_entry(&ndrxdumssw, xa_info, rmid, flags);
}
expublic int xa_close_entry_stat(char *xa_info, int rmid, long flags)
{
    return xa_close_entry(&ndrxdumssw, xa_info, rmid, flags);
}
expublic int xa_start_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_start_entry(&ndrxdumssw, xid, rmid, flags);
}

expublic int xa_end_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_end_entry(&ndrxdumssw, xid, rmid, flags);
}
expublic int xa_rollback_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_rollback_entry(&ndrxdumssw, xid, rmid, flags);
}
expublic int xa_prepare_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_prepare_entry(&ndrxdumssw, xid, rmid, flags);
}

expublic int xa_commit_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_commit_entry(&ndrxdumssw, xid, rmid, flags);
}

expublic int xa_recover_entry_stat(XID *xid, long count, int rmid, long flags)
{
    return xa_recover_entry(&ndrxdumssw, xid, count, rmid, flags);
}
expublic int xa_forget_entry_stat(XID *xid, int rmid, long flags)
{
    return xa_forget_entry(&ndrxdumssw, xid, rmid, flags);
}
expublic int xa_complete_entry_stat(int *handle, int *retval, int rmid, long flags)
{
    return xa_complete_entry(&ndrxdumssw, handle, retval, rmid, flags);
}

/**
 * API entry of loading the driver
 * @param symbol
 * @param descr
 * @return XA switch or null
 */
struct xa_switch_t *ndrx_get_xa_switch(void)
{
    return &ndrxdumssw;
}

/* vim: set ts=4 sw=4 et smartindent: */
