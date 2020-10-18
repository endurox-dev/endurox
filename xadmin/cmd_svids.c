/**
 * @brief `svqids' and `svsemdis' commands - return resources ids for the
 *   user.
 *
 * @file cmd_svids.c
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
    mdrx_sysv_res_t *p_res;
    int print_ids=EXFALSE;
    int print_keys=EXFALSE;
    
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_INT, (void *)&print_ids, sizeof(print_ids), 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Print IDs only"},
        {'k', BFLD_INT, (void *)&print_keys, sizeof(print_keys), 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Print Keys only"},
        {0}
    };

    memset(&list, 0, sizeof(list));
            
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_sys_sysv_user_res(&list, NDRX_SV_RESTYPE_QUE))
    {
        fprintf(stderr, "Failed to list System V queues\n");
        NDRX_LOG(log_error, "Failed to list System V queues");
        EXFAIL_OUT(ret);
    }
    
    if (print_ids)
    {
        fprintf(stderr, "    QUEUE ID\n");
        fprintf(stderr, "------------\n");
    }
    else if (print_keys)
    {
        fprintf(stderr, "   IPC KEY\n");
        fprintf(stderr, "----------\n");
    }
    else
    {
        fprintf(stderr, "    QUEUE ID    IPC KEY\n");
        fprintf(stderr, "------------ ----------\n");
    }
        
    p_res = (mdrx_sysv_res_t *)list.mem;
    
    for (i=0; i<=list.maxindexused; i++)
    {
        if (print_ids)
        {
            fprintf(stdout, "%12u\n", p_res[i].id);
        }
        else if (print_keys)
        {
            fprintf(stdout, "0x%08x\n", p_res[i].key);
        }
        else
        {
            fprintf(stdout, "%12u 0x%08x\n", p_res[i].id, p_res[i].key);
        }
    }
    
out:
    ndrx_growlist_free(&list);

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
    mdrx_sysv_res_t *p_res;
    int print_ids=EXFALSE;
    int print_keys=EXFALSE;
    
    ncloptmap_t clopt[] =
    {
        {'i', BFLD_INT, (void *)&print_ids, sizeof(print_ids), 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Print IDs only"},
        {'k', BFLD_INT, (void *)&print_keys, sizeof(print_keys), 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Print Keys only"},
        {0}
    };

    memset(&list, 0, sizeof(list));
            
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_sys_sysv_user_res(&list, NDRX_SV_RESTYPE_SEM))
    {
        fprintf(stderr, "Failed to list System V semaphores\n");
        NDRX_LOG(log_error, "Failed to list System V Semaphores");
        EXFAIL_OUT(ret);
    }
    
    if (print_ids)
    {
        fprintf(stderr, "SEMAPHORE ID\n");
        fprintf(stderr, "------------\n");
    }
    else if (print_keys)
    {
        fprintf(stderr, "   IPC KEY\n");
        fprintf(stderr, "----------\n");
    }
    else
    {
        fprintf(stderr, "SEMAPHORE ID    IPC KEY\n");
        fprintf(stderr, "------------ ----------\n");
    }
        
    p_res = (mdrx_sysv_res_t *)list.mem;
    
    for (i=0; i<=list.maxindexused; i++)
    {
        if (print_ids)
        {
            fprintf(stdout, "%12u\n", p_res[i].id);
        }
        else if (print_keys)
        {
            fprintf(stdout, "0x%08x\n", p_res[i].key);
        }
        else
        {
            fprintf(stdout, "%12u 0x%08x\n", p_res[i].id, p_res[i].key);
        }
    }
    
out:
    ndrx_growlist_free(&list);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
