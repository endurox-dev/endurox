/**
 * @brief DDR functionality tests - client
 *
 * @file atmiclt84.c
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
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include <nstopwatch.h>
#include <fcntl.h>
#include <unistd.h>
#include <nstdutil.h>
#include "test84.h"
#include <unistd.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Do the test call to the server
 */
int main(int argc, char** argv)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, 56000);
    long rsplen;
    int ret=EXSUCCEED;
    char svcnm_proc[1024]={EXEOS}; /* service name which shall process request */
    int e=0; /* error expected */
    int c;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1]={EXEOS};
    char tmp[1024];
    int do_conv=EXFALSE;
    
    /* We shall parse cli, so field with will either:
     * -l (long)
     * -s (string)
     * -c (carray value)
     * -S Service name
     * -d (double)
     * -e <error code expected>
     * -g <group value expected in return>
     */
    while ((c = getopt(argc, argv, "S:s:l:d:e:g:Cc:")) != -1) {
        
        switch (c)
        {
            case 'C':
                NDRX_LOG(log_debug, "Doing conv");
                do_conv=EXTRUE;
                break;
            case 'S':
                NDRX_STRCPY_SAFE(svcnm, optarg);
                break;
            case 'g':
                NDRX_STRCPY_SAFE(svcnm_proc, optarg);
                break;
            case 'e':
                e = atoi(optarg);
                break;
            case 'l':
                if (EXFAIL==CBchg(p_ub, T_LONG_2_FLD, 0, optarg, 0, BFLD_STRING))
                {
                    NDRX_LOG(log_debug, "Failed to set T_LONG_2_FLD[0]: %s", Bstrerror(Berror));
                    ret=EXFAIL;
                    goto out;
                }
                break;
            case 's':
                if (EXFAIL==CBchg(p_ub, T_STRING_2_FLD, 0, optarg, 0, BFLD_STRING))
                {
                    NDRX_LOG(log_debug, "Failed to set T_STRING_2_FLD[0]: %s", Bstrerror(Berror));
                    ret=EXFAIL;
                    goto out;
                }
                break;
            case 'c':
                if (EXFAIL==CBchg(p_ub, T_CARRAY_2_FLD, 0, optarg, 0, BFLD_STRING))
                {
                    /* load carray... */
                    NDRX_LOG(log_debug, "Failed to set T_CARRAY_2_FLD[0]: %s", Bstrerror(Berror));
                    ret=EXFAIL;
                    goto out;
                }
                break;
            case 'd':
                if (EXFAIL==CBchg(p_ub, T_DOUBLE_2_FLD, 0, optarg, 0, BFLD_STRING))
                {
                    NDRX_LOG(log_debug, "Failed to set T_DOUBLE_2_FLD[0]: %s", Bstrerror(Berror));
                    ret=EXFAIL;
                    goto out;
                }
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s -S <service_name> -e <errcode_expt> -g <proc_service> -l <long_val>|-s <string_val>|-d <double_val> \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    tplogprintubf(log_debug, "UBF buffer", p_ub);

    /* call the service */
    if (do_conv)
    {
        long ev;
        int cd;
        long rcvlen;
        /* try to connect */
        if (EXFAIL == (cd=tpconnect(svcnm, (char *)p_ub, 0L, TPRECVONLY)))
        {
            NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
            /* check error code */
            if (tperrno!=e)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected error %d got %d", e, tperrno);
                ret=EXFAIL;
            }
            goto out;
        }
        
        if (EXSUCCEED==tprecv(cd, (char **)&p_ub, &rcvlen, 0, &ev))
        {
            NDRX_LOG(log_error, "TESTERROR: Expected con error!");
            EXFAIL_OUT(ret);
        }
        else if (tperrno!=TPEEVENT)
        {
            NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
            /* check error code */
            if (tperrno!=e)
            {
                NDRX_LOG(log_error, "TESTERROR: Expected error %d got %d", e, tperrno);
                EXFAIL_OUT(ret);
            }
        }
    }
    else if (EXFAIL == tpcall(svcnm, (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
    {
        NDRX_LOG(log_error, "%s failed: %s", svcnm, tpstrerror(tperrno));
        
        /* check error code */
        if (tperrno!=e)
        {
            NDRX_LOG(log_error, "TESTERROR: Expected error %d got %d", e, tperrno);
            ret=EXFAIL;
        }
        goto out;
    }
    
    if (EXSUCCEED!=Bget(p_ub, T_STRING_FLD, 0, tmp, 0L))
    {
        NDRX_LOG(log_debug, "Failed to get T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    if (0!=strcmp(tmp, svcnm_proc))
    {
        NDRX_LOG(log_error, "TESTERROR: Expected service [%s] to process request, but got [%s]",
                svcnm_proc, tmp);
        EXFAIL_OUT(ret);
    }
    
out:
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
