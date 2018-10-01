/**
 * @brief `pc' (print clients) command implementation
 *
 * @file cmd_pc.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#include <sys/param.h>
#include <string.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <nstdutil.h>

#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <utlist.h>
#include <Exfields.h>

#include "xa_cmn.h"
#include <ndrx.h>
#include <cpm.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define SUBSECT_SEP     "/"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print the processes
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
exprivate int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = EXSUCCEED;
    char output[CPM_OUTPUT_SIZE];
    
    if (EXSUCCEED!=Bget(p_ub, EX_CPMOUTPUT, 0, (char *)output, 0L))
    {
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    printf("%s\n", output);
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
exprivate int call_cpm(char *svcnm, char *cmd, char *tag, char *subsect, long twait)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", NULL, CPM_DEF_BUFFER_SZ);
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    long flags = TPNOTRAN | TPRECVONLY;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        EXFAIL_OUT(ret);
    }
    
    if (twait > 0)
    {
        if (EXSUCCEED!=Bchg(p_ub, EX_CPMWAIT, 0, (char *)&twait, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CPMWAIT: %s!", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_CPMCOMMAND, 0, cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to set EX_CPMCOMMAND to %s!", EX_CPMCOMMAND);        
        EXFAIL_OUT(ret);
    }
    
    if (0!=strcmp(CPM_CMD_PC, cmd))
    {
        if (EXSUCCEED!=Bchg(p_ub, EX_CPMTAG, 0, tag, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CPMCOMMAND to %s!", tag);        
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=Bchg(p_ub, EX_CPMSUBSECT, 0, subsect, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CPMSUBSECT to %s!", subsect);        
            EXFAIL_OUT(ret);
        }
    }
    
    /* if we run batch commands, the no time-out please */
    
    if (EXFAIL == (cd = tpconnect(svcnm,
                                    (char *)p_ub,
                                    0,
                                    TPNOTRAN |
                                    TPRECVONLY)))
    {
        NDRX_LOG(log_error, "Connect error [%s]", tpstrerror(tperrno) );
        
        ret = EXFAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Connected OK, cd = %d", cd );

    while (recv_continue)
    {
        recv_continue=0;
        if (EXFAIL == tprecv(cd,
                            (char **)&p_ub,
                            0L,
                            0L,
                            &revent))
        {
            ret = EXFAIL;
            tp_errno = tperrno;
            if (TPEEVENT == tp_errno)
            {
                if (TPEV_SVCSUCC == revent)
                        ret = EXSUCCEED;
                else
                {
                    NDRX_LOG(log_error,
                             "Unexpected conv event %lx", revent );
                    EXFAIL_OUT(ret);
                }
            }
            else if (TPETIME == tp_errno)
            {
                fprintf(stderr, "Error ! Timeout waiting on server reply, -w shall be smaller than NDRX_TOUT env.\n");
                fprintf(stderr, "The cpmsrv will complete the operation in background...\n");
            }
            else
            {
                NDRX_LOG(log_error, "recv error %d", tp_errno  );
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /*
            fprintf(stderr, "Response: \n");
            Bfprint(p_ub, stderr);
            */
            if (EXSUCCEED!=print_buffer(p_ub, svcnm))
            {
                EXFAIL_OUT(ret);
            }
            rcv_count++;
            recv_continue=1;
        }
    }

out:

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
/*
    tpdiscon(cd);
*/
    return ret;
}

/**
 * Print Clients
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_pc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    
    call_cpm(NDRX_SVC_CPM, CPM_CMD_PC, NULL, NULL, 0);
    
out:
    return ret;
}

/**
 * Stop client
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_sc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char tag[CPM_TAG_LEN];
    char subsect[CPM_SUBSECT_LEN] = {"-"};
    long twait = 0;
    char *p;
    ncloptmap_t clopt[] =
    {
        {'t', BFLD_STRING, (void *)tag, sizeof(tag), 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Tag"},
        {'s', BFLD_STRING, (void *)subsect, sizeof(subsect), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Subsection"},
        {'w', BFLD_LONG, (void *)&twait, sizeof(twait), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Wait milliseconds"},
        {0}
    };
    
    if (argc>=2 && '-'!=argv[1][0])
    {
        /* parse the tag/sub */
        p = strtok(argv[1], SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(tag, p);
        }
        else
        {
            fprintf(stderr, "Missing tag! Syntax sc <TAG>[/SUBSECT], e.g. sc POS/1\n");
            EXFAIL_OUT(ret);
        }
        
        p = strtok(NULL, SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(subsect, p);
        }
    }
    else
    {
        /* parse command line */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
        {
            fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
            EXFAIL_OUT(ret);
        }
    }
    
    ret = call_cpm(NDRX_SVC_CPM, CPM_CMD_SC, tag, subsect, twait);
    
out:
    return ret;
}

/**
 * Boot client
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_bc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char tag[CPM_TAG_LEN];
    char subsect[CPM_SUBSECT_LEN] = {"-"};
    long twait = 0;
    char *p;
    ncloptmap_t clopt[] =
    {
        {'t', BFLD_STRING, (void *)tag, sizeof(tag), 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Tag"},
        {'s', BFLD_STRING, (void *)subsect, sizeof(subsect), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Subsection"},
        {'w', BFLD_LONG, (void *)&twait, sizeof(twait), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Wait milliseconds"},
        {0}
    };
    
    /* parse command line */
    if (argc>=2 && '-'!=argv[1][0])
    {
        /* parse the tag/sub */
        p = strtok(argv[1], SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(tag, p);
        }
        else
        {
            fprintf(stderr, "Missing tag! Syntax bc <TAG>[/SUBSECT], e.g. sc POS/1\n");
            EXFAIL_OUT(ret);
        }
        
        p = strtok(NULL, SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(subsect, p);
        }
    }
    else if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    ret = call_cpm(NDRX_SVC_CPM, CPM_CMD_BC, tag, subsect, twait);
    
out:
    return ret;
}

/**
 * Reload client
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_rc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char tag[CPM_TAG_LEN];
    char subsect[CPM_SUBSECT_LEN] = {"-"};
    long twait = 0;
    char *p;
    
    ncloptmap_t clopt[] =
    {
        {'t', BFLD_STRING, (void *)tag, sizeof(tag), 
                                NCLOPT_MAND | NCLOPT_HAVE_VALUE, "Tag"},
        {'s', BFLD_STRING, (void *)subsect, sizeof(subsect), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Subsection"},
        {'w', BFLD_LONG, (void *)&twait, sizeof(twait), 
                                NCLOPT_OPT | NCLOPT_HAVE_VALUE, "Wait milliseconds"},
        {0}
    };
    
    /* parse command line */
    if (argc>=2 && '-'!=argv[1][0])
    {
        /* parse the tag/sub */
        p = strtok(argv[1], SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(tag, p);
        }
        else
        {
            fprintf(stderr, "Missing tag! Syntax rc <TAG>[/SUBSECT], e.g. sc POS/1\n");
            EXFAIL_OUT(ret);
        }
        
        p = strtok(NULL, SUBSECT_SEP);
        
        if (NULL!=p)
        {
            NDRX_STRCPY_SAFE(subsect, p);
        }
    }
    else if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    ret = call_cpm(NDRX_SVC_CPM, CPM_CMD_RC, tag, subsect, twait);
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
