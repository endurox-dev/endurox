/**
 * @brief Simple message client
 *
 * @file atmiclt0_mqcls.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys_mqueue.h>
#include "test000.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Send some stuff to test000_server Q
 * TODO: Test cases
 * - queue not found in read mode... - fail
 * @param argc
 * @param argv
 * @return 
 */
int main( int argc , char **argv )
{
    int ret = EXSUCCEED;
    mqd_t mq = (mqd_t)EXFAIL;
    struct mq_attr attr;
    char buffer[TEST_REPLY_SIZE];
    int must_stop = 0;
    struct   timespec tm;
    int i;
    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEST_REPLY_SIZE;
    attr.mq_curmsgs = 0;

    /* create the message queue 
     * TODO: use mode flags!
     */
    if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(SV_QUEUE_NAME, 0, 0644, &attr)))
    {
        NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                SV_QUEUE_NAME, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<1000000; i++)
    {
        ssize_t bytes_read;
        /* receive the message 
         * Maybe have some timed receive
         */
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 5;  /* Set for 20 seconds */

        NDRX_LOG(log_debug, "About to SND!");
        
        if (EXFAIL==(bytes_read=ndrx_mq_timedsend(mq, buffer, 
                TEST_REPLY_SIZE, 0, &tm)))
        {
            NDRX_LOG(log_error, "Failed to send message: %s", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "Read bytes: %d", bytes_read);
        
        if (100==buffer[15])
        {
            must_stop = EXTRUE;
        }
        
    }

out:
    /* cleanup */
    
    if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_close(mq))
    {
        NDRX_LOG(log_error, "Failed to close queue: %s", tpstrerror(errno));
    }
    
    if ((mqd_t)EXFAIL!=mq && EXFAIL==ndrx_mq_unlink(SV_QUEUE_NAME))
    {
        NDRX_LOG(log_error, "Failed to unlink q: %s", tpstrerror(errno));
    }


    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
