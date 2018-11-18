/**
 * @brief Routines related with local RM management.
 *
 * @file local.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <tperror.h>
#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmsrv.h"
#include "../libatmisrv/srv_int.h"
#include <xa_cmn.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/******************************************************************************/
/*                         PREPARE SECTION                                    */
/******************************************************************************/
/**
 * Prepare local transaction.
 * @param p_xai
 * @return 
 */
expublic int tm_prepare_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    
    /* we should start new transaction... */
    if (EXSUCCEED!=(ret = atmi_xa_prepare_entry(atmi_xa_get_branch_xid(p_xai),
            0)))
    {
        NDRX_LOG(log_error, "Failed to prepare local transaction!");        
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, tperrno, atmi_xa_get_reason(), 
                    "Failed to prepare local transaction, "
                    "xa error: %d [%s]", ret, atmi_xa_geterrstr(ret));
        }
        /* ATMI error already set by lib */
        goto out;
    }
    
out:
    return ret;
}

/**
 * Do remote prepare call
 * @param p_xai
 * @return SUCCEED/FAIL
 */
expublic int tm_prepare_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMPREPARE, EXTRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return EXFAIL;
    else
    {
        tpfree((char *)p_ub);
        return EXSUCCEED;
    }
}

/**
 * Combined prepare of to commit. Selects automatically do the local prepare or
 * remote depending on RMID.
 */
expublic int tm_prepare_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = EXSUCCEED;
    
    /* Check is this local */
    if (rmid == G_atmi_env.xa_rmid)
    {
        ret  = tm_prepare_local(NULL, p_xai);
    }
    else
    {
        ret = tm_prepare_remote_call(p_xai, rmid);
    }
    
out:
    return ret;
}

/******************************************************************************/
/*                         ROLLBACK SECTION                                   */
/******************************************************************************/
/**
 * Abort current RM transaction
 * @param p_xai
 * @return XA error code.
 */
expublic int tm_rollback_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    
    /* we should start new transaction... */
    if (EXSUCCEED!=(ret = atmi_xa_rollback_entry(atmi_xa_get_branch_xid(p_xai), 
            0)))
    {
        NDRX_LOG(log_error, "Failed to abort transaction!");
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, tperrno, atmi_xa_get_reason(), 
                    "Failed to abort transaction, "
                    "xa error: %d [%s]", ret, atmi_xa_geterrstr(ret));
        }
        /* ATMI error already set by lib */
        goto out;
    }
    
out:
    return ret;
}

/**
 * Do remote rollback call
 * @param p_xai
 * @return SUCCEED/FAIL
 */
expublic int tm_rollback_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMABORT, EXTRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return EXFAIL;
    else
    {
        tpfree((char *)p_ub);
        return EXSUCCEED;
    }
}

/**
 * Combined rollback of to commit. Selects automatically do the local rollback or
 * remote depending on RMID.
 */
expublic int tm_rollback_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = EXSUCCEED;
    
    /* Check is this local */
    if (rmid == G_atmi_env.xa_rmid)
    {
        ret  = tm_rollback_local(NULL, p_xai);
    }
    else
    {
        ret = tm_rollback_remote_call(p_xai, rmid);
    }
    
out:
    return ret;
}


/******************************************************************************/
/*                         COMMIT SECTION                                   */
/******************************************************************************/
/**
 * Abort current RM transaction
 * @param p_xai
 * @return XA error code.
 */
expublic int tm_commit_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = EXSUCCEED;
    
    /* we should start new transaction... */
    if (EXSUCCEED!=(ret = atmi_xa_commit_entry(atmi_xa_get_branch_xid(p_xai), 0)))
    {
        NDRX_LOG(log_error, "Failed to commit transaction!");
        
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, tperrno, atmi_xa_get_reason(), 
                    "Failed to commit transaction, "
                    "xa error: %d [%s]", ret, atmi_xa_geterrstr(ret));
        }
        /* ATMI error already set by lib */
        goto out;
    }
    
out:
    return ret;
}

/**
 * Do remote commit call
 * @param p_xai
 * @return SUCCEED/FAIL
 */
expublic int tm_commit_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMCOMMIT, EXTRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return EXFAIL;
    else
    {
        tpfree((char *)p_ub);
        return EXSUCCEED;
    }
}

/**
 * Combined commit of to commit. Selects automatically do the local commit or
 * remote depending on RMID.
 */
expublic int tm_commit_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = EXSUCCEED;
    
    /* Check is this local */
    if (rmid == G_atmi_env.xa_rmid)
    {
        ret  = tm_commit_local(NULL, p_xai);
    }
    else
    {
        ret = tm_commit_remote_call(p_xai, rmid);
    }
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
