/* 
** Send buffer from stdin to specified service,
** i.e. using SRVCNM.
**
** @file xmemck.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <atmi.h>
#include <atmi_int.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <ubf.h>
#include <ubf_int.h>
#include <fdatatype.h>
#include <exmemck.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/    
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate volatile int M_keep_running = EXTRUE;
/*---------------------------Prototypes---------------------------------*/

/**
 * Singnal handler for deinti
 */
expublic void intHandler(int sig)
{
    M_keep_running = EXFALSE;
}

/**
 * Print leaky info
 * @param proc
 */
expublic void xmem_print_leaky(exmemck_process_t *proc)
{
    fprintf(stdout, ">>> LEAK pid=%d! rss: %ld -> %ld, vsz %ld -> %ld: [%s]\n",
            proc->pid, proc->avg_first_halve_rss, proc->avg_second_halve_rss,
            proc->avg_first_halve_vsz, proc->avg_second_halve_vsz, proc->psout);
    
    fflush(stdout);
}

/**
 * Print process exit summary..
 * @param proc
 */
expublic void xmem_print_exit(exmemck_process_t *proc)
{
    NDRX_LOG(log_info, "Terminated: %d [%s]", proc->pid, proc->psout);
}

/**
 * Main entry point for `xmemck' utility
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    int c;
    int period = 1;
    
    exmemck_settings_t settings;
    
    memset(&settings, sizeof(settings), 0);
    
    settings.pf_proc_exit = xmem_print_exit;
    settings.pf_proc_leaky = xmem_print_leaky;
    
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);
    
    
    while ((c = getopt(argc, argv, "p:d:s:t:m:")) != -1)
    {
        NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        switch(c)
        {
            case 'p':
                period = atoi(optarg);
                NDRX_LOG(log_debug, "Period set to: %d", period);
                break;
            case 'd':
                settings.percent_diff_allow = atoi(optarg);
                
                NDRX_LOG(log_debug, "Percent diff allow: %d%%", 
                        settings.percent_diff_allow);
                break;
            case 's':
                settings.interval_start_prcnt = atoi(optarg);
                
                NDRX_LOG(log_debug, "Percent interval start: %d%%", 
                        settings.interval_start_prcnt);
                break;
            case 't':
                settings.interval_stop_prcnt = atoi(optarg);
                
                NDRX_LOG(log_debug, "Percent interval stop: %d%%", 
                        settings.interval_stop_prcnt);
                break;
            case 'm':
                NDRX_LOG(log_debug, "Adding mask: [%s]", 
                        optarg);
                
                if (EXSUCCEED!=ndrx_memck_add(optarg,  NULL, &settings))
                {
                    NDRX_LOG(log_error, "ndrx_memck_add() failed to add [%s]", 
                            optarg);
                    EXFAIL_OUT(ret);
                }
                break;
        }
    }
    
    /* printf("Enduro/X memory leak checker starting...\n"); */
    
    while (M_keep_running)
    {
        if (EXSUCCEED!=ndrx_memck_tick())
        {
            NDRX_LOG(log_error, "ndrx_memck_tick() failed");
            EXFAIL_OUT(ret);
        }
        
        sleep(period);
    }
    
    
out:
                
    ndrx_memck_rmall();

    return ret;
}

