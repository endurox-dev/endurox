/**
 * @brief `pqa' print queues, all
 *
 * @file cmd_pqa.c
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
 * Print header
 * @return
 */
exprivate void print_hdr(void)
{
    fprintf(stderr, "MSG QUEUED Q NAME\n");
    fprintf(stderr, "---------- ---------------------------------"
                    "------------------------------------\n");
}

/**
 * Get service listings
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_pqa(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    short print_all = EXFALSE;
    struct mq_attr att;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;  
    ncloptmap_t clopt[] =
    {
        {'a', BFLD_SHORT, (void *)&print_all, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Print all"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }

    /* Print header at first step! */
    print_hdr();
    
    qlist = ndrx_sys_mqueue_list_make(G_config.qpath, &ret);
    
    if (EXSUCCEED!=ret)
    {
        NDRX_LOG(log_error, "posix queue listing failed!");
        EXFAIL_OUT(ret);
    }
    
    LL_FOREACH(qlist,elt)
    {
        if (!print_all)
        {
            /* if not print all, then skip this queue */
            if (0!=strncmp(elt->qname, 
                    G_config.qprefix, strlen(G_config.qprefix)))
            {
                continue;
            }
        }
        if (EXSUCCEED!=ndrx_get_q_attr(elt->qname, &att))
        {
            /* skip this one... */
            continue;
        }

        fprintf(stdout, "%-10d %s\n", (int)att.mq_curmsgs, elt->qname);
    }

out:
    if (NULL!=qlist)
    {
        ndrx_string_list_free(qlist);
    }
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
