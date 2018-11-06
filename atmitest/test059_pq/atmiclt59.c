/**
 * @brief Test print queue ops - client
 *
 * @file atmiclt59.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <sys_mqueue.h>
#include "test59.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{

    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    int ret=EXSUCCEED;
    mqd_t mq = (mqd_t)EXFAIL;
    int i;
    int num;
    
    if (argc < 4)
    {
        NDRX_LOG(log_error, "%s <command 's' - call service | 'q' - make q> <name> "
                "<nummsg>\n");
        EXFAIL_OUT(ret);
    }
    
    num = atoi(argv[3]);
    if ('s' == argv[1][0])
    {
        if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, VALUE_EXPECTED, 0, BFLD_STRING))
        {
            NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
            ret=EXFAIL;
            goto out;
        }    

        /* put some message in target service */
        for (i=0; i<num;i++)
        {
            if (EXFAIL == tpacall(argv[2], (char *)p_ub, 0L, 0))
            {
                NDRX_LOG(log_error, "%s failed: %s", argv[2], tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
        }
    }
    else if ('q' == argv[1][0])
    {
        /* create some arbitrary queue and enqueue some data to it */
        struct mq_attr attr;
        char qstr[PATH_MAX];
	char buf[128];
        /* initialize the queue attributes */
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = 128;
        attr.mq_curmsgs = 0;
        
        snprintf(qstr, sizeof(qstr), "%s", argv[2]);

        /* create the message queue */
        if ((mqd_t)EXFAIL==(mq = ndrx_mq_open(qstr, O_CREAT | O_RDWR, 0644, &attr)))
        {
            NDRX_LOG(log_error, "Failed to open queue: [%s]: %s", 
                    qstr, strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        for (i=0; i<num;i++)
        {
            if (EXSUCCEED!=ndrx_mq_send(mq, buf, sizeof(buf), 0))
            {
                NDRX_LOG(log_error, "Failed to send msg to [%s]: %s",
                        qstr, strerror(errno));
                EXFAIL_OUT(ret);
            }
        }
    }
    else
    {
        NDRX_LOG(log_error, "Unknown command [%s]", argv[1]);
        EXFAIL_OUT(ret);
    }

out:
    
    if ((mqd_t)EXFAIL!=mq)
    {
        ndrx_mq_close(mq);
    }

    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */

