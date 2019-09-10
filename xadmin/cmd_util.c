/**
 * @brief Various utility commands
 *
 * @file cmd_util.c
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
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>

#include "nclopt.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print all processes on stdout
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_ps(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret=EXSUCCEED;
    /* Add filter options... */
    char flt_a[PATH_MAX/4]={EXEOS};
    char flt_b[PATH_MAX/41]={EXEOS};
    char flt_c[PATH_MAX/4]={EXEOS};
    char flt_d[PATH_MAX/4]={EXEOS};
    char flt_r[PATH_MAX/4]={EXEOS};
    long pid_x = EXFAIL;
    short pid_only = EXFALSE;
    short mems = EXFALSE;
    pid_t me = getpid();
    pid_t they;
    
    string_list_t * pslist = NULL;
    string_list_t * proc = NULL;
    
    ncloptmap_t clopt[] =
    {
        {'a', BFLD_STRING, (void *)flt_a, sizeof(flt_a), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Filter A (grep style)"},
        {'b', BFLD_STRING, (void *)flt_b, sizeof(flt_b), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Filter B (grep style)"},
        {'c', BFLD_STRING, (void *)flt_c, sizeof(flt_c), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Filter C (grep style)"},
        {'d', BFLD_STRING, (void *)flt_d, sizeof(flt_d), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Filter D (grep style)"},
        {'r', BFLD_STRING, (void *)flt_r, sizeof(flt_r), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Posix regexp"},
        {'p', BFLD_SHORT,  (void *)&pid_only, sizeof(pid_only), 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Print pid only"},
        {'m', BFLD_SHORT,  (void *)&mems, sizeof(mems), 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL,
                            "Print memory stats (pid), -p mode only (pid:rss:vsz in kb)"},
        {'x', BFLD_LONG,   (void *)&pid_x, sizeof(pid_x), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Exclude pid"},
        {0}
    };
    
    /* parse command line */
    
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    if (mems && !pid_only)
    {
        fprintf(stderr, XADMIN_ERROR_FORMAT_PFX "-m can be used only with -p!\n");
        EXFAIL_OUT(ret);
    }
    
    pslist = ndrx_sys_ps_list(flt_a, flt_b, flt_c, flt_d, flt_r);
    
    if (NULL!=pslist)
    {
        /* print to stdout... */
        DL_FOREACH(pslist, proc)
        {
            if (EXSUCCEED==ndrx_proc_pid_get_from_ps(proc->qname, &they))
            {
                /* will not print our selves... as usually not needed */
                
                if (EXFAIL!=pid_x && (int)pid_x==they)
                {
                    /* Exclude pid... */
                    NDRX_LOG(log_debug, "Exclude %d", (int)they);
                }
                else if (me!=they)
                {
                    if (pid_only)
                    {
                        ndrx_proc_info_t inf;
                        
                        if (mems)
                        {
                            /* read memory stats... */
                            memset(&inf, 0, sizeof(inf));
                            
                            if (EXSUCCEED!=ndrx_proc_get_infos(they, &inf))
                            {
                                fprintf(stderr, "Failed to read %d stats\n", 
                                        (int)they);
                            }
                            printf("%d:%ld:%ld\n", (int)they, inf.rss, inf.vsz);
                        }
                        else
                        {
                            printf("%d\n", (int)they);
                        }
                    }
                    else
                    {
                        printf("%s\n", proc->qname);
                    }
                }
            }
            else if (!pid_only)
            {
                /* do not show our selves (our pid here) */
                printf("%s\n", proc->qname);
            }
        }
    }
    
out:
    
    if (NULL!=pslist)
    {
         ndrx_string_list_free(pslist);
    }

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
