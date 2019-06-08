/**
 * @brief `printtrans' aka `pt' command implementation
 *
 * @file cmd_pt.c
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
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Mapping for transaction stages messages
 */
expublic longstrmap_t M_txstatemap[] =
{
    {XA_TX_STAGE_ACTIVE,    "ACTIVE"},
    {XA_TX_STAGE_PREPARING, "PREPARING"},
    {XA_TX_STAGE_ABORTING,  "ABORTING"},
    {XA_TX_STAGE_ABORTED,   "ABORTED"},
    {XA_TX_STAGE_COMMITTING,"COMMITTING"},
    {XA_TX_STAGE_COMMITTED ,"COMMITTED"},
    {XA_RM_STATUS_COMMIT_HEURIS , "COMMITTED, HEURISTIC"},
    {EXFAIL,                "?"}
};

/**
 * String mappings for RM statuses
 */
expublic longstrmap_t M_rmstatus[] =
{
    {XA_RM_STATUS_ACTIVE,       "JOINED"},
    {XA_RM_STATUS_PREP,         "PREPARED"},
    {XA_RM_STATUS_ABORTED,      "ABORTED"},
    {XA_RM_STATUS_COMMITTED,    "COMMITTED"},
    {XA_RM_STATUS_COMMITTED_RO, "COMMITTED_RO"},
    {0,                         "?"}
};

/*---------------------------Prototypes---------------------------------*/

/**
 * List transactions in progress
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
exprivate int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = EXSUCCEED;
    char tmxid[NDRX_XID_SERIAL_BUFSIZE+1];
    short tmrmid;
    short tmnodeid;
    short tmsrvid;

    long tmtxtout;
    long tmtxtout_left;
    short tmtxstage;

    /* Process in loop: */
    short tmtxrmid;
    char tmtxrmstatus;
    long tmtxrmerrcode;
    short tmtxrmreason;
    long tmtxrmbtid;
    int i;
    int occ;
    long trycount, max_tries;
    
    if (
            EXSUCCEED!=Bget(p_ub, TMXID, 0, (char *)tmxid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMRMID, 0, (char *)&tmrmid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&tmnodeid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&tmsrvid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMTXTOUT, 0, (char *)&tmtxtout, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMTXTOUT_LEFT, 0, (char *)&tmtxtout_left, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMTXSTAGE, 0, (char *)&tmtxstage, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMTXTRYCNT, 0, (char *)&trycount, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMTXTRYMAXCNT, 0, (char *)&max_tries, 0L)
        )
    {
        fprintf(stderr, "Protocol error - TM did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }

    occ = Boccur(p_ub, TMTXRMSTATUS);
    
    /* TODO: need tx status human-readable version */
    printf("xid=[%s]\n"
            "TM: Node Id: %hd, grpno: %hd, srvid: %hd, Transaction stage: %hd-%s\n"
            "(TM ref: -t%s -x%s)\n"
            "Group count: %d, timeout: %ld, time left: %ld, tries: %ld of %ld\n"
            "Known participants:\n",
            tmxid, tmnodeid, tmrmid, tmsrvid, tmtxstage, 
            ndrx_dolongstrgmap(M_txstatemap, tmtxstage, EXFAIL),
            svcnm, tmxid,
            occ, tmtxtout, tmtxtout_left, trycount, max_tries);
    
    for (i=0; i<occ; i++)
    {
        if (
                EXSUCCEED!=Bget(p_ub, TMTXRMID, i, (char *)&tmtxrmid, 0L) ||
                EXSUCCEED!=Bget(p_ub, TMTXRMSTATUS, i, (char *)&tmtxrmstatus, 0L) ||
                EXSUCCEED!=Bget(p_ub, TMTXRMERRCODE, i, (char *)&tmtxrmerrcode, 0L) ||
                EXSUCCEED!=Bget(p_ub, TMTXRMREASON, i, (char *)&tmtxrmreason, 0L) ||
                EXSUCCEED!=Bget(p_ub, TMTXBTID, i, (char *)&tmtxrmbtid, 0L)
                )
        {
            /* TODO: need RM status human-readable version */
            fprintf(stderr, "Protocol error - TM did not return data, see logs!\n");
            NDRX_LOG(log_error, "Failed to return fields: [%s]", 
                        Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        else
        {
            printf("\tgrpno: %hd, btid: %ld status: %c-%s, errorcode: %ld, reason: %hd\n",
                    tmtxrmid, tmtxrmbtid, tmtxrmstatus, 
                    ndrx_docharstrgmap(M_rmstatus, tmtxrmstatus, 0), 
                    tmtxrmerrcode, tmtxrmreason);
        }
    }
    
    printf("\n");
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
exprivate int call_tm(char *svcnm)
{
  UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_PRINTTRANS);
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        EXFAIL_OUT(ret);
    }
    
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
            else
            {
                NDRX_LOG(log_error, "recv error %d", tp_errno  );
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            /*fprintf(stderr, "Response: \n");
            Bfprint(p_ub, stderr);*/
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
 * Filter the service names, return TRUE for those which matches individual TMs
 * @param svcnm
 * @return TRUE/FALSE
 */
exprivate int tmfilter(char *svcnm)
{
    int i, len;
    int cnt = 0;
    
    /*printf("Testing: [%s]\n", svcnm);*/
    /* example: @TM-1-1-310 */
    if (0==strncmp(svcnm, "@TM", 3))
    {
        /* Now it should have 3x dashes inside */
        len = strlen(svcnm);
        for (i=0; i<len; i++)
        {
            if ('-'==svcnm[i])
            {
                cnt++;
            }
        }
    }
    
    if (3==cnt)
        return EXTRUE;
    else
        return EXFALSE;
}

/**
 * Print XA transactions
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_pt(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    list = ndrx_get_svc_list(tmfilter);
    
    LL_FOREACH_SAFE(list,el,tmp)
    {
        
        NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

        call_tm(el->svcnm);
        /* Have some housekeep. */
        LL_DELETE(list,el);
        NDRX_FREE(el);
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
