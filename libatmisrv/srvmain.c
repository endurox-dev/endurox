/* 
** Enduro/X server main entry point
**
** @file srvmain.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
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
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/

/*---------------------------Includes-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <utlist.h>
#include <string.h>
#include <unistd.h>
#include <tperror.h>

#include <atmi.h>
#include "srv_int.h"
#include "userlog.h"
#include <atmi_int.h>
#include <typed_buf.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
srv_conf_t G_server_conf;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Parse service argument (-s)
 * The format is following:
 * -s<New Service1>[,|/]<New Service2>[,|/]..[,|/]<New Service N>:<existing service>
 * e.g.
 * -sNEWSVC1/NEWSVC2:EXISTINGSVC
 * @param argc
 * @param argv
 * @return
 */
int parse_svc_arg(char *arg)
{
    char alias_name[XATMI_SERVICE_NAME_LENGTH+1]={EXEOS};
    char *p;
    svc_entry_t *entry=NULL;

    NDRX_LOG(log_debug, "Parsing service entry: [%s]", arg);
    
    if (NULL!=(p=strchr(arg, ':')))
    {
        NDRX_LOG(log_debug, "Aliasing requested");
        /* extract alias name out */
        NDRX_STRNCPY(alias_name, p+1, XATMI_SERVICE_NAME_LENGTH);
        alias_name[XATMI_SERVICE_NAME_LENGTH] = 0;
        /* Put the EOS in place of : */
        *p=EXEOS;
    }
    
    /* Now loop through services and add them to the list. 
     * Seems that , is eat up by shell.. thus another symbol could be /
     */
    p = strtok(arg, ",/");
    while (NULL!=p)
    {
        /* allocate memory for entry */
        if ( (entry = (svc_entry_t*)NDRX_MALLOC(sizeof(svc_entry_t))) == NULL)
        {
                ndrx_TPset_error_fmt(TPMINVAL, "Failed to allocate %d bytes while parsing -s",
                                    sizeof(svc_entry_t));
                return EXFAIL; /* <<< return FAIL! */
        }

        NDRX_STRNCPY(entry->svc_nm, p, XATMI_SERVICE_NAME_LENGTH);
        entry->svc_nm[XATMI_SERVICE_NAME_LENGTH] = EXEOS;

        if (EXEOS!=alias_name[0])
        {
            NDRX_STRCPY_SAFE(entry->svc_alias, alias_name);
        }
        
        /*
         * Should we check duplicate names here?
         */
        DL_APPEND(G_server_conf.svc_list, entry);

        NDRX_LOG(log_debug, "-s [%s]:[%s]", entry->svc_nm, entry->svc_alias);
        p = strtok(NULL, ",/");
    }
    
    return EXSUCCEED;
}

/*
 * Lookup conversion function registered for hash
 */
expublic long xcvt_lookup(char *fn_nm)
{
    xbufcvt_entry_t *entry=NULL;
    
    EXHASH_FIND_STR( G_server_conf.xbufcvt_tab, fn_nm, entry); 
    
    if (NULL!=entry)
    {
        return entry->xcvtflags;
    }
    
    return 0;
}

/**
 * Parse flags
 * @param argc
 * @return
 */
int parse_xcvt_arg(char *arg)
{
    char cvtfunc[XATMI_SERVICE_NAME_LENGTH+1]={EXEOS};
    char *p;
    xbufcvt_entry_t *entry=NULL;
    int ret = EXSUCCEED;
    long flags = 0;
    NDRX_LOG(log_debug, "Parsing function buffer convert entry: [%s]", arg);
    
    if (NULL!=(p=strchr(arg, ':')))
    {
        /* Conversion function name */
        NDRX_STRNCPY(cvtfunc, p+1, XATMI_SERVICE_NAME_LENGTH);
        cvtfunc[XATMI_SERVICE_NAME_LENGTH] = 0;
        *p=EXEOS;
        
        /* Verify that function is correct */
        if (0==strcmp(cvtfunc, BUF_CVT_INCOMING_JSON2UBF_STR))
        {
            flags|=SYS_SRV_CVT_JSON2UBF;
        }
        else if (0==strcmp(cvtfunc, BUF_CVT_INCOMING_UBF2JSON_STR))
        {
            flags|=SYS_SRV_CVT_UBF2JSON;
        }
        if (0==strcmp(cvtfunc, BUF_CVT_INCOMING_JSON2VIEW_STR))
        {
            flags|=SYS_SRV_CVT_JSON2VIEW;
        }
        else if (0==strcmp(cvtfunc, BUF_CVT_INCOMING_VIEW2JSON_STR))
        {
            flags|=SYS_SRV_CVT_VIEW2JSON;
        }
        
        if (0==flags)
        {
            NDRX_LOG(log_error, "Invalid automatic buffer conversion function (%s)!", 
                    cvtfunc);
            EXFAIL_OUT(ret);    
        }
    }
    else
    {
        NDRX_LOG(log_error, "Invalid argument for -x (%s) missing `:'", arg);
        EXFAIL_OUT(ret);
    }
    
    p = strtok(arg, ",");
    while (NULL!=p)
    {
        /* allocate memory for entry */
        if ( (entry = (xbufcvt_entry_t*)NDRX_MALLOC(sizeof(xbufcvt_entry_t))) == NULL)
        {
                ndrx_TPset_error_fmt(TPMINVAL, "Failed to allocate %d bytes while parsing -s",
                                    sizeof(svc_entry_t));
                return EXFAIL; /* <<< return FAIL! */
        }

        NDRX_STRNCPY(entry->fn_nm, p, XATMI_SERVICE_NAME_LENGTH);
        entry->xcvtflags = flags;
        
        
        NDRX_LOG(log_debug, "Added have automatic convert option [%s] "
                "for function [%s] (-x)", cvtfunc, entry->fn_nm);
        
        EXHASH_ADD_STR( G_server_conf.xbufcvt_tab, fn_nm, entry );
        
        p = strtok(NULL, ",");
    }
    
out:
    return ret;
}


/**
 * Internal initialization.
 * Here we will:
 * - Determine server name (binary)
 * - Determine client ID (1,2,3,4,5, etc...) (flag -i <num>)
 * @param argc
 * @param argv
 * @return  SUCCEED/FAIL
 */
int ndrx_init(int argc, char** argv)
{
    int ret=EXSUCCEED;
    extern char *optarg;
    extern int optind, optopt, opterr;
    int c;
    int dbglev;
    char *p;
    char key[NDRX_MAX_KEY_SIZE]={EXEOS};

    /* set pre-check values */
    memset(&G_server_conf, 0, sizeof(G_server_conf));
    /* Set default advertise all */
    G_server_conf.advertise_all = 1;
    G_server_conf.time_out = EXFAIL;
    
    /* Load common atmi library environment variables */
    if (EXSUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=EXFAIL;
        goto out;
    }
    
    /* Parse command line, will use simple getopt */
    while ((c = getopt(argc, argv, "h?:D:i:k:e:rs:t:x:N--")) != EXFAIL)
    {
        switch(c)
        {
            case 'k':
                /* just ignore the key... */
                NDRX_STRCPY_SAFE(key, optarg);
                break;
            case 's':
                ret=parse_svc_arg(optarg);
                break;
            case 'x':
                ret=parse_xcvt_arg(optarg);
                break;
            case 'D': /* Not used. */
                dbglev = atoi(optarg);
                tplogconfig(LOG_FACILITY_NDRX, dbglev, NULL, NULL, NULL);
                break;
            case 'i': /* server id */
                /*fprintf(stderr, "got -i: %s\n", optarg);*/
                G_server_conf.srv_id = atoi(optarg);
                break;
            case 'N':
                /* Do not advertise all services */
                G_server_conf.advertise_all = 0;
                break;
            case 'r': /* Not used. */
                /* Not sure actually what does this mean, but ok, lets have it. */
                G_server_conf.log_work = 1;
                break;
            case 'e':
            {
                FILE *f;
                NDRX_STRCPY_SAFE(G_server_conf.err_output, optarg);

                /* Open error log, OK? */
		/* Do we need to close this on exec? */
                if (NULL!=(f=NDRX_FOPEN(G_server_conf.err_output, "a")))
                {
                     /* Bug #176 */
                     if (EXSUCCEED!=fcntl(fileno(f), F_SETFD, FD_CLOEXEC))
                     {
                         userlog("WARNING: Failed to set FD_CLOEXEC: %s", 
				 strerror(errno));
                     }

                    /* Redirect stdout & stderr to error file */
                    close(1);
                    close(2);

                    if (EXFAIL==dup(fileno(f)))
                    {
                        userlog("%s: Failed to dup(1): %s", __func__, strerror(errno));
                    }

                    if (EXFAIL==dup(fileno(f)))
                    {
                        userlog("%s: Failed to dup(2): %s", __func__, strerror(errno));
                    }
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to open error file: [%s]",
                            G_server_conf.err_output);
                }
            }
                break;
            case 't':
                /* Override timeout settings for communications with ndrxd
                 * i.e. for ndrxd resposne waiting & other msg requeuing...
                 */
                G_server_conf.time_out = atoi(optarg);
                break;
            case 'h': case '?':
                fprintf(stderr, "usage: %s [-D dbglev] -i server_id [-N - do "
                        "not advertise servers]"
                        " [-sSERVER:ALIAS] [-sSERVER]\n",
                                argv[0]);
                goto out;
                break;
            /* add support for s */
        }
    }
    
    /* Override the timeout with system value, if FAIL i.e. -t was not present */
    if (EXFAIL==G_server_conf.time_out)
    {
        /* Get timeout */
        if (NULL!=(p=getenv(CONF_NDRX_TOUT)))
        {
            G_server_conf.time_out = atoi(p);
        }
        else
        {
            ndrx_TPset_error_msg(TPEINVAL, "Error: Missing evn param: NDRX_TOUT, "
                    "cannot determine default timeout!");
            ret=EXFAIL;
            goto out;
        }
    }

    NDRX_LOG(log_debug, "Using comms timeout: %d",
                                    G_server_conf.time_out);

    /* Validate the configuration */
    if (G_server_conf.srv_id<1)
    {
        ndrx_TPset_error_msg(TPEINVAL, "Error: server ID (-i) must be >= 1");
        ret=EXFAIL;
        goto out;
    }
    
    /*
     * Extract the binary name
     */
    p=strrchr(argv[0], '/');
    if (NULL!=p)
    {
        NDRX_STRNCPY(G_server_conf.binary_name, p+1, MAXTIDENT);
    }
    else
    {
        NDRX_STRNCPY(G_server_conf.binary_name, argv[0], MAXTIDENT);
    }

    G_server_conf.binary_name[MAXTIDENT] = EXEOS;

    /*
     * Read queue prefix (This is mandatory to have)
     */

    if (NULL==(p=getenv("NDRX_QPREFIX")))
    {
        ndrx_TPset_error_msg(TPEINVAL, "Env NDRX_QPREFIX not set");
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(G_server_conf.q_prefix, p);
    }

    G_srv_id = G_server_conf.srv_id;
    
    /* Defaut number of events supported by e-poll */
    G_server_conf.max_events = 1;
    
out:
    return ret;
}

/**
 * Real processing starts here.
 * @param argc
 * @param argv
 * @return
 */
int ndrx_main(int argc, char** argv)
{
    int ret=EXSUCCEED;

    /* do internal initialisation, get configuration, request for admin q */
    if (EXSUCCEED!=ndrx_init(argc, argv))
    {
        NDRX_LOG(log_error, "ndrx_init() fail");
        userlog("ndrx_init() fail");
        EXFAIL_OUT(ret);
    }
    
    /*
     * Initialise polling subsystem
     */
    ndrx_epoll_sys_init();
    
    /*
     * Initialise services
     */
    if (EXSUCCEED!=tpsvrinit(argc, argv))
    {
        NDRX_LOG(log_error, "tpsvrinit() fail");
        userlog("tpsvrinit() fail");
        EXFAIL_OUT(ret);
    }

    /*
     * Push the services out!
     */
    if (EXSUCCEED!=atmisrv_build_advertise_list())
    {
        NDRX_LOG(log_error, "tpsvrinit() fail");
        userlog("tpsvrinit() fail");
        EXFAIL_OUT(ret);
    }

    /* initialise the library */
    if (EXSUCCEED!=atmisrv_initialise_atmi_library())
    {
        NDRX_LOG(log_error, "initialise_atmi_library() fail");
        userlog("initialise_atmi_library() fail");
        EXFAIL_OUT(ret);
    }
    
    /*
     * Open the queues
     */
    if (EXSUCCEED!=sv_open_queue())
    {
        NDRX_LOG(log_error, "sv_open_queue() fail");
        userlog("sv_open_queue() fail");
        EXFAIL_OUT(ret);
    }
    
    /* Do lib updates after Q open... */
    if (EXSUCCEED!=tp_internal_init_upd_replyq(G_server_conf.service_array[1]->q_descr,
                G_server_conf.service_array[1]->listen_q))
    {
        NDRX_LOG(log_error, "tp_internal_init_upd_replyq() fail");
        userlog("tp_internal_init_upd_replyq() fail");
        EXFAIL_OUT(ret);
    }
    
    /* As we can run even without ndrxd, then we ignore the result of send op */
    report_to_ndrxd();

    /* run process here! */
    if (EXSUCCEED!=(ret=sv_wait_for_request()))
    {
        NDRX_LOG(log_error, "sv_wait_for_request() fail %d", ret);
        userlog("sv_wait_for_request() fail %d", ret);
        goto out;
    }
    
out:
    /* finish up. */
    tpsvrdone();

    /*
     * un-initalize polling sub-system
     */
    ndrx_epoll_sys_uninit();
    
    atmisrv_un_initialize(EXTRUE);
    /*
     * Print error message on exit. 
     */
    if (EXSUCCEED!=ret)
    {
        printf("Error: %s\n", tpstrerror(tperrno));
    }
    
    fprintf(stderr, "Server exit: %d, id: %d\n", ret, G_srv_id);

    return ret;
}


