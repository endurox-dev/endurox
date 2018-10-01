/**
 * @brief System V specifics for 
 *
 * @file sanity_sv5.c
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
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <utlist.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <atmi_int.h>
#include <nstopwatch.h>

#include <ndebug.h>
#include <cmd_processor.h>
#include <signal.h>
#include <bridge_int.h>
#include <atmi_shm.h>
#include <userlog.h>
#include <sys_unix.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * System V sanity checks.
 * Includes following steps:
 * - get a copy of SV5 queue maps
 * - scan the shared memory of services, and mark the used sv5 qids.
 * - then scan the list of used qids for the request addresses
 * - remove any qid, that is not present in shm (the removal shall be done
 *  in sv5 library with the write lock present and checking the ctime again
 *  so that we have a real sync). Check the service rqaddr by NDRX_SVQ_MAP_RQADDR
 * @return SUCCEED/FAIL
 */
expublic int do_sanity_check_sv5(void)
{
    int ret=EXSUCCEED;
    ndrx_svq_status_t *svq;
    int len;
    
    /* TODO: */
    
    /* Get the list of queues */
    svq = ndrx_svqshm_statusget(&len);
    
    if (NULL==svq)
    {
        
    }
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
