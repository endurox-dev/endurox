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
    fprintf(stderr, "SLOT   MSGID      FLAGS             CTIME    Q NAME\n");
    fprintf(stderr, "------ ---------- ----------------- -------- -----------------------------------\n");
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
    short print_systemv = EXFALSE;
    short print_posix =  EXFALSE;
    short print_all =  EXFALSE;
    short print_isused =  EXFALSE;
    short print_wasused =  EXFALSE;
    int attach_status = EXFALSE;
    ndrx_shm_t *map_p2s;
    ndrx_shm_t *map_s2p;
    ndrx_sem_t *map_sem;
    int i, queuesmax;
    ndrx_svq_map_t *el;
    ndrx_shm_t *map;
    char flagsstr[128];
    int total = 0;
    ncloptmap_t clopt[] =
    {
        {'p', BFLD_SHORT, (void *)&print_posix, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Posix -> System V mapping table (default)"},
        {'s', BFLD_SHORT, (void *)&print_systemv, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, 
                                "Print System V -> Posix mapping table"},
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
    
    if (EXFAIL==(attach_status = ndrx_svqshm_attach()))
    {
        fprintf(stderr, "System V shared resources does not exists\n");
        NDRX_LOG(log_error, "System V shared resources does not exists");
        EXFAIL_OUT(ret);
    }
    
    if (!ndrx_svqshm_shmres_get(&map_p2s, &map_s2p, &map_sem, &queuesmax))
    {
        fprintf(stderr, "System malfunction, System V SHM must be open!\n");
        NDRX_LOG(log_error, "System malfunction, System V SHM must be open");
        userlog("System malfunction, System V SHM must be open");
        EXFAIL_OUT(ret);
    }
    
    /* Print header at first step! */
    print_hdr();
    
    /* select table & process that flags and print the entries... */
    
    /* Read lock resources  */
    map = map_p2s;
    
    if (print_systemv)
    {
        map = map_s2p;
    }
    
    /* lock the memory */
    /* ###################### CRITICAL SECTION ############################### */
    if (EXSUCCEED!=ndrx_sem_rwlock(map_sem, 0, NDRX_SEM_TYP_READ))
    {
        goto out;
    }
    
    for (i=0; i<queuesmax; i++)
    {
        el = NDRX_SVQ_INDEX(map->mem, i);
        
        if (print_all ||
                (print_isused && (el->flags & NDRX_SVQ_MAP_ISUSED)) ||
                
                (print_wasused && (el->flags & NDRX_SVQ_MAP_WASUSED) && 
                    !(el->flags & NDRX_SVQ_MAP_ISUSED))
                
                )
        {
            flagsstr[0]=EXEOS;
            
            if (el->flags & NDRX_SVQ_MAP_ISUSED)
            {
                if (EXEOS!=flagsstr[0])
                {
                    strcat(flagsstr, ",");
                }
                strcat(flagsstr, "iuse");
            }
            else if (el->flags & NDRX_SVQ_MAP_WASUSED)
            {
                if (EXEOS!=flagsstr[0])
                {
                    strcat(flagsstr, ",");
                }
                strcat(flagsstr, "wuse");
            }
            
            if (el->flags & NDRX_SVQ_MAP_RQADDR)
            {
                if (EXEOS!=flagsstr[0])
                {
                    strcat(flagsstr, ",");
                }
                strcat(flagsstr, "rqaddr");
            }
            
            fprintf(stdout, "%-6d %-10d %-17.17s %-8.8s %s\n",
                    i, el->qid, flagsstr, 
                    ndrx_decode_msec(ndrx_stopwatch_get_delta(&el->ctime), 0, 0, 2),
                    el->qstr);
            total++;
        }
    }
    
    /* un-lock the memory */
    ndrx_sem_rwunlock(map_sem, 0, NDRX_SEM_TYP_READ);
    /* ###################### CRITICAL SECTION, END ########################## */
    
    fprintf(stderr, "\nTOTAL: %d\n", total);
    

out:
    
    if (EXTRUE==attach_status)
    {
        ndrx_svqshm_detach();
    }

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
