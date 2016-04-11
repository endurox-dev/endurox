/* 
** Memory based structures for Q.
**
** @file tmqueue.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>

#include <exnet.h>
#include <ndrxdcmn.h>

#include "tmqueue.h"
#include "../libatmisrv/srv_int.h"
#include "tperror.h"
#include "userlog.h"
#include <xa_cmn.h>
#include "thpool.h"
#include "nstdutil.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define MAX_TOKEN_SIZE          64 /* max key=value buffer size of qdef element */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/

/* Handler for MSG Hash. */
public tmq_memmsg_t *G_msgid_hash;

/* Handler for Q hash */
public tmq_qhash_t *G_qhash;

/* Configuration section */
public tmq_qconfig_t *G_qconfig; 
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Load the key config parameter
 * @param qconf
 * @param key
 * @param value
 */
private int load_param(tmq_qconfig_t * qconf, char *key, char *value)
{
    int ret = SUCCEED; 
    
    NDRX_LOG(log_info, "loading q param: [%s] = [%s]", key, value);
    
    
    if (0==strcmp(key, "name"))
    {
        strncpy(qconf->qname, value, sizeof(qconf->qname)-1);
        qconf->qname[sizeof(qconf->qname)-1] = EOS;
    }
    else if (0==strcmp(key, "svc"))
    {
        strncpy(qconf->svcnm, value, sizeof(qconf->svcnm)-1);
        qconf->svcnm[sizeof(qconf->svcnm)-1] = EOS;
    }
    else if (0==strcmp(key, "auto"))
    {
        if (value[0]=='y' || value[0]=='Y')
        {
            qconf->autoq = TRUE;
        }
    }
    else if (0==strcmp(key, "waitinit"))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitinit = ival;
        
    }
    else if (0==strcmp(key, "waitretry"))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretry = ival;
    }
    else if (0==strcmp(key, "waitretryinc"))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretryinc = ival;
    }
    else if (0==strcmp(key, "waitretrymax"))
    {
        int ival = atoi(value);
        if (!nstdutil_isint(value) || ival < 0)
        {
            NDRX_LOG(log_error, "Invalid value [%s] for key [%s] (must be int>=0)", 
                    value, key);
            FAIL_OUT(ret);
        }
        
        qconf->waitretrymax = ival;
    }
    else if (0==strcmp(key, "memonly"))
    {
        if (value[0]=='y' || value[0]=='Y')
        {
            qconf->memonly = TRUE;
        }
    }
    
out:

    return ret;
}

/**
 * Add/update queue definition
 * Syntax: -q name=VISA,svc=VISAIF,auto=y|n,waitinit=30,waitretry=10,waitretryinc=5,waitretrymax=40,memonly=y|n
 * @param qdef
 * @return  SUCCEED/FAIL
 */
public int tmq_addupd_queue(char *qdef)
{
    tmq_qconfig_t * qconf = calloc(1, sizeof(tmq_qconfig_t));
    char * p;
    char * p2;
    char buf[MAX_TOKEN_SIZE];
    int ret = SUCCEED;
    
    NDRX_LOG(log_info, "Add new Q: [%s]", qdef);
    
    /* Try to load initial config from @ (TMQ_DEFAULT_Q) Q */
    
    p = strtok (qdef,",");
    while (p != NULL)
    {
        NDRX_LOG(log_info, "Got pair [%s]", p);
        
        strncpy(buf, p, sizeof(buf)-1);
        buf[sizeof(buf)-1] = EOS;
        
        p2 = strchr(buf, '=');
        
        if (NULL == p2)
        {
            NDRX_LOG(log_error, "Invalid key=value token [%s] expected '='", buf);
            
            userlog("Error defining queue (%s) expected in '=' in token (%s)", 
                    qdef, buf);
            FAIL_OUT(ret);
        }
        *p2 = EOS;
        p2++;
        
        if (EOS==*p2)
        {
            NDRX_LOG(log_error, "Empty value for token [%s]", buf);
            userlog("Error defining queue (%s) invalid value for token (%s)", 
                    qdef, buf);
            FAIL_OUT(ret);
        }
        
        /*
         * Load the value into structure
         */
        if (SUCCEED!=load_param(qconf, buf, p2))
        {
            NDRX_LOG(log_error, "Failed to load token [%s]/[%s]", buf, p2);
            userlog("Error defining queue (%s) failed to load token [%s]/[%s]", 
                    buf, p2);
            FAIL_OUT(ret);
        }
        
        p = strtok (NULL, ",");
    }
    
out:
    return ret;

}
