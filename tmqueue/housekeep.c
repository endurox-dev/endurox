/**
 * @brief Keep the message store clean from damaged files where possible
 *
 * @file hausekeep.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <tperror.h>
#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqd.h"
#include "../libatmisrv/srv_int.h"
#include "nstdutil.h"
#include "userlog.h"
#include <xa_cmn.h>
#include <atmi_int.h>
#include <ndrxdiag.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate int M_housekeep=TMQ_HOUSEKEEP_DEFAULT;    /**< default time for houskeeping */

/*---------------------------Prototypes---------------------------------*/


/**
 * Configure housekeep time
 * @param housekeep time in seconds
 */
expublic void tmq_configure_housekeep(int housekeep)
{
    M_housekeep=housekeep;
    
    NDRX_LOG(log_info, "Housekeep time set to [%d] seconds",
            M_housekeep);    
}

/**
 * Remove active file 
 * @param filename which file to check
 * @param tmq_err error code when processing the file
 */
expublic void tmq_housekeep(char *filename, int tmq_err)
{
    long diff;
    struct stat buf;
    struct timespec tm;
    
    NDRX_LOG(log_warn, "Housekeeping file [%s]", filename);
    if (M_housekeep<=0)
    {
        NDRX_LOG(log_debug, "Housekeeping disabled");
        goto out;
    }

    if (TMQ_ERR_EOF!=tmq_err &&
            TMQ_ERR_CORRUPT!=tmq_err)
    {
        /* keep the file it is not subject to delete */
        NDRX_LOG(log_debug, "Command file is not corrupted [%d]", tmq_err);
        goto out;
    }
    
    /* get the age of the file */
    if (EXSUCCEED!=stat(filename, &buf))
    {
        NDRX_LOG(log_error, "Failed to stat [%s]: %s", filename, strerror(errno));
        goto out;
    }
    
    clock_gettime(CLOCK_REALTIME, &tm);
    diff = ndrx_timespec_get_delta(&tm, &buf.st_ctim) / 1000;
    
    NDRX_LOG(log_warn, "File age is [%ld] limit [%d]", diff, M_housekeep);
    
    if (diff > M_housekeep)
    {
        if (EXSUCCEED==unlink(filename))
        {
            NDRX_LOG(log_warn, "Unlinked expired corrupted file [%s]", filename);
            userlog("Unlinked expired corrupted file [%s]", filename);
        }
        else
        {
            int err;
            
            err = errno;
            NDRX_LOG(log_warn, "Failed to unlink expired corrupted file [%s]: %s", 
                    filename, strerror(err));
            userlog("Failed to unlink expired corrupted file [%s]: %s", 
                    filename, strerror(err));
        }
    }
    
out:
    
    return;

}

/* vim: set ts=4 sw=4 et smartindent: */
