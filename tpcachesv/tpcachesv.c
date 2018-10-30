/**
 * @brief Cache event receiver, but what we do not want is to subscribe on local events
 *   if subscribed by mask, but if it is local node, then just ignore
 *   Also we advertise service name with node id in it. So that remote node in future
 *   may call service directly.
 *
 * @file tpcachesv.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <sys_unix.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <atmi_shm.h>
#include <exregex.h>
#include "tpcachesv.h"
#include <atmi_cache.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Process incoming events - put and delete
 * @param p_svc
 */
void CACHEEV (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    tp_command_call_t * last_call;
    char *extradata;
    char *extradata_p;
    char nodeidstr[3+1];
    char *op;
    int nodeid;
    char *flags;
    char *svcnm;
    int len;
    char type[XATMI_SUBTYPE_LEN+1];
    
    /* dump the buffer, if it is UBF... */
    
    if (NULL!=p_svc->data)
    {
        if (EXFAIL==tptypes(p_svc->data, type, NULL))
        {
            NDRX_LOG(log_error, "Faileld to get incoming buffer type: %s",
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (0==strcmp(type, "UBF"))
        {
            ndrx_debug_dump_UBF(log_debug, "Received UBF:", (UBFH *)p_svc->data);
        }
    }
    
    /* now understand what request this was 
     * also we need to get timestamps
     */
    last_call=ndrx_get_G_last_call();
    
    /* NDRX_STRCPY_SAFE(extradata, last_call->extradata); */
    
    extradata = extradata_p = NDRX_STRDUP(last_call->extradata);
    
    if (NULL==extradata)
    {
        NDRX_LOG(log_error, "strdup failed: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Received event op: [%s]", extradata);
    
    if (NULL==(op = strsep(&extradata, "/")))
    {
        NDRX_LOG(log_error, "Invalid event [%s] received - failed to get 'operation'", 
                last_call->extradata);
        EXFAIL_OUT(ret);
    }
    
    len = strlen(op);
    
    if (len != NDRX_CACHE_EV_PFXLEN)
    {
        NDRX_LOG(log_error, "Invalid event prefix, expected len: %d, got: %d",
                NDRX_CACHE_EV_PFXLEN, len);
        EXFAIL_OUT(ret);
    }
    
    nodeidstr[0] = op[3];
    nodeidstr[1] = op[3+1];
    nodeidstr[2] = op[3+2];
    nodeidstr[3] = EXEOS;
    
    nodeid = atoi(nodeidstr);
    
    if (nodeid<=0)
    {
        NDRX_LOG(log_error, "Invalid node id received [%d] must be > 0!", 
                nodeid);
        EXFAIL_OUT(ret);
    }
    
    /* if it is our node then skip update as processed locally by caller */
    
    if (nodeid == tpgetnodeid())
    {
        NDRX_LOG(log_debug, "Event received from our node (%d) - skip processing",
                nodeid);
        goto out;
    }
    
    /* why? 
    op[3] = EXEOS;
    */
    
    /* strtok cannot handle empty fields! it goes to next and we get here 
     * service name as flags... thus use strsep() */
    if (NULL==(flags = strsep(&extradata, "/")))
    {
        NDRX_LOG(log_error, "Invalid event [%s] received - failed to get 'flags'",
                last_call->extradata);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(svcnm = strsep(&extradata, "/")))
    {
        NDRX_LOG(log_error, "Invalid event [%s] received - failed to get "
                "'service name' for cache op", last_call->extradata);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Received operation [%s] flags [%s] for service [%s]",
            op, flags, svcnm);
    
    /* check is op correct? */
    if (0==strncmp(op, NDRX_CACHE_EV_PUTCMD, NDRX_CACHE_EV_LEN))
    {
        NDRX_LOG(log_debug, "performing put (save to cache)...");

        /* ok we shall get the cluster node id of the caller.. */
        if (EXSUCCEED!=ndrx_cache_save (svcnm, p_svc->data, 
            p_svc->len, last_call->user3, last_call->user4, nodeid, 0L,
                /* user1 & user2: */
                last_call->rval, last_call->rcode, EXTRUE))
        {
            NDRX_LOG(log_error, "Failed to save cache data: %s", 
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strncmp(op, NDRX_CACHE_EV_DELCMD, NDRX_CACHE_EV_LEN))
    {
        NDRX_LOG(log_debug, "Delete record by data");
        /* Delete cache according to flags, if FULL specified, then drop all matched 
         * cache (no matter of they keys)
         */
        if (EXSUCCEED!=ndrx_cache_inval_by_data(svcnm, p_svc->data, p_svc->len, flags))
        {
            NDRX_LOG(log_error, "Failed to save cache data: %s",
                    tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strncmp(op, NDRX_CACHE_EV_KILCMD, NDRX_CACHE_EV_LEN))
    {
        NDRX_LOG(log_debug, "Drop cache event");
        /*
         * In this case the svcnm is database and we remove all records from it
         */
        if (EXSUCCEED!=ndrx_cache_drop(svcnm, nodeid))
        {
            NDRX_LOG(log_error, "Failed to drop cache: %s", tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
    }
    else if (0==strncmp(op, NDRX_CACHE_EV_MSKDELCMD, NDRX_CACHE_EV_LEN))
    {
        int deleted;
        char keyexpr[NDRX_CACHE_OPEXPRMAX+1];
        UBFH *p_ub = (UBFH *)p_svc->data;
        BFLDLEN len = sizeof(keyexpr);
        
        if (EXSUCCEED!=Bget(p_ub, EX_CACHE_OPEXPR, 0, keyexpr, &len))
        {
            NDRX_CACHE_TPERROR(TPENOENT, "Missing expression for mask delete "
                    "for [%s] db!", svcnm);
            EXFAIL_OUT(ret);
        }
        
        if (0 > (deleted = ndrx_cache_inval_by_expr(svcnm, keyexpr, nodeid)))
        {
            NDRX_LOG(log_error, "Failed to delete db [%s] by key expression [%s]",
                    svcnm, keyexpr);
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_info, "Delete %ld cache records matching key expression [%s]",
                deleted, keyexpr);
    }
    else if (0==strncmp(op, NDRX_CACHE_EV_KEYDELCMD, NDRX_CACHE_EV_LEN))
    {
        int deleted;
        char key[NDRX_CACHE_OPEXPRMAX+1];
        UBFH *p_ub = (UBFH *)p_svc->data;
        BFLDLEN len = sizeof(key);
        
        if (EXSUCCEED!=Bget(p_ub, EX_CACHE_OPEXPR, 0, key, &len))
        {
            NDRX_CACHE_TPERROR(TPENOENT, "Missing expression for mask delete "
                    "for [%s] db!: %s", svcnm, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (0 > (deleted = ndrx_cache_inval_by_key(svcnm, NULL, key, nodeid, 
                NULL, EXFALSE)))
        {
            NDRX_LOG(log_error, "Failed to delete db [%s] by key [%s]: %s",
                    svcnm, key, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_info, "Delete %ld cache records matching key [%s]",
                deleted, key);
    }    
    else
    {
        NDRX_LOG(log_error, "Unsupported cache command received [%s]",
                op);
        EXFAIL_OUT(ret);
    }
    
out:

    if (NULL!=extradata_p)
    {
        NDRX_FREE(extradata_p);
    }

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                NULL,
                0L,
                0L);
}

/**
 * Standard server init
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    char cachesvc[MAXTIDENT+1];
    char mgmtsvc[MAXTIDENT+1];
    long nodeid;
    string_list_t *list = NULL, *el;
    TPEVCTL evctl;
    
    NDRX_LOG(log_debug, "%s called", __func__);
    
    memset(&evctl, 0, sizeof(evctl));
    
    nodeid = tpgetnodeid();
    
    if (EXFAIL==nodeid)
    {
        NDRX_LOG(log_error, "Failed to get nodeid: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    snprintf(cachesvc, sizeof(cachesvc), NDRX_CACHE_EVSVC, nodeid);

    if (EXSUCCEED!=tpadvertise(cachesvc, CACHEEV))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", cachesvc);
        EXFAIL_OUT(ret);
    }
    
    snprintf(mgmtsvc, sizeof(mgmtsvc), NDRX_CACHE_MGSVC, nodeid);
    
    if (EXSUCCEED!=tpadvertise(mgmtsvc, CACHEMG))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", mgmtsvc);
        EXFAIL_OUT(ret);
    }
        
    /* Process the caches and subscribe to corresponding events */
    
    if (EXSUCCEED!=ndrx_cache_events_get(&list))
    {
        NDRX_LOG(log_error, "Failed to get list of events to subscribe to!");
        EXFAIL_OUT(ret);
    }
    
    /* loop over the list and subscribe to */
    
    LL_FOREACH(list, el)
    {
        NDRX_STRCPY_SAFE(evctl.name1, cachesvc);
        evctl.flags|=TPEVSERVICE;
        
        if (EXFAIL==tpsubscribe(el->qname, NULL, &evctl, 0L))
        {
            NDRX_LOG(log_error, "Failed to subscribe to event: [%s] "
                "target service: [%s]", el->qname, evctl.name1);
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (NULL!=list)
    {
        ndrx_string_list_free(list);
    }

    return ret;
}

void NDRX_INTEGRA(tpsvrdone) (void)
{
    NDRX_LOG(log_debug, "%s called", __func__);
}
/* vim: set ts=4 sw=4 et smartindent: */
