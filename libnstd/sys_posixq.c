/* 
** Posix queues support
**
** @file sys_posixq.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
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
        M_first = FALSE;\
    }
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
static int M_first = TRUE;
static char M_qpath[PATH_MAX] = {EOS};
/*---------------------------Prototypes---------------------------------*/

/**
 * This is version with registry files to FS
 * - Firstly open the queue,
 * - The open fifo file
 */
public mqd_t ndrx_mq_open_with_registry(const char *name, int oflag, 
        mode_t mode, struct mq_attr *attr)
{
    mqd_t ret;
    char regpath[PATH_MAX];
    int err;
    API_ENTRY;
    
    sprintf(regpath, "%s%s", M_qpath, name);
    
    NDRX_LOG(log_debug, "opening, registry path built: [%s]", regpath);
    
    ret = mq_open(name, oflag, mode, attr);
    
    if ((mqd_t)FAIL!=ret && (oflag & O_CREAT))
    {
        if (SUCCEED!=mkfifo(regpath, S_IWUSR | S_IRUSR))
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
                ret=(mqd_t)FAIL;
                errno = err;
                NDRX_LOG(log_error, "Removing queue...");
                if (SUCCEED!=mq_unlink(name))
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
public int ndrx_mq_unlink_with_registry (const char *name)
{   
    char regpath[PATH_MAX];
    int ret, err;
    API_ENTRY;
    
    sprintf(regpath, "%s%s", M_qpath, name);
    
    NDRX_LOG(log_debug, "deleting, registry path built: [%s]", regpath);
    
    if (SUCCEED!=(ret = mq_unlink(name)))
    {
        err = errno;
        NDRX_LOG(log_error, "Failed to mq_unlink [%s]: %s", name, strerror(err));
    }
    
    if (SUCCEED!=unlink(regpath))
    {
        NDRX_LOG(log_error, "Failed to unlink [%s]: %s", regpath, strerror(errno));
    }
    
    errno = err;
    
    return ret;
}

