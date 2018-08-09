/**
 * @brief Command line argument parsing
 *
 * @file nclopt.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
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

/* Missing on AIX */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <nstdutil.h>
#include <nclopt.h>
#include <ndebug.h>

#include <ubf.h>

/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Parse command line options in standard way
 * @param opts - mapping list
 * @param print_values - should we print the values to debug?
 * @param argc - argument count
 * @param argv - argument list
 * @return SUCCEED/FAIL
 */
expublic int nstd_parse_clopt(ncloptmap_t *opts, int print_values, 
        int argc, char **argv, int ignore_unk)
{
    int ret = EXSUCCEED;
    char clopt_string[1024]={EXEOS};
    int len=0;
    signed char c; /* fix for rpi */
    ncloptmap_t *p = opts;
    
    optind=1; /* reset lib, so that we can scan again. */
    
    while (0!=p->key)
    {
        clopt_string[len]=p->key;
        p->have_loaded = EXFALSE;
        len++;
        if (p->flags & NCLOPT_HAVE_VALUE)
        {
            clopt_string[len]=':';
            len++;
        }
        clopt_string[len]=EXEOS;
        p++;
    }
    
    NDRX_LOG(log_debug, "clopt_string built: [%s] argc: [%d]", 
            clopt_string, argc);

    while ((c = getopt(argc, argv, clopt_string)) != EXFAIL)
    {
        
        /* search for key... */
        p = opts;
        while (0!=p->key)
        {
            if (p->key==c)
            {
                break;
            }
            p++;
        }
        
        if (0==p->key)
        {
            if (!ignore_unk)
            {
                NDRX_LOG(log_error, "Unknown command line option: [%c]", 
                        c);
                EXFAIL_OUT(ret);
            }
            else
            {
                continue;
            }
        }
        
        /* Translate the value */
        if (!(p->flags & NCLOPT_HAVE_VALUE))
        {
            short *val = (short *)p->ptr;
            *val = EXTRUE;
            NDRX_LOG(log_debug, "%c (%s) = [TRUE]", c, p->descr);
        }
        else
        {
            switch (p->datatype)
            {
                case BFLD_SHORT:
                    {
                        short *val = (short *)p->ptr;
                        *val = (short) atoi(optarg);
                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%hd]", c, 
                                    p->descr, *val);
                        }
                    }
                    break;
                case BFLD_LONG:
                    {
                        long *val = (long *)p->ptr;
                        *val = (long) atol(optarg);
                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%ld]", c, 
                                    p->descr, *val);
                        }
                    }
                    break;
                case BFLD_CHAR:
                    {
                        char *val = (char *)p->ptr;
                        *val = optarg[0];
                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%c]", c, 
                                    p->descr, *val);
                        }
                    }
                    break;
                case BFLD_FLOAT:
                    {
                        float *val = (float *)p->ptr;
                        *val = atof(optarg);

                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%f]", c, 
                                    p->descr, *val);
                        }

                    }
                    break;
                case BFLD_DOUBLE:
                    {
                        double *val = (double *)p->ptr;
                        *val = atof(optarg);
                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%lf]", c, 
                                    p->descr, *val);
                        }
                    }
                    break;
                case BFLD_STRING: 
                    {
                        int tmp = strlen(optarg);
                        if (tmp+1>p->buf_size)
                        {
                            NDRX_LOG(log_error, "Clopt [%c] invalid len: %d",
                                            c, tmp);
                            EXFAIL_OUT(ret);
                        }
                        strcpy((char *)p->ptr, optarg);

                        if (print_values)
                        {
                            NDRX_LOG(log_debug, "%c (%s) = [%s]", c, 
                                    p->descr, (char *)p->ptr);
                        }
                    }
                    break;
            }
        }
        
        p->have_loaded = EXTRUE;
    }
    
    /* Now cross check is all mandatory fields loaded... */
    p = opts;
    while (0!=p->key)
    {
        if (p->flags & NCLOPT_MAND && !p->have_loaded)
        {
            NDRX_LOG(log_error, "Missing command line option %c (%s)!",
                    p->key, p->descr);
            EXFAIL_OUT(ret);
        }
        
        p++;
    }
    
out:
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
