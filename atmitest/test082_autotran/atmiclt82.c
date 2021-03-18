/**
 * @brief Test auto-transaction functionality - client
 *  basically client runs in non-transaction env.
 *  We call the auto-tran server which forward to another auto tran server
 *  we control commit or not.
 *  from script we stop the tmsrv at certain points to simulate system
 *  response in case of begin/commit/abort failures.
 *  TODO: - put second server in the dom2
 *  TODO: - implement conversation error reply responder.
 *
 * @file atmiclt82.c
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
#include "test82.h"
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
    long olen;
    char *buf = NULL;
    long revent;
    int cd=EXFAIL;
    int tmp;
    
    /* We shall call commands:
     * - Commands are: "OK"
     * - Commands are: "FAIL"
     * - Commands are: "COUNT"
     */
    if (argc<2)
    {
        NDRX_LOG(log_error, "Missing command code");
        fprintf(stderr, "Usage: %s <command> [C]\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==CBchg(p_ub, T_STRING_FLD, 0, argv[1], 0, BFLD_STRING))
    {
        NDRX_LOG(log_debug, "Failed to set T_STRING_FLD[0]: %s", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    /* Let the shell count the messages added.. */
    if (0==strcmp(argv[1], "COUNT"))
    {
        /* W/O tran, try to dequeue in loop & print msgs... */
        TPQCTL qc;
        
        memset(&qc, 0, sizeof(qc));
        
        while (EXSUCCEED==tpdequeue("MYSPACE", "MSGQ", &qc, &buf, &olen, TPNOTRAN))
        {
            /* Print the buffer... OK? */
            Bprint((UBFH *)buf);
            
            tpfree(buf);
            memset(&qc, 0, sizeof(qc));
            buf=NULL;
        }
        
        if (tperrno!=TPEDIAGNOSTIC)
        {
            NDRX_LOG(log_debug, "TESTERROR: Expected TPEDIAGNOSTIC, got: %s", 
                    tpstrerror(tperrno));
            ret=EXFAIL;
            goto out;
        }
        
        if (QMENOMSG!=qc.diagnostic)
        {
            NDRX_LOG(log_debug, "TESTERROR: Expected QMENOMSG, got: %d", 
                    qc.diagnostic);
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        if (argc>2 && ('C'==argv[2][0] || 'S'==argv[2][0] || 'Z'==argv[2][0]))
        {
            if ('C'==argv[2][0])
            {
                tmp = TPRECVONLY;
            }
            else if ('S'==argv[2][0])
            {
                tmp = TPSENDONLY;
            }
            
            if (EXFAIL == (cd=tpconnect("TESTSV2", (char *)p_ub, 0L, tmp)))
            {
                NDRX_LOG(log_error, "TESTSV2 failed: %s", tpstrerror(tperrno));
                /* capture the error code from the script */
                printf("%s\n", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
            
            /* get event */
            if ('C'==argv[2][0])
            {
                tmp = tprecv(cd, &buf, &olen, 0, &revent);
            }
            else if ('S'==argv[2][0])
            {
                sleep(17); /* let server to return */
                /* check with send entry point... */
                tmp = tpsend(cd, NULL, 0, 0, &revent);
            }
            
            if (EXFAIL==tmp)
            {
                NDRX_LOG(log_error, "RECV failed: %s %ld", tpstrerror(tperrno), revent);
                
                if (TPEEVENT==tperrno)
                {
                    if (TPEV_SVCSUCC==revent)
                    {
                        /* OK */
                        goto out;
                    }

                    switch (revent)
                    {
                        case TPEV_DISCONIMM:
                            printf("TPEV_DISCONIMM\n");
                            break;
                        case TPEV_SENDONLY:
                            printf("TPEV_SENDONLY\n");
                            break;
                        case TPEV_SVCERR:
                            printf("TPEV_SVCERR\n");
                            break;
                        case TPEV_SVCFAIL:
                            printf("TPEV_SVCFAIL\n");
                            break;
                        default:
                            printf("TESTERROR event %ld\n", revent);
                            break;
                    }
                }
                else
                {
                    printf("RECV failed: %s %ld\n", tpstrerror(tperrno), revent);
                }
                
                ret=EXFAIL;
                goto out;
            }
            
            if (EXFAIL!=cd && EXSUCCEED!=tpdiscon(cd))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to discon: %s", 
                        tpstrerror(tperrno));
                EXFAIL_OUT(ret);
            }
            
        }
        else if (argc>2 && 'A'==argv[2][0])
        {
            /* 
             * Async check 
             * So either committed or aborted. It shall not be active transaction
             */
            if (EXFAIL == tpacall("TESTSV2", (char *)p_ub, 0L, TPNOREPLY))
            {
                NDRX_LOG(log_error, "TESTSV2 async call failed: %s", tpstrerror(tperrno));
                /* capture the error code from the script */
                printf("%s\n", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;
            }
            
            /* let service to finish */
            sleep(3);
        }
        else
        {
            if (EXFAIL == tpcall("TESTSV", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
            {
                NDRX_LOG(log_error, "TESTSV failed: %s", tpstrerror(tperrno));

                /* capture the error code from the script */
                printf("%s\n", tpstrerror(tperrno));
                ret=EXFAIL;
                goto out;

            }
        }
    }
    
out:

    if (NULL!=buf)
    {
        tpfree(buf);
    }
    tpterm();
    fprintf(stderr, "Exit with %d\n", ret);

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
