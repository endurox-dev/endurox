/**
 * @brief Persistent storage durability API 
 *
 * @file sys_fsync.c
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
#include <stdio.h>
#include <stdlib.h>
#include <nstdutil.h>
#include <fcntl.h>
#include "ndebug.h"
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define NDRX_FSYNC_SEP          ","     /**< separator used for config parse */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Parse fsync setting. Syntax is comma separated keywords
 * currently settings are: 
 * - fsync - full fsync on file
 * - fdatasync - only data content sync on file (no metadata update)
 * - dsync - perform directory meta data sync (for new files or moved)
 * @param setting_str setting string to parsz
 * @param flag parsed setting
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_fsync_parse(char *setting_str, long *flags)
{
    int ret = EXSUCCEED;
    char *parse, *p, *saveptr1;
    
    *flags = 0;
    
    if (NULL==setting_str || EXEOS==setting_str[0])
    {
        goto out;
    }

    /* remove whitespace */
    parse=ndrx_str_strip(setting_str, "\t\n ");
    
    for (p=strtok_r(parse, NDRX_FSYNC_SEP, &saveptr1); 
            NULL!=p; 
            p=strtok_r(NULL, NDRX_FSYNC_SEP, &saveptr1))
    {
        if (0==strcmp(p, NDRX_FSYNC_FSYNC_STR))
        {
            *flags |= NDRX_FSYNC_FSYNC;
        }
        else if (0==strcmp(p, NDRX_FSYNC_FDATASYNC_STR))
        {
            *flags |= NDRX_FSYNC_FDATASYNC;
        }
        else if (0==strcmp(p, NDRX_FSYNC_DSYNC_STR))
        {
            *flags |= NDRX_FSYNC_DSYNC;
        }
        else
        {
            NDRX_LOG(log_error, "Unknown fsync setting: [%s]", p);
            EXFAIL_OUT(ret);
        }
    }
    
out:
    
    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_warn, "fsync setting: 0x%lx", *flags);
    }
    
    return ret;
}

/**
 * Perform fsync on the file contents (if needed)
 * @param file handle to sync
 * @param setting parsed settings from ndrx_fsync_parse
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_fsync_fsync(FILE *file, long flags)
{
    int ret = EXSUCCEED;
    int fd, err;

    if (NULL==file)
    {
        NDRX_LOG(log_error, "Invalid fsync handle");
        EXFAIL_OUT(ret);
    }
    
    if ( (flags & NDRX_FSYNC_FSYNC) || (flags & NDRX_FSYNC_FDATASYNC))
    {
        fd = fileno(file);
        if (EXFAIL==fd)
        {
            err = errno;
            NDRX_LOG(log_error, "%s: fileno() failed on %p: %s", __func__,
                    file, strerror(err));
            userlog("%s: fileno() failed on %p: %s", __func__,
                    file, strerror(err));
            EXFAIL_OUT(ret);
        }
    }
    
    if (flags & NDRX_FSYNC_FSYNC)
    {
        if (EXSUCCEED!=fsync(fd))
        {
            int err = errno;
            NDRX_LOG(log_error, "%s: fsync() failed on %p / %d: %s", __func__,
                    file, fd, strerror(err));
            userlog("%s: fsync() failed on %p / %d: %s", __func__,
                    file, fd, strerror(err));
            EXFAIL_OUT(ret);
        }
    }
    else if (flags & NDRX_FSYNC_FDATASYNC)
    {
        if (EXSUCCEED!=fdatasync(fd))
        {
            int err = errno;
            NDRX_LOG(log_error, "%s: fdatasync() failed on %p / %d: %s", __func__,
                    file, fd, strerror(err));
            userlog("%s: fdatasync() failed on %p / %d: %s", __func__,
                    file, fd, strerror(err));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    return ret;
}

/**
 * Perform directory sync
 * @param dir directory to sync
 * @param flags parsed setting
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_fsync_dsync(char *dir, long flags)
{
    int ret = EXSUCCEED;
    int err, fd=EXFAIL;
    
    if (NULL==dir || EXEOS==dir[0])
    {
        NDRX_LOG(log_error, "NULL/empty dsync handle");
        EXFAIL_OUT(ret);
    }

    if (flags & NDRX_FSYNC_DSYNC)
    {
        fd = open(dir, O_RDONLY);
        
        if (EXFAIL==fd)
        {
            err = errno;
            NDRX_LOG(log_error, "%s: failed to open dir [%s]: %s", 
                    __func__, dir, strerror(err));
            userlog("%s: Failed to open dir [%s]: %s", 
                    __func__, dir, strerror(err));
            EXFAIL_OUT(ret);
        }

#ifdef EX_OS_AIX
        /* On aix getting "Bad file number" 
         * thus only what we can do is ignore these errors
         */
        fsync_range(fd, O_DSYNC, 0, 0);
#else
        if (EXSUCCEED!=fsync(fd))
        {
            int err = errno;
            NDRX_LOG(log_error, "%s: fsync() failed on %s / %d: %s", __func__,
                    dir, fd, strerror(err));
            userlog("%s: fsync() failed on %s / %d: %s", __func__,
                    dir, fd, strerror(err));
            EXFAIL_OUT(ret);
        }        
#endif
    }
    
out:
    
    if (EXFAIL!=fd)
    {
        close(fd);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
