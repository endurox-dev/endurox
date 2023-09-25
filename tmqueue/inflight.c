/**
 * @brief Infligth support routines, to manage inflight messages
 *
 * @file inflight.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <ndebug.h>
#include <exhash.h>
#include <atmi.h>

#include "tmqd.h"
#include <utlist.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Add message to inflight or (cur + cor )/fut (depending on p_msg->lockthreadid)
 * @param p_msg message to add
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_infl_addmsg(tmq_qhash_t *qhash, tmq_memmsg_t *p_mmsg)
{
    int ret = EXSUCCEED;

    
}

/** 
 * Move message from (cur+cor)/fut to inflight. 
 * This removes message from cur/fut+cor and adds to inflight
 * @param p_msg message to move
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_infl_mov2infl(tmq_qhash_t *qhash, tmq_memmsg_t *p_mmsg)
{
    int ret = EXSUCCEED;

}

/**
 * Move message from inflight to (cur+cor)/fut (in case if doing rollback) 
 * @param p_msg message to move
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_infl_mov2cur(tmq_qhash_t *qhash, tmq_memmsg_t *p_mmsg)
{
    int ret = EXSUCCEED;

}

/**
 * Remove message from inflight / (cur+cor)/fut depending on context 
 * @param p_msg message to remove
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_infl_delmsg(tmq_qhash_t *qhash, tmq_memmsg_t *p_mmsg)
{
    int ret = EXSUCCEED;

    /* Delete message from infligth queue */
    if (p_mmsg->flags & NDRX_TMQ_LOC_INFL)
    {
        CDL_DELETE(qhash->q_infligh, p_mmsg);
    }

    /* Delete current message rbt node from qhash rbt q tree */
    if (p_mmsg->flags & NDRX_TMQ_LOC_CURQ)
    {
        // ndrx_rbt_delete(qhash->q, &p_mmsg->cur);
    }
    return ret;
}
