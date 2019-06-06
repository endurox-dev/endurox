/**
 * @brief Transaction Branch ID handler.
 *  Fore each RM there is seperate TID counter.
 *  The TIDs start with 0 which is default.
 *
 * @file btid.c
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

/**
 * This assumes big transaction is locked.
 * This just gets new branch qualifier
 * @param rmid resource manager id
 * @return new TID
 */
expublic long tms_btid_gettid(atmi_xa_log_t *p_tl, short rmid)
{
    long ret = p_tl->rmstatus[rmid-1].tidcounter;
    
    p_tl->rmstatus[rmid-1].tidcounter++;
    
    NDRX_LOG(log_info, "New Branch TID %ld for rmid %hd", ret, rmid);
    
    return ret;
}

/**
 * Find branch transaction by id
 * @param[in] p_tl Transaction entry
 * @param[in] rmid Resource manager id
 * @param[in] btid transaction id
 * @return transaction branch or NULL if not found
 */
expublic atmi_xa_rm_status_btid_t *tms_btid_find(atmi_xa_log_t *p_tl, 
        short rmid, long btid)
{
    atmi_xa_rm_status_btid_t *ret = NULL;
    
    EXHASH_FIND_LONG(p_tl->rmstatus[rmid-1].btid_hash, (&btid), ret);
    
    return ret;
}


/**
 * Add transaction branch to whole transaction
 * @param[in] p_tl transaction obj
 * @param[in] rmid resource manager id
 * @param[in] btid branch transaction id
 * @param[in] rmstatus Resource status
 * @param[in] rmerrorcode last error reported for btid
 * @param[in] rmreason reason code reported for btid
 * @param[out] bt pointer to allocated TID
 * @return EXSUCCEED/EXFAIL
 */    
expublic int tms_btid_add(atmi_xa_log_t *p_tl, short rmid, 
            long btid, char rmstatus, int  rmerrorcode, short rmreason,
            atmi_xa_rm_status_btid_t **bt)
 {
    int ret = EXSUCCEED;
    *bt = NDRX_MALLOC(sizeof(atmi_xa_rm_status_btid_t));
    
    if (NULL==*bt)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
                sizeof(atmi_xa_rm_status_btid_t), strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    (*bt)->rmid = rmid;
    (*bt)->btid = btid;
    (*bt)->rmstatus = rmstatus;
    (*bt)->rmerrorcode = rmerrorcode;
    (*bt)->rmreason = rmreason;
    
    EXHASH_ADD_LONG((p_tl->rmstatus[rmid-1].btid_hash), btid, (*bt));
    
    /* If end point self assigned transaction ID,
     * then step up the counter.
     * This might happen only if join is used and process have used
     * 0 tid 
     */
    if (p_tl->rmstatus[rmid-1].tidcounter <= btid)
    {
        p_tl->rmstatus[rmid-1].tidcounter = btid + 1;
    }
    
out:
    return ret;
}

/**
 * Add or update branch tid entry
 * @param[in] p_tl transaction object
 * @param[in] rmid resource manager id, starting from 1
 * @param[in,out] btid branch tid
 * @param[in] rmstatus branch tid status
 * @param[in] rmerrorcode branch tid error code
 * @param[in] rmreason branch tid reasoncode
 * @param[out] exists did record exist? EXTRUE/EXFAIL
 * @param[out] bt btranch transaction object ptr output
 * @return EXSUCCEED/EXFAIL
 */
expublic int tms_btid_addupd(atmi_xa_log_t *p_tl, short rmid, 
            long *btid, char rmstatus, int  rmerrorcode, short rmreason,
            int *exists, atmi_xa_rm_status_btid_t **bt)
{
    int ret = EXSUCCEED;
    
    if (EXFAIL!=*btid)
    {   
        *bt = tms_btid_find(p_tl, rmid, *btid);
    }
    
    if (NULL!=(*bt))
    {
        /* only if new status is higher than old status
         * used by registering transaction
         */
        if (rmstatus > (*bt)->rmstatus)
        {
            (*bt)->rmstatus = rmstatus;
            (*bt)->rmerrorcode = rmerrorcode;
            (*bt)->rmreason = rmreason;
        }
        
        if (NULL!=exists)
        {
            *exists = EXTRUE;
        }
    }
    else
    {
        if (EXFAIL==*btid)
        {
            /* generate new TID */
            *btid = tms_btid_gettid(p_tl, rmid);
        }
        
        ret = tms_btid_add(p_tl, rmid, *btid, rmstatus, rmerrorcode, rmreason, bt);
        
        if (NULL!=exists)
        {
            *exists = EXFALSE;
        }
        
    }
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
