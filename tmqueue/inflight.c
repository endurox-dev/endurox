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
 * @return NOTE! migth return error code
 */
expublic int ndrx_infl_addmsg(tmq_qconfig_t * qconf, tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    int isNew = EXFALSE;
    char corrid_str[TMCORRIDLEN_STR+1];

    if (mmsg->msg->lockthreadid)
    {
        CDL_APPEND(qhash->q_infligh, mmsg);
        mmsg->qstate = NDRX_TMQ_LOC_INFL;
    }
    else if ( (mmsg->msg->qctl.flags & TPQTIME_ABS) && 
            (mmsg->msg->qctl.deq_time >= (long)time(NULL)))
    {
        ndrx_rbt_insert(qhash->q_fut, (ndrx_rbt_node_t *)mmsg, &isNew);
        mmsg->qstate = NDRX_TMQ_LOC_FUTQ;
    }
    else
    {
        /* insert to cur */
        ndrx_rbt_insert(qhash->q, (ndrx_rbt_node_t *)mmsg, &isNew);
        mmsg->qstate = NDRX_TMQ_LOC_CURQ;

        if (mmsg->msg->qctl.flags & TPQCORRID)
        {
            tmq_msgid_serialize(mmsg->msg->qctl.corrid, corrid_str);
            NDRX_STRCPY_SAFE(mmsg->corrid_str, corrid_str);

            NDRX_LOG(log_debug, "Adding to corrid_hash [%s] of queue [%s]",
                corrid_str,mmsg->msg->hdr.qname);

            if (EXSUCCEED!=tmq_cor_msg_add(qconf, qhash, mmsg))
            {
                NDRX_LOG(log_error, "Failed to add msg to corhash!");
                EXFAIL_OUT(ret);
            }
            mmsg->qstate |= NDRX_TMQ_LOC_CORQ;
        }
    }
out:
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


/**
 * Remove message from Qs
 * NOTE that context must be with M_q_lock locked.
 */
expublic int ndrx_infl_delmsg(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;

    if (mmsg->qstate & NDRX_TMQ_LOC_CORQ)
    {
        /* remove from correlator Q */
        tmq_cor_msg_del(qhash, mmsg);
    }

    if (mmsg->qstate & NDRX_TMQ_LOC_INFL)
    {
        CDL_DELETE(qhash->q_infligh, mmsg);
    }
    else if (mmsg->qstate & NDRX_TMQ_LOC_FUTQ)
    {
        ndrx_rbt_delete(qhash->q_fut, (ndrx_rbt_node_t *)mmsg);
    }
    else if (mmsg->qstate & NDRX_TMQ_LOC_CURQ)
    {
        ndrx_rbt_delete(qhash->q, (ndrx_rbt_node_t *)mmsg);
    }

    return ret;
}
