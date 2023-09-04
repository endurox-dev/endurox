/**
 * @brief Singleton group tests - client
 *
 * @file atmiclt103.c
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
#include "test103.h"
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

    /* attempt to lock ping file (several times) */
    int ret=EXFAIL;
    
    if (argc>2 && 0==strcmp(argv[1], "lock_file"))
    {
        char *lockfile = argv[2];
        struct flock lck;
        int fd;
        memset(&lck, 0, sizeof(lck));

        lck.l_whence = SEEK_SET;
        lck.l_start = 0;
        lck.l_len = 1;
        lck.l_type = F_WRLCK;

        if (EXFAIL==(fd = open(lockfile, O_RDWR|O_CREAT, 0666)))
        {
            NDRX_LOG(log_error, "Failed to open lock file [%s]: %s",
                lockfile, strerror(errno));
            EXFAIL_OUT(ret);
        }
            
        while (EXFAIL==fcntl(fd, F_SETLK, &lck))
        {
            if (EACCES==errno || EAGAIN==errno)
            {
                sleep(1);
            } 
            else
            {
                NDRX_LOG(log_error, "Failed to lock file [%s]: %s",
                    lockfile, strerror(errno));
                EXFAIL_OUT(ret);
            }
        }
        NDRX_LOG(log_debug, "Locked OK");
        sleep(999);
    }


out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
