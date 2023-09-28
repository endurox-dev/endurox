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
 * @param qconf queue config
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

    mmsg->qconf=qconf;
    mmsg->qhash=qhash;

    if (mmsg->msg->lockthreadid)
    {
        CDL_APPEND(qhash->q_infligh, mmsg);
        mmsg->qstate = NDRX_TMQ_LOC_INFL;
    }
    else if ( (mmsg->msg->qctl.flags & TPQTIME_ABS) && 
            (mmsg->msg->qctl.deq_time > (long)time(NULL)))
    {
        ndrx_rbt_insert(&qhash->q_fut, (ndrx_rbt_node_t *)mmsg, &isNew);
        mmsg->qstate = NDRX_TMQ_LOC_FUTQ;
    }
    else
    {
        /* insert to cur */
        ndrx_rbt_insert(&qhash->q, (ndrx_rbt_node_t *)mmsg, &isNew);
        mmsg->qstate = NDRX_TMQ_LOC_CURQ;

        if (mmsg->msg->qctl.flags & TPQCORRID)
        {
            tmq_msgid_serialize(mmsg->msg->qctl.corrid, corrid_str);
            NDRX_STRCPY_SAFE(mmsg->corrid_str, corrid_str);

            NDRX_LOG(log_debug, "Adding to corrid_hash [%s] of queue [%s]",
                corrid_str,mmsg->msg->hdr.qname);

            if (EXSUCCEED!=tmq_cor_msg_add(mmsg))
            {
                NDRX_LOG(log_error, "Failed to add msg to corhash!");
                EXFAIL_OUT(ret);
            }
            mmsg->qstate |= NDRX_TMQ_LOC_CORQ;
        }
    }
    /* add message to G_msgid_hash */

    /*  mmsg->msgid_str must be setup too! */
    EXHASH_ADD_STR( G_msgid_hash, msgid_str, mmsg);
    mmsg->qstate |= NDRX_TMQ_LOC_MSGIDHASH;

out:
    return ret;
}

/**
 * Move message from current/cur/fut to inflight
 * NOTE that context must be with M_q_lock locked.
 */
expublic int ndrx_infl_mov2infl(tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;

    if (mmsg->qstate & NDRX_TMQ_LOC_FUTQ)
    {
        ndrx_rbt_delete(&mmsg->qhash->q_fut, (ndrx_rbt_node_t *)mmsg);
        mmsg->qstate &= ~NDRX_TMQ_LOC_FUTQ;
    }
    else if (mmsg->qstate & NDRX_TMQ_LOC_CURQ)
    {
        ndrx_rbt_delete(&mmsg->qhash->q, (ndrx_rbt_node_t *)mmsg);
        mmsg->qstate &= ~NDRX_TMQ_LOC_CURQ;

        /* remove from correlator too */
        if (mmsg->qstate & NDRX_TMQ_LOC_CORQ)
        {
            tmq_cor_msg_del(mmsg);
            mmsg->qstate &= ~NDRX_TMQ_LOC_CORQ;
        }
    }
    else
    {
        NDRX_LOG(log_error, "Message [%s] is not in cur/fut queue!",
                mmsg->msgid_str);
        userlog("Cannot move to inflight Q: messsage [%s] is not in cur/fut queue (state=%hd)!",
                mmsg->msgid_str, mmsg->qstate);
        /*EXFAIL_OUT(ret);*/
        abort();
    }

    /* add message to inflight */
    CDL_APPEND(mmsg->qhash->q_infligh, mmsg);
    mmsg->qstate |= NDRX_TMQ_LOC_INFL;

out:
    return ret;
}

/**
 * Move message from inflight to current/cur/fut
 * NOTE that context must be with M_q_lock locked.
 * @param qconf queue config
 * @param qhash queue hash
 * @param mmsg message to move
 * @return EXSUCCEED/EXFAIL 
 */
expublic int ndrx_infl_mov2cur(tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;

    if (mmsg->qstate & NDRX_TMQ_LOC_INFL)
    {
        CDL_DELETE(mmsg->qhash->q_infligh, mmsg);
        mmsg->qstate &= ~NDRX_TMQ_LOC_INFL;

        /* enqueue to cur/cor or fut */
        if ( (mmsg->msg->qctl.flags & TPQTIME_ABS) && 
                (mmsg->msg->qctl.deq_time > (long)time(NULL)))
        {
            ndrx_rbt_insert(&mmsg->qhash->q_fut, (ndrx_rbt_node_t *)mmsg, NULL);
            mmsg->qstate |= NDRX_TMQ_LOC_FUTQ;
        }
        else
        {
            /* insert to cur */
            ndrx_rbt_insert(&mmsg->qhash->q, (ndrx_rbt_node_t *)mmsg, NULL);
            mmsg->qstate |= NDRX_TMQ_LOC_CURQ;

            if (mmsg->msg->qctl.flags & TPQCORRID)
            {
                if (EXSUCCEED!=tmq_cor_msg_add(mmsg))
                {
                    NDRX_LOG(log_error, "Failed to add msg [%s] to corhash!",
                            mmsg->msgid_str);
                    EXFAIL_OUT(ret);
                }
                mmsg->qstate |= NDRX_TMQ_LOC_CORQ;
            }
        }
    }
    else
    {
        NDRX_LOG(log_error, "Message [%s] is not in inflight queue!",
                mmsg->msgid_str);
        userlog("Cannot move to cur Q: messsage [%s] is not in inflight queue (state=%hd)!",
                mmsg->msgid_str, mmsg->qstate);
        abort();
    }

out:
    return ret;
}


/**
 * Remove message from Qs
 * @param qhash queue hash
 * @param mmsg message to remove
 * NOTE that context must be with M_q_lock locked.
 */
expublic int ndrx_infl_delmsg(tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;

    if (mmsg->qstate & NDRX_TMQ_LOC_CORQ)
    {
        /* remove from correlator Q */
        tmq_cor_msg_del(mmsg);
    }

    if (mmsg->qstate & NDRX_TMQ_LOC_MSGIDHASH)
    {
        EXHASH_DEL(G_msgid_hash, mmsg);
    }

    if (mmsg->qstate & NDRX_TMQ_LOC_INFL)
    {
        CDL_DELETE(mmsg->qhash->q_infligh, mmsg);
    }

    if (mmsg->qstate & NDRX_TMQ_LOC_FUTQ)
    {
        ndrx_rbt_delete(&mmsg->qhash->q_fut, (ndrx_rbt_node_t *)mmsg);
    }

    if (mmsg->qstate & NDRX_TMQ_LOC_CURQ)
    {
        /* really shall not happenn.... firstly it shall go to inflight... */
        ndrx_rbt_delete(&mmsg->qhash->q, (ndrx_rbt_node_t *)mmsg);
    }

    mmsg->qstate=0;

    return ret;
}

/**
 * Move message from future to cur or/and cor
 * @param qhash queue hash
 */
expublic int ndrx_infl_fut2cur(tmq_qhash_t *qhash)
{
    int ret = EXSUCCEED;
    tmq_memmsg_t *mmsg = NULL;
    int isNew = EXFALSE;

    /* read from q_fut tree with smallest dec_time */
    mmsg = (tmq_memmsg_t*)ndrx_rbt_leftmost(&qhash->q_fut);

    /* no message in future */
    if (NULL==mmsg)
    {
        goto out;
    }

    /* enqueue to cur and cor if needed */
    if ( mmsg->msg->qctl.deq_time <= (long)time(NULL) )
    {
        NDRX_LOG(log_debug, "Moving [%s] from future message list",
                mmsg->msgid_str);

        /* remove from future */
        ndrx_rbt_delete(&mmsg->qhash->q_fut, (ndrx_rbt_node_t *)mmsg);
        mmsg->qstate &= ~NDRX_TMQ_LOC_FUTQ;

        /* insert to cur */
        ndrx_rbt_insert(&mmsg->qhash->q, (ndrx_rbt_node_t *)mmsg, &isNew);
        mmsg->qstate |= NDRX_TMQ_LOC_CURQ;

        /* do the correlator too... if needed */
        if (mmsg->msg->qctl.flags & TPQCORRID)
        {
            /* insert to cor */
            if (EXSUCCEED!=tmq_cor_msg_add(mmsg))
            {
                NDRX_LOG(log_error, "Failed to add msg [%s] to corhash!",
                        mmsg->msgid_str);
                EXFAIL_OUT(ret);
            }
            mmsg->qstate |= NDRX_TMQ_LOC_CORQ;
        }
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
