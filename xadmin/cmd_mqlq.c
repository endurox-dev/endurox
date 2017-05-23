/* 
** List persistent queues
**
** @file cmd_mqlq.c
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
#include <qcommon.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Print header
 * @return
 */
private void print_hdr(void)
{
    fprintf(stderr, "Nd SRVID QSPACE    QNAME     #QUEU #LOCK #ENQ  #DEQ  #SUCC #FAIL\n");
    fprintf(stderr, "-- ----- --------- --------- ----- ----- ----- ----- ----- -----\n");
}


/**
 * List transactions in progress
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
private int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = SUCCEED;
    
    short nodeid;
    short srvid;
    char qspace[XATMI_SERVICE_NAME_LENGTH+1];
    char qname[TMQNAMELEN+1];
    long msgs;
    long locked;
    long succ;
    long fail;
    
    long numenq;
    long numdeq;
            
    if (
            SUCCEED!=Bget(p_ub, EX_QSPACE, 0, qspace, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNAME, 0, qname, 0L) ||
            SUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            SUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMMSG, 0, (char *)&msgs, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMLOCKED, 0, (char *)&locked, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMSUCCEED, 0, (char *)&succ, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMFAIL, 0, (char *)&fail, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMENQ, 0, (char *)&numenq, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QNUMDEQ, 0, (char *)&numdeq, 0L)
            
        )
    {
        fprintf(stderr, "Protocol error - TMQ did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }    
    
    FIX_SVC_NM_DIRECT(qspace, 9);
    FIX_SVC_NM_DIRECT(qname, 9);
    
    fprintf(stdout, "%-2d %-5d %-9.9s %-9.9s %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s %-5.5s",
            nodeid, 
            srvid, 
            qspace, 
            qname,
            ndrx_decode_num(msgs, 0, 0, 1), 
            ndrx_decode_num(locked, 1, 0, 1),
            
            ndrx_decode_num(numenq, 2, 0, 2),
            ndrx_decode_num(numdeq, 3, 0, 2),
            
            ndrx_decode_num(succ, 4, 0, 2),
            ndrx_decode_num(fail, 5, 0, 2)
            );
    
    printf("\n");
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
private int call_tmq(char *svcnm)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=SUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    char cmd = TMQ_CMD_MQLQ;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to install command code");
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

    return ret;
}

/**
 * Filter the service names, return TRUE for those which matches individual TMs
 * @param svcnm
 * @return TRUE/FALSE
 */
public int mqfilter(char *svcnm)
{
    int i, len;
    int cnt = 0;
    
    if (0==strncmp(svcnm, "@TMQ", 4))
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
    
    if (2==cnt)
        return TRUE;
    else
        return FALSE;
}

/**
 * List queues
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_mqlq(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    
    /* we need to init TP subsystem... */
    if (SUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    print_hdr();
    
    list = ndrx_get_svc_list(mqfilter);
    
    LL_FOREACH_SAFE(list,el,tmp)
    {
        
        NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

        call_tmq(el->svcnm);
        /* Have some housekeep. */
        LL_DELETE(list,el);
        NDRX_FREE(el);
    }
    
out:
    return ret;
}

