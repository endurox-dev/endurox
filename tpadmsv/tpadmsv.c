/**
 * @brief Administrative server
 *
 * @file tpadmsv.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <Excompat.h>
#include <ubfutil.h>
#include <tpadm.h>

#include "tpadmsv.h"
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
extern char *optarg;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_adm_conf_t ndrx_G_adm_config;   /**< admin server config    */
/*---------------------------Statics------------------------------------*/

/*---------------------------Prototypes---------------------------------*/

expublic char ndrx_G_svcnm2[MAXTIDENT+1]; /** our service name, per instance. */

/**
 * MIB Service
 * @param p_svc
 */
void MIB (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    char clazz[MAXTIDENT+1];
    char op[MAXTIDENT+1];
    char cursid[MAXTIDENT+1];
    char cursid_svc[MAXTIDENT+1];
    ndrx_adm_cursors_t cursnew; /* New cursor */
    ndrx_adm_cursors_t *curs;
    BFLDLEN len;
    ndrx_adm_class_map_t *class_map;
    int is_get = EXFALSE;
    long ret_occurs = 0;
    long ret_more = 0;
    UBFH *p_ub = (UBFH *)p_svc->data;
    /* get the incoming buffer new size: */
    long bufsz = NDRX_MIN(ndrx_G_adm_config.buffer_minsz, NDRX_MSGSIZEMAX);
    
    /* Allocate some buffer size  */
    
    ndrx_debug_dump_UBF(log_info, "Request buffer:", p_ub);
    
    /* Realloc to size: */
    p_ub = (UBFH *)tprealloc (p_svc->data, bufsz);
        
    if (NULL==p_ub)
    {
        NDRX_LOG(log_error, "Failed realloc UBF to %d bytes: %s", 
                bufsz, tpstrerror(tperrno));
        p_ub = (UBFH *)p_svc->data;
        EXFAIL_OUT(ret);
    }
    
    len = sizeof(op);
    if (EXSUCCEED!=Bget(p_ub, TA_OPERATION, 0, op, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_OPERATION: %s", Bstrerror(Berror));
        
        ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_OPERATION, 
                "Failed to get TA_OPERATION: %s", Bstrerror(Berror));
        
        EXFAIL_OUT(ret);
    }
    
    /* get cursor if required */
    if (0==strcmp(NDRX_TA_GETNEXT, op))
    {
        char *p;
        len = sizeof(cursid);
        if (EXSUCCEED!=Bget(p_ub, TA_CURSOR, 0, cursid, &len))
        {
            NDRX_LOG(log_error, "Failed to get TA_CURSOR: %s", Bstrerror(Berror));

            ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_CURSOR, 
                        "Failed to get TA_CURSOR: %s", Bstrerror(Berror));

            EXFAIL_OUT(ret);
        }
        
        /* check the cursor target, if other MIB server, then forward
         * the call
         */
        
        NDRX_STRCPY_SAFE(cursid_svc, cursid);
        
        p=strchr(cursid_svc, '_');
        
        if (NULL!=p)
        {
            *p = EXEOS;
        }
        else
        {
            NDRX_LOG(log_error, "Invalid TA_CURSOR format [%s]", cursid_svc);

            ndrx_adm_error_set(p_ub, TAEINVAL, TA_CURSOR, 
                        "Invalid TA_CURSOR format [%s]", cursid_svc);
            EXFAIL_OUT(ret);
        }
        
        if (0!=strcmp(cursid_svc, ndrx_G_svcnm2))
        {
            NDRX_LOG(log_info, "Not our cursor our [%s] vs [%s] - forward",
                    ndrx_G_svcnm2, cursid_svc);
            tpforward(cursid_svc, (char *)p_ub, 0, 0L);
        }
    }
    
    /* Get request class: 
     * In case if we get cursor from other node, then we shall forward there
     * the current request.
     */
    len = sizeof(clazz);
    if (EXSUCCEED!=Bget(p_ub, TA_CLASS, 0, clazz, &len))
    {
        NDRX_LOG(log_error, "Failed to get TA_CLASS: %s", Bstrerror(Berror));
        
        ndrx_adm_error_set(p_ub, TAEREQUIRED, TA_CLASS, 
                "Failed to get TA_CLASS: %s", Bstrerror(Berror));
        
        EXFAIL_OUT(ret);
    }
    
    /* TODO: Needs functions for error handling:
     * with following arguments:
     * TA_ERROR code, TA_BADFLD (optional only for INVAL), TA_STATUS message.
     * The function shall receive UBF buffer and allow to process the format
     * string which is loaded into TA_STATUS.
     * 
     * We need a function to check is buffer approved, if so then we can set
     * the reject. Reject on reject is not possible (first is more significant).
     */

    
    /* find class */
    class_map = ndrx_adm_class_map_get(clazz);
    
    if (NULL==class_map)
    {
        NDRX_LOG(log_error, "Unsupported class [%s]", clazz);

        ndrx_adm_error_set(p_ub, TAESUPPORT, TA_CLASS, 
                    "Unsupported class [%s]", clazz);
        EXFAIL_OUT(ret);
    }
        
    if (0==strcmp(NDRX_TA_GET, op))
    {
        is_get = EXTRUE;
        memset(&cursnew, 0, sizeof(cursnew));
        
        /* get cursor data */
        NDRX_LOG(log_debug, "About to open cursor [%s]",  clazz);
        cursnew.list.maxindexused = EXFAIL;
        if (EXSUCCEED!=class_map->p_get(clazz, &cursnew, 0L))
        {
            NDRX_LOG(log_error, "Failed to open %s cursor", clazz);

            ndrx_adm_error_set(p_ub, TAESYSTEM, BBADFLDID, 
                        "Failed to open %s cursor", clazz);
            EXFAIL_OUT(ret);
        }
        
        /* Prepare cursor  */
        curs = ndrx_adm_curs_new(p_ub, class_map, &cursnew);
        
        if (NULL==curs)
        {
            /* ERR ! Failed to open cursor.. */
            NDRX_LOG(log_error, "Failed to open cursor for %s", clazz);

            ndrx_adm_error_set(p_ub, TAESYSTEM, BBADFLDID, 
                        "Failed to open cursor for %s", clazz);
            EXFAIL_OUT(ret);
        }
        
        /* Load cursor id */
        
        if (EXSUCCEED!=Bchg(p_ub, TA_CURSOR, 0, curs->cursorid, 0L))
        {
            NDRX_LOG(log_error, "Failed to add cursor name: %s", Bstrerror(Berror));

            ndrx_adm_error_set(p_ub, TAESYSTEM, BBADFLDID, 
                        "Failed to add cursor name: %s", Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
    }
    else if (0==strcmp(NDRX_TA_GETNEXT, op))
    {
        is_get = EXTRUE;
        /* read cursor id */
        curs = ndrx_adm_curs_get(cursid);
    }
    else
    {
        ndrx_adm_error_set(p_ub, TAESUPPORT, TA_OPERATION, 
                    "Unsupported operation [%s]", op);
        EXFAIL_OUT(ret);
    }
    
    if (is_get)
    {
        
        if (NULL==curs)
        {
            /* Load the fields in the buffer */  
            goto out_nomore;
        }
        
        /* Fetch cursor to UBF buffer */
        if (EXSUCCEED!=ndrx_adm_curs_fetch(p_ub, curs, &ret_occurs, &ret_more))
        {
            NDRX_LOG(log_error, "Failed to fetch!");
            ndrx_adm_error_set(p_ub, TAESYSTEM, BBADFLDID, 
                        "Failed to fetch!");
        }
    }
    
out_nomore:
    
    if (is_get)
    {
        if (EXSUCCEED!=Bchg(p_ub, TA_OCCURS, 0, (char *)&ret_occurs, 0L))
        {
            NDRX_LOG(log_error, "Failed to set TA_OCCURS to %ld: %s",
                    ret_occurs, Bstrerror(Berror));
        }
            
        if (EXSUCCEED!=Bchg(p_ub, TA_MORE, 0, (char *)&ret_more, 0L))
        {
            NDRX_LOG(log_error, "Failed to set TA_MORE to %ld: %s",
                    ret_more, Bstrerror(Berror));
        }
        
        /* approve the request... */
        ndrx_adm_error_set(p_ub, TAOK, BBADFLDID, "OK");
    }
    
out:
    
    ndrx_debug_dump_UBF(log_info, "Reply buffer:", p_ub);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Do initialization
 * Have a local MIB & shared MIB
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;
    signed char c;
    
    memset(&ndrx_G_adm_config, 0, sizeof(ndrx_G_adm_config));
    
    /* parse clopt: cursor limit and buffer size to alloc... */
    
    /* Parse command line  */
    while ((c = getopt(argc, argv, "n:b:v:s:")) != -1)
    {
        if (optarg)
        {
            NDRX_LOG(log_debug, "%c = [%s]", c, optarg);
        }
        else
        {
            NDRX_LOG(log_debug, "got %c", c);
        }

        switch(c)
        {
            case 'n':
                /* number of cursors allow */
                ndrx_G_adm_config.cursors_max = atoi(optarg);
                break;
            case 'b':
                /* initial buffer size */    
                ndrx_G_adm_config.buffer_minsz = atoi(optarg);
                break;
            case 'v':
                /* cursor validity time, seconds */
                ndrx_G_adm_config.validity = atoi(optarg);
                break;
            case 's':
                /* scan time for dead cursors */
                ndrx_G_adm_config.scantime = atoi(optarg);
                break;
        }
    }
    
    if (ndrx_G_adm_config.buffer_minsz<=0)
    {
        ndrx_G_adm_config.buffer_minsz = TPADM_DEFAULT_BUFFER_MINSZ;
    }
    
    if (ndrx_G_adm_config.validity<=0)
    {
        ndrx_G_adm_config.validity = TPADM_DEFAULT_VALIDITY;
    }
    
    if (ndrx_G_adm_config.scantime<=0)
    {
        ndrx_G_adm_config.scantime = TPADM_DEFAULT_SCANTIME;
    }
    
    if (ndrx_G_adm_config.cursors_max<=0)
    {
        ndrx_G_adm_config.cursors_max = TPADM_DEFAULT_CURSORS_MAX;
    }
    
    NDRX_LOG(log_info, "Max number of cursors allow: %d", 
            ndrx_G_adm_config.cursors_max);
    
    NDRX_LOG(log_info, "Alloc buffer size for return: %d bytes", 
            ndrx_G_adm_config.buffer_minsz);
    
    NDRX_LOG(log_info, "Cursor validity seconds: %d sec", 
            ndrx_G_adm_config.validity);
    
    NDRX_LOG(log_info, "Cursor housekeep: %d sec", 
            ndrx_G_adm_config.scantime);
    
    snprintf(ndrx_G_svcnm2, sizeof(ndrx_G_svcnm2), NDRX_SVC_TMIBNODE, 
            tpgetnodeid(), tpgetsrvid());

    if (EXSUCCEED!=tpadvertise(NDRX_SVC_TMIB, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", NDRX_SVC_TMIB);
        EXFAIL_OUT(ret);
    }
    else if (EXSUCCEED!=tpadvertise(ndrx_G_svcnm2, MIB))
    {
        NDRX_LOG(log_error, "Failed to initialize [%s]!", ndrx_G_svcnm2);
        EXFAIL_OUT(ret);
    }
    
        /* Register timer check.... */
    if (EXSUCCEED==ret &&
            EXSUCCEED!=tpext_addperiodcb(ndrx_G_adm_config.scantime, 
            ndrx_adm_curs_housekeep))
    {
        
        NDRX_LOG(log_error, "tpext_addperiodcb failed: %s",
                        tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

out:
    return ret;
}

void NDRX_INTEGRA(tpsvrdone)(void)
{
    /* just for build... */
}

/* vim: set ts=4 sw=4 et smartindent: */
