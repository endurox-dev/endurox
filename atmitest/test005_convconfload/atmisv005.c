/**
 * @brief This simulates ATM configuration server
 *
 * @file atmisv005.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ndebug.h>
#include <atmi.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <test.fd.h>


/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
int M_cd=EXFAIL;
UBFH *M_p_ub=NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Open configuration download
 * @return
 */
exprivate short cnf_openf(void)
{
    short   ret = EXSUCCEED;
    long    len;
    long    revent;
    char    *p;
    char    termcode[64];
    /* expect SENDONLY to pass rply */
    if (EXFAIL == tprecv(M_cd, (char **)&M_p_ub, &len, 0L, &revent))
    {
        if (TPEEVENT == tperrno )
        {
            if (TPEV_SENDONLY != revent)
            {
                if (TPEV_DISCONIMM == revent)
                {
                    NDRX_LOG(log_warn,
                     "Partner aborted conversation");
                }
                else
                {
                    NDRX_LOG(log_error,
                        "TESTERROR: Unexpected conv event %lx",
                         revent );
                }
                ret = EXFAIL;
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_error, "Conv receive err %d", tperrno );
            ret=EXFAIL;
            goto out;
        }
    }
    
    /*
     * Get the key stuff
     */
    if (EXFAIL==Bget(M_p_ub, T_STRING_2_FLD, 0, termcode, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD[0]");
        ret=EXFAIL;
    }
    else if (0!=strcmp(termcode, "TERMINAL_T"))
    {
        NDRX_LOG(log_error, "TESTERROR: Got invalid T_STRING_2_FLD=[%s]",
                                        termcode);
        ret=EXFAIL;
    }

out:
    return ret;
}

/**
 * Send configuration data
 * @param fld
 * @param data
 * @return 
 */
int send_config_data(BFLDID fld, char *data)
{
    int ret=EXSUCCEED;
    long revent;
    tpfree((char *)M_p_ub);

    if ( !(M_p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)) )
    {
        NDRX_LOG(log_error, "TESTERROR: FML alloc failed");
        ret=EXFAIL;
        goto out;
    }
    else if (EXSUCCEED == CBchg(M_p_ub, fld, 0,
                                            data,
                                            0L,
                                            BFLD_STRING) )
    {
            if (EXFAIL == tpsend(M_cd,
                                    (char *)M_p_ub,
                                    0L,
                                    TPNOTIME,       /* DIRTY HACK   */
                                    &revent))
            {
                    NDRX_LOG(log_error, "Send data failed %d %ld",
                                                    tperrno, revent);
            }
            else
            {
                    NDRX_LOG(log_debug,"sent");
            }

    }
    
out:
    return ret;
}

/**
 * Main server entry
 * @param p_svc
 */
void CONVSV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    int i;
    M_p_ub = (UBFH *)p_svc->data;
    M_cd = p_svc->cd; /* Our call descriptor (server side) */
    char databuf[512] = {EXEOS};

    NDRX_LOG(log_debug, "CONVSV got call");

    if (EXFAIL==cnf_openf())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open config!");
        ret=EXFAIL;
        goto out;
    }

    for (i=0; i<100; i++)
    {
        snprintf(databuf, sizeof(databuf), "svc send %d", i);
        if (EXFAIL==send_config_data(T_STRING_FLD, databuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to send config!");
            ret=EXFAIL;
            goto out;
        }
    }
    
out:
    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0L,
                (char *)NULL,
                0L,
                0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret = EXSUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (EXSUCCEED!=tpadvertise("CONVSV", CONVSV))
    {
        NDRX_LOG(log_error, "Failed to initialize CONVSV!");
        ret=EXFAIL;
    }
    
    return ret;
}

/**
 * Do de-initialization
 */
void NDRX_INTEGRA(tpsvrdone)(void)
{
    NDRX_LOG(log_debug, "tpsvrdone called");
}
/* vim: set ts=4 sw=4 et smartindent: */
