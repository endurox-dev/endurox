/**
 * @brief `svmqps' print System V mapping tables
 *
 * @file cmd_svmaps.c
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
    fprintf(stderr, "SLOT   MSGID     FLAGS             CTIME (MONOTONIC)   Q NAME\n");
    fprintf(stderr, "------ --------- ----------------- ------------------- -------------------------\n");
}

/**
 * Print System V queue mapping tables
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_svmaps(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    int n;
    short print_systemv = EXFALSE;
    short print_posix =  EXFALSE;
    short print_all =  EXFALSE;
    short print_inuse =  EXFALSE;
    short print_wasused =  EXFALSE;
    struct mq_attr att;
    string_list_t* qlist = NULL;
    string_list_t* elt = NULL;
    ncloptmap_t clopt[] =
    {
        {'p', BFLD_SHORT, (void *)&print_systemv, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Posix -> System V mapping table (default)"},
        {'s', BFLD_SHORT, (void *)&print_posix, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print System V -> Posix mapping table"},
        {'a', BFLD_SHORT, (void *)&print_all, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print all entries"},
        {'i', BFLD_SHORT, (void *)&print_inuse, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print entries in use"},
        {'w', BFLD_SHORT, (void *)&print_wasused, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print entries which was used"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    /* TODO: Print current monotonic clock */
    
    /* Print header at first step! */
    print_hdr();
    
    /* select table & process that flags and print the entries... */
   

out:
    
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
