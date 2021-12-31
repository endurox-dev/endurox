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
#include <unistd.h>
#include <memory.h>

#include <ndebug.h>
#include <atmi.h>

#include "Exfields.h"


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_tran = EXFALSE; /**< use distr tran */
exprivate int M_usleep = 0; /**< Number of microseconds to sleep */
/*---------------------------Prototypes---------------------------------*/

/**
 * Service entry
 * @return SUCCEED/FAIL
 */
void EXBENCHSV (TPSVCINFO *p_svc)
{
    char btype[16]={EXEOS};
    char stype[16]={EXEOS};
    CLIENTID cltid;
    long size;
    BFLDLEN len;
    int ret = TPSUCCESS;
    
    if (M_usleep > 0)
    {
        usleep(M_usleep);
    }
    
    size = tptypes (p_svc->data, btype, stype);
    
    len = sizeof(cltid.clientdata);
    
    if (0==strcmp("UBF", btype) && EXSUCCEED==Bget((UBFH*)p_svc->data, 
            EX_CLTID, 0, cltid.clientdata, &len))
    {
        /* send the notification... */
        if (EXFAIL==tpnotify(&cltid, p_svc->data, p_svc->len, 0))
        {
            ret = TPFAIL;
        }
    }

    /* Run the notification this is ubf buffer.. */
    tpreturn(  ret,
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
    char event[XATMI_EVENT_MAX+1]="";
    
    /* Parse command line, will use simple getopt */
    while ((c = getopt(argc, argv, "s:N:TU:e:--")) != EXFAIL)
    {
        switch(c)
        {
            case 'N':
                svcnum = atoi(optarg);
                break;
            case 's':
                NDRX_STRCPY_SAFE(svcnm_base, optarg);
                break;
            case 'U':
                M_usleep = atoi(optarg);
                break;
            case 'T':
                M_tran = EXTRUE;
                break;
            case 'e':
                NDRX_STRCPY_SAFE(event, optarg);
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
        NDRX_LOG(log_error, "Failed to initialise EXBENCH: %s!", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
    if (EXEOS!=event[0])
    {
        TPEVCTL evctl;
        memset(&evctl, 0, sizeof(evctl));
        evctl.flags|=TPEVSERVICE;
        NDRX_STRCPY_SAFE(evctl.name1, svcnm);
        if (EXFAIL==tpsubscribe(event, NULL, &evctl, 0L))
        {
            NDRX_LOG(log_error, "Failed to subscribe to [%s] event: %s", 
                    event, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }

    if (M_tran && EXSUCCEED!=tpopen())
    {
        NDRX_LOG(log_error, "Failed to initialise tpopen: %s!", tpstrerror(tperrno));
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
    TP_LOG(log_info, "uninit");
    if (M_tran)
    {
        tpclose();
    }
}

/**
 * thread init
 * @param argc
 * @param argv
 * @return 
 */
int thinit(int argc, char ** argv)
{
    int ret = EXSUCCEED;
    
    if (M_tran && EXSUCCEED!=tpopen())
    {
        TP_LOG(log_error, "thinit: tailed to initialise tpopen: %s!", tpstrerror(tperrno));
        ret=EXFAIL;
        goto out;
    }
    
out:
    return ret;
}

/**
 * Thread un-init
 */
void thuninit(void)
{
    TP_LOG(log_info, "thuninit");
    if (M_tran)
    {
        tpclose();
    }
}
  

/* Auto generated system advertise table */
expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {
    { "", "EXBENCHSV", EXBENCHSV, 0, 0 }
    , { NULL, NULL, NULL, 0, 0 }
};

/**
 * Server program main entry, multi-threaded support
 * @param argc	argument count
 * @param argv	argument values
 * @return SUCCEED/FAIL
 */
int main( int argc, char** argv )
{
    _tmbuilt_with_thread_option=EXTRUE;
    struct tmsvrargs_t tmsvrargs =
    {
        &tmnull_switch,
        &ndrx_G_tmdsptchtbl[0],
        0,
        init,
        uninit,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        thinit,
        thuninit
    };
    
    return( _tmstartserver( argc, argv, &tmsvrargs ));
    
}



/* vim: set ts=4 sw=4 et smartindent: */
