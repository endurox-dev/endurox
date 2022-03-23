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
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <ndebug.h>
#include <sys/time.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate MUTEX_LOCKDECL(M_uuid_lock); /**< Lock the random generator */
exprivate volatile unsigned int M_seedp;
exprivate volatile unsigned int M_counter;
exprivate volatile unsigned int M_init_done=EXFALSE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Get random bytes... (if available)
 * @param output where to unload good random
 * @param number bytes to unload
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_get_rnd_bytes(unsigned char *output, size_t len)
{
    int ret = EXSUCCEED;
    int fd=EXFAIL, flags, i;

    fd = open("/dev/urandom", O_RDONLY);
    
    if (fd == EXFAIL)
    {
        fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
    }
    
    if (fd >= 0)
    {
        flags = fcntl(fd, F_GETFD);
        
        if (flags >= 0)
        {
            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        }
    }
    else
    {
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<len; i++)
    {
        output[i]=0;
        if (EXSUCCEED!=read(fd, output+i, 1))
        {
            /* nothing todo... */
        }
        
    }
    
out:
    
    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/**
 * Initialize random seed
 */
expublic void ndrx_rand_seedset(unsigned int *seed)
{
    struct timeval  tv;
    unsigned char buf[sizeof(*seed)];
    int i;
    char *p;

    gettimeofday(&tv, 0);
    *seed = (getpid() << 16) ^ getuid() ^ (unsigned int)tv.tv_sec ^ (unsigned int)tv.tv_usec;

    /* Randomize seed counter.. */
    if (EXSUCCEED==ndrx_get_rnd_bytes(buf, sizeof(*seed)))
    {
        p = (unsigned char *)seed;
        for (i=0; i<sizeof(*seed); i++)
        {
            p[i] = p[i] ^ buf[i];
        }
    }
}

/**
 * Init UUID generator (once!)
 */
exprivate void ndrx_cid_init(void)
{
    unsigned int locl_seedp;
    unsigned char buf[sizeof(unsigned int)];
    unsigned char *p;
    int i;
    
    ndrx_rand_seedset(&locl_seedp);
    M_counter = rand_r(&locl_seedp);
    
    /* Randomize counter.. */
    if (EXSUCCEED==ndrx_get_rnd_bytes(buf, sizeof(unsigned int)))
    {
        p = (unsigned char *)&M_counter;
        for (i=0; i<sizeof(unsigned int); i++)
        {
            p[i] = p[i] ^ buf[i];
        }
    }
    M_seedp = locl_seedp;
}

/**
 * Generate random CID (cluster ID)
 * @param prefix Node id prefix
 * @param out value to fill
 */
expublic void ndrx_cid_generate(unsigned char prefix, exuuid_t out)
{
    int i;
    unsigned int counter;
    unsigned short rnd;
    char *out_p = (char *)out;
    unsigned pid = getpid();
    struct timeval tv;
    unsigned int locl_seedp;
    
    /* perform init... */
    if (!M_init_done)
    {
        MUTEX_LOCK_V(M_uuid_lock);
        if (!M_init_done)
        {
            ndrx_cid_init();
            M_init_done = EXTRUE;
        }
        MUTEX_UNLOCK_V(M_uuid_lock);
    }
    
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

    locl_seedp=M_seedp;
    
    rnd=0;
    for (i=0; i<sizeof(rnd); i++)
    {
        rnd<<=8;
        rnd|=rand_r(&locl_seedp) & 0xff;
    }
    
    M_seedp=locl_seedp;
    
    MUTEX_UNLOCK_V(M_uuid_lock);
    
    /* UTC: time section: */
    gettimeofday(&tv, 0);

    /* 16M - counter section */
/*
    out_p[5] = (counter >>24) & 0xff;
*/
    /* added usec randomization: */
    out_p[5] = (tv.tv_usec >>7) & 0xff;
    out_p[6] = (counter >>16) & 0xff;
    out_p[7] = (counter >>8) & 0xff;
    out_p[8] = (counter) & 0xff;
    
    /* UTC: time section: */
    gettimeofday(&tv, 0);
        
    /* Load usec in oldest 7bits with usec, as 1 bit is to solve 2038 problem, 
     * so 136 years we guarantee that cid is unique
     */
    out_p[9] =  (tv.tv_usec & 0xfe) | ((tv.tv_sec >>32) & 0xff);
    out_p[10] = (tv.tv_sec >>24) & 0xff;
    out_p[11] = (tv.tv_sec >>16) & 0xff;
    out_p[12] = (tv.tv_sec >>8) & 0xff;
    out_p[13] = (tv.tv_sec) & 0xff;

    /* random number section: */
    out_p[14] = (rnd >> 8) & 0xff;
    out_p[15] = (rnd) & 0xff;
}

/* vim: set ts=4 sw=4 et smartindent: */
