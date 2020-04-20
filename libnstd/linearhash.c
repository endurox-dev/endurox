/**
 * @brief Linear memory hashing routines
 *
 * @file linearhash.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <sys_unix.h>
#include <exsha1.h>
#include <excrypto.h>
#include <userlog.h>
#include <expluginbase.h>
#include <exaes.h>
#include <exbase64.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define GET_FLAGS(CONF, IDX)  *((short*) ((*CONF->memptr) + (CONF->elmsz * IDX) + CONF->flags_offset))

#define NDRX_LH_ENT_NONE            0       /**< Linear hash entry not used */
#define NDRX_LH_ENT_MATCH           1       /**< Linear hash entry matched  */
#define NDRX_LH_ENT_OLD             2       /**< Linear hash entry was used */

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Linear hash search routine.
 * NOTE, when installing value in hash NDRX_LH_FLAG_WASUSED must be always set.
 * Function is not thread safe (outside locking must be used)
 * @param conf hashing configuration
 * @param key_get key value to get
 * @param key_len key length (used where applicable)
 * @param oflag create flag (O_CREAT is checked)
 * @param pos output position
 * @param have_value EXTRUE -> Valid value is present, EXFALSE -> No valid value
 * @param key_typ key type msg
 * @return EXTRUE -> Position found (with valid or not valid value), POSITION not found
 *  either was not O_CREATE or if was, then hash is full
 */
expublic int ndrx_lh_position_get(ndrx_lh_config_t *conf, 
        void *key_get, size_t key_len, 
        int oflag, int *pos, int *have_value, char *key_typ)
{
    int ret=NDRX_LH_ENT_NONE;
    int try = EXFAIL;
    int start = try;
    int overflow = EXFALSE;
    int iterations = 0;
    char key_debug[256] = {EXEOS};
    char val_debug[256] = {EXEOS};
    short flags;
    
    /* get debug key */
    
    if (debug_get_ndrx_level()>=log_debug)
    {
        conf->p_key_debug(conf, key_get, key_len, key_debug, sizeof(key_debug));
    }
    
    /*
     * 20/10/2018 Got to loop twice!!!! 
     * Some definitions:
     * A - our Q
     * B - other Q
     * 
     * The problem:
     * 1) B gets installed in our cell / direct hash number
     * 2) A gets installed in cell+1 index
     * 3) B gets uninstalled / cell becomes as stale
     * 4) A tries installed Q again, and it hits the cell and not the cell+1
     * 
     * ----
     * Thus to solve this, we got to firstly perform "read only" lookup on the
     * queue tables to see, is queue already present there or not. And if not,
     * only then perform write to maps..
     * 
     * !!!! Needs to check with the _ndrx_shm_get_svc() wouldn't be there the
     * same problem !!!!
     * 
     */
    if (oflag & O_CREAT)
    {
        int try_read;
        int read_have_value;
        
        if (ndrx_lh_position_get(conf, key_get, key_len, 0, 
                &try_read, &read_have_value, key_typ) && read_have_value)
        {
            try = try_read;
        }
    }
    
    if (EXFAIL==try)
    {
        /*
        try = ndrx_hash_fn(pathname) % M_queuesmax;
         */
        try = conf->p_key_hash(conf, key_get, key_len);
    }
    else
    {
        NDRX_LOG(log_debug, "Got existing record at %d", try);
    }
    
    start=try;
    *pos=EXFAIL;
    
    NDRX_LOG(6, "Try key for [%s] is %d, shm is: %p oflag: %d", 
                                        key_debug, try, *(conf->memptr), oflag);
    /*
     * So we loop over filled entries until we found empty one or
     * one which have been initialised by this service.
     *
     * So if there was overflow, then loop until the start item.
     */
    while (( (flags=GET_FLAGS(conf, try)) & NDRX_LH_FLAG_WASUSED)
            && (!overflow || (overflow && try < start)))
    {
/*        if (0==strcmp(NDRX_SVQ_INDEX(svq, try)->qstr, pathname)) */
        if (0==conf->p_compare(conf, key_get, key_len, try))
        {
            *pos=try;
            
            if (flags & NDRX_LH_FLAG_ISUSED)
            {
                ret=NDRX_LH_ENT_MATCH;
            }
            else
            {
                ret=NDRX_LH_ENT_OLD;
            }
            
            break;  /* <<< Break! */
        }
        
	if (oflag & O_CREAT)
	{
            if (!(flags & NDRX_LH_FLAG_ISUSED))
            {
                /* found used position */
                ret=NDRX_LH_ENT_OLD;
                break; /* <<< break! */
            }
	}

        try++;
        
        /* we loop over... 
         * Feature #139 mvitolin, 09/05/2017
         * Fix potential overflow issues at the border... of SHM...
         */
        if (try>=conf->elmmax)
        {
            try = 0;
            overflow=EXTRUE;
            NDRX_LOG(log_debug, "Overflow reached for search of [%s]", key_debug);
        }
        iterations++;
        
        NDRX_LOG(log_debug, "Trying %d for [%s]", try, key_debug);
    }
    
    *pos=try;
    
    switch (ret)
    {
        case NDRX_LH_ENT_OLD:
            *have_value = EXFALSE;
            ret = EXTRUE;   /* have position */
            break;
        case NDRX_LH_ENT_NONE:
            
                /* if last one is not used, either we hit it first on empty hash
                 * or we started somewhere at the end, overflowed and hit
                 * an empty cell..
                 *  */
                if (!(flags & NDRX_LH_FLAG_WASUSED))
                {
                    *have_value = EXFALSE;
                    ret = EXTRUE;   /* has position */
                }
                else
                {
                    *have_value = EXFALSE;
                    ret = EXFALSE;   /* no position */
                }
            
            break;
        case NDRX_LH_ENT_MATCH:
            *have_value = EXTRUE;
            ret = EXTRUE;   /* have position */
            break;
        default:
            
            NDRX_LOG(log_error, "!!! should not get here...");
            break;
    }
    
    /* get debug value */
    if (debug_get_ndrx_level()>=log_debug)
    {
        conf->p_val_debug(conf, try, val_debug, sizeof(val_debug));
    }
    
    NDRX_LOG(6, "ndrx_lh_position_get %s [%s] - result: %d, "
                "iterations: %d, pos: %d, have_value: %d flags: %hd [%s]",
                key_typ, key_debug, ret, iterations, *pos, *have_value, 
                GET_FLAGS(conf, try), val_debug);
    
    return ret;
}



/* vim: set ts=4 sw=4 et smartindent: */
