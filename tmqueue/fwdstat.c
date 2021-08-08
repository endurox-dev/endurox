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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Forward statistics
 */
typedef struct {
    
    char qname[TMQNAMELEN+1];
    int busy;
    EX_hash_handle hh; /**< makes this structure hashable        */
} fwd_stats_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/** Lock for statistics hash */
exprivate MUTEX_LOCKDECL(M_statsh_lock);

/** statistics hash by it self */
exprivate fwd_stats_t *M_statsh = NULL;

/*---------------------------Prototypes---------------------------------*/

/**
 * Get forward busy count per queue
 * @param[in] qname queue name
 * @return Number of messages in forward threads
 */
expublic int tmq_fwd_busy_cnt(char *qname)
{
    int ret;
    fwd_stats_t *el = NULL;
    
    MUTEX_LOCK_V(M_statsh_lock);
    
    EXHASH_FIND_STR( M_statsh, qname, el);
    
    if (NULL==el)
    {
        ret=0;
    }
    else
    {
        ret=el->busy;
    }
    
    MUTEX_UNLOCK_V(M_statsh_lock);
    
    return ret;
}

/**
 * Increment the count by queue name
 * @param qname queue name for which to increment the counter
 * @return EXSUCCEED/EXFAIL
 */
expublic int tmq_fwd_busy_inc(char *qname)
{
    int ret = EXSUCCEED;
    fwd_stats_t *el = NULL;
    
    MUTEX_LOCK_V(M_statsh_lock);
    
    /* if found, increment, if not found add new with 1 */
    
    EXHASH_FIND_STR( M_statsh, qname, el);
    
    if (NULL!=el)
    {
        el->busy++;
    }
    else
    {
        /* Alloc + add with 1. */
        el = NDRX_FPMALLOC(sizeof(fwd_stats_t), 0);
        
        if (NULL==el)
        {
            NDRX_LOG(log_error, "Failed to malloc %d bytes", sizeof(fwd_stats_t));
            EXFAIL_OUT(ret);
        }
        
        NDRX_STRCPY_SAFE(el->qname, qname);
        el->busy=1;
        EXHASH_ADD_STR(M_statsh, qname, el);
    }
    
out:
    MUTEX_UNLOCK_V(M_statsh_lock);
    
    return ret;
}

/**
 * Decrement the queue counter. In case if 0, just remove the queue from the
 * hash.
 * @param qname queue name
 */
expublic void tmq_fwd_busy_dec(char *qname)
{
    fwd_stats_t *el = NULL;
    
    MUTEX_LOCK_V(M_statsh_lock);
    
    EXHASH_FIND_STR( M_statsh, qname, el);
    
    if (NULL==el)
    {
        userlog("Fatal error: No queue defined [%s] in fwd stats hash", qname);
        NDRX_LOG(log_always, "Fatal error: No queue defined [%s] in fwd stats hash", qname);
        abort();
    }
    else
    {
        el->busy--;
        assert(el->busy>=0);
    }
    
    MUTEX_UNLOCK_V(M_statsh_lock);
}

/* vim: set ts=4 sw=4 et smartindent: */
