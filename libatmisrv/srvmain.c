/* 
** EnduroX server main entry point
**
** @file srvmain.c
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
srv_conf_t G_server_conf;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Parse service argument (-s)
 * @param argc
 * @param argv
 * @return
 */
int parse_svc_arg(char *arg)
{
    char alias_name[XATMI_SERVICE_NAME_LENGTH+1]={EOS};
    char *p;
    svc_entry_t *entry=NULL;

    NDRX_LOG(log_debug, "Parsing service entry: [%s]", arg);
    
    if (NULL!=(p=strchr(arg, ':')))
    {
        NDRX_LOG(log_debug, "Aliasing requested");
        /* extract alias name out */
        strncpy(alias_name, p+1, XATMI_SERVICE_NAME_LENGTH);
        alias_name[XATMI_SERVICE_NAME_LENGTH] = 0;
        /* Put the EOS in place of : */
        *p=EOS;
    }
    
    /* Now loop thourght services and add them to the list. */
    p = strtok(arg, ",");
    while (NULL!=p)
    {
        /* allocate memory for entry */
        if ( (entry = (svc_entry_t*)malloc(sizeof(svc_entry_t))) == NULL)
        {
                _TPset_error_fmt(TPMINVAL, "Failed to allocate %d bytes while parsing -s",
                                    sizeof(svc_entry_t));
                return FAIL; /* <<< return FAIL! */
        }

        strncpy(entry->svc_nm, p, XATMI_SERVICE_NAME_LENGTH);
        entry->svc_nm[XATMI_SERVICE_NAME_LENGTH] = EOS;

        if (EOS!=alias_name[0])
        {
            strcpy(entry->svc_alias, alias_name);
        }
        
        /*
         * Should we check duplicate names here?
         */
        DL_APPEND(G_server_conf.svc_list, entry);

        NDRX_LOG(log_debug, "-s [%s]:[%s]", entry->svc_nm,
                                                         entry->svc_alias);
        p = strtok(NULL, ",");
    }
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
    int ret=SUCCEED;
    extern char *optarg;
    extern int optind, optopt, opterr;
    int c;
    int dbglev;
    char *p;
    char key[NDRX_MAX_KEY_SIZE]={EOS};

    /* set pre-check values */
    memset(&G_server_conf, 0, sizeof(G_server_conf));
    /* Set default advertise all */
    G_server_conf.advertise_all = 1;
    G_server_conf.time_out = FAIL;
    
    /* Load common atmi library environment variables */
    if (SUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=FAIL;
        goto out;
    }
    
    /* Parse command line, will use simple getopt */
    while ((c = getopt(argc, argv, "h?:D:i:k:e:rs:t:N--")) != FAIL)
    {
        switch(c)
        {
            case 'k':
                /* just ignore the key... */
                strcpy(key, optarg);
                break;
            case 's':
                ret=parse_svc_arg(optarg);
                break;
            case 'D': /* Not used. */
                dbglev = atoi(optarg);
                NDRX_DBG_SETLEV(dbglev);
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
                strcpy(G_server_conf.err_output, optarg);

                /* Open error log, OK? */
                if (NULL!=(f=fopen(G_server_conf.err_output, "a")))
                {
                    /* Redirect stdout & stderr to error file */
                    close(1);
                    close(2);
                    dup(fileno(f));
                    dup(fileno(f));
                }
                else
                {
                    NDRX_LOG(log_error, "Failed to open error file: [%s]",
                            G_server_conf.err_output);
                }
            }
                break;
            case 't':
                /* Looks liek */
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
    if (FAIL==G_server_conf.time_out)
    {
        /* Get timeout */
        if (NULL!=(p=getenv("NDRX_TOUT")))
        {
            G_server_conf.time_out = atoi(p);
        }
        else
        {
            _TPset_error_msg(TPEINVAL, "Error: Missing evn param: NDRX_TOUT, "
                    "cannot determine default timeout!");
            ret=FAIL;
            goto out;
        }
    }

    NDRX_LOG(log_debug, "Using comms timeout: %d",
                                    G_server_conf.time_out);

    /* Validate the configuration */
    if (G_server_conf.srv_id<1)
    {
        _TPset_error_msg(TPEINVAL, "Error: server ID (-i) must be >= 1");
        ret=FAIL;
        goto out;
    }
    
    /*
     * Extract the binary name
     */
    p=strrchr(argv[0], '/');
    if (NULL!=p)
    {
        strncpy(G_server_conf.binary_name, p+1, MAXTIDENT);
    }
    else
    {
        strncpy(G_server_conf.binary_name, argv[0], MAXTIDENT);
    }

    G_server_conf.binary_name[MAXTIDENT] = EOS;

    /*
     * Read queue prefix (This is mandatory to have)
     */

    if (NULL==(p=getenv("NDRX_QPREFIX")))
    {
        _TPset_error_msg(TPEINVAL, "Env NDRX_QPREFIX not set");
        ret=FAIL;
        goto out;
    }
    else
    {
        strcpy(G_server_conf.q_prefix, p);
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
    int ret=SUCCEED;

    /* do internal initialization, get configuration, request for admin q */
    ret=ndrx_init(argc, argv);
    
    /*
     * Initialize services
     */
    if (SUCCEED==ret)
    {
        ret=tpsvrinit(argc, argv);
    }

    /*
     * Push the services out!
     */
    if (SUCCEED==ret)
    {
        ret=build_advertise_list();
    }

    if (SUCCEED==ret)
    {
        /* initialize the library */
        ret=initialize_atmi_library();
    }
    
    /*
     * Open the queues
     */
    if (SUCCEED==ret)
    {
        ret=sv_open_queue();
    }
    
    /* Do lib updates after Q open... */
    if (SUCCEED==ret)
    {
        ret=tp_internal_init_upd_replyq(G_server_conf.service_array[1]->q_descr,
                G_server_conf.service_array[1]->listen_q);
    }
    
    if (SUCCEED==ret)
    {
        /* As we can run even witout ndrxd, then we ingore the result of send op */
        report_to_ndrxd();
    }

    /* run process here! */
    if (SUCCEED==ret)
    {
        ret=sv_wait_for_request();
    }

    /* finish up. */
    tpsvrdone();

    un_initialize();
    /*
     * Print error message on exit. 
     */
    if (SUCCEED!=ret)
    {
        printf("Error: %s\n", tpstrerror(tperrno));
    }
    
    fprintf(stderr, "Server exit: %d, id: %d\n", ret, G_srv_id);

    return ret;
}
#if 0
/*
 * EnduroX server main entry point
 */
int main(int argc, char** argv)
{
    return ndrx_main(argc, argv);
}
#endif

