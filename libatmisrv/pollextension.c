/* 
** EnduroX server poller extension.
** TODO: Use hash table for searching for FD!
**
** @file pollextension.c
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

/*---------------------------Includes-----------------------------------*/
#include <ndrx_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include "srv_int.h"
#include "tperror.h"
#include <atmi_int.h>
#include <atmi_shm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
pollextension_rec_t * G_pollext=NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Compares two buffers in linked list (for search purposes)
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
private int ext_find_poller_cmp(pollextension_rec_t *a, pollextension_rec_t *b)
{
    return (a->fd==b->fd?SUCCEED:FAIL);
}


/**
 * Find the buffer in list of known buffers
 * @param ptr
 * @return NULL - buffer not found/ptr - buffer found
 */
public pollextension_rec_t * ext_find_poller(int fd)
{
    pollextension_rec_t *ret=NULL, eltmp;

    eltmp.fd = fd;
    DL_SEARCH(G_pollext, ret, &eltmp, ext_find_poller_cmp);
    
    return ret;
}

/**
 * Extension for polling.
 * @param fd
 * @param events
 * @param p_pollevent
 * @return 
 */
public int _tpext_addpollerfd(int fd, uint32_t events, 
        void *ptr1, int (*p_pollevent)(int fd, uint32_t events, void *ptr1))
{
    int ret=SUCCEED;
    pollextension_rec_t * pollext = NULL;
    pollextension_rec_t * existing = NULL;
    struct ndrx_epoll_event ev;
    
    if (NULL==G_server_conf.service_array)
    {
        _TPset_error_fmt(TPEPROTO, "Cannot add custom poller at init stage!");
        ret=FAIL;
        goto out;
    }
    
    /* Search for existing stuff, will search for FD */
    existing = ext_find_poller(fd);
    if (NULL!=existing)
    {
        _TPset_error_fmt(TPEMATCH, "Poller for fd %d already exists", fd);
        NDRX_LOG(log_error, "Poller for fd %d already exists!", fd);
        ret=FAIL;
        goto out;
    }
    
    pollext = malloc(sizeof(pollextension_rec_t));
    if (NULL==pollext)
    {
        _TPset_error_fmt(TPEOS, "failed to malloc pollextension_rec_t (%d bytes): %s", 
                sizeof(pollextension_rec_t), strerror(errno));
        ret=FAIL;
        goto out;
    }    
    
    /* We are good to go! Add epoll stuff here */
    ev.events = events; /* hmmm what to do? */
    ev.data.fd = fd;
    
    if (FAIL==ndrx_epoll_ctl(G_server_conf.epollfd, EX_EPOLL_CTL_ADD,
                            fd, &ev))
    {
        _TPset_error_fmt(TPEOS, "epoll_ctl failed: %s", ndrx_poll_strerror(ndrx_epoll_errno()));
        ret=FAIL;
        goto out;
    }
    
    /* Add stuff to doubly linked list! */
    pollext->p_pollevent = p_pollevent;
    pollext->fd = fd;
    pollext->ptr1 = ptr1;
/*
    Does not build on AIX!
    pollext->events = events;
*/
    
    DL_APPEND(G_pollext, pollext);
    NDRX_LOG(log_debug, "Function 0x%lx fd=%d successfully added "
            "for polling", p_pollevent, fd);
    
out:

    /* Free resources if dead on arrival */
    if (SUCCEED!=ret && NULL!=pollext)
    {
        free(pollext);
    }

    return ret;
}

/**
 * Remove polling FD
 * @param fd
 * @param events
 * @param p_pollevent
 * @return 
 */
public int _tpext_delpollerfd(int fd)
{
    int ret=SUCCEED;
    pollextension_rec_t * existing = NULL;
    
    if (NULL==G_server_conf.service_array)
    {
        _TPset_error_fmt(TPEPROTO, "Cannot remove custom poller at init stage!");
        ret=FAIL;
        goto out;
    }
    
    /* Search for existing stuff, will search for FD */
    existing = ext_find_poller(fd);
    if (NULL==existing)
    {
        _TPset_error_fmt(TPEMATCH, "No polling extension found for fd %d", fd);
        ret=FAIL;
        goto out;
    }

    /* OK, stuff found, remove from Epoll */
    if (FAIL==ndrx_epoll_ctl(G_server_conf.epollfd, EX_EPOLL_CTL_DEL,
                        fd, NULL))
    {
        _TPset_error_fmt(TPEOS, "epoll_ctl failed to remove fd %d from epollfd: %s", 
                fd, ndrx_poll_strerror(ndrx_epoll_errno()));
        ret=FAIL;
        goto out;
    }
    
    /* Remove from linked list */
    DL_DELETE(G_pollext, existing);
    free(existing);
    
out:

    return ret;
}


/**
 * Add periodical callback
 * @param secs
 * @param p_pollevent
 * @return 
 */
public int _tpext_addperiodcb(int secs, int (*p_periodcb)(void))
{
    int ret=SUCCEED;
    
    G_server_conf.p_periodcb = p_periodcb;
    G_server_conf.periodcb_sec = secs;
    NDRX_LOG(log_debug, "Registering periodic callback func ptr "
            "%p, period: %d sec(s)", 
            G_server_conf.p_periodcb, G_server_conf.periodcb_sec);
    
out:
    return ret;
}

/**
 * Disable periodical callback.
 * @return 
 */
public int _tpext_delperiodcb(void)
{
    int ret=SUCCEED;
    
    NDRX_LOG(log_debug, "Disabling periodical callback, was: 0x%lx",
            G_server_conf.p_periodcb);
    G_server_conf.p_periodcb = NULL;
    G_server_conf.periodcb_sec = 0;
out:
    return ret;
}



/**
 * Add before poll callback
 * @param secs
 * @param p_pollevent
 * @return 
 */
public int _tpext_addb4pollcb(int (*p_b4pollcb)(void))
{
    int ret=SUCCEED;
    
    G_server_conf.p_b4pollcb = p_b4pollcb;
    NDRX_LOG(log_debug, "Registering before poll callback func ptr "
            "0x%lx", 
            G_server_conf.p_b4pollcb);
    
out:
    return ret;
}

/**
 * Disable before poll callback.
 * @return 
 */
public int _tpext_delb4pollcb(void)
{
    int ret=SUCCEED;
    
    NDRX_LOG(log_debug, "Disabling before poll callback, was: 0x%lx",
            G_server_conf.p_b4pollcb);
    G_server_conf.p_b4pollcb = NULL;
out:
    return ret;
}

