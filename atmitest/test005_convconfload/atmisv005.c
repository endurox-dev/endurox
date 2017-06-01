/* 
** This simulates ATM configuration server
**
** @file atmisv005.c
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
int M_cd=FAIL;
UBFH *M_p_ub=NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Open configuration download
 * @return
 */
private short cnf_openf(void)
{
    short   ret = SUCCEED;
    long    len;
    long    revent;
    char    *p;
    char    termcode[64];
    /* expect SENDONLY to pass rply */
    if (FAIL == tprecv(M_cd, (char **)&M_p_ub, &len, 0L, &revent))
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
                ret = FAIL;
                goto out;
            }
        }
        else
        {
            NDRX_LOG(log_error, "Conv receive err %d", tperrno );
            ret=FAIL;
            goto out;
        }
    }
    
    /*
     * Get the key stuff
     */
    if (FAIL==Bget(M_p_ub, T_STRING_2_FLD, 0, termcode, NULL))
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to get T_STRING_2_FLD[0]");
        ret=FAIL;
    }
    else if (0!=strcmp(termcode, "TERMINAL_T"))
    {
        NDRX_LOG(log_error, "TESTERROR: Got invalid T_STRING_2_FLD=[%s]",
                                        termcode);
        ret=FAIL;
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
    int ret=SUCCEED;
    long revent;
    tpfree((char *)M_p_ub);

    if ( !(M_p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)) )
    {
        NDRX_LOG(log_error, "TESTERROR: FML alloc failed");
        ret=FAIL;
        goto out;
    }
    else if (SUCCEED == CBchg(M_p_ub, fld, 0,
                                            data,
                                            0L,
                                            BFLD_STRING) )
    {
            if (FAIL == tpsend(M_cd,
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
    int ret=SUCCEED;
    int i;
    M_p_ub = (UBFH *)p_svc->data;
    M_cd = p_svc->cd; /* Our call descriptor (server side) */
    char databuf[512] = {EOS};

    NDRX_LOG(log_debug, "CONVSV got call");

    if (FAIL==cnf_openf())
    {
        NDRX_LOG(log_error, "TESTERROR: Failed to open config!");
        ret=FAIL;
        goto out;
    }

    for (i=0; i<100; i++)
    {
        snprintf(databuf, sizeof(databuf), "svc send %d", i);
        if (FAIL==send_config_data(T_STRING_FLD, databuf))
        {
            NDRX_LOG(log_error, "TESTERROR: Failed to send config!");
            ret=FAIL;
            goto out;
        }
    }
    
out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
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
    int ret = SUCCEED;
    NDRX_LOG(log_debug, "tpsvrinit called");

    if (SUCCEED!=tpadvertise("CONVSV", CONVSV))
    {
        NDRX_LOG(log_error, "Failed to initialize CONVSV!");
        ret=FAIL;
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
