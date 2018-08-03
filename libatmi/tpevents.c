/**
 * @brief API functions for Events:
 *   - tpsubscribe
 *   - tpunsubscribe
 *   - tppost
 *
 * @file tpevents.c
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

#include <ubf.h>
#include <atmi.h>
#include <atmi_int.h>
#include <ndebug.h>
#include <ndrstandard.h>
#include <Exfields.h>
#include <tperror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Internal version of tpsubscribe.
 * Actually do the main logic of the function
 */
expublic long ndrx_tpsubscribe(char *eventexpr, char *filter, TPEVCTL *ctl, long flags)
{
    long ret=EXSUCCEED;
    UBFH *p_ub = NULL;
    char *ret_buf;
    long ret_len;
    short nodeid = (short)tpgetnodeid();
    char tmpsvc[MAXTIDENT+1];
    
    NDRX_LOG(log_debug, "%s enter", __func__);

    if (NULL==eventexpr || EXEOS==eventexpr[0])
    {
        ndrx_TPset_error_fmt(TPEINVAL, "eventexpr cannot be null/empty!");
        ret=EXFAIL;
        goto out;
    }
    /* Check the lenght */
    if (strlen(eventexpr)>255)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "eventexpre longer than 255 bytes!");
        ret=EXFAIL;
        goto out;
    }

    if (NULL==ctl)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "ctl cannot be null/empty!");
        ret=EXFAIL;
        goto out;
    }

    if (EXEOS==ctl->name1[0])
    {
        ndrx_TPset_error_fmt(TPEINVAL, "ctl->name1 cannot be null!");
        ret=EXFAIL;
        goto out;
    }

    /* All ok, we can now allocate the buffer... */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
    {
        NDRX_LOG(log_error, "%s: failed to allocate 1024", __func__);
        ret=EXFAIL;
        goto out;
    }

    /* Set up paramters */
    if (EXFAIL==Badd(p_ub, EV_MASK, (char *)eventexpr, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EV_MASK/eventexpr: [%s]", Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    /* Check the len */
    if (NULL!=filter && EXEOS!=filter[0] && strlen(filter)>255)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "filter longer than 255 bytes!");
        ret=EXFAIL;
    }

    /* setup filter argument is set */
    if (NULL!=filter && EXEOS!=filter[0] &&
            EXFAIL==Badd(p_ub, EV_FILTER, filter, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EV_FILTER/filter: [%s]",
                                            Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }


    if (EXFAIL==CBadd(p_ub, EV_FLAGS, (char *)&ctl->flags, 0L, BFLD_LONG))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EV_FLAGS/flags: [%s]",
                                            Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    if (EXFAIL==CBadd(p_ub, EV_SRVCNM, ctl->name1, 0L, BFLD_STRING))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EV_SRVCNM/name1: [%s]",
                                            Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }
    
    snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SYS_SVC_PFX EV_TPEVSUBS, nodeid);
    
    if (EXFAIL!=(ret=tpcall(tmpsvc, (char *)p_ub, 0L, &ret_buf, &ret_len, flags)))
    {
        ret=tpurcode; /* Return code - count of events applied */
    }

out:
    /* free up any allocated resources */
    if (NULL!=p_ub)
    {
        atmi_error_t err;
        /* Save the original error/needed later! */
        ndrx_TPsave_error(&err);
        tpfree((char*)p_ub);
        ndrx_TPrestore_error(&err);
    }

    NDRX_LOG(log_debug, "%s returns %ld", __func__, ret);
    
    return ret;
}

/**
 * Internal version of tpunsubscribe.
 * Actually do the main logic of the function
 */
expublic long ndrx_tpunsubscribe(long subscription, long flags)
{
    long ret=EXSUCCEED;
    UBFH *p_ub = NULL;
    char *ret_buf;
    long ret_len;
    short nodeid = (short)tpgetnodeid();
    char tmpsvc[MAXTIDENT+1];

    NDRX_LOG(log_debug, "%s enter", __func__);

    /* All ok, we can now allocate the buffer... */
    if (NULL==(p_ub = (UBFH *)tpalloc("UBF", NULL, 512)))
    {
        NDRX_LOG(log_error, "%s: failed to allocate 512", __func__);
        ret=EXFAIL;
        goto out;
    }

    /* Check the validity of arguments */
    if (subscription<-1)
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: subscription %ld cannot be < -1",
                                     __func__, subscription);
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL==CBadd(p_ub, EV_SUBSNR, (char *)&subscription, 0L, BFLD_LONG))
    {
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set EV_SUBSNR/flags: [%s]",
                                            Bstrerror(Berror));
        ret=EXFAIL;
        goto out;
    }

    snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SYS_SVC_PFX EV_TPEVUNSUBS, nodeid);
    if (EXFAIL!=(ret=tpcall(tmpsvc, (char *)p_ub, 0L, 
            &ret_buf, &ret_len, flags)))
    {
        ret=tpurcode; /* Return code - count of events applied */
    }

out:
    /* free up any allocated resources */
    if (NULL!=p_ub)
    {
        atmi_error_t err;
        /* Save the original error/needed later! */
        ndrx_TPsave_error(&err);
        tpfree((char*)p_ub);
        ndrx_TPrestore_error(&err);
    }

    NDRX_LOG(log_debug, "%s returns %ld", __func__, ret);
    return ret;
}

/**
 * Internal version of tppost
 * @param eventname
 * @param data
 * @param len
 * @param flags
 * @param user1 user data field 1
 * @param user2 user data field 2
 * @return
 */
expublic int ndrx_tppost(char *eventname, char *data, long len, long flags,
            int user1, long user2, int user3, long user4)
{
    int ret=EXSUCCEED;
    char *ret_buf;
    long ret_len;
    short nodeid = (short)tpgetnodeid();
    char tmpsvc[MAXTIDENT+1];
    
    NDRX_LOG(log_debug, "%s enter", __func__);

    if (NULL==eventname || EXEOS==eventname[0])
    {
        ndrx_TPset_error_fmt(TPEINVAL, "%s: eventname cannot be null/empty", __func__);
        ret=EXFAIL;
        goto out;
    }

    /* Post the */
    snprintf(tmpsvc, sizeof(tmpsvc), NDRX_SYS_SVC_PFX EV_TPEVPOST, nodeid);
    if (EXFAIL!=(ret=ndrx_tpcall(tmpsvc, data, len, &ret_buf, &ret_len, flags, 
            eventname, EXFAIL, 0, user1, user2, user3, user4)))
    {
        ret=tpurcode; /* Return code - count of events applied */
    }

out:

    NDRX_LOG(log_debug, "%s returns %d", __func__, ret);
    return ret;
}
