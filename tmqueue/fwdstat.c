/**
 * @brief Forward statistics used for QoS
 *
 * @file fwdstat.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
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

/** Lock for statistics hash */
exprivate MUTEX_LOCKDECL(M_statsh_lock);

/** statistics hash by it self */
exprivate fwd_stats_t *M_statsh = NULL;

/*---------------------------Prototypes---------------------------------*/

/**
 * Init the statistics engine
 */
expublic int tmq_fwd_stat_init(void)
{   
    return EXSUCCEED;
}

/**
 * Get forward busy count per queue
 * Allocate the handle here, if missing..
 * @param[in] qname queue name
 * @param[out] p_stats stats pointer out
 * @return Number of messages in forward threads
 */
expublic int tmq_fwd_busy_cnt(char *qname, fwd_stats_t **p_stats)
{
    int ret;
    fwd_stats_t *el = NULL;
    
    /* Have mutex sync, as wakeup-notify threads will lookup the queues
     * here
     */
    MUTEX_LOCK_V(M_statsh_lock);
    
    EXHASH_FIND_STR( M_statsh, qname, el);
    
    if (NULL==el)
    {
        el = NDRX_FPMALLOC(sizeof(fwd_stats_t), 0);

        if (NULL==el)
        {
            NDRX_LOG(log_error, "Failed to malloc %d bytes", sizeof(fwd_stats_t));
            EXFAIL_OUT(ret);
        }

        NDRX_STRCPY_SAFE(el->qname, qname);
        el->busy=0;
        
        NDRX_SPIN_INIT_V(el->busy_spin);
        NDRX_SPIN_INIT_V(el->sync_spin);
        MUTEX_VAR_INIT(el->sync_mut);
        
        EXHASH_ADD_STR(M_statsh, qname, el);
    }
    
    ret=el->busy;
    
out:
    
    /* clean off... */
    MUTEX_UNLOCK_V(M_statsh_lock);

    return ret;
}

/**
 * Increment the count by queue name
 * @param p_stats queue name for which to increment the counter
 * @return EXSUCCEED/EXFAIL
 */
expublic void tmq_fwd_busy_inc(fwd_stats_t *p_stats)
{
    NDRX_SPIN_LOCK_V(p_stats->busy_spin);
    p_stats->busy++;
    NDRX_SPIN_UNLOCK_V(p_stats->busy_spin);
}

/**
 * Decrement the queue counter. In case if 0, just remove the queue from the
 * hash.
 * @param p_stats stats entry to decrement
 */
expublic void tmq_fwd_busy_dec(fwd_stats_t *p_stats)
{
    NDRX_SPIN_LOCK_V(p_stats->busy_spin);
    p_stats->busy--;
    NDRX_SPIN_UNLOCK_V(p_stats->busy_spin);
    
}

/**
 * Add message to forward stat Q
 * @param p_stats statistics entry
 * @param msg message to add to statst list
 */
expublic void tmq_fwd_sync_add(fwd_stats_t *p_stats, fwd_msg_t *msg)
{
    NDRX_SPIN_LOCK_V(p_stats->sync_spin);
    DL_APPEND(p_stats->sync_head, msg);
    NDRX_SPIN_UNLOCK_V(p_stats->sync_spin);
}

/**
 * Delete message from sync list
 * @param p_stats stats with sync details
 * @param msg message to remove from list
 */
expublic void tmq_fwd_sync_del(fwd_stats_t *p_stats, fwd_msg_t *msg)
{
    NDRX_SPIN_LOCK_V(p_stats->sync_spin);
    DL_APPEND(p_stats->sync_head, msg);
    NDRX_SPIN_UNLOCK_V(p_stats->sync_spin);
}

/**
 * Check is current our order for msg to process
 * @param p_stats queue statistics
 * @param msg message to verify
 * @return EXTRUE (mine), EXFALSE (not mine turn)
 */
expublic int tmq_fwd_sync_mine(fwd_stats_t *p_stats, fwd_msg_t *msg)
{
    int ret = EXFALSE;
    NDRX_SPIN_LOCK_V(p_stats->sync_spin);
    
    if (p_stats->sync_head == msg)
    {
        ret = EXTRUE;
    }
    
    NDRX_SPIN_UNLOCK_V(p_stats->sync_spin);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
