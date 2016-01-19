/* 
** Client Process Monitor Server
**
** @file cpmsrv.c
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

#include "cpmsrv.h"
#include "userlog.h"
  
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public cpmsrv_config_t G_config;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
public int cpm_pc(UBFH *p_ub, int cd);

/**
 * Client Process Monitor, main thread entry 
 * @param p_svc
 */
void CPMSVC (TPSVCINFO *p_svc)
{
    int ret=SUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    char cmd[2+1];
    char tag[CPM_TAG_LEN]={EOS};
    char subsect[CPM_SUBSECT_LEN]={EOS};
    BFLDLEN len = sizeof(cmd);
    
    p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 4096);
    
    
    if (SUCCEED!=Bget(p_ub, EX_CPMCOMMAND, 0, cmd, &len))
    {
        NDRX_LOG(log_error, "missing EX_CPMCOMMAND!");
        FAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, "Got command: [%s]", cmd);
    
    if (0==strcmp(cmd, "bc") || 0==strcmp(cmd, "sc"))
    {
        /* get tag & subsect */
        len=sizeof(tag);
        Bget(p_ub, EX_CPMTAG, 0, tag, &len);
        len=sizeof(subsect);
        Bget(p_ub, EX_CPMSUBSECT, 0, subsect, &len);
        
        if (EOS==subsect[0])
        {
            strcpy(subsect, "-");
        }
    }
    else if (0==strcmp(cmd, "pc"))
    {
        /* Just print the stuff */
    }
    else 
    {
        Bchg(p_ub, EX_CPMOUTPUT, 0, "Invalid command!", 0L);
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=load_config())
    {
        Bchg(p_ub, EX_CPMOUTPUT, 0, "Failed to load/parse configuration file!", 0L);
        FAIL_OUT(ret);
    }
    
    if (0==strcmp(cmd, "bc"))
    {
        /* start the process */
    } 
    else if (0==strcmp(cmd, "sc"))
    {
        /* stop the process */
    }
    else if (0==strcmp(cmd, "pc"))
    {
        /* Just print the stuff */
        cpm_pc(p_ub, p_svc->cd);
    }

out:
    tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/**
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
    int ret=SUCCEED;

    NDRX_LOG(log_debug, "tpsvrinit called");
    
    /* Get the env */
    if (NULL==(G_config.config_file = getenv(CONF_NDRX_CONFIG)))
    {
        NDRX_LOG(log_error, "%s missing env", CONF_NDRX_CONFIG);
        userlog("%s missing env", CONF_NDRX_CONFIG);
        FAIL_OUT(ret);
    }
    
    /* Load initial config */
    if (SUCCEED!=load_config())
    {
        NDRX_LOG(log_error, "Failed to load client config!");
        FAIL_OUT(ret);
    }
    
    if (SUCCEED!=tpadvertise(NDRX_SVC_CPM, CPMSVC))
    {
        NDRX_LOG(log_error, "Failed to initialize CPMSVC!");
        FAIL_OUT(ret);
    }
    
out:
    return ret;
}


/**
 * Print clients
 * @param p_ub
 * @param cd - call descriptor
 * @return 
 */
public int cpm_pc(UBFH *p_ub, int cd)
{
    int ret = SUCCEED;
    long revent;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    char output[256];
    char buffer [80];
    struct tm * timeinfo;
    
    NDRX_LOG(log_info, "cpm_pc: listing clients");
    /* Remove dead un-needed processes (killed & not in new config) */
    HASH_ITER(hh, G_clt_config, c, ct)
    {
        
        NDRX_LOG(log_info, "cpm_pc: %s/%s", c->tag, c->subsect);
        
        timeinfo = localtime (&c->stattime);
        strftime (buffer,80,"%c",timeinfo);
    
        if (c->is_running)
        {
            sprintf(output, "%s/%s - running (%s)",c->tag, c->subsect, buffer);
        }
        else {
            sprintf(output, "%s/%s - exit %d (%s)", c->tag, c->subsect, 
                    c->exit_status, buffer);
        }
        
        if (SUCCEED!=Bchg(p_ub, EX_CPMOUTPUT, 0, output, 0L))
        {
            NDRX_LOG(log_error, "Failed to read fields: [%s]", 
                Bstrerror(Berror));
            FAIL_OUT(ret);
        }
        
        /*
        fprintf(stderr, "Sending: \n");
        Bfprint(p_ub, stderr);
        */
        
        if (FAIL == tpsend(cd,
                            (char *)p_ub,
                            0L,
                            0,
                            &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            FAIL_OUT(ret);
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }
    }
    
out:

    return ret;
}
