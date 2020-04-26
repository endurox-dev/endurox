/**
 * @brief Enduro/X server main entry point
 *
 * @file srvmain.c
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
#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

    
/** Alloc the CLOPTS */
#define REALLOC_CLOPT_STEP    10
#define REALLOC_CLOPT alloc_args+=REALLOC_CLOPT_STEP; \
    if (NULL==argv) \
        argv = NDRX_MALLOC(sizeof(char *)*alloc_args); \
    else \
        argv = NDRX_REALLOC(argv, sizeof(char *)*alloc_args); \
    if (NULL==argv) \
    {\
        int err = errno;\
        fprintf(stderr, "%s: failed to realloc %ld bytes: %s\n", __func__, \
            (long)sizeof(char *)*alloc_args, strerror(err));\
        userlog("%s: failed to realloc %ld bytes: %s\n", __func__, \
            (long)sizeof(char *)*alloc_args, strerror(err));\
        exit(1);\
    }

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
srv_conf_t G_server_conf;
ndrx_svchash_t *ndrx_G_svchash_skip = NULL;

/**
 * List of defer messages so that we can call self services during the
 * startup...
 */
exprivate ndrx_tpacall_defer_t *M_deferred_tpacalls = NULL;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
exprivate int ndrx_tpacall_noservice_hook_defer(char *svc, char *data, long len, long flags);
/**
 * Add service to skip advertise list
 * @param svcnm
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_skipsvc_add(char *svc_nm)
{
    int ret = EXSUCCEED;
    ndrx_svchash_t *el = NULL;
    
    if (NULL==(el = NDRX_MALLOC(sizeof(ndrx_svchash_t))))
    {
        NDRX_LOG(log_error, "%s: Failed to malloc: %s", 
                __func__, strerror(errno));
        userlog("%s: Failed to malloc: %s", 
                __func__, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE(el->svc_nm, svc_nm);
    EXHASH_ADD_STR( ndrx_G_svchash_skip, svc_nm, el);
    
out:
    return ret;
}

/**
 * Check service is it for advertise
 * @param svcnm
 * @return EXFALSE/EXTRUE
 */
expublic int ndrx_skipsvc_chk(char *svc_nm)
{
    ndrx_svchash_t *el = NULL;
    
    EXHASH_FIND_STR( ndrx_G_svchash_skip, svc_nm, el);
    
    if (NULL!=el)
    {
        return EXTRUE;
    }
    
    return EXFALSE;
}

/**
 * Delete hash list (un-init)
 */
expublic void ndrx_skipsvc_delhash(void)
{
    ndrx_svchash_t *el = NULL, *elt = NULL;
    
    EXHASH_ITER(hh, ndrx_G_svchash_skip, el, elt)
    {
        EXHASH_DEL(ndrx_G_svchash_skip, el);
        NDRX_FREE(el);
    }
}

/**
 * Parse service argument (-s)
 * The format is following:
 * -s<New Service1>[,|/]<New Service2>[,|/]..[,|/]<New Service N>:<existing service>
 * e.g.
 * -sNEWSVC1/NEWSVC2:EXISTINGSVC
 * @param msg1 debug msg1
 * @param argc
 * @param argv
 * @return
 */
expublic int ndrx_parse_svc_arg_cmn(char *msg1,
        svc_entry_t **root_svc_list, char *arg)
{
    char alias_name[XATMI_SERVICE_NAME_LENGTH+1]={EXEOS};
    char *p;
    svc_entry_t *entry=NULL;

    NDRX_LOG(log_debug, "Parsing %s entry: [%s]", msg1, arg);
    
    if (NULL!=(p=strchr(arg, ':')))
    {
        NDRX_LOG(log_debug, "Aliasing requested");
        /* extract alias name out */
        NDRX_STRCPY_SAFE(alias_name, p+1);
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
            ndrx_TPset_error_fmt(TPMINVAL, 
                    "Failed to allocate %d bytes while parsing -s",
                    sizeof(svc_entry_t));
            return EXFAIL; /* <<< return FAIL! */
        }

        NDRX_STRCPY_SAFE(entry->svc_nm, p);
        entry->svc_aliasof[0]=EXEOS;
                
        if (EXEOS!=alias_name[0])
        {
            NDRX_STRCPY_SAFE(entry->svc_aliasof, alias_name);
        }
        
        /*
         * Should we check duplicate names here?
         */
        DL_APPEND((*root_svc_list), entry);

        NDRX_LOG(log_debug, "%s [%s]:[%s]", msg1, entry->svc_nm, entry->svc_aliasof);
        p = strtok(NULL, ",/");
    }
    
    return EXSUCCEED;
}

/**
 * Process -s CLI flag / fill array, service:service mapping flag
 * @param arg -s flag value
 * @return 
 */
expublic int ndrx_parse_svc_arg(char *arg)
{
    return ndrx_parse_svc_arg_cmn("-s", &G_server_conf.svc_list, arg);
}

/**
 * parse -S service:function mapping flag
 * @param root_svc_list
 * @param arg -S flag value
 */
expublic int ndrx_parse_func_arg(char *arg)
{
    return ndrx_parse_svc_arg_cmn("-S", &G_server_conf.funcsvc_list, arg);
}

/*
 * Lookup conversion function registered for hash
 */
expublic long ndrx_xcvt_lookup(char *fn_nm)
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
int ndrx_parse_xcvt_arg(char *arg)
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
        NDRX_STRCPY_SAFE(cvtfunc, p+1);
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

        NDRX_STRCPY_SAFE(entry->fn_nm, p);
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
expublic int ndrx_init(int argc, char** argv)
{
    int ret=EXSUCCEED;
    int c;
    int dbglev;
    char *p;
    char key[NDRX_MAX_KEY_SIZE]={EXEOS};
    char rqaddress[NDRX_MAX_Q_SIZE+1] = "";
    char tmp[NDRX_MAX_Q_SIZE+1];
    
    /* Create ATMI context */
    ATMI_TLS_ENTRY;

    /* set pre-check values */
    memset(&G_server_conf, 0, sizeof(G_server_conf));
    /* Set default advertise all */
    G_server_conf.advertise_all = 1;
    G_server_conf.time_out = EXFAIL;
    G_server_conf.mindispatchthreads = 1;
    G_server_conf.maxdispatchthreads = 1;
    
    /* Load common atmi library environment variables */
    if (EXSUCCEED!=ndrx_load_common_env())
    {
        NDRX_LOG(log_error, "Failed to load common env");
        ret=EXFAIL;
        goto out;
    }
    
    /* Parse command line, will use simple getopt */
    while ((c = getopt(argc, argv, "h?:D:i:k:e:R:rs:t:x:Nn:S:--")) != EXFAIL)
    {
        switch(c)
        {
            case 'k':
                /* just ignore the key... */
                NDRX_STRCPY_SAFE(key, optarg);
                break;
            case 'R':
                /* just ignore the key... */
                
                if (NDRX_SYS_SVC_PFXC==optarg[0])
                {
                    NDRX_LOG(log_error, "-R request address cannot start with [%c]",
                            NDRX_SYS_SVC_PFXC);
                    userlog("-R request address cannot start with [%c]",
                            NDRX_SYS_SVC_PFXC);
                    ndrx_TPset_error_fmt(TPEINVAL, "-R request address cannot start with [%c]",
                            NDRX_SYS_SVC_PFXC);
                    EXFAIL_OUT(ret);
                }
                
                NDRX_STRCPY_SAFE(rqaddress, optarg);
                break;
            case 's':
                ret=ndrx_parse_svc_arg(optarg);
                break;
            case 'S':
                ret=ndrx_parse_func_arg(optarg);
                break;
            case 'x':
                ret=ndrx_parse_xcvt_arg(optarg);
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
            case 'n':
                /* Do not advertise single service */
                if (EXSUCCEED!=ndrx_skipsvc_add(optarg))
                {
                    ndrx_TPset_error_msg(TPESYSTEM, "Malloc failed");
                    EXFAIL_OUT(ret);
                }
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
        NDRX_STRCPY_SAFE(G_server_conf.binary_name, p+1);
    }
    else
    {
        NDRX_STRCPY_SAFE(G_server_conf.binary_name, argv[0]);
    }

    /*
     * Read queue prefix (This is mandatory to have)
     */
    if (NULL==(p=getenv(CONF_NDRX_QPREFIX)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Env [%s] not set", CONF_NDRX_QPREFIX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(G_server_conf.q_prefix, p);
    }
    
    /* configure server threads... */
    if (NULL==(p=getenv(CONF_NDRX_QPREFIX)))
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Env [%s] not set", CONF_NDRX_QPREFIX);
        ret=EXFAIL;
        goto out;
    }
    else
    {
        NDRX_STRCPY_SAFE(G_server_conf.q_prefix, p);
    }
    
    if (NULL!=(p=getenv(CONF_NDRX_MINDISPATCHTHREADS)))
    {
        G_server_conf.mindispatchthreads = atoi(p);
    }
    
    if (NULL!=(p=getenv(CONF_NDRX_MAXDISPATCHTHREADS)))
    {
        G_server_conf.maxdispatchthreads = atoi(p);
    }
    
    /* check thread option.. */
    if (!_tmbuilt_with_thread_option && G_server_conf.mindispatchthreads > 1)
    {
        NDRX_LOG(log_error, "Error ! Buildserver thread option says single-threaded, "
                "but MINDISPATCHTHREADS=%d MAXDISPATCHTHREADS=%d", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads
                );
        userlog("Error ! Buildserver thread option says single-threaded, "
                "but MINDISPATCHTHREADS=%d MAXDISPATCHTHREADS=%d", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads);
        
        ndrx_TPset_error_fmt(TPEINVAL, "Error ! Buildserver thread option says single-threaded, "
                "but MINDISPATCHTHREADS=%d MAXDISPATCHTHREADS=%d", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads);
        EXFAIL_OUT(ret);
    }
    
    if (G_server_conf.mindispatchthreads <=0 )
    {
        NDRX_LOG(log_error, "Error ! MINDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.mindispatchthreads);
        userlog("Error ! MINDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.mindispatchthreads);
        ndrx_TPset_error_fmt(TPEINVAL, "Error ! MINDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.mindispatchthreads);
        EXFAIL_OUT(ret);
    }
    
    if (G_server_conf.maxdispatchthreads <=0 )
    {
        NDRX_LOG(log_error, "Error ! MAXDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.maxdispatchthreads);
        userlog("Error ! MAXDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.maxdispatchthreads);
        ndrx_TPset_error_fmt(TPEINVAL, "Error ! MAXDISPATCHTHREADS(=%d) <=0", 
                G_server_conf.maxdispatchthreads);
        EXFAIL_OUT(ret);
    }
    
    if (G_server_conf.mindispatchthreads > G_server_conf.maxdispatchthreads)
    {
        NDRX_LOG(log_error, "Error ! MINDISPATCHTHREADS(=%d) > MAXDISPATCHTHREADS(=%d)", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads
                );
        userlog("Error ! MINDISPATCHTHREADS(=%d) > MAXDISPATCHTHREADS(=%d)", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads);
        
        ndrx_TPset_error_fmt(TPEINVAL, "Error ! MINDISPATCHTHREADS(=%d) > MAXDISPATCHTHREADS(=%d)", 
                G_server_conf.mindispatchthreads,
                G_server_conf.maxdispatchthreads);
        EXFAIL_OUT(ret);
    }
    
    /* start as multi-threaded */
    if (G_server_conf.mindispatchthreads > 1)
    {
        G_server_conf.is_threaded = EXTRUE;
        EX_SPIN_INIT_V(G_server_conf.mt_lock);
    }

    G_srv_id = G_server_conf.srv_id;
    
    /* Default number of events supported by e-poll */
    G_server_conf.max_events = 1;
    
    /* format the request queue */
    if (EXEOS==rqaddress[0])
    {
        /* so name not set, lets build per binary request address... */
        snprintf(rqaddress, sizeof(rqaddress), NDRX_SVR_SVADDR_FMT, 
                G_server_conf.q_prefix, G_server_conf.binary_name, G_srv_id);
        
        ndrx_epoll_mainq_set(rqaddress);
        NDRX_STRCPY_SAFE(G_server_conf.rqaddress, rqaddress);
    }
    else
    {
        snprintf(tmp, sizeof(tmp), NDRX_SVR_RQADDR_FMT, 
                G_server_conf.q_prefix, rqaddress);
        ndrx_epoll_mainq_set(tmp);
        NDRX_STRCPY_SAFE(G_server_conf.rqaddress, tmp);
    }
    
out:
    return ret;
}

/**
 * terminate server session after fork in child process
 * as it is not valid there.
 */
exprivate void childsrvuninit(void)
{
    NDRX_LOG(log_debug, "Server un-init in forked child thread...");
    atmisrv_un_initialize(EXTRUE);
}

/**
 * Wrapper for server thread done.
 * This calls the users' tpsvrthrdone. Additionally tpterm() is called
 * to terminate the thread ATMI session (basically a client)
 */
exprivate void ndrx_call_tpsvrthrdone(void)
{
    if (NULL!=ndrx_G_tpsvrthrdone)
    {
        ndrx_G_tpsvrthrdone();
    }
    
    /* terminate the session */
    tpterm();
}


/**
 * Thread basic init. Performs initial tpinit.
 * @param argc command line argument count
 * @param argv cli arguments
 */
exprivate int ndrx_call_tpsvrthrinit(int argc, char ** argv)
{
    int ret = EXSUCCEED;
    
    /* start the session... */
    if (EXSUCCEED!=tpinit(NULL))
    {
        EXFAIL_OUT(ret);
    }
    
    G_atmi_tls->pf_tpacall_noservice_hook = &ndrx_tpacall_noservice_hook_defer;
    
    if (NULL!=ndrx_G_tpsvrthrinit 
            && ndrx_G_tpsvrthrinit(argc, argv) < 0)
    {
        EXFAIL_OUT(ret);
    }
    
    /* remove handler.. */
    G_atmi_tls->pf_tpacall_noservice_hook = NULL;
out:
    return ret;
}

/**
 * Enqueue messages for later send
 * firstly search the service.
 * We need locking so that if doing advertise / unadvertise from server
 * threads
 * @param svc service name to call
 * @param data XATMI allocated buffer
 * @param len data len
 * @param flags tpacall flags
 * @return EXSUCCEED/EXFAIL 
 */
exprivate int ndrx_tpacall_noservice_hook_defer(char *svc, char *data, long len, long flags)
{
    int ret = EXSUCCEED;
    svc_entry_fn_t *existing=NULL, eltmp;
    ndrx_tpacall_defer_t *call = NULL;
    int err;
    
    NDRX_STRCPY_SAFE(eltmp.svc_nm, svc);
    
    /* use the same advertise lock, the may not advertise, if msg is being defered */
    ndrx_sv_advertise_lock();
    
    DL_SEARCH(G_server_conf.service_raw_list, existing, &eltmp, ndrx_svc_entry_fn_cmp);
    
    /* OK add to linked list for reply... */
    if (!existing)
    {
        /* generate error as service not found */
        ndrx_TPset_error_fmt(TPENOENT, "%s: Service is not available %s by %s", 
	    __func__, svc, "server_init");
        EXFAIL_OUT(ret);
    }
    
    /* duplicate the data and add to queue */
    call = NDRX_FPMALLOC(sizeof(ndrx_tpacall_defer_t), 0);
    
    if (NULL==call)
    {
        err=errno;
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", tpstrerror(err));
        ndrx_TPset_error_fmt(TPEOS, "%s: Service is not available %s by %s", 
	    __func__, svc, "server_init");
        EXFAIL_OUT(ret);
        
    }
    
    call->flags=flags;
    call->len=len;
    NDRX_STRCPY_SAFE(call->svcnm, svc);
    
    if (NULL!=data)
    {
        char type[16+1]={EXEOS};
        char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
        long xatmi_len;
        /* call->data */
        
        xatmi_len=tptypes(data, type, subtype);
        
        if (EXFAIL==xatmi_len)
        {
            NDRX_LOG(log_error, "Failed to get data type for defered tpacall buffer");
            EXFAIL_OUT(ret);
        }
        
        call->data = tpalloc(type, subtype, xatmi_len);
        
        if (NULL==call->data)
        {
            NDRX_LOG(log_error, "Failed to alloc defered msg data buf");
            EXFAIL_OUT(ret);
        }
        
        /* copy full msg, no interpretation, for UBF we could make shorter
         * copy...
         */
        memcpy(call->data, data, xatmi_len);
        
    }
    else
    {
        call->data = NULL;
    }
    
    /* Add the MSG */
    NDRX_LOG(log_info, "Enqueue deferred tpacall svcnm=[%s] org_buf=%p "
            "buf=%p (copy) len=%ld flags=%ld",
            call->svcnm, data, call->data, call->len, call->flags);
    
    DL_APPEND(M_deferred_tpacalls, call);
    
out:
                
    if (EXSUCCEED!=ret)
    {
        /* delete any left overs... */
        if (NULL!=call)
        {
            if (NULL!=call->data)
            {
                tpfree(call->data);
            }
            
            NDRX_FPFREE(call);
        }
    }

    ndrx_sv_advertise_unlock();

    return ret;
}

/**
 * Perform the sending of tpacall messages, once services are open...
 * and we are about to poll
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_tpacall_noservice_hook_send(void)
{
    int ret=EXSUCCEED;
    ndrx_tpacall_defer_t *el, *elt;
    
    DL_FOREACH_SAFE(M_deferred_tpacalls, el, elt)
    {
        NDRX_LOG(log_info, "Performing deferred tpacall svcnm=[%s] buf=%p len=%ld flags=%ld",
            el->svcnm, el->data, el->len, el->flags);
        
        if (EXFAIL==tpacall(el->svcnm, el->data, el->len, el->flags))
        {
            NDRX_LOG(log_info, "Deferred tpacall failed (svcnm=[%s] buf=%p len=%ld flags=%ld): %s",
                el->svcnm, el->data, el->len, el->flags, tpstrerror(tperrno));
            userlog("Deferred tpacall failed (svcnm=[%s] buf=%p len=%ld flags=%ld): %s",
                el->svcnm, el->data, el->len, el->flags, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        if (NULL!=el->data)
        {
            tpfree(el->data);
        }
        
        DL_DELETE(M_deferred_tpacalls, el);
        NDRX_FPFREE(el);
    }
    
out:

    /* delete messages if for error something have left here */
    if (EXSUCCEED!=ret)
    {
        DL_FOREACH_SAFE(M_deferred_tpacalls, el, elt)
        {
            if (NULL!=el->data)
            {
                tpfree(el->data);
            }

            DL_DELETE(M_deferred_tpacalls, el);
            NDRX_FPFREE(el);
        }
    }

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
    char *env_procname;
    char *env_clopt = NULL;
    
    /* in case of argc/argv are empty, we shall attempt  */
    
    if (argc<=1 || NULL==argv)
    {
        char *p;
        char *saveptr1;
        char *tok;
        int alloc_args = 0;
        /* try to lookup env variables */
        
        /* well, for server process we need a real binary name
         * the env is just logical server process name
         * thus we have to use our macros here
         */
        env_procname = (char *)EX_PROGNAME;/* getenv(CONF_NDRX_SVPROCNAME); */
        
        p = getenv(CONF_NDRX_SVCLOPT);
        
        if (NULL==p)
        {
            NDRX_LOG(log_error, "%s: argc/argv are empty and %s/%s env vars not "
                    "present - missing server params", __func__, 
                    CONF_NDRX_SVPROCNAME, CONF_NDRX_SVCLOPT);
            userlog("%s: argc/argv are empty and %s/%s env vars not "
                    "present - missing server params", __func__, 
                    CONF_NDRX_SVPROCNAME, CONF_NDRX_SVCLOPT);
            ndrx_TPset_error_fmt(TPEINVAL, "%s: argc/argv are empty and %s/%s env vars not "
                    "present - missing server params", __func__, 
                    CONF_NDRX_SVPROCNAME, CONF_NDRX_SVCLOPT);
            EXFAIL_OUT(ret);
        }
        
        if (NULL==(env_clopt=NDRX_STRDUP(p)))
        {
            int err;
            
            NDRX_LOG(log_error, "%s: Failed to strdup: %s", __func__, 
                    strerror(err));
            userlog("%s: Failed to strdup: %s", __func__, 
                    strerror(err));
            ndrx_TPset_error_fmt(TPEOS, "%s: Failed to strdup: %s", __func__, 
                    strerror(err));
            EXFAIL_OUT(ret);
        }
        
        /* realloc some space */
        argv = NULL;
        REALLOC_CLOPT;
        
        argc=1;
        argv[0] = env_procname;
        
        tok = strtok_r(env_clopt, " \t", &saveptr1);
        while (NULL!=tok)
        {
            argc++;
            
            if (argc > alloc_args)
            {
                REALLOC_CLOPT;
            }
            
            argv[argc-1] = tok;
            
            /* Get next */
            tok = strtok_r(NULL, " \t", &saveptr1);
        }
        
    }
    
    /* do internal initialization, get configuration, request for admin q */
    if (EXSUCCEED!=ndrx_init(argc, argv))
    {
        NDRX_LOG(log_error, "ndrx_init() fail");
        userlog("ndrx_init() fail");
        EXFAIL_OUT(ret);
    }
    
    /*
     * Initialize polling subsystem
     */
    if (EXSUCCEED!=ndrx_epoll_sys_init())
    {
        NDRX_LOG(log_error, "ndrx_epoll_sys_init() fail");
        userlog("ndrx_epoll_sys_init() fail");
        EXFAIL_OUT(ret);
    }
    
    
    /*
     * Initialize services, system..
     */
    if (NULL!=ndrx_G_tpsvrinit_sys && EXSUCCEED!=ndrx_G_tpsvrinit_sys(argc, argv))
    {
        NDRX_LOG(log_error, "tpsvrinit_sys() fail");
        userlog("tpsvrinit_sys() fail");
        EXFAIL_OUT(ret);
    }
    
    /* hook up atmitls with tpacall() service not found callback */
    
    G_atmi_tls->pf_tpacall_noservice_hook=&ndrx_tpacall_noservice_hook_defer;
    
    /*
     * Initialize services
     */
    if (NULL!=G_tpsvrinit__ && EXSUCCEED!=G_tpsvrinit__(argc, argv))
    {
        NDRX_LOG(log_error, "tpsvrinit() fail");
        userlog("tpsvrinit() fail");
        EXFAIL_OUT(ret);
    }
    
    /* unset hook...  */
    G_atmi_tls->pf_tpacall_noservice_hook = NULL;
    
    
    /* initialize the library - switch main thread to server ... */
    if (EXSUCCEED!=atmisrv_initialise_atmi_library())
    {
        NDRX_LOG(log_error, "initialise_atmi_library() fail");
        userlog("initialise_atmi_library() fail");
        EXFAIL_OUT(ret);
    }
    
    /*
     * Run off thread init if any
     * We could provide noservice hook here too..
     * Needs to configure hook handler, but then somehow remove it..
     */
    if (G_server_conf.is_threaded)
    {
        NDRX_LOG(log_debug, "About to init dispatch thread pool");
        G_server_conf.dispthreads = ndrx_thpool_init(G_server_conf.mindispatchthreads, 
                &ret, ndrx_call_tpsvrthrinit, ndrx_call_tpsvrthrdone, argc, argv);
        
        if (EXSUCCEED!=ret)
        {
            NDRX_LOG(log_error, "Thread pool init failure");
            EXFAIL_OUT(ret);
        }
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
    
    if (EXSUCCEED!=ndrx_atfork(NULL, NULL, childsrvuninit))
    {
        NDRX_LOG(log_error, "Failed to add atfork hanlder!");
        userlog("Failed to add atfork hanlder!");
        EXFAIL_OUT(ret);
    }
    
    /* reply the linked list of any internal tpacall("X", TPNOREPLY)
     * so that infinite servers may start...
     */
    if (EXSUCCEED!=(ret=ndrx_tpacall_noservice_hook_send()))
    {
        NDRX_LOG(log_error, "ndrx_tpacall_noservice_hook_send() fail %d", ret);
        userlog("ndrx_tpacall_noservice_hook_send() fail %d", ret);
        goto out;
    }

    /* run process here! */
    if (EXSUCCEED!=(ret=sv_wait_for_request()))
    {
        NDRX_LOG(log_error, "sv_wait_for_request() fail %d", ret);
        userlog("sv_wait_for_request() fail %d", ret);
        goto out;
    }
    
out:

    /* finish up. */
    if (NULL!=G_tpsvrdone__)
    {
        G_tpsvrdone__();
    }

    /* if thread pool was in place, then perform de-init... */
    if (NULL!=G_server_conf.dispthreads)
    {
        ndrx_thpool_destroy(G_server_conf.dispthreads);
    }

    /*
     * un-initalize polling sub-system
     */
    ndrx_epoll_sys_uninit();
    
    atmisrv_un_initialize(EXFALSE);
    /*
     * Print error message on exit. 
     */
    if (EXSUCCEED!=ret)
    {
        printf("Error: %s\n", tpstrerror(tperrno));
    }
    
    fprintf(stderr, "Server exit: %d, id: %d\n", ret, G_srv_id);
    
    if (NULL!=env_clopt)
    {
        NDRX_FREE(env_clopt);
        
        /* all pointers comes from other variables */
        if (NULL!=argv)
        {
            NDRX_FREE(argv);
        }
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
