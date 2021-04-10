/**
 * @brief Benchmark tool server (echo responder)
 *
 * @file exbenchsv.c
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

#include <ndebug.h>
#include <atmi.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Service entry
 * @return SUCCEED/FAIL
 */
void EXBENCHSV (TPSVCINFO *p_svc)
{
    tpreturn(  TPSUCCESS,
		0L,
		(char *)p_svc->data,
		0L,
		0L);
}

/**
 * Initialize the application
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int init(int argc, char** argv)
{
    int ret = EXSUCCEED;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char svcnm_base[XATMI_SERVICE_NAME_LENGTH+1]="EXBENCH";
    int c;
    int svcnum=0;
    
        /* Parse command line, will use simple getopt */
    while ((c = getopt(argc, argv, "s:N:--")) != EXFAIL)
    {
        switch(c)
        {
            case 'N':
                svcnum = atoi(optarg);
                break;
            case 's':
                NDRX_STRCPY_SAFE(svcnm_base, optarg);
                break;
            
        }
    }
    
    
    if (svcnum > 0)
    {
        snprintf(svcnm, sizeof(svcnm), "%s%03d", svcnm_base, (tpgetsrvid() % 1000) % svcnum );
    }
    else
    {
        NDRX_STRCPY_SAFE(svcnm, svcnm_base);
    }

    /* Advertise our service */
    if (EXSUCCEED!=tpadvertise(svcnm, EXBENCHSV))
    {
        NDRX_LOG(log_error, "Failed to initialise EXBENCH!");
        ret=EXFAIL;
        goto out;
    }

out:

	
    return ret;
}

/**
 * Terminate the application
 */
void uninit(void)
{
    TP_LOG(log_info, "Uninit");
}

/**
 * Server program main entry
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int main(int argc, char** argv)
{
    /* Launch the Enduro/x thread */
    return ndrx_main_integra(argc, argv, init, uninit, 0);
}

/* vim: set ts=4 sw=4 et smartindent: */
