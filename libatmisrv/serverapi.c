/* 
** Server API functions
**
** @file serverapi.c
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
#define API_ENTRY {_TPunset_error();}
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
public void tpreturn (int rval, long rcode, char *data, long len, long flags)
{
    API_ENTRY;

    return _tpreturn(rval, rcode, data, len, flags);
}

/**
 * API function of tpforward
 * @param svc
 * @param data
 * @param len
 * @param flags
 */
public void tpforward (char *svc, char *data, long len, long flags)
{
    API_ENTRY;

    _tpforward (svc, data, len, flags);
}

/**
 * Main server thread, continue with polling.
 */
public void tpcontinue (void)
{
    API_ENTRY;
    _tpcontinue ();
}


/**
 * Return server id
 * @return Server id (-i argument value). Can be 0 or random value, if server
 *                  not intialized
 */
public int tpgetsrvid (void)
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
public int tpext_addpollerfd(int fd, uint32_t events, 
        void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1))
{
    int ret=SUCCEED;
    char *fn="tpext_addpollerfd";
    API_ENTRY;
    
    if (FAIL==fd)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid fd, %d", fn, fd);
        ret=FAIL;
        goto out;
    }
    
    if (NULL==p_pollevent)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid p_pollevent=NULL!", fn);
        ret=FAIL;
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
public int tpext_delpollerfd(int fd)
{
    int ret=SUCCEED;
    char *fn="tpext_delpollerfd";
    API_ENTRY;
    
    if (FAIL==fd)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid fd, %d", fn, fd);
        ret=FAIL;
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
public int tpext_addperiodcb(int secs, int (*p_periodcb)(void))
{
    int ret=SUCCEED;
    char *fn="tpext_addperiodcb";
    API_ENTRY;
    
    if (secs<=0)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid secs %d, must be >=0", fn, secs);
        ret=FAIL;
        goto out;
    }
    
    if (NULL==p_periodcb)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid p_periodcb, it is NULL!", fn);
        ret=FAIL;
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
public int tpext_delperiodcb(void)
{
    API_ENTRY;
    
    return _tpext_delperiodcb();
}




/**
 * Add before poll check
 * @param fd
 * @return 
 */
public int tpext_addb4pollcb(int (*p_b4pollcb)(void))
{
    int ret=SUCCEED;
    char *fn="tpext_addb4pollcb";
    API_ENTRY;
    
    if (NULL==p_b4pollcb)
    {
        _TPset_error_fmt(TPEINVAL, "%s - invalid p_b4pollcb, it is NULL!", fn);
        ret=FAIL;
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
public int tpext_delb4pollcb(void)
{
    API_ENTRY;
    
    return _tpext_delb4pollcb();
}

/**
 * Set server flags, this will be used by bridge.
 * @param flags
 */
public void	tpext_configbrige 
    (int nodeid, int flags, int (*p_qmsg)(char *buf, int len, char msg_type))
{
    
    _TPunset_error();
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
 * @param data
 * @param flags
 * @return 
 */
public char * tpsrvgetctxdata (void)
{
    server_ctx_info_t *ret = NDRX_MALLOC(sizeof(server_ctx_info_t));
    tp_command_call_t *last_call = ndrx_get_G_last_call();
    tp_conversation_control_t *p_accept_con;
    
    API_ENTRY;
    if (NULL==ret)
    {
        _TPset_error_fmt(TPEOS, "Failed to malloc ctx data: %s", strerror(errno));
        goto out;
    }
    
    if (tpgetlev())
    {
        ret->is_in_global_tx = TRUE;
        if (SUCCEED!=tpsuspend(&ret->tranid, 0))
        {
            userlog("Failed to suspend transaction: [%s]", tpstrerror(tperrno));
            NDRX_FREE((char *)ret);
            ret = NULL;
            goto out;
        }
    }
    else
    {
        ret->is_in_global_tx = FALSE;
    }
    
    /* reset thread data */
    memcpy(&ret->G_last_call, last_call, sizeof(ret->G_last_call));
    memset(last_call, 0, sizeof(ret->G_last_call));
    
    p_accept_con = ndrx_get_G_accepted_connection();
    memcpy(&ret->G_accepted_connection, p_accept_con, sizeof(*p_accept_con));
    memset(p_accept_con, 0, sizeof(*p_accept_con));
    
out:    
    return (char *)ret;
}

/**
 * Set context data
 * @param data
 * @param flags
 * @return 
 */
public int tpsrvsetctxdata (char *data, long flags)
{
    int ret=SUCCEED;
    API_ENTRY;
    server_ctx_info_t *ctxdata  = (server_ctx_info_t *)data;
    char *fn = "tpsrvsetctxdata";
    tp_conversation_control_t *p_accept_con;
    tp_command_call_t * last_call = ndrx_get_G_last_call();
    if (NULL==data)
    {
        _TPset_error_fmt(TPEINVAL, "%s - data is NULL", fn);
        FAIL_OUT(ret);
    }
    
#if 0
    if (flags & SYS_SRV_THREAD)
    {
        /* init the client + set reply possible...! */
        if (SUCCEED!=tpinit(NULL))
        {
            NDRX_LOG(log_error, "Failed to initialise server thread");
            ret=FAIL;
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
    
    if (ctxdata->is_in_global_tx && SUCCEED!=tpresume(&ctxdata->tranid, 0))
    {
        userlog("Failed to resume transaction: [%s]", tpstrerror(tperrno));
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}

