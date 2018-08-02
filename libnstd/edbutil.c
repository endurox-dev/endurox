/* 
 * @brief LMDB support function
 *
 * @file edbutil.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <stdlib.h>
#include <stdio.h>
#include <ndrstandard.h>
#include <nstdutil.h>
#include <nstd_tls.h>
#include <string.h>
#include <errno.h>
#include <edbutil.h>
#include "thlock.h"
#include "userlog.h"
#include "ndebug.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Unlink MDB files (instead of drop - full kill)
 * @param resource LMDB directory
 * @param errdet where to print error detail
 * @param errdetbufsz error detail buffer size
 * @param log_facility log, if set to LOG_FACILITY_UBF, 
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_mdb_unlink(char *resource, char *errdet, int errdetbufsz, 
        int log_facility)
{
    int ret = EXSUCCEED;
    char data_file[PATH_MAX+1];
    char lock_file[PATH_MAX+1];

    snprintf(data_file, sizeof(data_file), "%s/data.edb", resource);
    snprintf(lock_file, sizeof(data_file), "%s/lock.edb", resource);

    if (LOG_CODE_UBF==log_facility)
    {
        NDRX_LOG(log_info, "Removing data file: [%s], lock file: [%s]", 
                data_file, lock_file);
    }
    else
    {
        UBF_LOG(log_info, "Removing data file: [%s], lock file: [%s]", 
                data_file, lock_file);
    }

    if (EXSUCCEED!=unlink(data_file))
    {
        int err = errno;
        
        if (LOG_CODE_UBF==log_facility)
        {
            UBF_LOG(log_info, "unlink [%s] failed: %s", data_file, strerror(err));
        }
        else
        {
            NDRX_LOG(log_info, "unlink [%s] failed: %s", data_file, strerror(err));
        }
        
        if (ENOENT!=err)
        {
            snprintf(errdet, errdetbufsz, "Failed to unlink: [%s]",
                    strerror(err));
            ret=EXFAIL;
        }
    }

    if (EXSUCCEED!=unlink(lock_file))
    {
        int err = errno;
        
        if (LOG_CODE_UBF==log_facility)
        {
            UBF_LOG(log_error, "unlink [%s] failed: %s", lock_file, strerror(err));
        }
        else
        {
            NDRX_LOG(log_error, "unlink [%s] failed: %s", lock_file, strerror(err));
        }
        if (ENOENT!=err)
        {
            snprintf(errdet, errdetbufsz,
                    "Failed to unlink: [%s]", strerror(err));
            ret=EXFAIL;
        }
    }

    return ret;
}

