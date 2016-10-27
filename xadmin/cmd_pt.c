/* 
** `printtrans' aka `pt' command implementation
**
** @file cmd_pt.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
public longstrmap_t M_txstatemap[] =
{
    {XA_TX_STAGE_ACTIVE,    "ACTIVE"},
    {XA_TX_STAGE_PREPARING, "PREPARING"},
    {XA_TX_STAGE_ABORTING,  "ABORTING"},
    {XA_TX_STAGE_ABORTED,   "ABORTED"},
    {XA_TX_STAGE_COMMITTING,  "COMMITTING"},
    {XA_TX_STAGE_COMMITTED , "COMMITTED"},
    {XA_RM_STATUS_COMMIT_HEURIS , "COMMITTED, HEURISTIC"},
    {FAIL,                  "?"}
};

/**
 * String mappings for RM statuses
 */
public longstrmap_t M_rmstatus[] =
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
private int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = SUCCEED;
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
    int i;
    int occ;
    long trycount, max_tries;
    
    if (
            SUCCEED!=Bget(p_ub, TMXID, 0, (char *)tmxid, 0L) ||
            SUCCEED!=Bget(p_ub, TMRMID, 0, (char *)&tmrmid, 0L) ||
            SUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&tmnodeid, 0L) ||
            SUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&tmsrvid, 0L) ||
            SUCCEED!=Bget(p_ub, TMTXTOUT, 0, (char *)&tmtxtout, 0L) ||
            SUCCEED!=Bget(p_ub, TMTXTOUT_LEFT, 0, (char *)&tmtxtout_left, 0L) ||
            SUCCEED!=Bget(p_ub, TMTXSTAGE, 0, (char *)&tmtxstage, 0L) ||
            SUCCEED!=Bget(p_ub, TMTXTRYCNT, 0, (char *)&trycount, 0L) ||
            SUCCEED!=Bget(p_ub, TMTXTRYMAXCNT, 0, (char *)&max_tries, 0L)
        )
    {
        fprintf(stderr, "Protocol error - TM did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }

    occ = Boccur(p_ub, TMTXRMSTATUS);
    
    /* TODO: need tx status human-readable version */
    printf("xid=[%s]\n"
            "TM: Node Id: %hd, grpno: %hd, srvid: %hd, Transaction stage: %hd-%s\n"
            "(TM ref: -t%s -x%s)\n"
            "Group count: %d, timeout: %ld, time left: %ld, tries: %ld of %ld\n"
            "Known participants:\n",
            tmxid, tmnodeid, tmrmid, tmsrvid, tmtxstage, 
            ndrx_dolongstrgmap(M_txstatemap, tmtxstage, FAIL),
            svcnm, tmxid,
            occ, tmtxtout, tmtxtout_left, trycount, max_tries);
    
    for (i=0; i<occ; i++)
    {
        if (
                SUCCEED!=Bget(p_ub, TMTXRMID, i, (char *)&tmtxrmid, 0L) ||
                SUCCEED!=Bget(p_ub, TMTXRMSTATUS, i, (char *)&tmtxrmstatus, 0L) ||
                SUCCEED!=Bget(p_ub, TMTXRMERRCODE, i, (char *)&tmtxrmerrcode, 0L) ||
                SUCCEED!=Bget(p_ub, TMTXRMREASON, i, (char *)&tmtxrmreason, 0L)
                )
        {
            /* TODO: need RM status human-readable version */
            fprintf(stderr, "Protocol error - TM did not return data, see logs!\n");
            NDRX_LOG(log_error, "Failed to return fields: [%s]", 
                        Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        else
        {
            printf("\tgrpno: %hd, status: %c-%s, errorcode: %ld, reason: %hd\n",
                    tmtxrmid, tmtxrmstatus, ndrx_docharstrgmap(M_rmstatus, tmtxrmstatus, 0), 
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
int call_tm(char *svcnm)
{
  UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_PRINTTRANS);
    int ret=SUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        FAIL_OUT(ret);
    }
    
    if (FAIL == (cd = tpconnect(svcnm,
                                    (char *)p_ub,
                                    0,
                                    TPNOTRAN |
                                    TPRECVONLY)))
    {
        NDRX_LOG(log_error, "Connect error [%s]", tpstrerror(tperrno) );
        ret = FAIL;
        goto out;
    }
    NDRX_LOG(log_debug, "Connected OK, cd = %d", cd );

    while (recv_continue)
    {
        recv_continue=0;
        if (FAIL == tprecv(cd,
                            (char **)&p_ub,
                            0L,
                            0L,
                            &revent))
        {
            ret = FAIL;
            tp_errno = tperrno;
            if (TPEEVENT == tp_errno)
            {
                    if (TPEV_SVCSUCC == revent)
                            ret = SUCCEED;
                    else
                    {
                        NDRX_LOG(log_error,
                                 "Unexpected conv event %lx", revent );
                        FAIL_OUT(ret);
                    }
            }
            else
            {
                NDRX_LOG(log_error, "recv error %d", tp_errno  );
                FAIL_OUT(ret);
            }
        }
        else
        {
            /*fprintf(stderr, "Response: \n");
            Bfprint(p_ub, stderr);*/
            if (SUCCEED!=print_buffer(p_ub, svcnm))
            {
                FAIL_OUT(ret);
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
private int tmfilter(char *svcnm)
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
        return TRUE;
    else
        return FALSE;
}

/**
 * Print XA transactions
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_pt(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    
    /* we need to init TP subsystem... */
    if (SUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    list = ndrx_get_svc_list(tmfilter);
    
    LL_FOREACH_SAFE(list,el,tmp)
    {
        
        NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

        call_tm(el->svcnm);
        /* Have some housekeep. */
        NDRX_FREE(el);
    }
    
out:
    return ret;
}

