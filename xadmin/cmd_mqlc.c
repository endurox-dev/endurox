/**
 * @brief List queue configuration
 *
 * @file cmd_mqlc.c
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
exprivate void print_hdr(void)
{
    fprintf(stderr, "Nd SRVID QSPACE    QNAME     FLAGS QDEF\n");
    fprintf(stderr, "-- ----- --------- --------- ----- --------------------\n");
}

/**
 * List queue definitions
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
exprivate int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = EXSUCCEED;
    
    short nodeid;
    short srvid;
    char qspace[XATMI_SERVICE_NAME_LENGTH+1];
    char qname[TMQNAMELEN+1];
    char qdef[TMQ_QDEF_MAX];
    char strflags[128];
    
    if (
            EXSUCCEED!=Bget(p_ub, EX_QSPACE, 0, qspace, 0L) ||
            EXSUCCEED!=Bget(p_ub, EX_QNAME, 0, qname, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            EXSUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            EXSUCCEED!=CBget(p_ub, EX_DATA, 0, qdef, 0L, BFLD_STRING) ||
            EXSUCCEED!=Bget(p_ub, EX_QSTRFLAGS, 0, strflags, 0L)
        )
    {
        fprintf(stderr, "Protocol error - TMQ did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }    
    
    FIX_SVC_NM_DIRECT(qspace, 9);
    FIX_SVC_NM_DIRECT(qname, 9);
    
    fprintf(stdout, "%2d %5d %-9.9s %-9.9s %-5.5s %s",
            nodeid, 
            srvid, 
            qspace, 
            qname,
            strflags,
            qdef
            );
    
    printf("\n");
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
exprivate int call_tmq(char *svcnm)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    char cmd = TMQ_CMD_MQLC;
    
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");        
        EXFAIL_OUT(ret);
    }
    
    if (EXSUCCEED!=Bchg(p_ub, EX_QCMD, 0, &cmd, 0L))
    {
        NDRX_LOG(log_error, "Failed to install command code");
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

    return ret;
}

/**
 * List queues
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_mqlc(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
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

/* vim: set ts=4 sw=4 et smartindent: */
