/**
 * @brief ATMI lib part for XA api
 *   Responsible for:
 *   - Loading the drivers in app.
 *   Think about automatic open...
 *
 * @file xa.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <xa_cmn.h>
#include <tperror.h>
#include <atmi_tls.h>
#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define XA_API_ENTRY(X) {\
    ATMI_TLS_ENTRY;\
    if (!M_is_xa_init) { \
        if (EXSUCCEED!=(ret = atmi_xa_init()))\
        {\
            goto out;\
        }\
    }\
    if (!G_atmi_tls->M_is_curtx_init)\
    {\
        if (EXSUCCEED!=(ret=atmi_xa_init_thread(X)))\
        {\
            goto out;\
        }\
    }\
}\
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* current library status */
exprivate int M_is_xa_init = EXFALSE;

/*---------------------------Prototypes---------------------------------*/
exprivate int atmi_xa_init_thread(int do_open);


/******************************************************************************/
/*                          LIB INIT                                          */
/******************************************************************************/

/**
 * Initialize current thread
 */
exprivate int atmi_xa_init_thread(int do_open)
{
    int ret = EXSUCCEED;
    
    /* ATMI_TLS_ENTRY; - not needed called already from macros which does the init */
    
    memset(&G_atmi_tls->G_atmi_xa_curtx, 0, sizeof(G_atmi_tls->G_atmi_xa_curtx));
    G_atmi_tls->M_is_curtx_init = EXTRUE;
    
out:
    return ret;
}
/**
 * Un-initialize XA lib (for thread)
 * @return 
 */
expublic void atmi_xa_uninit(void)
{
    ATMI_TLS_ENTRY;
    /* do only thread based stuff un-init */
    if (G_atmi_tls->M_is_curtx_init)
    {
        if (G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
        {
            atmi_xa_close_entry();
            G_atmi_tls->G_atmi_xa_curtx.is_xa_open = EXFALSE;
        }
        G_atmi_tls->M_is_curtx_init = EXFALSE;
    }
}


/**
 * Initialize the XA drivers
 * we should load the Enduro/X driver for target XA resource manager 
 * and get handler for XA api.
 *
 * @return 
 */
expublic int atmi_xa_init(void)
{
    int ret=EXSUCCEED;
    void *handle; /* keep the handle, so that we have a reference */
    ndrx_get_xa_switch_loader func;
    char *error;
    char *xa_flags = NULL; /* dynamically allocated... */
    
    if (!M_is_xa_init)
    {
        /* how about thread safety? */
        NDRX_LOG(log_info, "Loading XA driver: [%s]", G_atmi_env.xa_driverlib);
        handle = dlopen (G_atmi_env.xa_driverlib, RTLD_NOW);
        if (!handle)
        {
            error = dlerror();
            NDRX_LOG(log_error, "Failed to load XA lib [%s]: %s", 
                    G_atmi_env.xa_driverlib, error?error:"no dlerror provided");
            
            ndrx_TPset_error_fmt(TPEOS, "Failed to load XA lib [%s]: %s", 
                    G_atmi_env, error?error:"no dlerror provided");
            EXFAIL_OUT(ret);
        }

        func = (ndrx_get_xa_switch_loader)dlsym(handle, "ndrx_get_xa_switch");

        if ((error = dlerror()) != NULL) 
        {
            NDRX_LOG(log_error, "Failed to get symbol `ndrx_get_xa_switch': %s", 
                G_atmi_env.xa_driverlib, error);

            ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get symbol `ndrx_get_xa_switch': %s", 
                G_atmi_env.xa_driverlib, error);
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_info, "About to call ndrx_get_xa_switch()");

        /* Do not deallocate the lib... */
        if (NULL==(G_atmi_env.xa_sw = func()))
        {
            NDRX_LOG(log_error, "Cannot get XA switch handler - "
                            "`ndrx_get_xa_switch()' - returns NULL");
            
            ndrx_TPset_error_fmt(TPESYSTEM,  "Cannot get XA switch handler - "
                            "`ndrx_get_xa_switch()' - returns NULL");
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_info, "Using XA %s", 
                (G_atmi_env.xa_sw->flags&TMREGISTER)?"dynamic registration":"static registration");
        
        /* Parse the flags... and store the config 
         * This is done for Feature #160. Customer have an issue with xa_start
         * after a while. Suspect that firewall closes connections and oracle XA
         * lib looses the connection (fact that there was xa_open()). Thus 
         * at xa_start allow to retry with xa_close(), xa_open() and xa_start.
         */
        NDRX_LOG(log_debug, "xa_flags = [%s]", G_atmi_env.xa_flags);
        if (EXEOS!=G_atmi_env.xa_flags[0])
        {
            char *tag_ptr;
            /* Note this will be parsed and EOS inserted. */
            char *tag_first;
            char *tag_token;
            int token_nr = 0;
            
            char *value_ptr, *value_first, *value_token;
            
            if (NULL==(xa_flags = NDRX_STRDUP(G_atmi_env.xa_flags)))
            {
                int err = errno;
                ndrx_TPset_error_fmt(TPEOS,  "Failed to allocate xa_flags temp buffer: %s", 
                        strerror(err));
                
                userlog("Failed to allocate xa_flags temp buffer: %s", strerror(err));
                
                EXFAIL_OUT(ret);
            }
            
            tag_first = xa_flags;
            NDRX_LOG(log_debug, "About token: [%s]", tag_first);
            while ((tag_token = strtok_r(tag_first, ";", &tag_ptr)))
            {
                if (NULL!=tag_first)
                {
                    tag_first = NULL; /* now loop over the string */
                }
                
                NDRX_LOG(log_debug, "Got tag [%s]", tag_token);
                
                /* format: RECON:<1,2,* - error codes>:<tries>:<sleep_millisec>
                 * "*" - used for any error.
                 * example:
                 * RECON:*:3:250
                 * Meaning: on any xa_start error, reconnect (tpclose/tpopen/tpbegin)
                 * 3x times, between attempts sleep 250ms.
                 */
                if (0==strncmp(tag_token, NDRX_XA_FLAG_RECON_TEST, strlen(NDRX_XA_FLAG_RECON_TEST)))
                {
                    value_first = tag_token;
                    G_atmi_env.xa_recon_usleep = EXFAIL;
                    
                    NDRX_LOG(log_warn, "Parsing RECON tag... [%s]", value_first);
                    
                    while ((value_token = strtok_r(value_first, ":", &value_ptr)))
                    {
                        token_nr++;
                        if (NULL!=value_first)
                        {
                            value_first = NULL; /* now loop over the string */
                        }
                        
                        switch (token_nr)
                        {
                            case 1:
                                /* This is "RECON" */
                                NDRX_LOG(log_debug, "RECON: 1: [%s]", value_token);
                                break;
                            case 2:
                                /* This is list of error codes */
                                NDRX_LOG(log_debug, "RECON: 2: [%s]", value_token);
                                snprintf(G_atmi_env.xa_recon_retcodes, 
                                        sizeof(G_atmi_env.xa_recon_retcodes),
                                        ",%s,", value_token);
                                
                                /* Remove spaces and tabs.. */
                                ndrx_str_strip(G_atmi_env.xa_recon_retcodes, "\t ");
                                
                                break;
                                
                            case 3:
                                NDRX_LOG(log_debug, "RECON: 3: [%s]", value_token);
                                G_atmi_env.xa_recon_times = atoi(value_token);
                                break;
                            case 4:
                                /* so user gives us milliseconds */
                                NDRX_LOG(log_debug, "RECON: 4: [%s]", value_token);
                                G_atmi_env.xa_recon_usleep = atol(value_token)*1000;
                                break;
                        }
                    }
                    
                    if (G_atmi_env.xa_recon_usleep < 0)
                    {
                        NDRX_LOG(log_error, "Invalid [%s] settings in "
                                "XA_FLAGS [%s] (usleep not set)", 
                                NDRX_XA_FLAG_RECON, G_atmi_env.xa_flags);
                        
                        ndrx_TPset_error_fmt(TPEINVAL, "Invalid [%s] settings in "
                                "XA_FLAGS [%s] (usleep not set)", 
                                NDRX_XA_FLAG_RECON, G_atmi_env.xa_flags);
                        
                        EXFAIL_OUT(ret);
                    }
                    
                    NDRX_LOG(log_error, "XA flag: [%s]: on xa_start ret codes: [%s],"
                            " recon number of %d times, sleep %ld "
                            "microseconds between attempts",
                            NDRX_XA_FLAG_RECON, 
                            G_atmi_env.xa_recon_retcodes, 
                            G_atmi_env.xa_recon_times, 
                            G_atmi_env.xa_recon_usleep);
                } /* If tag is RECON */
            } /* for tag.. */
        } /* if xa_flags set */
        
        M_is_xa_init = EXTRUE;
    }
    
out:
                                
    if (NULL!=xa_flags)
    {
        NDRX_FREE(xa_flags);
    }

    if (EXSUCCEED!=ret && NULL!=handle)
    {
        /* close the handle */
        dlclose(handle);
    }

    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_info, "XA lib initialized.");
        /* M_is_xa_init = TRUE; */
    }

    return ret;
}

/******************************************************************************/
/*                          XA ENTRY FUNCTIONS                                */
/******************************************************************************/

/**
 * Wrapper for `open_entry'
 * @return 
 */
expublic int atmi_xa_open_entry(void)
{
    int ret = EXSUCCEED;
    XA_API_ENTRY(EXFALSE); /* already does ATMI_TLS_ENTRY; */
    
    NDRX_LOG(log_debug, "atmi_xa_open_entry RMID=%hd", G_atmi_env.xa_rmid);
    
    if (G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        NDRX_LOG(log_warn, "xa_open_entry already called for context!");
        goto out;
    }
    
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_open_entry(G_atmi_env.xa_open_str, 
                                    G_atmi_env.xa_rmid, 0)))
    {
        NDRX_LOG(log_error, "atmi_xa_open_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        
        /* we should  generate atmi error */
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "atmi_xa_open_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        
        goto out;
    }
    
    G_atmi_tls->G_atmi_xa_curtx.is_xa_open = EXTRUE;
    
    NDRX_LOG(log_info, "XA interface open");
    
out:
    return ret;
}

/**
 * Wrapper for `close_entry'
 * TODO: Maybe we do not need XA_API_ENTRY here...
 * @return 
 */
expublic int atmi_xa_close_entry(void)
{
    int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY */
    
    NDRX_LOG(log_debug, "atmi_xa_close_entry");
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        NDRX_LOG(log_warn, "xa_close_entry already called for context!");
        goto out;
    }
    
    /* lets assume it is closed... */
    G_atmi_tls->G_atmi_xa_curtx.is_xa_open = EXFALSE;
    
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_close_entry(G_atmi_env.xa_close_str, 
                                    G_atmi_env.xa_rmid, 0)))
    {
        NDRX_LOG(log_error, "atmi_xa_close_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        
      /* we should  generate atmi error */
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "atmi_xa_close_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        goto out;
    }
    
out:
    return ret;
}

/**
 * Test the RECON settings for error 
 * @param retcode return code for xa_start to test for
 * @return TRUE - do retry, FALSE - no retry
 */
exprivate int is_error_in_recon_list(int retcode)
{
    char scanstr[16];
    char scanstr2[4] = ",*,";
    int ret = EXFALSE;
    
    snprintf(scanstr, sizeof(scanstr), ",%d,", retcode);
    
    NDRX_LOG(log_warn, "%s testing return code [%s] in recon list [%s]", 
            __func__, scanstr, G_atmi_env.xa_recon_retcodes);
    
    if (NULL!=strstr(G_atmi_env.xa_recon_retcodes, scanstr))
    {
        NDRX_LOG(log_warn, "matched by code - DO RETRY");
        ret = EXTRUE;
        goto out;
    }
    else if (NULL!=strstr(G_atmi_env.xa_recon_retcodes, scanstr2))
    {
        NDRX_LOG(log_warn, "matched by wildcard - DO RETRY");
        ret = EXTRUE;
        goto out;
    }
    
out:
    return ret;
    
}
/**
 * Start transaction (or join..) depending on flags.
 * @param xid
 * @param flags
 * @return 
 */
expublic int atmi_xa_start_entry(XID *xid, long flags, int ping_try)
{
    int ret = EXSUCCEED;
    int need_retry;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "%s", __func__);
    
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_start_entry(xid, 
                                    G_atmi_env.xa_rmid, flags)))
    {
        int tries;
        
        if ((flags & TMJOIN || flags & TMRESUME) && XAER_NOTA==ret)
        {
            need_retry = EXFALSE;
        }
        else
        {
            need_retry = EXTRUE;
        }
        
        if (!ping_try || need_retry)
        {
            NDRX_LOG(log_error, "%s - fail: %d [%s]", 
                    __func__, ret, atmi_xa_geterrstr(ret));
        }
        
        /* Check that return code is in list... */
        if (G_atmi_env.xa_recon_times > 1 && need_retry && is_error_in_recon_list(ret))
        {
            for (tries=1; tries<G_atmi_env.xa_recon_times; tries++)
            {
                NDRX_LOG(log_warn, "RECON: Attempt %d, sleeping %ld micro-sec", 
                        tries, G_atmi_env.xa_recon_usleep);
                usleep(G_atmi_env.xa_recon_usleep);
                
                
                NDRX_LOG(log_warn, "RECON: Retrying...");
                
                /* xa_close */
                
                NDRX_LOG(log_warn, "RECON: atmi_xa_close_entry()");
                
                atmi_xa_close_entry();
                
                NDRX_LOG(log_warn, "RECON: atmi_xa_open_entry()");
                if (EXSUCCEED==atmi_xa_open_entry())
                {

                    /* restart...: */
                    NDRX_LOG(log_warn, "RECON: %s()", __func__);
                    if (XA_OK==(ret = G_atmi_env.xa_sw->xa_start_entry(xid, 
                                        G_atmi_env.xa_rmid, flags)))
                    {
                        NDRX_LOG(log_warn, "RECON: Succeed");
                        break;
                    }
                }
                else
                {
                    NDRX_LOG(log_error, "%s: RECON: Attempt %d - "
                            "fail: %d [%s]", 
                    __func__, tries, ret, atmi_xa_geterrstr(ret));
                }
            } /* for tries */
        } /* if retry supported. */
        
        if (XA_OK!=ret)
        
        {
            if (ping_try && XAER_NOTA==ret)
            {
                /* needs to set error silently.. */
                ndrx_TPset_error_fmt_rsn_silent(TPERMERR,  
                        ret, "finally %s - fail: %d [%s]", 
                        __func__, ret, atmi_xa_geterrstr(ret));
            }
            else
            {
                NDRX_LOG(log_error, "finally %s - fail: %d [%s]", 
                        __func__, ret, atmi_xa_geterrstr(ret));

                ndrx_TPset_error_fmt_rsn(TPERMERR,  
                        ret, "finally %s - fail: %d [%s]", 
                        __func__, ret, atmi_xa_geterrstr(ret));
            }
            goto out;
        }
    }
    
out:
    return ret;
}


/**
 * Disassociate current thread from transaction
 * @param xid
 * @param flags
 * @return 
 */
expublic int atmi_xa_end_entry(XID *xid)
{
    int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "atmi_xa_end_entry");
    
    /* we do always success (as TX intiator decides commit or abort...! */
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_end_entry(xid, 
                                    G_atmi_env.xa_rmid, TMSUCCESS)))
    {
        NDRX_LOG(log_error, "xa_end_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "xa_end_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        goto out;
    }
    
out:
    return ret;
}

/**
 * Rollback the transaction.
 * @param xid
 * @param flags
 * @return XA error code
 */
expublic int atmi_xa_rollback_entry(XID *xid, long flags)
{
int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "atmi_xa_rollback_entry");
    
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_rollback_entry(xid, 
                                    G_atmi_env.xa_rmid, flags)))
    {
        NDRX_LOG(log_error, "xa_rollback_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "xa_rollback_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        goto out;
    }
    
out:
    return ret;
}

/**
 * Prepare for commit current transaction on local resource manager.
 * @param xid
 * @param flags
 * @return XA error code
 */
expublic int atmi_xa_prepare_entry(XID *xid, long flags)
{
int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "atmi_xa_prepare_entry");
     
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_prepare_entry(xid, 
                                    G_atmi_env.xa_rmid, flags)))
    {
        NDRX_LOG(log_error, "xa_prepare_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "xa_prepare_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        goto out;
    }
    
out:
    return ret;
}

/**
 * Prepare for commit current transaction on local resource manager.
 * @param xid
 * @param flags
 * @return XA error code
 */
expublic int atmi_xa_commit_entry(XID *xid, long flags)
{
    int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "atmi_xa_commit_entry");
    if (XA_OK!=(ret = G_atmi_env.xa_sw->xa_commit_entry(xid, 
                                    G_atmi_env.xa_rmid, flags)))
    {
        NDRX_LOG(log_error, "xa_commit_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        ndrx_TPset_error_fmt_rsn(TPERMERR,  ret, "xa_commit_entry - fail: %d [%s]", 
                ret, atmi_xa_geterrstr(ret));
        goto out;
    }
    
out:
    return ret;
}

/**
 * Get the list of transactions associate with RMID
 * @param xids xid array
 * @param count array len of xids
 * @param rmid resource manager id
 * @param flags flags
 * @return >= number of trans in xid, 
 */
expublic int atmi_xa_recover_entry(XID *xids, long count, int rmid, long flags)
{
    int ret = EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
    
    NDRX_LOG(log_debug, "%s", __func__);
    
    if (0 > (ret = G_atmi_env.xa_sw->xa_recover_entry(xids, count,
                                    G_atmi_env.xa_rmid, flags)))
    {
        int tries;
        
        NDRX_LOG(log_error, "%s - fail: %d [%s]", 
                __func__, ret, atmi_xa_geterrstr(ret));
        
        /* Check that return code is in list... */
        if (G_atmi_env.xa_recon_times > 1 && is_error_in_recon_list(ret))
        {
            for (tries=1; tries<G_atmi_env.xa_recon_times; tries++)
            {
                NDRX_LOG(log_warn, "RECON: Attempt %d, sleeping %ld micro-sec", 
                        tries, G_atmi_env.xa_recon_usleep);
                usleep(G_atmi_env.xa_recon_usleep);
                
                
                NDRX_LOG(log_warn, "RECON: Retrying...");
                
                /* xa_close */
                
                NDRX_LOG(log_warn, "RECON: atmi_xa_close_entry()");
                
                atmi_xa_close_entry();
                
                NDRX_LOG(log_warn, "RECON: atmi_xa_open_entry()");
                if (EXSUCCEED==atmi_xa_open_entry())
                {

                    /* restart...: */
                    NDRX_LOG(log_warn, "RECON: %s()", __func__);
                    if (0<=(ret = G_atmi_env.xa_sw->xa_recover_entry(xids, count,
                                    G_atmi_env.xa_rmid, flags)))
                    {
                        NDRX_LOG(log_warn, "RECON: Succeed");
                        break;
                    }
                }
                else
                {
                    NDRX_LOG(log_error, "%s: RECON: Attempt %d - "
                            "fail: %d [%s]", 
                    __func__, tries, ret, atmi_xa_geterrstr(ret));
                }
            } /* for tries */
        } /* if retry supported. */
        
        if (0 > ret)
        {
            NDRX_LOG(log_error, "finally %s - fail: %d [%s]", 
                    __func__, ret, atmi_xa_geterrstr(ret));

            ndrx_TPset_error_fmt_rsn(TPERMERR,  
                    ret, "finally %s - fail: %d [%s]", 
                    __func__, ret, atmi_xa_geterrstr(ret));

            goto out;
        }
    }
    
out:
    return ret;
}

/******************************************************************************/
/*                          ATMI API                                          */
/******************************************************************************/

/**
 * Begin the global transaction.
 * - We should check the context already, maybe we run in transaction already?
 * 
 * But basically we should call the server, to get the transaction id.
 * 
 * @param timeout
 * @param flags
 * @return 
 */
expublic int ndrx_tpbegin(unsigned long timeout, long flags)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = atmi_xa_alloc_tm_call(ATMI_XA_TPBEGIN);
    atmi_xa_tx_info_t xai;
    long tmflags = 0;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY */
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    
    memset(&xai, 0, sizeof(atmi_xa_tx_info_t));
    
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        NDRX_LOG(log_error, "tpbegin: - tpopen() was not called!");
        ndrx_TPset_error_msg(TPEPROTO,  "tpbegin - tpopen() was not called!");
        EXFAIL_OUT(ret);
    }

    if (0!=flags)
    {
        NDRX_LOG(log_error, "tpbegin: flags != 0");
        ndrx_TPset_error_msg(TPEINVAL,  "tpbegin: flags != 0");
        EXFAIL_OUT(ret);
    }
    
    /* If we have active transaction, then we are in txn mode.. */
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_error, "tpbegin: - already in transaction mode XID: [%s]", 
                G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        ndrx_TPset_error_fmt(TPEPROTO,  "tpbegin: - already in transaction mode XID: [%s]", 
                G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "About to call TM");
    /* Load the timeout param to FB... */
    if (EXSUCCEED!=Bchg(p_ub, TMTXTOUT, 0, (char *)&timeout, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "tpbegin: - failed to fill FB - set TMTXTOUT!");
        EXFAIL_OUT(ret);
    }
    
    if (XA_IS_DYNAMIC_REG)
    {
        /* tell RM, then we use dynamic reg (so that it does not register 
         * local RMs work)
         */
        tmflags|=TMTXFLAGS_DYNAMIC_REG;
    }
    
    if (EXSUCCEED!=Bchg(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
    {
        ndrx_TPset_error_fmt(TPESYSTEM,  "tpbegin: - failed to fill FB - set TMTXFLAGS!");
        EXFAIL_OUT(ret);
    }
    
    /* OK, we should call the server, request for transaction...  */
    if (NULL==(p_ub=atmi_xa_call_tm_generic_fb(ATMI_XA_TPBEGIN, NULL, EXTRUE, EXFAIL, 
            NULL, p_ub)))
    {
        NDRX_LOG(log_error, "Failed to execute TM command [%c]", 
                    ATMI_XA_TPBEGIN);
        /* _TPoverride_code(TPETRAN);  - WHY?*/
        EXFAIL_OUT(ret);
    }
    /* We should load current context with transaction info we got 
     * + we should join the transaction i.e. current thread.
     */
    
    /* Load tx info... */
    
    if (EXSUCCEED!=atmi_xa_read_tx_info(p_ub, &xai))
    {
         NDRX_LOG(log_error, "tpbegin: - failed to read TM response");
        ndrx_TPset_error_msg(TPEPROTO,  "tpbegin: - failed to read TM response");
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "About to load tx info");
    
    
    /* Only when we have in transaction, then install the handler */
    if (EXSUCCEED!= atmi_xa_set_curtx_from_xai(&xai))
    {
        NDRX_LOG(log_error, "tpbegin: - failed to set curren tx");
        ndrx_TPset_error_msg(TPEPROTO,  "tpbegin: - failed to set curren tx");
        EXFAIL_OUT(ret);
    }
    
    /* OK... now join the transaction (if we are static...) (only if static) */
    if (!XA_IS_DYNAMIC_REG)
    {
        if (EXSUCCEED!=atmi_xa_start_entry(atmi_xa_get_branch_xid(&xai), TMJOIN, EXFALSE))
        {
            /* TODO: Unset current transaction */
            atmi_xa_reset_curtx();
            NDRX_LOG(log_error, "Failed to join transaction!");
            EXFAIL_OUT(ret);
        }
        
        /* Already set by TM
        atmi_xa_curtx_set_cur_rmid(&xai);
         */
    }
    else
    {
        NDRX_LOG(log_debug, "Working in dynamic mode...");
    }
    
    /*G_atmi_xa_curtx.is_in_tx = TRUE;*/
    G_atmi_tls->G_atmi_xa_curtx.txinfo->is_tx_initiator = EXTRUE;

    NDRX_LOG(log_debug, "Process joined to transaction [%s] OK",
                        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
    
out:

    /* TODO: We need remove curren transaction from HASH! */
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }
    return ret;
}

/**
 * API implementation of tpcommit
 *
 * @param timeout
 * @param flags
 * @return 
 */
expublic int ndrx_tpcommit(long flags)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = NULL;
    int do_abort = EXFALSE;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY; */
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        NDRX_LOG(log_error, "tpcommit: - tpopen() was not called!");
        ndrx_TPset_error_msg(TPEPROTO,  "tpcommit - tpopen() was not called!");
        EXFAIL_OUT(ret);
    }

    if (0!=flags)
    {
        NDRX_LOG(log_error, "tpcommit: flags != 0");
        ndrx_TPset_error_msg(TPEINVAL,  "tpcommit: flags != 0");
        EXFAIL_OUT(ret);
    }
    
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_error, "tpcommit: Not in global TX");
        ndrx_TPset_error_msg(TPEPROTO,  "tpcommit: Not in global TX");
        EXFAIL_OUT(ret);
        
    }
            
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo->is_tx_initiator)
    {
        NDRX_LOG(log_error, "tpcommit: Not not initiator");
        ndrx_TPset_error_msg(TPEPROTO,  "tpcommit: Not not initiator");
        EXFAIL_OUT(ret);
    }
    
    /* Check situation with call descriptors */
    if (atmi_xa_cd_isanyreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->call_cds)))
    {
        NDRX_LOG(log_error, "tpcommit: Open call descriptors found - abort!");
        do_abort = EXTRUE;
    }
    
    if (atmi_xa_cd_isanyreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->conv_cds)))
    {
        NDRX_LOG(log_error, "tpcommit: Open conversation descriptors found - abort!");
        do_abort = EXTRUE;
    }
    
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
    {
        NDRX_LOG(log_error, "tpcommit: Transaction marked as abort only!");
        do_abort = EXTRUE;
    }
    
    if (do_abort)
    {
        ret = ndrx_tpabort(0); /*<<<<<<<<<< RETURN!!! */
        if (EXSUCCEED==ret)
        {
            ndrx_TPset_error_msg(TPEABORT,  "tpcommit: Transaction was marked for "
                    "abort and aborted now!");
            ret=EXFAIL;
        }
        
        return ret;
    }
    
    /* Disassoc from transaction! */
    
    /* TODO: Detect when we need the END entry. 
     * it should be work_done, or static reg!!!
     */
    if (!XA_IS_DYNAMIC_REG || 
            G_atmi_tls->G_atmi_xa_curtx.txinfo->is_ax_reg_called)
    {
        if (EXSUCCEED!= (ret=atmi_xa_end_entry(
                atmi_xa_get_branch_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo))))
        {
            NDRX_LOG(log_error, "Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
            userlog("Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
        }
    }
    
    NDRX_LOG(log_debug, "About to call TM");
    /* OK, we should call the server, request for transaction...  */
    
    if (NULL==(p_ub=atmi_xa_call_tm_generic(ATMI_XA_TPCOMMIT, EXFALSE, EXFAIL, 
            G_atmi_tls->G_atmi_xa_curtx.txinfo)))
    {
        NDRX_LOG(log_error, "Failed to execute TM command [%c]", 
                    ATMI_XA_TPCOMMIT);
        
        /* _TPoverride_code(TPETRAN); */
        
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Transaction [%s] commit OK",
                        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
        
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    /* reset global transaction info */
    atmi_xa_reset_curtx();

    return ret;
}


/**
 * API implementation of tpabort
 * @param timeout
 * @param flags
 * @return 
 */
expublic int ndrx_tpabort(long flags)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = NULL;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY; */
    
    NDRX_LOG(log_debug, "_tpabort enter");
    
    if (!G_atmi_tls->G_atmi_xa_curtx.is_xa_open)
    {
        NDRX_LOG(log_error, "tpabort: - tpopen() was not called!");
        ndrx_TPset_error_msg(TPEPROTO,  "tpabort - tpopen() was not called!");
        EXFAIL_OUT(ret);
    }

    if (0!=flags)
    {
        NDRX_LOG(log_error, "tpabort: flags != 0");
        ndrx_TPset_error_msg(TPEINVAL,  "tpabort: flags != 0");
        EXFAIL_OUT(ret);
    }
    
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_error, "tpabort: Not in global TX");
        ndrx_TPset_error_msg(TPEPROTO,  "tpabort: Not in global TX");
        EXFAIL_OUT(ret);
        
    }
            
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo->is_tx_initiator)
    {
        NDRX_LOG(log_error, "tpabort: Not not initiator");
        ndrx_TPset_error_msg(TPEPROTO,  "tpabort: Not not initiator");
        EXFAIL_OUT(ret);
    }
    
    /* Disassoc from transaction! */
    if (!XA_IS_DYNAMIC_REG || 
            G_atmi_tls->G_atmi_xa_curtx.txinfo->is_ax_reg_called)
    {
        if (EXSUCCEED!= (ret=atmi_xa_end_entry(
                atmi_xa_get_branch_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo))))
        {
            NDRX_LOG(log_error, "Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
            userlog("Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
        }
    }
    
    NDRX_LOG(log_debug, "About to call TM");
    /* OK, we should call the server, request for transaction...  */
    if (NULL==(p_ub=atmi_xa_call_tm_generic(ATMI_XA_TPABORT, EXFALSE, EXFAIL, 
            G_atmi_tls->G_atmi_xa_curtx.txinfo)))
    {
        NDRX_LOG(log_error, "Failed to execute TM command [%c]", 
                    ATMI_XA_TPBEGIN);
        
        /* _TPoverride_code(TPETRAN); */
        
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, "Transaction [%s] abort OK",
                        G_atmi_tls->G_atmi_xa_curtx.txinfo->tmxid);
out:
    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    /* reset global transaction info */
    atmi_xa_reset_curtx();

    return ret;
}

/**
 * Open the entry to XA.
 * @return 
 */
expublic int ndrx_tpopen(void)
{
    int ret=EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
   
    ret = atmi_xa_open_entry();
    
out:
    return ret;
}

/**
 * Close the entry to XA.
 * @return 
 */
expublic int ndrx_tpclose(void)
{
    int ret=EXSUCCEED;
    XA_API_ENTRY(EXTRUE);
   
    ret = atmi_xa_close_entry();
    
out:
    return ret;
}
 
/**
 * Suspend the current transaction in progress.
 * Note we do not care is it server or client. Global transaction (even participants)
 * can be moved to another external process.
 * 
 * NODE: we might get additional error code:TPERMERR when xa_end fails.
 * @param tranid
 * @param flags
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tpsuspend (TPTRANID *tranid, long flags, int is_contexting)
{
    int ret=EXSUCCEED;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY; */
    NDRX_LOG(log_info, "Suspending global transaction...");
    if (NULL==tranid)
    {
        ndrx_TPset_error_msg(TPEINVAL,  "_tpsuspend: trandid = NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (0!=flags)
    {
        ndrx_TPset_error_msg(TPEINVAL,  "_tpsuspend: flags!=0!");
        EXFAIL_OUT(ret);
    }
    
    if (!G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_error, "_tpsuspend: Not in global TX");
        ndrx_TPset_error_msg(TPEPROTO,  "_tpsuspend: Not in global TX");
        EXFAIL_OUT(ret);
    }
    
#if 0
    - I guess this is not a problem. Must be able to suspend abort only transaction
    because of object-api
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo->tmtxflags & TMTXFLAGS_IS_ABORT_ONLY)
    {
        NDRX_LOG(log_error, "_tpsuspend: Abort only transaction!");
        ndrx_TPset_error_msg(TPEPROTO,  "_tpsuspend: Abort only transaction!");
        EXFAIL_OUT(ret);
    }
#endif
    
    /* Check situation with call descriptors */
    if (!is_contexting  /* do not check call descriptors in contexting mode */
            && atmi_xa_cd_isanyreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->call_cds)))
    {
        NDRX_LOG(log_error, "_tpsuspend: Call descriptors still open!");
        ndrx_TPset_error_msg(TPEPROTO,  "_tpsuspend: Call descriptors still open!");
        EXFAIL_OUT(ret);
    }
    
    if (!is_contexting /* do not check call descriptors in contexting mode */
            && atmi_xa_cd_isanyreg(&(G_atmi_tls->G_atmi_xa_curtx.txinfo->conv_cds)))
    {
        NDRX_LOG(log_error, "_tpsuspend: Conversation descriptors still open!");
        ndrx_TPset_error_msg(TPEPROTO,  "_tpsuspend: Conversation descriptors still open!");
        EXFAIL_OUT(ret);
    }
    
    /* Now transfer current transaction data from one struct to another... */
    XA_TX_COPY(tranid, G_atmi_tls->G_atmi_xa_curtx.txinfo);
    tranid->is_tx_initiator = G_atmi_tls->G_atmi_xa_curtx.txinfo->is_tx_initiator;
    
    /* Disassoc from transaction! */
    if (!XA_IS_DYNAMIC_REG || 
            G_atmi_tls->G_atmi_xa_curtx.txinfo->is_ax_reg_called)
    {
        if (EXSUCCEED!= (ret=atmi_xa_end_entry(
                atmi_xa_get_branch_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo))))
        {
            NDRX_LOG(log_error, "Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
            userlog("Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
            goto out;
        }
    }

    atmi_xa_reset_curtx();
    
    NDRX_LOG(log_debug, "Suspend ok xid: [%s]", 
            tranid->tmxid);
out:

    return ret;
}

/**
 * Resume suspended transaction
 * @param tranid
 * @param flags
 * @return 
 */
expublic int  ndrx_tpresume (TPTRANID *tranid, long flags)
{
    int ret=EXSUCCEED;
    int was_join = EXFALSE;
    atmi_xa_tx_info_t xai;
    
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ETNRY; */
    NDRX_LOG(log_info, "Resuming global transaction...");
    
    if (NULL==tranid)
    {
        ndrx_TPset_error_msg(TPEINVAL,  "_tpresume: trandid = NULL!");
        EXFAIL_OUT(ret);
    }
    
    if (0!=flags)
    {
        ndrx_TPset_error_msg(TPEINVAL,  "_tpresume: flags!=0!");
        EXFAIL_OUT(ret);
    }
    
    /* NOTE: TPEMATCH - not tracked. */
    if (G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        ndrx_TPset_error_msg(TPEPROTO,  "_tpresume: Already in global TX!");
        EXFAIL_OUT(ret);
    }
    
    /* Copy off the tx info to call */
    XA_TX_COPY((&xai), tranid);
    
    if (EXSUCCEED!=_tp_srv_join_or_new(&xai, EXFALSE, &was_join))
    {
        ndrx_TPset_error_msg(TPESYSTEM,  "_tpresume: Failed to enter in global TX!");
        EXFAIL_OUT(ret);
    }
    
    G_atmi_tls->G_atmi_xa_curtx.txinfo->is_tx_initiator = tranid->is_tx_initiator;
    
    NDRX_LOG(log_debug, "Resume ok xid: [%s] is_tx_initiator: %d", 
            tranid->tmxid, tranid->is_tx_initiator);
    
out:
    return EXSUCCEED;
}

/**
 * Database is registering transaction in progress...
 * @param rmid
 * @param xid
 * @param flags
 * @return 
 */
expublic int ax_reg(int rmid, XID *xid, long flags)
{
    int ret = TM_OK;
    int was_join = EXFALSE;
    ATMI_TLS_ENTRY;
    
    NDRX_LOG(log_warn, "ax_reg called");
    if (NULL==G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_error, "ERROR: No global transaction registered "
                "with process/thread!");
        userlog("ERROR: No global transaction reigstered with process/thread!");
        memset(xid, 0, sizeof(XID));
        ret = TMER_TMERR;
        goto out;
    }
    
    if (EXSUCCEED!=_tp_srv_join_or_new(G_atmi_tls->G_atmi_xa_curtx.txinfo, EXTRUE, &was_join))
    {
        ret = TMER_TMERR;
        goto out;
    }
    
    if (was_join)
    {
        ret = TM_JOIN;
    }
    
    memcpy(xid, atmi_xa_get_branch_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo), sizeof(*xid));
    
    G_atmi_tls->G_atmi_xa_curtx.txinfo->is_ax_reg_called = EXTRUE;
  
out:
    NDRX_LOG(log_info, "ax_reg returns: %d", ret);
    return ret;
}
 
/**
 * DB is un-registering the transaction
 * @param rmid
 * @param flags
 * @return 
 */
int ax_unreg(int rmid, long flags)
{
    NDRX_LOG(log_warn, "ax_unreg called");
    return EXSUCCEED;
}

/**
 * Wrapper with call structure 
 * @param call
 * @return 
 */
expublic int _tp_srv_join_or_new_from_call(tp_command_call_t *call,
        int is_ax_reg_callback)
{
    int is_known = EXFALSE;
    atmi_xa_tx_info_t xai;
    memset(&xai, 0, sizeof(xai));
    /* get the xai struct */
    /*
    atmi_xa_xai_from_call(&xai, call);
     */
    XA_TX_COPY((&xai), call)
    
    return _tp_srv_join_or_new(&xai, is_ax_reg_callback, &is_known);
}
/**
 * Process should try to join the XA, if fails, then create new transaction
 * @param call
 * @return 
 */
expublic int _tp_srv_join_or_new(atmi_xa_tx_info_t *p_xai,
        int is_ax_reg_callback, int *p_is_known)
{
    int ret = EXSUCCEED;
    UBFH *p_ub = NULL;
    short reason;
    int new_rm = EXFALSE;
    char src_tmknownrms[2];
    long tmflags = 0;
    XA_API_ENTRY(EXTRUE); /* already does ATMI_TLS_ENTRY; */
    
    /* If we are static, then register together... 
     * Dynamic code must be done this already
     */
    if (XA_IS_DYNAMIC_REG)
    {
        if (!is_ax_reg_callback)
        {
            NDRX_LOG(log_debug, "Dynamic reg + process start "
                                "just remember the transaction");
            if (EXSUCCEED!=atmi_xa_set_curtx_from_xai(p_xai))
            {
                EXFAIL_OUT(ret);
            }
            /* Do not do anything more... */
            goto out;
        }
        else
        {
            /* mark current thread as involved (needs xa_end()!) */
            p_xai->is_ax_reg_called=EXTRUE;
        }
    }   /* continue with static... */
    else if (EXSUCCEED!=atmi_xa_set_curtx_from_xai(p_xai))
    {
        EXFAIL_OUT(ret);
    }
    
    if (atmi_xa_is_current_rm_known(p_xai->tmknownrms))
    {    
        *p_is_known=EXTRUE;
        
        if (XA_IS_DYNAMIC_REG)
        {
                NDRX_LOG(log_debug, "Dynamic reg - no start/join!");
        }
        /* Continue with static ...  ok it is known, then just join the transaction */
        else if (EXSUCCEED!=atmi_xa_start_entry(atmi_xa_get_branch_xid(p_xai), TMJOIN, EXFALSE))
        {
            NDRX_LOG(log_error, "Failed to join transaction!");
            EXFAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug, "tx join ok!");
        }
    }
    else
    {
        NDRX_LOG(log_info, "RM not aware of this transaction");
        
        /* register new tx branch/rm */
        if (NULL==(p_ub=atmi_xa_call_tm_generic(ATMI_XA_TMREGISTER, EXFALSE, EXFAIL, p_xai)))
        {
            NDRX_LOG(log_error, "Failed to execute TM command [%c]", 
                        ATMI_XA_TPBEGIN);   
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=Bget(p_ub, TMTXFLAGS, 0, (char *)&tmflags, 0L))
        {
            NDRX_LOG(log_error, "Failed to read TMTXFLAGS!");   

            EXFAIL_OUT(ret);
        }
        
        if (tmflags & TMTXFLAGS_RMIDKNOWN)
        {
            *p_is_known = EXTRUE;
        }
        
        if (XA_IS_DYNAMIC_REG)
        {
            NDRX_LOG(log_debug, "Dynamic reg - no new tx start!");
        }
        /* Continue with static... */
        else if (*p_is_known)
        {
            if (EXSUCCEED!=atmi_xa_start_entry(atmi_xa_get_branch_xid(p_xai), TMJOIN, EXFALSE))
            {
                NDRX_LOG(log_error, "Failed to join transaction!");
                EXFAIL_OUT(ret);
            }
            else
            {
                NDRX_LOG(log_debug, "tx join ok!");
            }
        }
        /* Open new transaction in branch */
        else if (EXSUCCEED!=atmi_xa_start_entry(atmi_xa_get_branch_xid(p_xai), TMNOFLAGS, EXFALSE))
        {
            reason=atmi_xa_get_reason();
            NDRX_LOG(log_error, "Failed to create new tx under local RM (reason: %hd)!", 
                    reason);
            if (XAER_DUPID == (reason=atmi_xa_get_reason()))
            {
                /* It is already known... then join... */
                *p_is_known=EXTRUE;
                
                if (EXSUCCEED!=atmi_xa_start_entry(atmi_xa_get_branch_xid(p_xai), TMJOIN, EXFALSE))
                {
                    NDRX_LOG(log_error, "Failed to join transaction!");
                    EXFAIL_OUT(ret);
                }
                else
                {
                    NDRX_LOG(log_debug, "tx join ok!");
                }
            }
            else
            {
                EXFAIL_OUT(ret);
            }
        }
        new_rm = EXTRUE;
    }
        
    if (new_rm)
    {
        src_tmknownrms[0] = G_atmi_env.xa_rmid;
        src_tmknownrms[1] = EXEOS;
        
        if (EXSUCCEED!=atmi_xa_update_known_rms(G_atmi_tls->G_atmi_xa_curtx.txinfo->tmknownrms, 
                src_tmknownrms))
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:

    if (EXSUCCEED!=ret)
    {
        /* Remove current, if was set... */
        atmi_xa_reset_curtx();
    }

    if (NULL!=p_ub)
    {
        tpfree((char *)p_ub);
    }

    return ret;
}

/**
 * Disassociate current process from transaction 
 * TODO: What about CD's?
 * @return 
 */
expublic int _tp_srv_disassoc_tx(void)
{
    int ret = EXSUCCEED;
    ATMI_TLS_ENTRY;
    
    if (NULL==G_atmi_tls->G_atmi_xa_curtx.txinfo)
    {
        NDRX_LOG(log_warn, "Not in global tx!");
        goto out;
    }
    
    /* Only for static...  or if work done */
    if ( !XA_IS_DYNAMIC_REG || 
            G_atmi_tls->G_atmi_xa_curtx.txinfo->is_ax_reg_called)
    {
        if (EXSUCCEED!= (ret=atmi_xa_end_entry(
                atmi_xa_get_branch_xid(G_atmi_tls->G_atmi_xa_curtx.txinfo))))
        {
            NDRX_LOG(log_error, "Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
            userlog("Failed to end XA api: %d [%s]", 
                    ret, atmi_xa_geterrstr(ret));
        }
    }
    
    /* Remove current transaction from list */
    atmi_xa_curtx_del(G_atmi_tls->G_atmi_xa_curtx.txinfo);
    
    G_atmi_tls->G_atmi_xa_curtx.txinfo = NULL;
    
out:
    return ret;
}

/**
 * Tell master TM that current transaction have been failed.
 * (so that TM can mark transaction as abort only)
 * @return SUCCEED/FAIL
 */
expublic int _tp_srv_tell_tx_fail(void)
{
    int ret = EXSUCCEED;
    
    /* TODO: */
    
out:
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
