/**
 * @brief Red-Black tree support routines for future Q, cur Q, cor Q
 *  Note that delete is not implemented for trees, as tmq_memmsg_t is deleted by
 *  parent processes.
 *
 * @file rbtsupp.c
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
 * Found duplicate entry in current
 * print error and abort
 */
expublic void tmq_rbt_combine_cur(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg)
{
    tmq_memmsg_t *existing_msg = (tmq_memmsg_t *)existing;
    tmq_memmsg_t *newdata_msg = (tmq_memmsg_t *)newdata;
    char tmp[256];

    /* NOTE: Clock skew might happen, time is moved backwards and we get duplicate sequeuences. */
    snprintf(tmp, sizeof(tmp), "ERROR ! (cur) Two messages with the same key in current Q! "
            "Existing msgid [%s] new msgid [%s] dup key: msgtstamp=%ld, "
            "msgtstamp_usec=%ld, msgtstamp_cntr=%d. Clock skew?",
            existing_msg->msgid_str, newdata_msg->msgid_str, existing_msg->msg->msgtstamp, 
            existing_msg->msg->msgtstamp_usec, 
            existing_msg->msg->msgtstamp_cntr);
    NDRX_LOG(log_error, "%s", tmp);
    userlog("%s", tmp);
    abort();
}

/**
 * Found duplicate entry in correlator
 * print error and abort
 */
expublic void tmq_rbt_combine_fut(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg)
{
    tmq_memmsg_t *existing_msg = (tmq_memmsg_t *)existing;
    tmq_memmsg_t *newdata_msg = (tmq_memmsg_t *)newdata;
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "ERROR ! (fut) Two messages with the same key in future Q! "
            "Existing msgid [%s] new msgid [%s] dup key: deq_time=%ld, msgtstamp=%ld, "
            "msgtstamp_usec=%ld, msgtstamp_cntr=%d. Clock skew?",
            existing_msg->msgid_str, newdata_msg->msgid_str, 
            existing_msg->msg->qctl.deq_time,
            existing_msg->msg->msgtstamp, 
            existing_msg->msg->msgtstamp_usec, 
            existing_msg->msg->msgtstamp_cntr);
    NDRX_LOG(log_error, "%s", tmp);
    userlog("%s", tmp);
    abort();
}

/**
 * Got duplicate messages in correlation Q
 */
expublic void tmq_rbt_combine_cor(ndrx_rbt_node_t *existing, const ndrx_rbt_node_t *newdata, void *arg)
{
    tmq_memmsg_t *existing_msg = TMQ_COR_GETMSG(existing);
    tmq_memmsg_t *newdata_msg = TMQ_COR_GETMSG(newdata);
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "ERROR ! (cor) Two messages with the same key in correlation Q! "
            "Existing msgid [%s] new msgid [%s] dup key: msgtstamp=%ld, msgtstamp_usec=%ld, "
            "msgtstamp_cntr=%d. Clock skew?",
            existing_msg->msgid_str, newdata_msg->msgid_str, existing_msg->msg->msgtstamp, 
            existing_msg->msg->msgtstamp_usec, 
            existing_msg->msg->msgtstamp_cntr);
    NDRX_LOG(log_error, "%s", tmp);
    userlog("%s", tmp);
    abort();
}

/**
 * Compare current Q. This is based 
 */
expublic int tmq_rbt_cmp_cur(const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg)
{
    tmq_memmsg_t *aa = (tmq_memmsg_t *)a;
    tmq_memmsg_t *bb = (tmq_memmsg_t *)b;

    return ndrx_compare3(aa->msg->msgtstamp, aa->msg->msgtstamp_usec, aa->msg->msgtstamp_cntr,
            bb->msg->msgtstamp, bb->msg->msgtstamp_usec, bb->msg->msgtstamp_cntr);
}

/**
 * Compare elements from the correlator Q
 */
expublic int tmq_rbt_cmp_cor(const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg)
{
    tmq_memmsg_t *aa = TMQ_COR_GETMSG(a);
    tmq_memmsg_t *bb = TMQ_COR_GETMSG(b);

    return ndrx_compare3(aa->msg->msgtstamp, aa->msg->msgtstamp_usec, aa->msg->msgtstamp_cntr,
            bb->msg->msgtstamp, bb->msg->msgtstamp_usec, bb->msg->msgtstamp_cntr);
}

/**
 * Compare elements of the future Q.
 * If doing the re-scheduling of the failed forward/automatic message, will use
 * the same deq_time to schedule the next attempt for the message.
 */
expublic int tmq_rbt_cmp_fut (const ndrx_rbt_node_t *a, const ndrx_rbt_node_t *b, void *arg)
{
    /* future Q compare */
    tmq_memmsg_t *aa = (tmq_memmsg_t *)a;
    tmq_memmsg_t *bb = (tmq_memmsg_t *)b;

    return ndrx_compare4(aa->msg->qctl.deq_time, aa->msg->msgtstamp, 
                         aa->msg->msgtstamp_usec, aa->msg->msgtstamp_cntr,
                         bb->msg->qctl.deq_time, bb->msg->msgtstamp, 
                         bb->msg->msgtstamp_usec, bb->msg->msgtstamp_cntr);
}


/* vim: set ts=4 sw=4 et smartindent: */
