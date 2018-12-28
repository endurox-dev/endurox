/**
 * @brief ndrxd daemon ping
 *
 * @file cmd_dping.c
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
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include "nclopt.h"
#include <ndrx.h>
#include <qcommon.h>
#include <atmi_cache.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Put ndrxd in sleep for certain period of time (used for debugging).
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @param p_have_next
 * @return 
 */
expublic int cmd_dsleep(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    command_dsleep_t call;
    
    memset(&call, 0, sizeof(call));
    
    call.secs = atoi(argv[1]);
    
    ret=cmd_generic_listcall(p_cmd_map->ndrxd_cmd, NDRXD_SRC_ADMIN,
                    NDRXD_CALL_TYPE_DSLEEP,
                    (command_call_t *)&call, sizeof(call),
                    G_config.reply_queue_str,
                    G_config.reply_queue,
                    G_config.ndrxd_q,
                    G_config.ndrxd_q_str,
                    argc, argv,
                    p_have_next,
                    G_call_args,
                    EXFALSE,
                    G_config.listcall_flags);
    
out:
    return ret;    
}

/**
 * Print NDRXD pid number (from file)
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @param p_have_next
 * @return 
 */
expublic int cmd_dpid(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    pid_t pid;
    int ret = EXSUCCEED;
    
    if (EXFAIL==(pid = ndrx_ndrxd_pid_get()))
    {
        EXFAIL_OUT(ret);
    }
    
    fprintf(stdout, "%d\n", pid);
    
out:
    return ret;
    
}
/**
 * Perform ndrxd ping
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_dping(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    long loops = 4;
    long i;
    int seq;
    long tim;
    
    ncloptmap_t clopt[] =
    {
        {'c', BFLD_LONG, (void *)&loops, sizeof(loops), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Count of pings, default 4"},
        {0}
    };
        
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }

    /* we need to init TP subsystem... (daemon queues, etc...) */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    for (i=0; i<loops; i++)
    {
        if (EXSUCCEED!=ndrx_ndrxd_ping(&seq, &tim, G_config.reply_queue, 
                G_config.reply_queue_str))
        {
            fprintf(stdout, "loop %ld: ndrxd_ping_seq=%d: timeout or system error\n", 
                    i, seq);
        }
        else
        {
            fprintf(stdout, "loop %ld: ndrxd_ping_seq=%d time=%ld ms\n", i, seq, tim);
        }
        
        sleep(1);
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
