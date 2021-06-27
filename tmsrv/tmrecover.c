/**
 * @brief tmrecover logic driver
 *
 * @file tmrecover.c
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
#include <ubfutil.h>

#include "xa_cmn.h"
#include "exbase64.h"
#include <nclopt.h>
#include <exhash.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * transaction status hashmap
 */
typedef struct {
    
    /** transaction base id */
    char xid_str[NDRX_XID_SERIAL_BUFSIZE];
    
    /** atmi status of the transaction */
    int atmi_err;
    
    EX_hash_handle hh; /**< makes this structure hashable               */
} xid_status_t;

/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate long M_aborted;   /**< Number of aborted global transactions  */
exprivate xid_status_t *M_trans=NULL; /** list of transactions          */

/*---------------------------Prototypes---------------------------------*/

/**
 * Free up the list of cached transaction results
 */
exprivate void free_trans(void)
{
    xid_status_t * el, *elt;
    
    EXHASH_ITER(hh, M_trans, el, elt)
    {
        EXHASH_DEL(M_trans, el);
        NDRX_FPFREE(el);
    }
}

/**
 * This returns cached ATMI result
 * Cache is needed due to fact that we might have several branches for the
 * transaction, and if transaction is alive, then no need to query TM again.
 * @param xid_str base xid (btid/rmid is 0)
 * @param rmid_start RMID which started the transactions
 * @param nodeid cluster node id which started the transaction
 * @param srvid server id which started the transaction
 * @return 0 - transaction is alive, atmi error code in case if not found  (TPEMATCH) / error
 */
exprivate int get_xid_status(char *xid_str, unsigned char rmid_start, short nodeid, short srvid)
{
    int ret = EXSUCCEED;
    xid_status_t *stat;
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    UBFH *p_ub = NULL;
    char cmd = ATMI_XA_STATUS;
    long rsplen;
    
    EXHASH_FIND_STR( M_trans, xid_str, stat);
    
    if (NULL!=stat)
    {
        NDRX_LOG(log_info, "transaction [%s] result cached: %d", xid_str, stat->atmi_err);
        ret = stat->atmi_err;
    }
    
    /* if not found, query the service */
    snprintf(svcnm, sizeof(svcnm), NDRX_SVC_TM_I, (int)nodeid,  (int)rmid_start, (int)srvid);
    
    p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to malloc UBF: %s", tpstrerror(tperrno));
        ret = tperrno;
        goto out;
        
    }
    
    /* set the xid field
     * set: TMCMD = ATMI_XA_STATUS:
     * set: TMXID = xid_str
     * out:
     * where is the error loaded?
     */
    if (EXSUCCEED!=Bchg(p_ub, TMCMD, 0, &cmd, 0L) 
            || EXSUCCEED!=Bchg(p_ub, TMXID, 0, xid_str, 0L))
    {
        NDRX_LOG(log_error, "Failed to set TMCMD or TMXID: %s", Bstrerror(Berror));
        ret = TPESYSTEM;
        goto out;
    }
    
    NDRX_LOG(log_debug, "Calling [%s] for transaction status", svcnm);
    
    if (EXSUCCEED!=tpcall(svcnm, (char *)p_ub, 0, (char **)&p_ub, &rsplen, 0))
    {
        NDRX_LOG(log_error, "Failed to call [%s]: %s", svcnm, tpstrerror(tperrno));
        
        /* save the current results.. */
        ret = tperrno;
        
        if (tperrno!=TPESVCFAIL)
        {
            /* nothing more to do here... */
            goto out;
        }
        
    }

    /* dump the response buffer... */
    ndrx_debug_dump_UBF(log_debug, "Response buffer:", p_ub);
    
    /* if got the error code back, read it... */
    if (Bpres(p_ub, TMERR_CODE, 0))
    {
        short sret;
        if (EXSUCCEED==Bget(p_ub, TMERR_CODE, 0, (char *)&sret, 0L))
        {
            ret=sret;
        }
    }
    
    /* OK got the final result: */
    NDRX_LOG(log_debug, "Transaction [%s] service [%s] reported status: %d - %s",
            xid_str, svcnm, ret, (TPEMATCH==ret?"tx not found":"tx found or error"));
    
    
    /* cache the result */
    stat = NDRX_FPMALLOC(sizeof(*stat), 0);
    
    if (NULL!=stat)
    {
        stat->atmi_err = ret;
        NDRX_STRCPY_SAFE(stat->xid_str, xid_str);
    }
    
    EXHASH_ADD_STR( M_trans, xid_str, stat);
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Perform xid command on service
 * @param cmd ATMI_XA_ABORTLOCAL or ATMI_XA_FORGETLOCAL
 * @param recover_svcnm
 * @param tmxid
 * @return XA error code, so that we can decide is forget needed.
 */
exprivate int tran_finalize(char cmd, char *recover_svcnm, char *tmxid)
{
    short ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    long rsplen, flags;

    p_ub = (UBFH *)tpalloc("UBF", NULL, 1024);
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to malloc UBF: %s", tpstrerror(tperrno));
        ret = tperrno;
        goto out;
    }
    
    flags = TMFLAGS_NOCON;
    
    if (EXSUCCEED!=Bchg(p_ub, TMCMD, 0, &cmd, 0L) 
            || EXSUCCEED!=Bchg(p_ub, TMXID, 0, tmxid, 0L)
            || EXSUCCEED!=Bchg(p_ub, TMTXFLAGS, 0, (char *)&flags, 0L))
    {
        NDRX_LOG(log_error, "Failed to set TMCMD/TMXID/TMTXFLAGS: %s", Bstrerror(Berror));
        ret = XAER_RMERR;
        goto out;
    }
    
    NDRX_LOG(log_debug, "Calling command [%c] on [%s] for xid [%s]", cmd, recover_svcnm, tmxid);
    
    /* thought these only work on conv mode
     * here no conversion shall be established, just tpreturn
     * thus needs some kind of flag.
     */
    if (EXSUCCEED!=tpcall(recover_svcnm, (char *)p_ub, 0, (char **)&p_ub, &rsplen, 0))
    {
        NDRX_LOG(log_error, "Failed to call [%s]: %s", recover_svcnm, tpstrerror(tperrno));
        
        ret = XAER_RMERR;
        
        if (tperrno!=TPESVCFAIL)
        {
            /* nothing more to do here... */
            goto out;
        }
    }
    
    /* try to get xa reason
     * and override current status with it
     */
    if (Bpres(p_ub, TMERR_REASON, 0) && 
            EXSUCCEED!=Bget(p_ub, TMERR_REASON, 0, (char *)&ret, 0L))
    {
        NDRX_LOG(log_error, "Failed to get reason field: %s", Bstrerror(Berror));
        ret=XAER_RMERR;
    }
    
out:
                
    NDRX_LOG(log_debug, "Returning %d", ret);
    return ret;
}

/**
 * Process xid...
 * to avoid possible deadlocks while we recover the
 * transactions, we need to download full list of xid to the memory
 * and only the process xid by xid, as we might request the particular resource
 * to abort the transaction.
 * 
 * @param tmxid raw DB xid to process
 * @param recover_svcnm service name which returned a list of transactions
 * @return EXSUCCEED/EXFAIL
 */
exprivate int process_xid(char *tmxid, char *recover_svcnm)
{
    int ret = EXSUCCEED;
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
        NDRX_LOG(log_debug, "DATA: formatID: 0x%lx (%s) gtrid_length: %ld bqual_length: %ld\n",
                xid.formatID, 
                ((NDRX_XID_FORMAT_ID==xid.formatID ||
                    NDRX_XID_FORMAT_ID==(long)ntohll(xid.formatID))?
                    "fmt OK":"Not Enduro/X or different arch"),
                xid.gtrid_length, xid.bqual_length);

        /* 
         * both formats are fine, as contents are platform agnostic
         * Maybe the ID shall be stored in such way... ?
         */
        if (NDRX_XID_FORMAT_ID==xid.formatID ||
                NDRX_XID_FORMAT_ID==(long)ntohll(xid.formatID))
        {
            short nodeid;
            short srvid;
            unsigned char rmid_start;
            unsigned char rmid_cur;
            long btid;
            char xid_str[NDRX_XID_SERIAL_BUFSIZE];

            NDRX_LOG(log_debug, "Format OK");

            /* get base xid... */

            /* Print Enduro/X related xid data */
            atmi_xa_xid_get_info(&xid, &nodeid, 
                &srvid, &rmid_start, &rmid_cur, &btid);

            /* the generic xid would be with out branch & current rmid */
            /* reset xid trailer to have original xid_str */
            memset(xid.data + xid.gtrid_length - 
                sizeof(long) - sizeof(unsigned char), 
                    0, sizeof(long)+sizeof(unsigned char));

            /* serialize to base xid, btid=0? */
            atmi_xa_serialize_xid(&xid, xid_str);
                
            NDRX_LOG(log_debug, "Got base xid: [%s]", xid_str);
                
            /* query status? */
            if (TPEMATCH==get_xid_status(xid_str, rmid_start,  nodeid, srvid))
            {
                NDRX_LOG(log_error, "Aborting transaction: [%s]", xid_str);
                
                /* Load TMXID with tmxid.
                 * Load TMCMD with ATMI_XA_ABORTLOCAL
                 * If return codes are:
                 * - XA_HEURHAZ
                 * - XA_HEURRB
                 * - XA_HEURCOM
                 * - XA_HEURMIX
                 * we shall call the ATMI_XA_FORGETLOCAL too.
                 */
                
                ret = tran_finalize(ATMI_XA_ABORTLOCAL, recover_svcnm, tmxid);
                
                switch (ret)
                {
                    
                    case XA_OK:
                    case XA_RDONLY:
                        M_aborted++;
                        NDRX_LOG(log_debug, "Aborted OK");
                        break;
                    case XA_HEURHAZ:
                    case XA_HEURCOM:
                    case XA_HEURRB:
                    case XA_HEURMIX:
                        NDRX_LOG(log_debug, "Heuristic result -> forgetting");
                        if (EXSUCCEED!=tran_finalize(ATMI_XA_FORGETLOCAL, recover_svcnm, tmxid))
                        {
                            NDRX_LOG(log_error, "Failed to forget [%s]: %d - ignore",
                                    tmxid, ret);
                        }
                            
                        break;
                }
                ret = EXSUCCEED;
            }
        }
    }
        
out:
    return ret;
}

/**
 * Fill up the growlist of xids from the server.
 * And then process xids one by one, if needed, perform abortlocal
 * @return EXSUCCEED/EXFAIL
 */
exprivate int call_tm(UBFH *p_ub, char *svcnm, short parse)
{
    int ret=EXSUCCEED;
    int cd, i;
    long revent;
    int recv_continue = 1;
    int tp_errno;
    ndrx_growlist_t list;
    char tmp[sizeof(XID)*3];
    BFLDLEN len;
    
    /* what is size of raw xid? */
    ndrx_growlist_init(&list, 100, sizeof(XID)*3);
            
    /* Setup the call buffer... */
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc FB!");
        EXFAIL_OUT(ret);
    }
    
    /* reset the call buffer only to request fields... */
    if (EXSUCCEED!=atmi_xa_reset_tm_call(p_ub))
    {
        NDRX_LOG(log_error, "Failed to prepare UBF for TM call!");
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
            
            /* get the xid bytes... */            
            /* the xid is binary XID recovered in hex */
            len = sizeof(tmp);
            if (EXSUCCEED!=Bget(p_ub, TMXID, 0, (char *)tmp, &len))
            {
                NDRX_LOG(log_error, "Failed to TMXID fld: [%s]",  Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED!=ndrx_growlist_append(&list, tmp))
            {
                NDRX_LOG(log_error, "Failed to add data to growlist of xids");
                EXFAIL_OUT(ret);
            }
            recv_continue=1;
        }
    }
    
    /* OK scan the list and process the xids */
    for (i=0; i<=list.maxindexused; i++)
    {
        if (EXSUCCEED!=process_xid(list.mem + (sizeof(XID)*3)*i, svcnm))
        {
            NDRX_LOG(log_error, "Failed to process xid [%s] at index %d for service [%s]",
                    list.mem + (sizeof(XID)*3)*i, i, svcnm);
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        tpdiscon(cd);
    }

    /* delete the growlist */
    ndrx_growlist_free(&list);


    return ret;
}

/**
 * Filter the service names, return TRUE for those which matches individual TMs
 * Well we shall work only at resource ID level.
 * Thus only on -1
 * @param svcnm
 * @return TRUE/FALSE
 */
exprivate int tmfilter(char *svcnm)
{
    int i, len;
    int cnt = 0;
    
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
    
    if (1==cnt)
        return EXTRUE;
    else
        return EXFALSE;
}

/**
 * Scan all the TMSRVs for transaction
 * Check the status of transaction (base branch tid 0) by the originator
 * if TPEMATCH, then abort transaction & cache the result
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_tmrecover_do(void)
{
    int ret = EXSUCCEED;
    atmi_svc_list_t *el, *tmp, *list;
    UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_RECOVERLOCAL);
    short parse = EXFALSE;
    int lev;
    
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed to alloc UBF!");
        EXFAIL_OUT(ret);
    }
    
    M_aborted = 0;

    list = ndrx_get_svc_list(tmfilter);

    LL_FOREACH_SAFE(list,el,tmp)
    {
        NDRX_LOG(log_info, "About to call service: [%s]", el->svcnm);
        ret = call_tm(p_ub, el->svcnm, parse);
        LL_DELETE(list,el);
        NDRX_FREE(el);
    }
    
    lev = log_warn;

    
    if (M_aborted>0 || EXSUCCEED!=ret)
    {
        lev = log_error;
    }
    
    NDRX_LOG(lev, "Rolled back %d orphan transactions branches(ret=%d)", M_aborted, ret);
    
out:
    
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    if (NULL!=M_trans)
    {
        free_trans();
    }

    if (EXSUCCEED==ret)
    {
        return M_aborted;
    }
    else
    {
        return ret;
    }
}



/* vim: set ts=4 sw=4 et smartindent: */
