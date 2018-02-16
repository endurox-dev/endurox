/* 
** Test Tool 48 - basically call service and test for cache data
** Field T_STRING_10_FLD - this stores the name of timestamp field, so that
** service knows where to store the timestamp
**
** @file testtool48.c
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
#include <math.h>

#include <ctype.h>
#include <unistd.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include <ubf_int.h>
#include "test48.h"
/*---------------------------Externs------------------------------------*/

extern int optind, optopt, opterr;
extern char *optarg;

/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate char M_svcnm[MAXTIDENT+1] = {EXEOS};
exprivate UBFH *M_p_ub = NULL;
exprivate int M_result_must_from_cache = EXTRUE;
exprivate int M_numcalls = 1;
exprivate long M_tpurcode = 0;
exprivate int M_tperrno = 0;
exprivate int M_first_goes_to_cache = EXTRUE; /* first call goes to cache (basically from svc) */
exprivate long M_tpcall_flags = 0; /* Additional tpcall flags */
/*---------------------------Prototypes---------------------------------*/

/**
 * Perform the testing
 * @return 
 */
exprivate int main_loop(void)
{
    int ret = EXSUCCEED;
    long i;
    UBFH *p_ub = NULL;
    long t;
    long tusec;
    
    long t_svc;
    long tusec_svc;
    
    long olen;
    int data_from_cache;
    /* take a copy from UBF */
    
    
    for (i=0; i<M_numcalls; i++)
    {
        NDRX_LOG(log_debug, "into loop %ld", i);
        usleep(2000); /* sleep 2ms for new timestamp */
        
        if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, Bsizeof(M_p_ub)+1024)))
        {
            NDRX_LOG(log_error, "Failed to allocate test buffer: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Bcpy(p_ub, M_p_ub))
        {
            NDRX_LOG(log_error, "Failed to copy test buffer: %s", 
                    Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        /* get tstamp here... */
        ndrx_utc_tstamp2(&t, &tusec);
        
        /* call services */
        
        ret=tpcall(M_svcnm, (char *)p_ub, 0L, (char **)&M_p_ub, &olen, M_tpcall_flags);
        
        if (M_tperrno!=0)
        {
            if (tperrno!=M_tperrno)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected tperrno=%d got %d", 
                        M_tperrno, tperrno);
                EXFAIL_OUT(ret);
            }
            
            ret = EXSUCCEED;
        }
        else if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "TESTERROR: service call shall SUCCEED, but FAILED!");
            EXFAIL_OUT(ret);
        }
        
        if (M_tpurcode!=tpurcode)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected tpurcode=%ld got %ld", 
                    M_tpurcode, tpurcode);
            EXFAIL_OUT(ret);
        }
        
        /* if tstamp from service >= tstamp, then data comes from service and
         * not from cache
         * Store stamps here:
         * T_LONG_2_FLD
         * T_LONG_3_FLD
         */
        
        
        
        
        
        if (i==0 && M_first_goes_to_cache && data_from_cache)
        {
            /* TESTERROR, record must be new */
        }
        else if (M_result_must_from_cache && !data_from_cache)
        {
            /* TESTERROR, record must be from cache */
        }
        else if (!M_result_must_from_cache && data_from_cache)
        {
            /* TESTERROR, record must be new!!! */
        }
        
        
        tpfree((char *)p_ub);
        p_ub = NULL;
    }
    
    
out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * We shall load arguments like:
 * -s service_name
 * -b "json2ubf buffer"
 * -t tstamp_field
 * [-c Y|N - should result be out from cache or newly allocated?, default TRUE]
 * [-n <number of calls>, dflt 1]
 * [-r <tpurcode expected>, dflt 0]
 * [-e <error code expected>, dftl 0]
 * [-f <first_should_cache Y|N, if -n > 1 >, dftl Y]
 * [-l <look in cache flag>, TPNOCACHELOOK tpcall flag]
 * [-x <do not add to cache>, TPNOCACHEADD tpcall flag]
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    int c;
    
    opterr = 0;
    
    if (NULL==(M_p_ub = (UBFH *)tpalloc("UBF", NULL, 56000)))
    {
        NDRX_LOG(log_error, "Failed to allocate UBF: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    while ((c = getopt (argc, argv, "s:b:t:c:n:r:e:f:lx")) != EXFAIL)
    {
        NDRX_LOG(log_debug, "%c = [%s]", (char)c, optarg);
        
        switch (c)
        {
            case 's':
                NDRX_STRCPY_SAFE(M_svcnm, optarg);
                break;
            case 'b':
                /* JSON buffer, build UBF... */

                NDRX_LOG(log_debug, "Parsing: [%s]", optarg);

                if (EXSUCCEED!=tpjsontoubf(M_p_ub, optarg))
                {
                    NDRX_LOG(log_error, "Failed to parse [%s]", optarg);
                    EXFAIL_OUT(ret);
                }

                break;
            case 'c':

                if ('Y'==optarg[0] || 'y'==optarg[0])
                {
                    M_result_must_from_cache=EXTRUE;
                }
                else
                {
                    M_result_must_from_cache=EXFALSE;
                }

                break;
            case 'n':
                M_numcalls = atoi(optarg);
                break;
            case 'r':
                M_tpurcode = atol(optarg);
                break;
            case 'e':
                M_tperrno = atoi(optarg);
                break;
            case 'f':
                
                if ('Y'==optarg[0] || 'y'==optarg[0])
                {
                    M_first_goes_to_cache=EXTRUE;
                }
                else
                {
                    M_first_goes_to_cache=EXFALSE;
                }
                
                break;
            case 'l':
                M_tpcall_flags|=TPNOCACHELOOK;
                break;
            case 'x':
                M_tpcall_flags|=TPNOCACHEADD;
                break;
            case '?':
                if (optopt == 'c')
                {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                    EXFAIL_OUT(ret);
                }
                else if (isprint (optopt))
                {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                    EXFAIL_OUT(ret);
                }
                else
                {
                    fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                    EXFAIL_OUT(ret);
                }
                return 1;
            default:
                abort ();
        }
    }
    
    /* validate config! */    
    
    NDRX_LOG(log_debug, "M_svcnm = [%s]", M_svcnm);
    NDRX_LOG(log_debug, "M_p_ub = %p", M_p_ub);
    NDRX_LOG(log_debug, "M_result_must_from_cache=%d", M_result_must_from_cache);
    NDRX_LOG(log_debug, "M_numcalls=%d", M_numcalls);
    NDRX_LOG(log_debug, "%M_tpurcode=ld", M_tpurcode);
    NDRX_LOG(log_debug, "M_errcode=%d", M_tperrno);
    NDRX_LOG(log_debug, "M_first_goes_to_cache=%d", M_first_goes_to_cache);
    NDRX_LOG(log_debug, "M_tpcall_flags %ld", M_tpcall_flags);
    
    
    /* TODO: Copy to string 10! */
    
    if (EXEOS==M_svcnm[0])
    {
        NDRX_LOG(log_error, "-s: Service name cannot be empty!");
        EXFAIL_OUT(ret);
    }
    
    if (NULL==M_p_ub)
    {
        NDRX_LOG(log_error, "-b: Mandatory!");
        EXFAIL_OUT(ret);
    }
    
    if (M_numcalls <= 0)
    {
        NDRX_LOG(log_error, "-n: Number of call must be possitive!");
        EXFAIL_OUT(ret);
    }
    
    /* loop over */
    if (EXSUCCEED!=main_loop())
    {
        EXFAIL_OUT(ret);
    }
    
out:
                
    if (NULL!=M_p_ub)
    {
        tpfree((char *)M_p_ub);
    }

    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
