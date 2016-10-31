/* 
** List messages
**
** @file cmd_mqlc.c
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
#include <qcommon.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
private char M_qspace[XATMI_SERVICE_NAME_LENGTH+1] = "";
/*---------------------------Prototypes---------------------------------*/

/**
 * Print header
 * @return
 */
private void print_hdr(void)
{
    fprintf(stderr, "Nd SRVID MSGID (STR/Base64 mod)                       TSTAMP (UTC)      TRIES L\n");
    fprintf(stderr, "-- ----- -------------------------------------------- ----------------- ----- -\n");
}

/**
 * List queue definitions
 * We will run in conversation mode.
 * @param svcnm
 * @return SUCCEED/FAIL
 */
private int print_buffer(UBFH *p_ub, char *svcnm)
{
    int ret = SUCCEED;
    
    short nodeid;
    short srvid;
    
    char msgid_str[TMMSGIDLEN_STR+1];
    
    char tstamp1[20];
    char tstamp2[20];
    long tires;
    short is_locked;
    
    if (
            SUCCEED!=Bget(p_ub, TMNODEID, 0, (char *)&nodeid, 0L) ||
            SUCCEED!=Bget(p_ub, TMSRVID, 0, (char *)&srvid, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QMSGIDSTR, 0, msgid_str, 0L)  ||
            SUCCEED!=Bget(p_ub, EX_TSTAMP1_STR, 0, tstamp1, 0L) ||
            SUCCEED!=Bget(p_ub, EX_TSTAMP2_STR, 0, tstamp2, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QMSGTRIES, 0, (char *)&tires, 0L) ||
            SUCCEED!=Bget(p_ub, EX_QMSGLOCKED, 0, (char *)&is_locked, 0L)
        )
    {
        fprintf(stderr, "Protocol error - TMQ did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        FAIL_OUT(ret);
    }    
       
    fprintf(stdout, "%-2d %-5d %-44.44s %-17.17s %-5.5s %s",
            nodeid, 
            srvid, 
            msgid_str,
            tstamp1+2, 
            ndrx_decode_num(tires, 0, 0, 1),
            is_locked?"Y":"N"
            );
    
    printf("\n");
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
private int call_tmq(char *svcnm, char *qname)
{
    UBFH *p_ub = (UBFH *)tpalloc("UBF", "", 1024);
    int ret=SUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    int rcv_count = 0;
    char cmd = TMQ_CMD_MQLM;
    
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
    
    if (SUCCEED!=Bchg(p_ub, EX_QNAME, 0, qname, 0L))
    {
        NDRX_LOG(log_error, "Failed to install qname");
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
private int mqfilter_qspace(char *svcnm)
{
    int i, len;
    int cnt = 0;
    char tmp[XATMI_SERVICE_NAME_LENGTH+1];
    
    
    sprintf(tmp, NDRX_SVC_QSPACE, M_qspace);
    
    
    if (0==strcmp(svcnm, tmp))
    {
        return TRUE;
    }
    
    return FALSE;
}


/**
 * List messages in q
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
public int cmd_mqlm(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = SUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    char qname[TMQNAMELEN+1];
    
    ncloptmap_t clopt[] =
    {
        {'s', BFLD_STRING, M_qspace, sizeof(M_qspace)-3, 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Qspace name"},
        {'q', BFLD_STRING, qname, sizeof(qname), 
                                NCLOPT_MAND|NCLOPT_HAVE_VALUE, "Queue name"},
        {0}
    };
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, TRUE,  argc, argv, FALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        FAIL_OUT(ret);
    }
    
    
    /* we need to init TP subsystem... */
    if (SUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
    print_hdr();
    
    list = ndrx_get_svc_list(mqfilter_qspace);
    
    LL_FOREACH_SAFE(list,el,tmp)
    {
        
        NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

        call_tmq(el->svcnm, qname);
        /* Have some housekeep. */
        LL_DELETE(list,el);
        NDRX_FREE(el);
    }
    
out:
    return ret;
}

