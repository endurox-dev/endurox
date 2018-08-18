/**
 * @brief Posix queues support
 *
 * @file sys_posixq.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>

#include <ndrx_config.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>
#include <limits.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

/*
 * We assume that this is thread safe.
 * Even if two threads get here. The work result will be the same.
 */
#define API_ENTRY  if (M_first)\
    {\
        strcpy(M_qpath, getenv(CONF_NDRX_QPATH));\
        M_first = EXFALSE;\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
static int M_first = EXTRUE;
static char M_qpath[PATH_MAX] = {EXEOS};
/*---------------------------Prototypes---------------------------------*/

/**
 * This is version with registry files to FS
 * - Firstly open the queue,
 * - The open fifo file
 */
expublic mqd_t ndrx_mq_open_with_registry(char *name, int oflag, 
        mode_t mode, struct mq_attr *attr)
{
    mqd_t ret;
    char regpath[PATH_MAX];
    int err;
    API_ENTRY;
    
    sprintf(regpath, "%s%s", M_qpath, name);
    
    NDRX_LOG(log_debug, "opening, registry path built: [%s]", regpath);
    
    ret = mq_open(name, oflag, mode, attr);
    
    if ((mqd_t)EXFAIL!=ret && (oflag & O_CREAT))
    {
        if (EXSUCCEED!=mkfifo(regpath, S_IWUSR | S_IRUSR))
        {
            err = errno;
            NDRX_LOG(log_error, "Failed to open fifo file [%s]: %s", 
                    regpath, strerror(errno));
            if (EEXIST==err)
            {
                NDRX_LOG(log_warn, "File already exists, ignore error...");
                errno = 0;
            }
            else
            {
                ret=(mqd_t)EXFAIL;
                errno = err;
                NDRX_LOG(log_error, "Removing queue...");
                if (EXSUCCEED!=mq_unlink(name))
                {
                    NDRX_LOG(log_error, "Failed to mq_unlink [%s]: %s", 
                            name, strerror(errno));
                }
            }
        }
    }
    
    return ret;
}

/**
 * Remove the queue name from registry
 * - Remove the queue
 * - Then remove fifo file
 */
expublic int ndrx_mq_unlink_with_registry (char *name)
{   
    char regpath[PATH_MAX];
    int ret, err;
    API_ENTRY;
    
    sprintf(regpath, "%s%s", M_qpath, name);
    
    NDRX_LOG(log_debug, "deleting, registry path built: [%s]", regpath);
    
    if (EXSUCCEED!=(ret = mq_unlink(name)))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to mq_unlink [%s]: %s", name, strerror(err));
    }
    
    if (EXSUCCEED!=unlink(regpath))
    {
        NDRX_LOG(log_error, "Failed to unlink [%s]: %s", regpath, strerror(errno));
    }
    
    errno = err;
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
