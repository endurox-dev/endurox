/**
 * @brief Correlation id queue handler
 *
 * @file corhandle.c
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
#include <regex.h>
#include <stdarg.h>

#include <ndebug.h>
#include "qcommon.h"
#include "tmqd.h"
#include <utlist2.h>
#include <rbtree.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Find the correlator entry in the hash
 * @param qhash queue entry
 * @param corrid_str correlator id 
 * @return ptr or NULL (if not found)
 */
expublic tmq_corhash_t * tmq_cor_find(tmq_qhash_t *qhash, char *corrid_str)
{
    tmq_corhash_t *ret = NULL;

    EXHASH_FIND_STR( (qhash->corhash), corrid_str, ret);

    return ret;

}

/**
 * Add correlator to the hash
 * @param qhash queue entry
 * @param corrid_str identifier to add
 * @return ptr or NULL (if failed to add)
 */
expublic tmq_corhash_t * tmq_cor_add(tmq_qhash_t *qhash, char *corrid_str)
{
    /* allocate the handle */
    tmq_corhash_t *corhash = NDRX_FPMALLOC(sizeof(tmq_corhash_t), 0);

    if (NULL==corhash)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s",
            sizeof(tmq_corhash_t), strerror(errno));
        goto out;
    }

    /* add stuff to hash: */
    memset(corhash, 0, sizeof(tmq_corhash_t));
    NDRX_STRCPY_SAFE(corhash->corrid_str, corrid_str);
    EXHASH_ADD_STR( qhash->corhash, corrid_str, corhash);

    /* setup red-black trees */
    ndrx_rbt_init(&corhash->corq, tmq_rbt_cmp_cor, tmq_rbt_combine_cor, NULL, corhash);

    NDRX_LOG(log_debug, "Added corrid_str [%s] %p",
            corhash->corrid_str, corhash);

out:
    return corhash;
}

/**
 * Remove message from correlator hash / linked list
 * @param qhash queue entry
 * @param mmsg remove from CDL, remove from hash, if CDL is free, remove HASH
 *  entry
 */
expublic void tmq_cor_msg_del(tmq_memmsg_t *mmsg)
{
    /* find the corhash entry, if have one remove from from hash
     * remove msg from CDL
     */
    tmq_corhash_t * corhash = mmsg->corhash;

    /* remove correlator from hash if empty */
    ndrx_rbt_delete(&corhash->corq, (ndrx_rbt_node_t *)&mmsg->cor);

    mmsg->corhash = NULL;

    /* if sub-Q is empty, remove correlator */
    if (ndrx_rbt_is_empty(&corhash->corq))
    {

        NDRX_LOG(log_debug, "Removing corrid_str [%s] %p",
            corhash->corrid_str, corhash);

        /* remove empty hash node */
        EXHASH_DEL(mmsg->qhash->corhash, corhash);
        NDRX_FPFREE(corhash);
    }

    return;
}

/**
 * Add message to corelator hash / linked list
 * @param mmsg message to add
 * @return Qerror code 
 */
expublic int tmq_cor_msg_add(tmq_memmsg_t *mmsg)
{
    int ret = EXSUCCEED;
    int isNew = EXFALSE;
    tmq_corhash_t * corhash;

    corhash = tmq_cor_find(mmsg->qhash, mmsg->corrid_str);
    
    if (NULL==corhash)
    {
        corhash=tmq_cor_add(mmsg->qhash, mmsg->corrid_str);
    }
    
    if (NULL==corhash)
    {
        ret = QMEOS;
        goto out;
    }
    
    /* add backref */
    mmsg->corhash = corhash;

    ndrx_rbt_insert(&corhash->corq, (ndrx_rbt_node_t *)&mmsg->cor, &isNew);
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
