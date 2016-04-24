/* 
** Tmqueue server - transaction monitor
** After that log transaction to hash & to disk for tracking the stuff...
** TODO: We should have similar control like "TP_COMMIT_CONTROL" -
** either return after stuff logged or after really commit completed.
** Error handling:
** - System errors we will track via ATMI interface error functions
** - XA errors will be tracked via XA error interface
**
** @file tmqapi.c
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

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqueue.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include <qcommon.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/******************************************************************************/
/*                               Q API COMMANDS                               */
/******************************************************************************/

/**
 * Do the internal commit of transaction (request sent from other TM)
 * @param p_ub
 * @return 
 */
public int tmq_enqueue(UBFH *p_ub)
{
    int ret = SUCCEED;
    tmq_msg_t *p_msg = NULL;
    char *data = NULL;
    BFLDLEN len = 0;
    /* Add message to Q */
    NDRX_LOG(log_debug, "Into tmq_enqueue()");
    
    if (NULL==(data = Bgetalloc(p_ub, EX_DATA, 0, &len)))
    {
        NDRX_LOG(log_error, "Missing EX_DATA!");
        userlog("Missing EX_DATA!");
        FAIL_OUT(ret);
    }
    
    /*
     * Get the message size in EX_DATA
     */
    p_msg = malloc(sizeof(tmq_msg_t)+len);
    
    if (NULL==p_msg)
    {
        NDRX_LOG(log_error, "Failed to malloc tmq_msg_t!");
        userlog("Failed to malloc tmq_msg_t!");
        FAIL_OUT(ret);
    }
    
    memset(p_msg, 0, sizeof(tmq_msg_t));
    
    memcpy(p_msg->msg, data, len);
    p_msg->len = len;    
    
    NDRX_DUMP(log_debug, "Got message for Q: ", p_msg->msg, p_msg->len);
    
    /* Restore back the C structure */
    if (SUCCEED!=tmq_tpqctl_from_ubf_enqreq(p_ub, &p_msg->qctl))
    {
        _TPset_error_msg(TPEINVAL,  "tmq_enqueue: failed convert ctl "
                "to internal UBF buf!");
        FAIL_OUT(ret);
    }
    
    /* Build up the message. */
    tmq_setup_cmdheader_newmsg(&p_msg->hdr);
    
    /*
     *  TODO: Setup of:
     * unsigned char status;
     * long trycounter;
     * long long timestamp;
     * long long trytstamp;
     */
    
    p_msg->lockthreadid = ndrx_gettid(); /* Mark as locked by thread */
    
out:
    return ret;
}

/**
 * Local transaction branch abort command. Sent from Master TM
 * @param p_ub
 * @return 
 */
public int tmq_dequeue(UBFH *p_ub)
{
    int ret = SUCCEED;
    
out:
    return ret;
}

/******************************************************************************/
/*                         COMMAND LINE API                                   */
/******************************************************************************/
/**
 * Return list of transactions
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
public int tmq_printqueue(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
        
    /* Bfprint(p_ub, stderr); */

    if (FAIL == tpsend(cd,
                        (char *)p_ub,
                        0L,
                        0,
                        &revent))
    {
        NDRX_LOG(log_error, "Send data failed [%s] %ld",
                            tpstrerror(tperrno), revent);
        FAIL_OUT(ret);
    }
    else
    {
        NDRX_LOG(log_debug,"sent ok");
    }

    
out:

    return ret;
}
