/**
 * @brief Correlation id queue handle
 *
 * @file corhandle.c
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
#include <regex.h>
#include <stdarg.h>

#include <ndebug.h>
#include "qcommon.h"
#include "tmqd.h"
#include <utlist2.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/* TODO: 
- add to queue corhash/queue
- remove from queue corash/queue (incl free-up of hash)
- dequeue from queue corhash  LIFO/FIFO
*/

/**
 * Find the correlator entry in the hash
 * @param qhash queue entry
 * @param corid_str correlator id 
 * @return ptr or NULL (if not found)
 */
expublic tmq_cormsg_t * tmq_cor_find(tmq_qhash_t *qhash, char *corid_str)
{
    return NULL;
}

/**
 * Add correlator
 * @param qhash queue entry
 * @param corid_str identifier to add
 * @return ptr or NULL (if failed to add)
 */
expublic tmq_cormsg_t * tmq_cor_add(tmq_qhash_t *qhash, char *corid_str)
{
    return NULL;
}

/**
 * Remove message from correlator hash / linked list
 * @param qhash queue entry
 * @param mmsg remove from CDL, remove from hash, if CDL is free, remove HASH
 *  entry
 */
expublic void tmq_cor_msg_del(tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    return;
}

/**
 * Add message to corelator hash / linked list
 * @param qconf queue configuration
 * @param qhash queue entry (holds the ptr to cor hash)
 * @param mmsg message to add
 * @return Qerror code 
 */
expublic int tmq_cor_msg_add(tmq_qconfig_t * qconf, tmq_qhash_t *qhash, tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    
    tmq_cormsg_t * cormsg =  tmq_cor_find(qhash, mmsg->corid_str);
    
    if (NULL==cormsg)
    {
        cormsg=tmq_cor_add(qhash, mmsg->corid_str);
    }
    
    if (NULL==cormsg)
    {
        ret = QMEOS;
        goto out;
    }
    
    /* TODO: add to CDL */
    
out:
    return ret;
    
}

/**
 * Sort the prev2/next2
 * @param q where the corhash lives. Loop over corhash and sort
 *  each sub-queue.
 */
expublic void tmq_cor_sort_queues(tmq_qhash_t *q)
{
    
}


/* vim: set ts=4 sw=4 et smartindent: */
