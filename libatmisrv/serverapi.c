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
 * @param data
 * @param flags
 * @return 
 */
public char * tpsrvgetctxdata (void)
{
    char *ret = malloc(sizeof(G_last_call));
    API_ENTRY;
    
    if (NULL==ret)
    {
        _TPset_error_fmt(TPEOS, "Failed to malloc ctx data: %s", strerror(errno));
        goto out;
    }
    
    memcpy(ret, &G_last_call, sizeof(G_last_call));
out:    
    return ret;
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
    memcpy(&G_last_call, data, sizeof(G_last_call));
    
    /* Add the additional flags to the user. */
    G_last_call.sysflags |= flags;
    
out:
    return ret;
}

