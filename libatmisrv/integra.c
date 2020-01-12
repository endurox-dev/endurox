/**
 * @brief Integration library with different platforms.
 *   Currently provides linked entry points of tpsvrinit() and tpsvrdone(),
 *   but does call the registered callbacks (currently needed for golang linking)
 *
 * @file integra.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include <unistd.h>

#include "srv_int.h"
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/** server init call */
expublic int (*G_tpsvrinit__)(int, char **) = NULL;

/** system call for server init */
expublic int (*ndrx_G_tpsvrinit_sys)(int, char **) = NULL;

/** call for server done */
expublic void (*G_tpsvrdone__)(void) = NULL;

/** No jump please (default for integra) */
expublic long G_libatmisrv_flags     =   ATMI_SRVLIB_NOLONGJUMP; 

/** Server boot structure           */
expublic NDRX_API_EXPORT struct tmsvrargs_t *ndrx_G_tmsvrargs = NULL;
/** XA Switch passed to server      */
expublic NDRX_API_EXPORT struct xa_switch_t *ndrx_G_p_xaswitch = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Dummy for build compatibility
 * @param 
 * @return 
 */
expublic int _tmrunserver(int arg1)
{
    
  return 0;
}

/**
 * System init, advertise by table.
 * TODO: This shall use -N flag. Also new param -S shall be introduced
 * which would allow to advertise by function names from CLI. Probably parsed
 * lists and flags shall be pushed in some global variable so that we do not corrupt
 * the current position of the opts parser.
 * Also -N shall be applied to these bellow services which we are about to advertise???
 * Also -s flag shall be processed here against the system services...
 * or that will be done later, not ?
 * So if we use -N and we have -S aliases, then not having -s will kill the service first
 * maybe we need to automatically "unblacklist particular service" ??
 * Thus any function aliased service must be added to G_server_conf.svc_list with out further
 * aliase. Then it will pass the advertise loop
 * @param argc CLI arg count
 * @param argv CLI values
 * @return EXSUCCEED/EXFAIL
 */
exprivate int tpsrvinit_sys(int argc, char** argv)
{
    int ret = EXSUCCEED;
    struct tmdsptchtbl_t *tab = ndrx_G_tmsvrargs->svctab;
    svc_entry_t *el;
    int found;
    if (NULL!=tab)
    {
        /* run advertise loop over all services */
        while (NULL!=tab->svcnm)
        {
            /* advertise only if have service name */
            if (EXEOS!=tab->svcnm[0] &&
                    EXSUCCEED!=tpadvertise_full(tab->svcnm, tab->p_func, tab->funcnm))
            {
                if (tperrno!=TPEMATCH)
                {
                    NDRX_LOG(log_error, "Failed to advertise svcnm "
                        "[%s] funcnm [%s] ptr=%p: %s",
                        tab->svcnm, tab->funcnm, tab->p_func,
                        tpstrerror(tperrno));
                    EXFAIL_OUT(ret);
                }
            }
        
            tab++;
        }
        
        /* run -S loop function maps -> loop over G_server_conf.funcsvc_list
         * and loop over the "svctab", find the functions, and call the
         * tpadvertise_full call.
         */
        DL_FOREACH(G_server_conf.funcsvc_list, el)
        {
            tab = ndrx_G_tmsvrargs->svctab;
            found = EXFALSE;
            while (NULL!=tab->svcnm)
            {
                if (0==strcmp(el->svc_aliasof, tab->funcnm))
                {
                    /* advertise only if have service name */
                    if (EXSUCCEED!=tpadvertise_full(el->svc_nm, tab->p_func, tab->funcnm))
                    {
                        if (tperrno!=TPEMATCH)
                        {
                            NDRX_LOG(log_error, "Failed to advertise svcnm "
                                "[%s] funcnm [%s] ptr=%p: %s",
                                el->svc_nm, tab->funcnm, tab->p_func,
                                tpstrerror(tperrno));
                            EXFAIL_OUT(ret);
                        }
                    }
                    found = EXTRUE;
                    break;
                }
                tab++;
            }
            
            if (!found)
            {
                ndrx_TPset_error_fmt(TPEMATCH, "ERROR Function not found for "
                        "service mapping (-S) service name [%s] function [%s]!",
                        el->svc_nm, el->svc_aliasof);
                EXFAIL_OUT(ret);
            }
        }
        
    } /* if there is tab */
    
out:
    return ret;
}

/**
 * Forward the call to NDRX
 */
int ndrx_main_integra(int argc, char** argv, int (*in_tpsvrinit)(int, char **), 
            void (*in_tpsvrdone)(void), long flags) 
{

    G_tpsvrinit__ =  in_tpsvrinit;
    G_tpsvrdone__ =  in_tpsvrdone;
    G_libatmisrv_flags = flags;

    return ndrx_main(argc, argv);
}

/**
 * Boot the Enduro/X 
 * Feature #397
 * @param argc command line argument count
 * @param argv command line values
 * @param tmsvrargs server startup info
 * @return EXSUCCEED/EXFAIL
 */
int _tmstartserver( int argc, char **argv, struct tmsvrargs_t *tmsvrargs)
{
    int ret = EXSUCCEED;
    
    if (NULL==tmsvrargs)
    {
        NDRX_LOG(log_error, "Error ! tmsvrargs is NULL!");
        userlog("Error ! tmsvrargs is NULL!");
        EXFAIL_OUT(ret);
    }
    
    ndrx_G_tmsvrargs = tmsvrargs;
    if (NULL!=tmsvrargs)
    {
        ndrx_G_p_xaswitch = tmsvrargs->xa_switch;
    }
    G_libatmisrv_flags = 0;
    
    G_tpsvrinit__ =  tmsvrargs->p_tpsvrinit;
    /* use system init locally provided */
    ndrx_G_tpsvrinit_sys = tpsrvinit_sys;
    G_tpsvrdone__ =  tmsvrargs->p_tpsvrdone;
    
    
out:
    return ndrx_main(argc, argv);
}

/* vim: set ts=4 sw=4 et smartindent: */
