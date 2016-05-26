/* 
** Solaris Abstraction Layer (SAL)
**
** @file sys_linux.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>


#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>

#include <sys_unix.h>

#include <utlist.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return the list of queues (build the list according to /tmp/.MQD files.
 * e.g ".MQPn00b,srv,admin,atmi.sv1,123,2229" translates as
 * "/n00b,srv,admin,atmi.sv1,123,2229")
 * the qpath must point to /tmp
 */
public mq_list_t* ex_sys_mqueue_list_make(char *qpath, int *return_status)
{
    mq_list_t* ret = NULL;
    struct dirent **namelist;
    int n;
    mq_list_t* tmp;
    int len;
    
    *return_status = SUCCEED;
    
    n = scandir(qpath, &namelist, 0, alphasort);
    if (n < 0)
    {
        NDRX_LOG(log_error, "Failed to open queue directory: %s", 
                strerror(errno));
        goto exit_fail;
    }
    else 
    {
        while (n--)
        {
            if (0==strcmp(namelist[n]->d_name, ".") || 
                        0==strcmp(namelist[n]->d_name, "..") ||
                        0!=strncmp(namelist[n]->d_name, ".MQP", 4))
                continue;
            
            len = strlen(namelist[n]->d_name) -3 /*.MQP*/ + 1 /* EOS */;
            
            if (NULL==(tmp = calloc(1, sizeof(mq_list_t))))
            {
                NDRX_LOG(log_always, "alloc of mq_list_t (%d) failed: %s", 
                        sizeof(mq_list_t), strerror(errno));
                
                
                goto exit_fail;
            }
            
            if (NULL==(tmp->qname = malloc(len)))
            {
                NDRX_LOG(log_always, "alloc of %d bytes failed: %s", 
                        len, strerror(errno));
                free(tmp);
                goto exit_fail;
            }
            
            strcpy(tmp->qname, "/");
            strcat(tmp->qname, namelist[n]->d_name+4); /* strip off .MQP */
            
            /* Add to LL */
            LL_APPEND(ret, tmp);
            
            free(namelist[n]);
        }
        free(namelist);
    }
    
    return ret;
    
exit_fail:

    *return_status = FAIL;

    if (NULL!=ret)
    {
        ex_sys_mqueue_list_free(ret);
        ret = NULL;
    }

    return ret;   
}
