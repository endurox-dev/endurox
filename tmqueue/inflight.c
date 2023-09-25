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
#include <rbtree.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * @brief Add message to inflight or (cur + cor )/fut (depending on p_msg->flags)
 *
 * @param qhash queue hash
 * @param p_msg message to add
 *
 * @return EXSUCCEED/EXFAIL
 */
int ndrx_infl_addmsg(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    int isNew = EXFALSE;

    /* check if message is for future time */
    if ( p_msg->flags & NDRX_TMQ_LOC_FUT )
    {
        /* add message only in future queue */
        mmsg->cur = ndrx_rbt_insert(&qhash->q_fut, mmsg->cur, &isNew);
    }

    return ret;
}

int ndrx_infl_mov2infl(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    return ret;
}

int ndrx_infl_mov2cur(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    return ret;
}

int ndrx_infl_del(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    return ret;
}
