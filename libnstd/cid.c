/**
 * @brief Enduro/X Cluster ID generator
 *
 * @file cid.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <ndebug.h>
#include <sys/time.h>
#include <xatmi.h>
#include <atmi_int.h>
#include <inttypes.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate MUTEX_LOCKDECL(M_uuid_lock); /**< Lock the random generator */
exprivate volatile unsigned int M_seedp;
exprivate volatile unsigned int M_counter;
/*---------------------------Prototypes---------------------------------*/

/**
 * Init UUID generator (once!)
 */
expublic void ndrx_cid_init(void)
{
    unsigned int locl_seedp;
    struct timeval  tv;
    gettimeofday(&tv, 0);
    locl_seedp = (getpid() << 16) ^ getuid() ^ (unsigned int)tv.tv_sec ^ (unsigned int)tv.tv_usec;
    M_counter = rand_r(&locl_seedp);
    M_seedp = locl_seedp;
}

/**
 * Generate random CID (cluster ID)
 * @param prefix Node id prefix
 * @param out value to fill
 */
expublic void ndrx_cid_generate(unsigned char prefix, exuuid_t out)
{
    unsigned int counter;
    unsigned int rnd;
    char *out_p = (char *)out;
    unsigned pid = getpid();
    struct timeval tv;
    unsigned int locl_seedp;
    /* So node ID 1 byte: */
    out_p[0] = prefix;
    
    /* PID 4x bytes: */
    out_p[1] = (pid >>24) & 0xff;
    out_p[2] = (pid >>16) & 0xff;
    out_p[3] = (pid >>8) & 0xff;
    out_p[4] = (pid) & 0xff;
    
    /* Counter  */
    
    MUTEX_LOCK_V(M_uuid_lock);
    
    M_counter++;
    counter = M_counter;
    rnd=rand_r(&locl_seedp);
    
    /* put seed back to volatile */
    M_seedp=locl_seedp;
    
    MUTEX_UNLOCK_V(M_uuid_lock);

    /* counter section: */
    out_p[5] = (counter >>24) & 0xff;
    out_p[6] = (counter >>16) & 0xff;
    out_p[7] = (counter >>8) & 0xff;
    out_p[8] = (counter) & 0xff;
    
    /* UTC: time section: */
    gettimeofday(&tv, 0);
        
    out_p[9] = (tv.tv_sec >>32) & 0xff;
    out_p[10] = (tv.tv_sec >>24) & 0xff;
    out_p[11] = (tv.tv_sec >>16) & 0xff;
    out_p[12] = (tv.tv_sec >>8) & 0xff;
    out_p[13] = (tv.tv_sec) & 0xff;
    
    /* random number section: */
    out_p[14] = (rnd >> 8) & 0xff;
    out_p[15] = (rnd) & 0xff;
}

/* vim: set ts=4 sw=4 et smartindent: */
