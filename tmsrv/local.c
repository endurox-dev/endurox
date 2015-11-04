/* 
** Routines related with local RM management.
**
** @file local.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
#include <uuid/uuid.h>
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
public int tm_prepare_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    
    /* we should start new transaction... */
    if (SUCCEED!=(ret = atmi_xa_prepare_entry(atmi_xa_get_branch_xid(p_xai),
            0)))
    {
        NDRX_LOG(log_error, "Failed to prepare local transaction!");        
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, TPEABORT, ret, 
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
public int tm_prepare_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMPREPARE, TRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return FAIL;
    else
    {
        tpfree((char *)p_ub);
        return SUCCEED;
    }
}

/**
 * Combined prepare of to commit. Selects automatically do the local prepare or
 * remote depending on RMID.
 */
public int tm_prepare_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = SUCCEED;
    
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
public int tm_rollback_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    
    /* we should start new transaction... */
    if (SUCCEED!=(ret = atmi_xa_rollback_entry(atmi_xa_get_branch_xid(p_xai), 
            0)))
    {
        NDRX_LOG(log_error, "Failed to abort transaction!");
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, TPETRAN, ret, 
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
public int tm_rollback_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMABORT, TRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return FAIL;
    else
    {
        tpfree((char *)p_ub);
        return SUCCEED;
    }
}

/**
 * Combined rollback of to commit. Selects automatically do the local rollback or
 * remote depending on RMID.
 */
public int tm_rollback_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = SUCCEED;
    
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
public int tm_commit_local(UBFH *p_ub, atmi_xa_tx_info_t *p_xai)
{
    int ret = SUCCEED;
    
    /* we should start new transaction... */
    if (SUCCEED!=(ret = atmi_xa_commit_entry(atmi_xa_get_branch_xid(p_xai), 0)))
    {
        NDRX_LOG(log_error, "Failed to abort transaction!");
        
        if (NULL!=p_ub)
        {
            atmi_xa_set_error_fmt(p_ub, TPEHEURISTIC, NDRX_XA_ERSN_NONE, 
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
public int tm_commit_remote_call(atmi_xa_tx_info_t *p_xai, short rmid)
{
    UBFH* p_ub;
            
    /* Call the remote TM.
     * TODO: How about error handling? 
     */
    p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMCOMMIT, TRUE, rmid, p_xai);

    if (NULL==p_ub)    
        return FAIL;
    else
    {
        tpfree((char *)p_ub);
        return SUCCEED;
    }
}

/**
 * Combined commit of to commit. Selects automatically do the local commit or
 * remote depending on RMID.
 */
public int tm_commit_combined(atmi_xa_tx_info_t *p_xai, short rmid)
{
    int ret = SUCCEED;
    
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
