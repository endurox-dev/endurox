/* 
** Test Tool 48 - basically call service and test for cache data
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
exprivate BFLDID  M_tstamp_fld = BBADFLDID;
exprivate int M_result_is_cached = EXTRUE;
exprivate int M_numcalls = 1;
/*---------------------------Prototypes---------------------------------*/

/**
 * We shall load arguments like:
 * -s service_name
 * -b "json2ubf buffer"
 * -t tstamp_field
 * [-c Y|N - should result be out from cache or newly allocated?, default TRUE]
 * [-n <number of calls>, dflt 1]
 * [-r <tpurcode expected>, dflt 0]
 * [-e <error code expected>, dftl 0]
 * [-f <first_should_cache Y|N, if -n > 1 >, dftl N]
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
    
    while ((c = getopt (argc, argv, "s:b:t:c:n:r:e:")) != EXFAIL)
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
            case 't':
                NDRX_LOG(log_debug, "Timestamp field [%s]", optarg);

                if (BBADFLDID==(M_tstamp_fld = Bfldid(optarg)))
                {
                    NDRX_LOG(log_error, "Failed to parse: [%s]: %s", Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }

                break;
            case 'c':

                if ('Y'==optarg[0] || 'y'==optarg[0])
                {
                    M_result_is_cached=EXTRUE;
                }
                else
                {
                    M_result_is_cached=EXFALSE;
                }

                break;
            case 'n':
                M_numcalls = atoi(optarg);
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

out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}
