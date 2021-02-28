/**
 * @brief `prtsvc' Print routing services
 *
 * @file cmd_prtvsc.c
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
#include <lcfint.h>
#include <ndrx_ddr.h>
#include <atmi_shm.h>
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
    
    fprintf(stderr, "  SLOT SERVICE NAME                   PRIO ROUTE           T TRANTIME FLAGS\n");
    fprintf(stderr, "------ ------------------------------ ---- --------------- - -------- ---------\n");
}

/**
 * Print routing data, requires that Enduro/X is started
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_prtsvc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    short print_all =  EXFALSE;
    short print_isused =  EXFALSE;
    short print_wasused =  EXFALSE;
    int i;
    ndrx_services_t *el;
    ndrx_services_t *ptr;
    char flagsstr[128];
    int total = 0;
    int page;
    ncloptmap_t clopt[] =
    {
        {'a', BFLD_SHORT, (void *)&print_all, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print all entries"},
        {'i', BFLD_SHORT, (void *)&print_isused, 0, 
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
    
    if (!print_isused && !print_wasused && !print_all)
    {
        print_isused = EXTRUE;
    }
    
    if (EXFAIL==tpinit(NULL))
    {
        fprintf(stderr, "* Failed to become client\n");
        EXFAIL_OUT(ret);
    }
    
    /* Print header at first step! */
    print_hdr();
    
    page = ndrx_G_shmcfg->ddr_page;
    ptr = (ndrx_services_t *) (ndrx_G_routsvc.mem + page*G_atmi_env.rtsvcmax * sizeof(ndrx_services_t));
    for (i=0; i<G_atmi_env.rtsvcmax; i++)
    {
        el = NDRX_DDRV_SVC_INDEX(ptr, i);
        
        if (print_all ||
                (print_isused && (el->flags & NDRX_LH_FLAG_ISUSED)) ||
                
                (print_wasused && (el->flags & NDRX_LH_FLAG_WASUSED) && 
                    !(el->flags & NDRX_LH_FLAG_ISUSED))
                
                )
        {
            flagsstr[0]=EXEOS;
            
            if (el->flags & NDRX_LH_FLAG_ISUSED)
            {
                if (EXEOS!=flagsstr[0])
                {
                    NDRX_STRCAT_S(flagsstr, sizeof(flagsstr), ",");
                }
                NDRX_STRCAT_S(flagsstr, sizeof(flagsstr), "iuse");
            }
            else if (el->flags & NDRX_LH_FLAG_WASUSED)
            {
                if (EXEOS!=flagsstr[0])
                {
                    NDRX_STRCAT_S(flagsstr, sizeof(flagsstr), ",");
                }
                NDRX_STRCAT_S(flagsstr, sizeof(flagsstr), "wuse");
            }
            
            fprintf(stdout, "%6d %-30.30s %4d %-15.15s %c %8lu %s\n",
                    i, el->svcnm, el->prio, el->criterion, el->autotran?'Y':'N',
                    el->trantime, flagsstr);
            total++;
        }
    }
    
    fprintf(stderr, "\nTOTAL: %d\n", total);
    
out:
                        
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
