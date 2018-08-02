/* 
 * @brief Common routines for system benchmarking
 *
 * @file benchmark.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
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

/*---------------------------Includes-----------------------------------*/
#include <ndrstandard.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "ndebug.h"
#include "nstdutil.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/* Enduro/X Benchmarking: */
#define CONF_NDRX_BENCH_FILE        "NDRX_BENCH_FILE"       /* Benchmark output file */
#define CONF_NDRX_BENCH_CONFIGNAME  "NDRX_BENCH_CONFIGNAME" /* Benchmark configuration description */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Write the benchmark statistics for later document processing (i.e. charting)
 * We will use following env variables:
 * - NDRX_BENCH_FILE - where to plot the output
 * @param param1
 * @param param2
 * @return SUCCEED/FAIL
 */
expublic int ndrx_bench_write_stats(double msgsize, double callspersec)
{
    static char *file;
    static char *config_name;
    static int first = EXTRUE;
    int ret = EXSUCCEED;
    FILE *f = NULL;
    
    if (first)
    {
        file = getenv(CONF_NDRX_BENCH_FILE);
        config_name = getenv(CONF_NDRX_BENCH_CONFIGNAME);
        first = EXFALSE;
    }
    
    if (NULL!=file && NULL!=config_name)
    {
        if( access( file, F_OK ) != EXFAIL )
        {
            if (NULL==(f=NDRX_FOPEN(file, "a")))
            {
                NDRX_LOG(log_error, "Failed to open [%s]: %s", file, strerror(errno));
                EXFAIL_OUT(ret);
            }   
        }
        else
        {
            /* file doesn't exist - create */
            if (NULL==(f=NDRX_FOPEN(file, "w")))
            {
                NDRX_LOG(log_error, "Failed to open [%s]: %s", file, strerror(errno));
                EXFAIL_OUT(ret);
            }
            
            fprintf(f, "\"Configuration\" \"MsgSize\" \"CallsPerSec\"\n");
        }
        fprintf(f, "\"%s\" %.0lf %.0lf\n", config_name, msgsize, callspersec);
    }
    else
    {
        NDRX_LOG(log_error, "%s or %s not set!", CONF_NDRX_BENCH_FILE, CONF_NDRX_BENCH_CONFIGNAME);
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    return ret;
}
