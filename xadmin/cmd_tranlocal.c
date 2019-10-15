/**
 * @brief `recoverlocal', `commitlocal' and `abortlocal' implementation
 *
 * @file cmd_tranlocal.c
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
#include <xa.h>

#include "xa_cmn.h"
#include "exbase64.h"
#include <ndrx.h>
#include <nclopt.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate int M_rcv_count;
/*---------------------------Prototypes---------------------------------*/

/**
 * List in-doubt transactions
 * @param[in] p_ub UBF buffer with TM response
 * @param[in] svcnm TM service name
 * @param[in] parse shall we parse the XID
 * @return SUCCEED/FAIL
 */
exprivate int print_buffer(UBFH *p_ub, char *svcnm, short parse)
{
    int ret = EXSUCCEED;
    char tmxid[1024];
    BFLDLEN len;
    
    /* the xid is binary XID recovered in hex */
    if (EXSUCCEED!=Bget(p_ub, TMXID, 0, (char *)tmxid, 0L))
    {
        fprintf(stderr, "Protocol error - TM did not return data, see logs!\n");
        NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    /* list in copy/past for commit/abort local format */
    printf("XID: -s %s -x %s\n", svcnm, tmxid);
    
    /* TODO: Print Enduro/X related data... (if have one) */
    
    if (parse)
    {
        XID xid;
        size_t sz;
        
        memset(&xid, 0, sizeof(xid));

        sz = sizeof(xid);
        if (NULL==ndrx_xa_base64_decode((unsigned char *)tmxid, strlen(tmxid),
                &sz, (char *)&xid))
        {
            NDRX_LOG(log_warn, "Failed to parse XID -> Corrupted base64?");
        }
        else
        {
            /* print general info */
            printf("DATA: formatID: 0x%lx (%s) gtrid_length: %ld bqual_length: %ld\n",
                    xid.formatID, 
                    ((NDRX_XID_FORMAT_ID==xid.formatID ||
                        NDRX_XID_FORMAT_ID==(long)ntohll(xid.formatID))?
                        "fmt OK":"Not Enduro/X or different arch"),
                    xid.gtrid_length, xid.bqual_length);
            
            /* print raw xid */
            STDOUT_DUMP(log_info,"RAW XID", &xid, sizeof(xid));
            
            if (NDRX_XID_FORMAT_ID==xid.formatID)
            {
                short nodeid;
                short srvid;
                unsigned char rmid_start;
                unsigned char rmid_cur;
                long btid;
                char xid_str[NDRX_XID_SERIAL_BUFSIZE];
                
                NDRX_LOG(log_debug, "Format OK");
                
                /* Print Enduro/X related xid data */
                
                atmi_xa_xid_get_info(&xid, &nodeid, 
                    &srvid, &rmid_start, &rmid_cur, &btid);
                
                /* the generic xid would be with out branch & current rmid */
                
                /* reset xid trailer to have original xid_str */
                memset(xid.data + xid.gtrid_length - 
                    sizeof(long) - sizeof(unsigned char), 
                        0, sizeof(long)+sizeof(unsigned char));
                
                atmi_xa_serialize_xid(&xid, xid_str);
                
                printf("DETAILS: tm_nodeid: %hd tm_srvid: %hd tm_rmid: %d cur_rmid: %d btid: %ld\n",
                        nodeid, srvid, (int)rmid_start, (int)rmid_cur, btid);
                printf("XID_STR: [%s]\n", xid_str);
            }
            
        }
    }
    
     /* Check do we have error fields? of so then print the status? */
    
    if (Bpres(p_ub, TMERR_CODE, 0))
    {
        char msg[MAX_TP_ERROR_LEN+1] = {EXEOS};
        short code;
        short reason = 0;

        Bget(p_ub, TMERR_CODE, 0, (char *)&code, 0L);
        Bget(p_ub, TMERR_REASON, 0, (char *)&reason, 0L);
        len = sizeof(msg);
        Bget(p_ub, TMERR_MSG, 0, msg, &len);
        
        if (0==code && EXEOS==msg[0])
        {
            NDRX_STRCPY_SAFE(msg, "SUCCEED");
        }
        
        printf("RESULT: tp code: %hd, xa reason: %hd, msg: %s\n", code, reason, msg);
        
    }
    
out:
    return ret;
}

/**
 * This basically tests the normal case when all have been finished OK!
 * @return
 */
exprivate int call_tm(UBFH *p_ub, char *svcnm, short parse)
{
    int ret=EXSUCCEED;
    int cd;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    
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
            if (EXSUCCEED!=print_buffer(p_ub, svcnm, parse))
            {
                EXFAIL_OUT(ret);
            }
            M_rcv_count++;
            recv_continue=1;
        }
    }

out:
    
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
 * Print local transactions for all or particular TM
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_recoverlocal(cmd_mapping_t *p_cmd_map, int argc, 
        char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    char svcnm[MAXTIDENT+1]={EXEOS};
    UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_RECOVERLOCAL);
    short parse = EXFALSE;
    
    ncloptmap_t clopt[] =
    {
        {'s', BFLD_STRING, (void *)svcnm, sizeof(svcnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "TM Service Name"},
        {'p', BFLD_SHORT, (void *)&parse, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Parse xid"},
        {0}
    };
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc UBF!");
        EXFAIL_OUT(ret);
    }
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    M_rcv_count = 0;
    
    if (EXEOS!=svcnm[0])
    {
        NDRX_LOG(log_debug, "TM Service name specified: [%s]", svcnm);
        ret = call_tm(p_ub, svcnm, parse);
    }
    else
    {
        NDRX_LOG(log_debug, "TM Service name not specified - query all");
        
        list = ndrx_get_svc_list(tmfilter);

        LL_FOREACH_SAFE(list,el,tmp)
        {

            NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

            ret = call_tm(p_ub, el->svcnm, parse);
            /* Have some housekeep. */
            LL_DELETE(list,el);
            NDRX_FREE(el);
        }
    }
    
    fprintf(stderr, "Recovered %d transactions\n", M_rcv_count);
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Perform commit/abort/forget for particular xid.
 * @param msg message for confirm
 * @param tmcmd ATMI_XA_COMMITLOCAL /  ATMI_XA_ABORTLOCAL / ATMI_XA_FORGETLOCAL
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @param p_have_next
 * @return 
 */
exprivate int cmd_x_local(char *msg, char tmcmd, cmd_mapping_t *p_cmd_map, 
        int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    char svcnm[MAXTIDENT+1]={EXEOS};
    char xid[sizeof(XID)*2] = {EXEOS};
    char msgfmt[128];
    short confirm = EXFALSE;
    short parse = EXFALSE;
    
    UBFH *p_ub = atmi_xa_alloc_tm_call(tmcmd);
    
    ncloptmap_t clopt[] =
    {
        {'s', BFLD_STRING, (void *)svcnm, sizeof(svcnm), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "TM Service Name"},
        {'x', BFLD_STRING, (void *)xid, sizeof(xid), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, 
                                "XID to process (for recoverlocal output)"},
        {'y', BFLD_SHORT, (void *)&confirm, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Confirm"},
        {'p', BFLD_SHORT, (void *)&parse, 0, 
                                NCLOPT_OPT | NCLOPT_TRUEBOOL, "Parse xid"},
                                
        {0}
    };
    
    /* we need to init TP subsystem... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        fprintf(stderr, "Failed to tpinit(): %s\n", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc UBF!");
        EXFAIL_OUT(ret);
    }
    
    /* parse command line */
    if (nstd_parse_clopt(clopt, EXTRUE,  argc, argv, EXFALSE))
    {
        fprintf(stderr, XADMIN_INVALID_OPTIONS_MSG);
        EXFAIL_OUT(ret);
    }
    
    
    if (EXEOS!=xid[0] && EXEOS==svcnm[0])
    {
        fprintf(stderr, "ERROR ! If -x is specified, the -s must be set too!\n");
    }
    
    snprintf(msgfmt, sizeof(msgfmt), "Are you sure you want to %s the transaction?", 
            msg);
    
    /* Check for confirmation */
    if (!chk_confirm(msgfmt, confirm))
    {
        EXFAIL_OUT(ret);
    }
    
    /* load the settings to UBF */
    M_rcv_count = 0;
    
    if (EXEOS!=xid[0] && EXSUCCEED!=Bchg(p_ub, TMXID, 0, xid, 0))
    {
        NDRX_LOG(log_error, "Failed to set TMXID: %s", Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    if (EXEOS!=svcnm[0])
    {
        NDRX_LOG(log_debug, "TM Service name specified: [%s]", svcnm);
        ret = call_tm(p_ub, svcnm, parse);
    }
    else
    {
        NDRX_LOG(log_debug, "TM Service name not specified - query all");
        
        list = ndrx_get_svc_list(tmfilter);

        LL_FOREACH_SAFE(list,el,tmp)
        {

            NDRX_LOG(log_info, "About to call service: [%s]\n", el->svcnm);

            ret = call_tm(p_ub, el->svcnm, parse);
            /* Have some housekeep. */
            LL_DELETE(list,el);
            NDRX_FREE(el);
        }
    }
    
    fprintf(stderr, "Processed %d transactions\n", M_rcv_count);
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Commit local transactions
 */
expublic int cmd_commitlocal(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    return cmd_x_local("commit", ATMI_XA_COMMITLOCAL, p_cmd_map, argc, argv, p_have_next);
}

/**
 * Abort local transactions
 */
expublic int cmd_abortlocal(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    return cmd_x_local("abort", ATMI_XA_ABORTLOCAL, p_cmd_map, argc, argv, p_have_next);
}

/**
 * Forget local transactions
 */
expublic int cmd_forgetlocal(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    return cmd_x_local("forget", ATMI_XA_FORGETLOCAL, p_cmd_map, argc, argv, p_have_next);
}


/* vim: set ts=4 sw=4 et smartindent: */
