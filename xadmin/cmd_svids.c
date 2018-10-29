/**
 * @brief `svqids' and `svsemdis' commands - return resources ids for the
 *  user.
 *
 * @file cmd_svids.c
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
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>

#include <nstopwatch.h>
#include <nclopt.h>
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
 * Print user queue ids
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_svqids(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    ndrx_growlist_t list;
    int i;
    
    memset(&list, 0, sizeof(list));
    
    if (EXSUCCEED!=ndrx_sys_sysv_user_res(&list, EXTRUE))
    {
        fprintf(stderr, "Failed to list System V queues\n");
        NDRX_LOG(log_error, "Failed to list System V queues");
        EXFAIL_OUT(ret);
    }
    
    fprintf(stderr, "MSG QUEUE ID\n");
    fprintf(stderr, "------------\n");
        
    for (i=0; i<=list.maxindexused; i++)
    {
        fprintf(stdout, "%d\n", ((int *)list.mem)[i]);
    }
    
out:
    if (NULL!=list.mem)
    {
        NDRX_FREE(list.mem);
    }
    return ret;
}

/**
 * Print Semaphore IDs, System V for user.
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_svsemids(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    ndrx_growlist_t list;
    int i;
    
    memset(&list, 0, sizeof(list));
    
    if (EXSUCCEED!=ndrx_sys_sysv_user_res(&list, EXFALSE))
    {
        fprintf(stderr, "Failed to list System V semaphores\n");
        NDRX_LOG(log_error, "Failed to list System V Semaphores");
        EXFAIL_OUT(ret);
    }
    
    fprintf(stderr, "SEMAPHORE ID\n");
    fprintf(stderr, "------------\n");
        
    for (i=0; i<=list.maxindexused; i++)
    {
        fprintf(stdout, "%d\n", ((int *)list.mem)[i]);
    }
    
out:
    if (NULL!=list.mem)
    {
        NDRX_FREE(list.mem);
    }
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
