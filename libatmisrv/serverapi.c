/* 
 * @brief Server API functions
 *
 * @file serverapi.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <errno.h>

#include <atmi.h>
#include <ndebug.h>
#include <tperror.h>
#include <typed_buf.h>
#include <atmi_int.h>
#include "srv_int.h"
#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {ndrx_TPunset_error();}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * API function tpreturn
 * @param rval
 * @param rcode
 * @param data
 * @param len
 * @param flags
 */
expublic void tpreturn (int rval, long rcode, char *data, long len, long flags)
{
    API_ENTRY;

    _tpreturn(rval, rcode, data, len, flags);
}

/**
 * API function of tpforward
 * @param svc
 * @param data
 * @param len
 * @param flags
 */
expublic void tpforward (char *svc, char *data, long len, long flags)
{
    API_ENTRY;

    _tpforward (svc, data, len, flags);
}

/**
 * Main server thread, continue with polling.
 */
expublic void tpcontinue (void)
{
    API_ENTRY;
    _tpcontinue ();
}


/**
 * Return server id
 * @return Server id (-i argument value). Can be 0 or random value, if server
 *                  not intialized
 */
expublic int tpgetsrvid (void)
{
    API_ENTRY;
    return  G_server_conf.srv_id;
}

/**
 * Add poller extension
 * @param fd
 * @param events
 * @param p_pollevent
 * @return 
 */
expublic int tpext_addpollerfd(int fd, uint32_t events, 
        void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1))
{
    int ret=EXSUCCEED;
    char *fn="tpext_addpollerfd";
    API_ENTRY;
    
    if (EXFAIL==fd)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid fd, %d", fn, fd);
        ret=EXFAIL;
        goto out;
    }
    
    if (NULL==p_pollevent)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid p_pollevent=NULL!", fn);
        ret=EXFAIL;
        goto out;
    }
    
    ret=_tpext_addpollerfd(fd, events, ptr1, p_pollevent);
    
out:
    return ret;
}

/**
 * Delete poller extension
 * @param fd
 * @return 
 */
expublic int tpext_delpollerfd(int fd)
{
    int ret=EXSUCCEED;
    char *fn="tpext_delpollerfd";
    API_ENTRY;
    
    if (EXFAIL==fd)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid fd, %d", fn, fd);
        ret=EXFAIL;
        goto out;
    }
    
    ret=_tpext_delpollerfd(fd);
    
out:
    return ret;
}

/**
 * Add periodical check
 * @param fd
 * @return 
 */
expublic int tpext_addperiodcb(int secs, int (*p_periodcb)(void))
{
    int ret=EXSUCCEED;
    char *fn="tpext_addperiodcb";
    API_ENTRY;
    
    if (secs<=0)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid secs %d, must be >=0", fn, secs);
        ret=EXFAIL;
        goto out;
    }
    
    if (NULL==p_periodcb)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid p_periodcb, it is NULL!", fn);
        ret=EXFAIL;
        goto out;
    }
    
    ret=_tpext_addperiodcb(secs, p_periodcb);
    
out:
    return ret;
    
}

/**
 * Remove periodical check
 * @param fd
 * @return 
 */
expublic int tpext_delperiodcb(void)
{
    API_ENTRY;
    
    return _tpext_delperiodcb();
}




/**
 * Add before poll check
 * @param fd
 * @return 
 */
expublic int tpext_addb4pollcb(int (*p_b4pollcb)(void))
{
    int ret=EXSUCCEED;
    char *fn="tpext_addb4pollcb";
    API_ENTRY;
    
    if (NULL==p_b4pollcb)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - invalid p_b4pollcb, it is NULL!", fn);
        ret=EXFAIL;
        goto out;
    }
    
    ret=_tpext_addb4pollcb(p_b4pollcb);
    
out:
    return ret;
    
}

/**
 * Remove periodical check
 * @param fd
 * @return 
 */
expublic int tpext_delb4pollcb(void)
{
    API_ENTRY;
    
    return _tpext_delb4pollcb();
}

/**
 * Set server flags, this will be used by bridge.
 * @param flags
 */
expublic void tpext_configbrige 
    (int nodeid, int flags, int (*p_qmsg)(char *buf, int len, char msg_type))
{
    
    ndrx_TPunset_error();
    G_server_conf.flags = flags;
    G_server_conf.nodeid = nodeid;
    G_server_conf.p_qmsg = p_qmsg;
    NDRX_LOG(log_debug, "Bridge %d, flags set to: %d, p_qmsg=%p", 
            nodeid, flags, p_qmsg);
    
}




/**
 * Read current call context data
 * NOTE: buffer must be freed by caller!
 * + Server context is being reset. Assuming that next action by main thread
 * is tpcontinue()
 * WARNING! This suspends global tx!
 * 
 * @param p_buf if not NULL, then use as input buffer, if NULL, then will allocate the buf
 * @param p_len if pp_buf not NULL, size of input buffer, on output real size put in pp_buf is set
 * @return ptr to p_buf or allocated memory or NULL and tperrno will set to error
 */
expublic char * tpsrvgetctxdata2 (char *p_buf, long *p_len)
{
    server_ctx_info_t *ret = NULL;
    tp_command_call_t *last_call = ndrx_get_G_last_call();
    tp_conversation_control_t *p_accept_con;
    
    API_ENTRY;
    
    if (NULL==p_buf)
    {
        ret = NDRX_MALLOC(sizeof(server_ctx_info_t));
        if (NULL==ret)
        {
            ndrx_TPset_error_fmt(TPEOS, "Failed to malloc ctx data: %s", 
                    strerror(errno));
            goto out;
        }
    }
    else
    {
        /* p_buf is not NULL */
        if (*p_len < sizeof(server_ctx_info_t))
        {
            ndrx_TPset_error_fmt(TPEOS, "%s: ERROR ! Context data size: %d, "
                    "but non NULL buffer size %ld", __func__, 
                    strerror(errno), (int)sizeof(server_ctx_info_t), *p_len);
            goto out;
        }
        
        ret = (server_ctx_info_t *)p_buf;
    }
    
    *p_len = sizeof(server_ctx_info_t);
    
    if (tpgetlev())
    {
        ret->is_in_global_tx = EXTRUE;
        if (EXSUCCEED!=tpsuspend(&ret->tranid, 0))
        {
            userlog("Failed to suspend transaction: [%s]", tpstrerror(tperrno));
            NDRX_FREE((char *)ret);
            ret = NULL;
            goto out;
        }
    }
    else
    {
        ret->is_in_global_tx = EXFALSE;
    }
    
    /* reset thread data */
    memcpy(&ret->G_last_call, last_call, sizeof(ret->G_last_call));
    memset(last_call, 0, sizeof(ret->G_last_call));
    
    p_accept_con = ndrx_get_G_accepted_connection();
    memcpy(&ret->G_accepted_connection, p_accept_con, sizeof(*p_accept_con));
    memset(p_accept_con, 0, sizeof(*p_accept_con));
    
out:
    NDRX_LOG(log_debug, "%s: returning %p (last call cd: %d)", 
        __func__, ret, ret->G_last_call.cd);
    return (char *)ret;
}

/**
 * Original version tpsrvgetctxdata, just wrap the automatically allocated memory
 * @return 
 */
expublic char * tpsrvgetctxdata (void)
{
    long len;
    return tpsrvgetctxdata2 (NULL, &len);
}

/**
 * Free up the memory buffer allocated by tpsrvgetctxdata
 * @param p_buf buffer allocated by tpsrvgetctxdata or tpsrvgetctxdata2 with p_buf NULL
 */
expublic void tpsrvfreectxdata(char *p_buf)
{
    NDRX_FREE(p_buf);
}

/**
 * Set context data
 * @param data
 * @param flags
 * @return 
 */
expublic int tpsrvsetctxdata (char *data, long flags)
{
    int ret=EXSUCCEED;
    API_ENTRY;
    server_ctx_info_t *ctxdata  = (server_ctx_info_t *)data;
    char *fn = "tpsrvsetctxdata";
    tp_conversation_control_t *p_accept_con;
    tp_command_call_t * last_call = ndrx_get_G_last_call();
    
    NDRX_LOG(log_debug, "%s: data=%p flags=%ld (last call cd: %d)", 
            fn, data, flags, ctxdata->G_last_call.cd);
    
    if (NULL==data)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s - data is NULL", fn);
        EXFAIL_OUT(ret);
    }
    
#if 0
    if (flags & SYS_SRV_THREAD)
    {
        /* init the client + set reply possible...! */
        if (EXSUCCEED!=tpinit(NULL))
        {
            NDRX_LOG(log_error, "Failed to initialise server thread");
            ret=EXFAIL;
            goto out;
        }
    }
#endif
    memcpy(last_call, &ctxdata->G_last_call, sizeof(ctxdata->G_last_call));
    
    p_accept_con = ndrx_get_G_accepted_connection();
    memcpy(p_accept_con, &ctxdata->G_accepted_connection, 
                sizeof(*p_accept_con));
    
    /* Add the additional flags to the user. */
    last_call->sysflags |= flags;
    
    if (flags & TPNOAUTBUF)
    {
        last_call->autobuf = NULL;
    }
    
    if (ctxdata->is_in_global_tx && EXSUCCEED!=tpresume(&ctxdata->tranid, 0))
    {
        userlog("Failed to resume transaction: [%s]", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

