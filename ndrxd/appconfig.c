/**
 * @brief Configuration handling.
 *
 * @file appconfig.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <memory.h>
#include <libxml/xmlreader.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndrxd.h>
#include <exenv.h>
#include <exenvapi.h>

#include "ndebug.h"
#include "utlist.h"
#include "nstdutil.h"
#include "exsha1.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define     CHECK_PM_DEFAULT              2   /**< Check process model on every */

#define     DDRREALOAD_DEFAULT           60  /**< Number of seconds route is not reloaded */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*---------------------------Globals------------------------------------*/

/**
 * Active monitor configuration
 */
config_t *G_app_config=NULL;

/**
 * Active process model handler - linked list
 */
pm_node_t *G_process_model = NULL;

/**
 * Active Hash list by service IDs
 */
pm_node_t **G_process_model_hash = NULL;

/**
 * PID Hash table
 */
pm_pidhash_t **G_process_model_pid_hash = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Validate request address, also strip down any un-needed chars
 * @param rqaddr request address from config
 * @param section this is where error occurred
 * @return EXSUCCEED/EXFAIL
 */
exprivate int rqaddr_chk(char *rqaddr, char *section)
{
    int ret = EXSUCCEED;
    
    ndrx_str_strip(rqaddr, "\t ");
    
    if (NDRX_SYS_SVC_PFXC == rqaddr[0])
    {
        NDRX_LOG(log_error, "Request address cannot start with [%c]", 
                NDRX_SYS_SVC_PFXC);
        
        NDRXD_set_error_fmt(NDRXD_EINVPARAM, "(%s) Request address "
                "cannot start with [%c] at %s", G_sys_config.config_file_short, 
                NDRX_SYS_SVC_PFXC, section);
        
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}


/**
 * Free up config memory
 * Free up the list of environments in defaults section and in process
 * sections.
 * @param app_config
 * @param process_model
 * @param process_model_hash
 * @param process_model_pid_hash
 */
exprivate void config_free(config_t **app_config, pm_node_t **process_model,
            pm_node_t ***process_model_hash, pm_pidhash_t ***process_model_pid_hash)
{
    NDRX_LOG(log_debug, "Free up config memory...");
    
    if (NULL!=*app_config)
    {
        conf_server_node_t *elt, *tmp;

        if (NULL!=(*app_config)->monitor_config)
        {
            DL_FOREACH_SAFE((*app_config)->monitor_config,elt,tmp)
            {
                
                /* free up envs, if any */
                ndrx_ndrxconf_envs_envs_free(&elt->envlist);
                ndrx_ndrxconf_envs_grouplists_free(&elt->envgrouplist);
                
                DL_DELETE((*app_config)->monitor_config,elt);
                NDRX_FREE(elt);
            }
        }
        
        /* kill any env groups */
        ndrx_ndrxconf_envs_groups_free(&(*app_config)->envgrouphash);
        
        /* remove any ddr segments */
        ndrx_ddr_free_all(*app_config);
        
        NDRX_FREE(*app_config);

        *app_config = NULL;
    }
    
    /* Free up process model? */
    if (NULL!=*process_model)
    {
        pm_node_t *elt, *tmp;
        DL_FOREACH_SAFE(*process_model,elt,tmp)
        {
            /* Remove any stuff from PID hash! */
            delete_from_pid_hash(*process_model_pid_hash,
                pid_hash_get(*process_model_pid_hash, elt->pid));
            
            /* Remove process model by it self! */
            DL_DELETE(*process_model,elt);
            NDRX_FREE(elt);
        }
    }
    
    /* Free up the services */
    

    if (*process_model_hash)
    {
        NDRX_FREE(*process_model_hash);
        *process_model_hash=NULL;
    }

    if (*process_model_pid_hash)
    {
        NDRX_FREE(*process_model_pid_hash);
        *process_model_pid_hash=NULL;
    }
    
}

/**
 * Load live config
 * @return 
 */
expublic int load_active_config_live(void)
{
    int ret = EXSUCCEED;
    ret = load_active_config(&G_app_config, &G_process_model,
                &G_process_model_hash, &G_process_model_pid_hash);
    
    if (EXSUCCEED==ret)
    {
        /* apply DDR */
        ndrx_ddr_apply();
    }
    
    return ret;
}
/**
 * Load active configuration.
 * This should also build the main process model i.e. get ready for startup!
 * @return SUCCED/FAIL
 */
expublic int load_active_config(config_t **app_config, pm_node_t **process_model,
            pm_node_t ***process_model_hash, pm_pidhash_t ***process_model_pid_hash)
{
    int ret=EXSUCCEED;
    int cfg_ok = EXFALSE;

    if (*app_config!=NULL)
    {
        NDRX_LOG(log_debug, "Active configuration present - nothing to do");
        /* Reply to caller, that active configuration present */
        NDRXD_set_error(NDRXD_ECFGLDED);
        ret=EXFAIL;
        cfg_ok = EXTRUE;
        goto out;
    }
    else
    {
        NDRX_LOG(log_debug, "Active configuration not loaded - will load!");
        *app_config = NDRX_CALLOC(1, sizeof(config_t));
    }

    if (EXSUCCEED!=load_config(*app_config, G_sys_config.config_file))
    {
        NDRX_LOG(log_debug, "Failed to load configuration");
        /* Reply to caller that config failed to load */
        NDRXD_set_error(NDRXD_ECFGINVLD);
        ret=EXFAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "building process model");

    /* Allocate hash, OK? */
    NDRX_LOG(log_debug, "G_sys_config.max_servers = %d", 
            ndrx_get_G_atmi_env()->max_servers);
    *process_model_hash = (pm_node_t **)NDRX_CALLOC(ndrx_get_G_atmi_env()->max_servers, 
            sizeof(pm_node_t *));

    if (NULL==*process_model_hash)
    {
        NDRXD_set_error_msg(NDRXD_EOS, "Failed to allocate *process_model_hash");
        ret = EXFAIL;
        goto out;
    }

    *process_model_pid_hash = (pm_pidhash_t **)NDRX_CALLOC(ndrx_get_G_atmi_env()->max_servers, 
            sizeof(pm_pidhash_t *));

    if (NULL==*process_model_pid_hash)
    {
        NDRXD_set_error_fmt(NDRXD_EOS, "(%s) Failed to allocate *process_model_pid_hash - %d bytes",
                            G_sys_config.config_file_short,
                            ndrx_get_G_atmi_env()->max_servers * sizeof(pm_pidhash_t *) );
        ret = EXFAIL;
        goto out;
    }

    if (EXSUCCEED!=build_process_model((*app_config)->monitor_config,
                                &*process_model, /* proces model linked list */
                                *process_model_hash/* Hash table models */))
    {
        NDRXD_set_error_msg(NDRXD_EOS, "Failed to allocate *process_model_hash");
        ret = EXFAIL;
        goto out;
    }

out:

    /* Release memory, if config not loaded */
    if (EXSUCCEED!=ret && !cfg_ok)
    {
        config_free(app_config, process_model, process_model_hash, process_model_pid_hash);
    }

    NDRX_LOG(log_warn, "load_active_config returns with status %d", ret);

    return ret; /* Do not want to break the system! */
}

/**
 * Parse defaults
 * @param doc
 * @param cur
 * @return
 */
exprivate int parse_defaults(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    char *p;
    char tmp[PATH_MAX];
    int last_line=0;
    
    if (!config->ctl_had_defaults)
    {
        config->default_respawn = 1; /* we want respawn by default! */

        config->default_rssmax = EXFAIL; /** Disabled */
        config->default_vszmax = EXFAIL; /** Disabled */
     
        /* next time reuse what ever we have */
        config->ctl_had_defaults = EXTRUE;
        config->default_mindispatchthreads = 1; /**< assumed as 1 by atmisrv */
        config->default_maxdispatchthreads = 1; /**< assumed as 1 by atmisrv */
    }
    
    if (NULL!=cur)
    {
        do
        {
            if (0==strcmp((char*)cur->name, "min"))
            {
               
                p = (char *)xmlNodeGetContent(cur);
                
                /* we should be able to setup this from env variables
                 * for external scripting...
                 */
                NDRX_QENV_SUBST(tmp, p);
                config->default_min = atoi(tmp);
                NDRX_LOG(log_debug, "Got default min: [%s] - %d",
                                                  p, config->default_min);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                NDRX_QENV_SUBST(tmp, p);
                
                config->default_max = atoi(tmp);
                NDRX_LOG(log_debug, "Got default max: [%s] - %d",
                                                  p, config->default_max);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "autokill"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_autokill = atoi(p);
                NDRX_LOG(log_debug, "Got default autokill: [%s] - %d",
                                                  p, config->default_autokill);
                xmlFree(p);
            }
            /* Load Environment override */
            else if (0==strcmp("env", (char *)cur->name))
            {
                p = (char *)xmlNodeGetContent(cur);
                NDRX_STRCPY_SAFE(config->default_env, p);
                
                /* process env */
                ndrx_str_env_subs_len(config->default_env, sizeof(config->default_env));
                
                xmlFree(p);
            }
            else if (0==strcmp("envs", (char *)cur->name))
            {
                if (EXSUCCEED!=ndrx_ndrxconf_envs_group_parse(doc, cur, 
                    &config->envgrouphash))
                {
                    NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to parse <envs> tag!",
                            G_sys_config.config_file_short);
                    NDRX_LOG(log_error, "Failed to parse <envs> tag!");
                    EXFAIL_OUT(ret);
                }
            }
            /* Startup control moved here...: */
            else if (0==strcmp((char*)cur->name, "start_max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_start_max = atoi(p);
                NDRX_LOG(log_debug, "start_max: [%s] - %d sty",
                                                  p, config->default_start_max);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "end_max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_end_max = atoi(p);
                NDRX_LOG(log_debug, "end_max: [%s] - %d sty",
                                                  p, config->default_end_max);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "pingtime"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_pingtime = atoi(p);
                NDRX_LOG(log_debug, "pingtime: [%s] - %d sty",
                                                  p, config->default_pingtime);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "ping_max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_ping_max = atoi(p);
                NDRX_LOG(log_debug, "ping_max: [%s] - %d sty",
                                                  p, config->default_ping_max);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "exportsvcs"))
            {
                p = (char *)xmlNodeGetContent(cur);
                ndrx_str_strip(p, " \t"); /* strip spaces & tabs */
                if (strlen(p)>=sizeof(config->default_exportsvcs))
                {
                    NDRX_LOG(log_warn, "Trimming default exportsvcs");
                    p[sizeof(config->default_exportsvcs)-3] = EXEOS;
                }
                snprintf(config->default_exportsvcs, 
                        sizeof(config->default_exportsvcs), ",%s,", p);
                NDRX_LOG(log_debug, "exportsvcs: [%s]", 
                            config->default_exportsvcs);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "blacklistsvcs"))
            {
                p = (char *)xmlNodeGetContent(cur);
                ndrx_str_strip(p, " \t"); /* strip spaces & tabs */
                if (strlen(p)>=sizeof(config->default_blacklistsvcs))
                {
                    NDRX_LOG(log_warn, "Trimming default blacklistsvcs");
                    p[sizeof(config->default_blacklistsvcs)-3] = EXEOS;
                }
                snprintf(config->default_blacklistsvcs, 
                        sizeof(config->default_blacklistsvcs), ",%s,", p);
                NDRX_LOG(log_debug, "blacklistsvcs: [%s]", 
                            config->default_blacklistsvcs);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "killtime"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_killtime = atoi(p);
                NDRX_LOG(log_debug, "killtime: [%s] - %d sty",
                                                  p, config->default_killtime);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "srvstartwait"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_srvstartwait = atoi(p)*1000;
                NDRX_LOG(log_debug, "srvstartwait: [%s] - %d msec",
                                            p, config->default_srvstartwait);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "srvstopwait"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_srvstopwait = atoi(p)*1000;
                NDRX_LOG(log_debug, "srvstopwait: [%s] - %d msec",
                                        p, config->default_srvstopwait);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "cctag"))
            {
                p = (char *)xmlNodeGetContent(cur);
                NDRX_STRCPY_SAFE(config->default_cctag, p);
                
                /* process env */
                ndrx_str_env_subs_len(config->default_cctag, sizeof(config->default_cctag));
                
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "protected"))
            {
                p = (char *)xmlNodeGetContent(cur);
                if ('Y' == *p || 'y' == *p)
                {
                    config->default_isprotected = 1;
                }
                else
                {
                    config->default_isprotected = 0;
                }
                NDRX_LOG(log_debug, "protected: %c", config->default_isprotected?'Y':'N');
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "reloadonchange"))
            {
                p = (char *)xmlNodeGetContent(cur);
                if ('Y' == *p || 'y' == *p)
                {
                    config->default_reloadonchange = 1;
                }
                else
                {
                    config->default_reloadonchange = 0;
                }
                NDRX_LOG(log_debug, "reloadonchange: %c", config->default_reloadonchange?'Y':'N');
                xmlFree(p);
            }
	    else if (0==strcmp((char*)cur->name, "respawn"))
            {
                p = (char *)xmlNodeGetContent(cur);
                if ('N' == *p || 'n' == *p || '0' == *p)
                {
                    config->default_respawn = 0;
                }

                NDRX_LOG(log_debug, "respawn: %c", config->default_reloadonchange?'Y':'N');
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "rqaddr"))
            {
                p = (char *)xmlNodeGetContent(cur);
                NDRX_STRCPY_SAFE(config->default_rqaddr, p);
                xmlFree(p);

                /* validate the request address - it must not start with @ 
                 * also we need to strip down any tabs & spaces
                 */
                if (EXSUCCEED!=rqaddr_chk(config->default_rqaddr, "defaults section"))
                {
                    EXFAIL_OUT(ret);
                }

                NDRX_LOG(log_debug, "rqaddr: [%s]", config->default_rqaddr);
            }
            else if (0==strcmp((char*)cur->name, "rssmax"))
            {
                p = (char *)xmlNodeGetContent(cur);
                
                if (EXSUCCEED!=ndrx_storage_decode(p, &config->default_rssmax))
                {
                    NDRX_LOG(log_error, "Failed to parse `rssmax', invalid value");
                    
                    NDRXD_set_error_fmt(NDRXD_EINVPARAM, "Invalid value `rssmax' "
                            "at defaults section");
                    
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "rssmax: %ld bytes", config->default_rssmax);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "vszmax"))
            {
                p = (char *)xmlNodeGetContent(cur);
                
                if (EXSUCCEED!=ndrx_storage_decode(p, &config->default_vszmax))
                {
                    NDRX_LOG(log_error, "Failed to parse `vszmax', invalid value");
                    
                    NDRXD_set_error_fmt(NDRXD_EINVPARAM, "Invalid value `vszmax' "
                            "at defaults section");
                    
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "vszmax: %ld bytes", config->default_vszmax);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "mindispatchthreads"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_mindispatchthreads = atoi(p);
                NDRX_LOG(log_debug, "default mindispatchthreads: [%s] - %d",
                                        p, config->default_mindispatchthreads);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "maxdispatchthreads"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_maxdispatchthreads = atoi(p);
                NDRX_LOG(log_debug, "default maxdispatchthreads: [%s] - %d",
                                        p, config->default_maxdispatchthreads);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "threadstacksize"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_threadstacksize = atoi(p);
                NDRX_LOG(log_debug, "default threadstacksize: [%s] - %d",
                                        p, config->default_threadstacksize);
                xmlFree(p);
            }
            
#if 0
            else
            {
                NDRX_LOG(log_error, "Unknown element %s", cur->name);
            }
#endif
            last_line=cur->line;
            cur = cur->next;
        } while (cur);
    }
    
    if (!config->default_start_max)
    {
        NDRX_LOG(log_debug, "(%s) `start_max' not set at defaults section near line %d!", 
                G_sys_config.config_file_short, last_line);
        ret=EXFAIL;
        goto out;
    }/* - Optional param. Default 0. 
    else if (!config->default_pingtime)
    {
        NDRX_LOG(log_debug, "(ndrxconfig.xml) `pingtime' not set!");
        ret=FAIL;
        goto out;
    }
    */
    else if (config->default_pingtime && !config->default_ping_max)
    {
        NDRX_LOG(log_debug, "(%s) `pingtime' set but `ping_max' not "
                "set at default section", G_sys_config.config_file_short);
        
        NDRXD_set_error_fmt(NDRXD_ECFGDEFAULTS, "(%s)`pingtime' set but `ping_max' not "
                "set at default section near line %d", 
                G_sys_config.config_file_short, last_line);
        
        ret=EXFAIL;
        goto out;
    }
    else if (!config->default_end_max)
    {
        NDRX_LOG(log_debug, "`end_max' not set!");
        
        NDRXD_set_error_fmt(NDRXD_ECFGDEFAULTS, "(%s) `end_max' not set "
                "at default section near line %d!", 
                G_sys_config.config_file_short, 
                last_line);
        
        ret=EXFAIL;
        goto out;
    }
    
    if (!config->default_srvstartwait)
    {
        config->default_srvstartwait = DEF_SRV_STARTWAIT;
        config->default_srvstartwait*=1000;
        NDRX_LOG(log_debug, "Server start wait defaulted to %ld msec",
                    config->default_srvstartwait);
    }
    
    if (!config->default_srvstopwait)
    {
        config->default_srvstopwait = DEF_SRV_STARTWAIT;
        config->default_srvstopwait*=1000;
        NDRX_LOG(log_debug, "Server stop wait defaulted to %ld msec",
                    config->default_srvstopwait);
    }
    

out:
    return ret;
}


/**
 * Parse sysconfig section in config file
 * @param doc
 * @param cur
 * @return
 */
exprivate int parse_appconfig(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    char *p;
    int last_line=0;
    
    if (NULL!=cur)
    {
        do
        {
            if (0==strcmp((char*)cur->name, "sanity"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->sanity = atoi(p);
                NDRX_LOG(log_debug, "Sanity Check: [%s] - %d",
                                                  p, config->sanity);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "restart_min"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->restart_min = atoi(p);
                NDRX_LOG(log_debug, "restart_min: [%s] - %d",
                                                  p, config->restart_min);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "restart_step"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->restart_step = atoi(p);
                NDRX_LOG(log_debug, "restart_step: [%s] - %d",
                                                  p, config->restart_step);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "restart_max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->restart_max = atoi(p);
                NDRX_LOG(log_debug, "restart_max: [%s] - %d",
                                                  p, config->restart_max);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "brrefresh"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->brrefresh = atoi(p);
                NDRX_LOG(log_debug, "brrefresh: [%s] - %d sty",
                                                  p, config->brrefresh);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "restart_to_check"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->restart_to_check = atoi(p);
                NDRX_LOG(log_debug, "restart_to_check: [%s] - %d sec",
                                                  p, config->restart_to_check);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "checkpm"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->checkpm = atoi(p);
                NDRX_LOG(log_debug, "checkpm: [%s] - %d sty",
                                                  p, config->checkpm);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "gather_pq_stats"))
            {
                p = (char *)xmlNodeGetContent(cur);
                if ('Y' == *p || 'y' == *p)
                {
                    config->gather_pq_stats = 1;
                }
                else
                {
                    config->gather_pq_stats = 0;
                }
                NDRX_LOG(log_debug, "gather_pq_stats: %c",
                                                  config->gather_pq_stats?'Y':'N');
                xmlFree(p);
            }
            /* this is used by system v only */
            else if (0==strcmp((char*)cur->name, "rqaddrttl"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->rqaddrttl = atoi(p);
                NDRX_LOG(log_debug, "rqaddrttl: [%s] - %d sec",
                                                  p, config->rqaddrttl);
                xmlFree(p);
            }
            
            else if (0==strcmp((char*)cur->name, "ddrreload"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->ddrreload = atoi(p);
                NDRX_LOG(log_debug, "ddrreload: [%s] - %d sty",
                                                  p, config->ddrreload);
                xmlFree(p);
            }
            
            last_line=cur->line;
            cur = cur->next;
        } while (cur);
    }
    
    /* Now validate configuration - all values should have not be zero! */
    
    if (!config->sanity)
    {
        NDRX_LOG(log_debug, "`sanity' not set!");
        NDRXD_set_error_fmt(NDRXD_ECFGAPPCONFIG, "(%s) `sanity' not "
                "set at <appconfig> section near line %d!", 
                G_sys_config.config_file_short, last_line);
        ret=EXFAIL;
        goto out;
    } 
    else if (!config->restart_min)
    {
        NDRX_LOG(log_debug, "`restart_min' not set!");
        NDRXD_set_error_fmt(NDRXD_ECFGAPPCONFIG, "(%s) `restart_min' "
                "not set at <appconfig> section near line %d!", 
                G_sys_config.config_file_short, last_line);
        ret=EXFAIL;
        goto out;
    }
    /* else if (!config->restart_step) - why  ? */
    else if (config->restart_step < 0)
    {
        NDRX_LOG(log_debug, "`restart_step' invalid value %d!", config->restart_step);
        NDRXD_set_error_fmt(NDRXD_ECFGAPPCONFIG, "(%s) `restart_step' "
                "has invalid value %d in <appconfig> section near line %d!", 
                G_sys_config.config_file_short, config->restart_step, last_line);
        ret=EXFAIL;
        goto out;
    }
    else if (!config->restart_max)
    {
        NDRX_LOG(log_debug, "`restart_max' not set!");
        NDRXD_set_error_fmt(NDRXD_ECFGAPPCONFIG, "(%s) `restart_max' "
                "not set at <appconfig> section near line %d!",
                G_sys_config.config_file_short, last_line);
        ret=EXFAIL;
        goto out;
    }
    else if (!config->restart_to_check)
    {
        NDRX_LOG(log_debug, "`restart_to_check' not set!");
        NDRXD_set_error_fmt(NDRXD_ECFGAPPCONFIG, "(%s) "
                "`restart_to_check' not set at <appconfig> section near line %d!", 
                G_sys_config.config_file_short, last_line);
        ret=EXFAIL;
        goto out;
    }
    
    if (!config->brrefresh)
    {
        NDRX_LOG(log_warn, "`brrefresh' not set - "
                "period refreshes will not be sent!");
    }
    
    if (!config->checkpm)
    {
        config->checkpm = CHECK_PM_DEFAULT;
        NDRX_LOG(log_debug, "`checkpm' not set using "
                "default %d sty!", config->checkpm);
    }
    
    if (0 >= config->rqaddrttl)
    {
        config->rqaddrttl = DEF_RQADDRTTL;
        NDRX_LOG(log_debug, "`rqaddrtt' not set using "
                "default %d sty!", config->rqaddrttl);
    }
    
    if (0>= config->ddrreload)
    {
        config->ddrreload = DDRREALOAD_DEFAULT;
        NDRX_LOG(log_debug, "`routereload' not set using "
                "default %d sty!", config->ddrreload);
    }

out:
    return ret;
}

/**
 * Parser server entry
 * @param doc
 * @param cur
 * @param srvnm - name of the server
 * @return 
 */
exprivate int parse_server(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    xmlAttrPtr attr;
    char srvnm[MAXTIDENT+1]={EXEOS};
    char tmp[128];
    conf_server_node_t *p_srvnode=NULL;
    char *p;
    int last_line=EXFAIL;
    /* first of all, we need to get server name from attribs */
    
    p_srvnode = NDRX_CALLOC(1, sizeof(conf_server_node_t));
    if (NULL==p_srvnode)
    {
        NDRX_LOG(log_error, "malloc failed for srvnode!");
        NDRXD_set_error_fmt(NDRXD_EOS, "(%s) malloc failed for srvnode!", 
                G_sys_config.config_file_short);
        EXFAIL_OUT(ret);
    }
    
    p_srvnode->srvid = EXFAIL;
    p_srvnode->min = EXFAIL;
    p_srvnode->max = EXFAIL;
    p_srvnode->autokill = EXFAIL;
    p_srvnode->start_max = EXFAIL;
                    
    p_srvnode->pingtime = EXFAIL;
    p_srvnode->ping_max = EXFAIL;
    p_srvnode->end_max = EXFAIL;
    p_srvnode->killtime = EXFAIL;
    p_srvnode->exportsvcs[0] = EXEOS;
    p_srvnode->isprotected = EXFAIL;
    p_srvnode->reloadonchange = EXFAIL;
    p_srvnode->respawn = EXFAIL;
    
    p_srvnode->rssmax = config->default_rssmax;
    p_srvnode->vszmax = config->default_vszmax;
    
    p_srvnode->mindispatchthreads = config->default_mindispatchthreads;
    p_srvnode->maxdispatchthreads = config->default_maxdispatchthreads;
    p_srvnode->threadstacksize= config->default_threadstacksize;

    for (attr=cur->properties; attr; attr = attr->next)
    {
        if (0==strcmp((char *)attr->name, "name"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRCPY_SAFE(srvnm, p);
            xmlFree(p);
        }
    }

    if (EXEOS==srvnm[0])
    {
        NDRX_LOG(log_error, "No server name at line %hd", cur->line);
        ret=EXFAIL;
        goto out;
    }

    /* Now grab the all stuff for server */
    cur=cur->children;

    for (; cur; cur=cur->next)
    {
        if (0==strcmp("srvid", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->srvid = atoi(p);
            xmlFree(p);
        }
        else if (0==strcmp("min", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            
            NDRX_QENV_SUBST(tmp, p);

            p_srvnode->min = atoi(tmp);
            xmlFree(p);
        }
        else if (0==strcmp("max", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            
            NDRX_QENV_SUBST(tmp, p);
            
            p_srvnode->max = atoi(tmp);
            xmlFree(p);
        }
        else if (0==strcmp("autokill", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->autokill = atoi(p);
            xmlFree(p);
        }
        else if (0==strcmp("sleep_after", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->sleep_after = atoi(p);
            xmlFree(p);
        }
        /* Startup control moved here...: */
        else if (0==strcmp((char*)cur->name, "start_max"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->start_max = atoi(p);
            NDRX_LOG(log_debug, "start_max: [%s] - %d", p, p_srvnode->start_max);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "end_max"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->end_max = atoi(p);
            NDRX_LOG(log_debug, "end_max: [%s] - %d", p, p_srvnode->end_max);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "pingtime"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->pingtime = atoi(p);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "ping_max"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->ping_max = atoi(p);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "killtime"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->killtime = atoi(p);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "exportsvcs"))
        {
            p = (char *)xmlNodeGetContent(cur);
            ndrx_str_strip(p, " \t");
            if (strlen(p)>=sizeof(p_srvnode->exportsvcs)-3)
            {
                NDRX_LOG(log_warn, "Trimming server exportsvcs");
                p[sizeof(p_srvnode->exportsvcs)-3] = EXEOS;
            }
            snprintf(p_srvnode->exportsvcs, sizeof(p_srvnode->exportsvcs), 
                    ",%s,", p);
            NDRX_LOG(log_debug, "exportsvcs: [%s]", 
                    p_srvnode->exportsvcs);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "blacklistsvcs"))
        {
            p = (char *)xmlNodeGetContent(cur);
            ndrx_str_strip(p, " \t");
            if (strlen(p)>=sizeof(p_srvnode->blacklistsvcs)-3)
            {
                NDRX_LOG(log_warn, "blacklistsvcs server blacklistsvcs");
                p[sizeof(p_srvnode->exportsvcs)-3] = EXEOS;
            }
            snprintf(p_srvnode->blacklistsvcs, sizeof(p_srvnode->blacklistsvcs), 
                    ",%s,", p);
            NDRX_LOG(log_debug, "blacklistsvcs: [%s]", 
                    p_srvnode->blacklistsvcs);
            xmlFree(p);
        }
        else if (0==strcmp("sysopt", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->SYSOPT, p);
            xmlFree(p);
        }
        else if (0==strcmp("appopt", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->APPOPT, p);
            /* Feature #331
             * process env - do later when building model..
            ndrx_str_env_subs_len(p_srvnode->APPOPT, sizeof(p_srvnode->APPOPT));
             * */
            xmlFree(p);
        }
        else if (0==strcmp("env", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->env, p);

            /* Ensure that we terminate... */
            p_srvnode->env[sizeof(p_srvnode->env)-1] = EXEOS;

            /* process env */
            ndrx_str_env_subs_len(p_srvnode->env, sizeof(p_srvnode->env));

            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "srvstartwait"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->srvstartwait = atoi(p)*1000;
            NDRX_LOG(log_debug, "srvstartwait: [%s] - %d msec",
                                              p, p_srvnode->srvstartwait);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "srvstopwait"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->srvstopwait = atoi(p)*1000;
            NDRX_LOG(log_debug, "srvstopwait: [%s] - %d msec",
                                              p, p_srvnode->srvstopwait);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "cctag"))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->cctag, p);
            /* process env */
            ndrx_str_env_subs_len(p_srvnode->cctag, sizeof(p_srvnode->cctag));
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "protected"))
        {
            p = (char *)xmlNodeGetContent(cur);
            if ('Y' == *p || 'y' == *p)
            {
                p_srvnode->isprotected = 1;
            }
            else
            {
                p_srvnode->isprotected = 0;
            }
            NDRX_LOG(log_debug, "protected: %c",
                                              p_srvnode->isprotected?'Y':'N');
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "reloadonchange"))
        {
            p = (char *)xmlNodeGetContent(cur);
            if ('Y' == *p || 'y' == *p)
            {
                p_srvnode->reloadonchange = 1;
            }
            else
            {
                p_srvnode->reloadonchange = 0;
            }
            NDRX_LOG(log_debug, "reloadonchange: %c",
                                              p_srvnode->reloadonchange?'Y':'N');
            xmlFree(p);
        }
	else if (0==strcmp((char*)cur->name, "respawn"))
        {
            p = (char *)xmlNodeGetContent(cur);
            if ('N' == *p || 'n' == *p || '0' == *p)
            {
                p_srvnode->respawn = 0;
            }
            else
            {
                p_srvnode->respawn = 1;
            }
            NDRX_LOG(log_debug, "reloadonchange: %c",
                                              p_srvnode->reloadonchange?'Y':'N');
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "fullpath"))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->fullpath, p);
            /* process env */
            ndrx_str_env_subs_len(p_srvnode->fullpath, sizeof(p_srvnode->fullpath));
            xmlFree(p);
            
            NDRX_LOG(log_debug, "fullpath: [%s]", p_srvnode->fullpath);
        }
        else if (0==strcmp((char*)cur->name, "cmdline"))
        {
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->cmdline, p);
            xmlFree(p);
            NDRX_LOG(log_debug, "cmdline: [%s]", p_srvnode->cmdline);
        }
        else if (0==strcmp((char*)cur->name, "envs"))
        {
            
            if (EXSUCCEED!=ndrx_ndrxconf_envs_parse(doc, cur, 
                &p_srvnode->envlist, config->envgrouphash, 
                        &p_srvnode->envgrouplist))
            {
                NDRX_LOG(log_error, "Failed to load environment variables for server [%d]", 
                        p_srvnode->srvid);
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to load <envs> tag "
                        "for server srvid=%d",
                        p_srvnode->srvid);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp((char*)cur->name, "rqaddr"))
        {
            char tmpbuf[64];
            p = (char *)xmlNodeGetContent(cur);
            NDRX_STRCPY_SAFE(p_srvnode->rqaddr, p);
            xmlFree(p);
            
            /* validate the request address - it must not start with @ 
             * also we need to strip down any tabs & spaces
             */
            snprintf(tmpbuf, sizeof(tmpbuf), "srvid=%d near line %d", 
                    p_srvnode->srvid, (int)cur->line);
            
            if (EXSUCCEED!=rqaddr_chk(p_srvnode->rqaddr, tmpbuf))
            {
                EXFAIL_OUT(ret);
            }
            
            NDRX_LOG(log_debug, "rqaddr: [%s]", p_srvnode->rqaddr);
        }
        else if (0==strcmp((char*)cur->name, "rssmax"))
        {
            p = (char *)xmlNodeGetContent(cur);

            if (EXSUCCEED!=ndrx_storage_decode(p, &p_srvnode->rssmax))
            {
                NDRX_LOG(log_error, "Failed to parse `rssmax', invalid value");
                
                NDRXD_set_error_fmt(NDRXD_EINVPARAM, "(%s) Invalid value `rssmax' "
                            "at srvid=%d near lines %d", 
                        G_sys_config.config_file_short, p_srvnode->srvid,
                        (int)cur->line);
                
                EXFAIL_OUT(ret);
            }

            NDRX_LOG(log_debug, "rssmax: %ld bytes", p_srvnode->rssmax);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "vszmax"))
        {
            p = (char *)xmlNodeGetContent(cur);

            if (EXSUCCEED!=ndrx_storage_decode(p, &p_srvnode->vszmax))
            {
                NDRX_LOG(log_error, "Failed to parse `vszmax', invalid value");
                
                NDRXD_set_error_fmt(NDRXD_EINVPARAM, "(%s) Invalid value `vszmax' "
                            "at srvid=%d near line %d", 
                            G_sys_config.config_file_short, p_srvnode->srvid,
                            (int)cur->line);
                
                EXFAIL_OUT(ret);
            }

            NDRX_LOG(log_debug, "vszmax: %ld bytes", p_srvnode->vszmax);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "mindispatchthreads"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->mindispatchthreads = atoi(p);
            NDRX_LOG(log_debug, "mindispatchthreads: [%s] - %d",
                                    p, p_srvnode->mindispatchthreads);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "maxdispatchthreads"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->maxdispatchthreads = atoi(p);
            NDRX_LOG(log_debug, "maxdispatchthreads: [%s] - %d",
                                    p, p_srvnode->maxdispatchthreads);
            xmlFree(p);
        }
        else if (0==strcmp((char*)cur->name, "threadstacksize"))
        {
            p = (char *)xmlNodeGetContent(cur);
            p_srvnode->threadstacksize = atoi(p);
            NDRX_LOG(log_debug, "threadstacksize: [%s] - %d",
                                    p, p_srvnode->threadstacksize);
            xmlFree(p);
        }
        
        last_line = cur->line;
    }
    
    /* get rqaddr defaults */
    
    if (EXEOS==p_srvnode->rqaddr[0])
    {
        NDRX_STRCPY_SAFE(p_srvnode->rqaddr, config->default_rqaddr);
    }
    
    if (EXEOS!=p_srvnode->rqaddr[0])
    {
        snprintf(p_srvnode->clopt, sizeof(p_srvnode->clopt),
                "%s -R %s -- %s", p_srvnode->SYSOPT, 
                p_srvnode->rqaddr, p_srvnode->APPOPT);
    }
    else
    {
        snprintf(p_srvnode->clopt, sizeof(p_srvnode->clopt),
                "%s -- %s", p_srvnode->SYSOPT, p_srvnode->APPOPT);
    }
    
    NDRX_STRCPY_SAFE(p_srvnode->binary_name, srvnm);
    
    if (EXFAIL==p_srvnode->max)
        p_srvnode->max=config->default_max;

    if (EXFAIL==p_srvnode->min)
        p_srvnode->min=config->default_min;
    
    if (EXFAIL==p_srvnode->autokill)
        p_srvnode->autokill=config->default_autokill;
    
    /* Process control params: */
    if (EXFAIL==p_srvnode->start_max)
        p_srvnode->start_max=config->default_start_max;
    
    if (EXFAIL==p_srvnode->end_max)
        p_srvnode->end_max=config->default_end_max;
    
    if (EXFAIL==p_srvnode->pingtime)
        p_srvnode->pingtime=config->default_pingtime;
    
    if (EXFAIL==p_srvnode->ping_max)
        p_srvnode->ping_max=config->default_ping_max;
    
    /* Copy default env override */
    if (EXEOS==p_srvnode->env[0] && EXEOS!=config->default_env[0])
        NDRX_STRCPY_SAFE(p_srvnode->env, config->default_env);
    
    if (EXFAIL==p_srvnode->killtime)
        p_srvnode->killtime=config->default_killtime;
    
    /* it could be empty for defaults too - then no problem.  */
    if (EXEOS==p_srvnode->exportsvcs[0])
        NDRX_STRCPY_SAFE(p_srvnode->exportsvcs, config->default_exportsvcs);
    
    if (EXEOS==p_srvnode->blacklistsvcs[0])
        NDRX_STRCPY_SAFE(p_srvnode->blacklistsvcs, config->default_blacklistsvcs);
    
    if (!p_srvnode->srvstartwait)
        p_srvnode->srvstartwait=config->default_srvstartwait;
    
    if (!p_srvnode->srvstopwait)
        p_srvnode->srvstopwait=config->default_srvstopwait;
    
    if (EXEOS==p_srvnode->cctag[0])
        NDRX_STRCPY_SAFE(p_srvnode->cctag, config->default_cctag);
    
    if (EXFAIL==p_srvnode->isprotected)
        p_srvnode->isprotected = config->default_isprotected;
    
    if (EXFAIL==p_srvnode->reloadonchange)
        p_srvnode->reloadonchange = config->default_reloadonchange;
    
    if (EXFAIL==p_srvnode->respawn)
        p_srvnode->respawn = config->default_respawn;
    
    if (p_srvnode->ping_max && !p_srvnode->ping_max)
    {
        NDRX_LOG(log_error, "`ping_max' not set for server! srvid=%hd", 
                p_srvnode->srvid);
        
        NDRXD_set_error_fmt(NDRXD_ECFGSERVER, "(%s) `ping_max' not set for server! "
                "srvid=%hd near %d line", G_sys_config.config_file_short, 
                p_srvnode->srvid, last_line);
        
        ret=EXFAIL;
        goto out;
    }
    
    /* check the config.. */
    if (p_srvnode->maxdispatchthreads< p_srvnode->mindispatchthreads)
    {
        NDRX_LOG(log_error, "maxdispatchthreads (%d) < mindispatchthreads (%d) "
                "srvid=%hd near line %d", 
                p_srvnode->maxdispatchthreads, p_srvnode->mindispatchthreads, 
                p_srvnode->srvid, last_line);
        
        NDRXD_set_error_fmt(NDRXD_ECFGSERVER, "(%s) maxdispatchthreads "
                "(%d) < mindispatchthreads (%d) "
                "srvid=%hd near line %d", 
                G_sys_config.config_file_short,
                p_srvnode->maxdispatchthreads, p_srvnode->mindispatchthreads, 
                p_srvnode->srvid, last_line);
        
        ret=EXFAIL;
        goto out;
    }
    
    if (EXFAIL==p_srvnode->srvid)
    {
        NDRX_LOG(log_error, "No <srvid> near of line %d", last_line);
        NDRXD_set_error_fmt(NDRXD_ECFGSERVER, "(%s) No <srvid> for "
                "server block near of line %d", G_sys_config.config_file_short,
                last_line);
        ret=EXFAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "Adding: %s SRVID=%d MIN=%d MAX=%d "
            "CLOPT=\"%s\" ENV=\"%s\" START_MAX=%d END_MAX=%d PINGTIME=%d PING_MAX=%d "
            "EXPORTSVCS=\"%s\" START_WAIT=%d STOP_WAIT=%d CCTAG=\"%s\" RELOADONCHANGE=\"%c\""
	    "RESPAWN=\"%c\" FULLPATH=\"%s\" CMDLINE=\"%s\" RSSMAX=%ld VSZMAX=%ld "
            "MINDISPATCHTHREADS=%d MAXDISPATCHTHREADS=%d THREADSTACKSIZE=%d",
                    p_srvnode->binary_name, p_srvnode->srvid, p_srvnode->min,
                    p_srvnode->max, p_srvnode->clopt, p_srvnode->env,
                    p_srvnode->start_max, p_srvnode->end_max, p_srvnode->pingtime, 
                    p_srvnode->ping_max,
                    p_srvnode->exportsvcs,
                    p_srvnode->srvstartwait,
                    p_srvnode->srvstopwait,
                    p_srvnode->cctag,
                    p_srvnode->reloadonchange?'Y':'N',
		    p_srvnode->respawn?'Y':'N',
                    p_srvnode->fullpath,
                    p_srvnode->cmdline,
                    p_srvnode->rssmax,
                    p_srvnode->vszmax,
                    p_srvnode->mindispatchthreads,
                    p_srvnode->maxdispatchthreads,
                    p_srvnode->threadstacksize
                    );
    DL_APPEND(config->monitor_config, p_srvnode);

out:
    if (EXFAIL==ret && p_srvnode)
    {
        NDRX_FREE(p_srvnode);
    }

    return ret;
}
/**
 * parse server entries
 * @param doc
 * @param cur
 * @return
 */
exprivate int parse_servers(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    char *p;
    
    for (; cur ; cur=cur->next)
    {
        if (0==strcmp((char*)cur->name, "server"))
        {
            /* Get the server name */
            if (EXSUCCEED!=parse_server(config, doc, cur))
            {
                NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                        "parse <server> section", G_sys_config.config_file_short);
                ret=EXFAIL;
                goto out;
            }
        }
    }
out:
    return ret;
}

/**
 * Parse config out...
 * @param doc
 * @return
 */
exprivate int parse_config(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    int appconfig_found=EXFALSE;

    if (NULL==cur)
    {
        NDRX_LOG(log_error, "Empty config?");
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Emtpy config?",
                G_sys_config.config_file_short);
        ret=EXFAIL;
        goto out;
    }

    config->ctl_had_defaults = EXFALSE;
    /* Loop over root elements */
    do
    {
        
        if (0==strcmp((char*)cur->name, "defaults")
                && EXSUCCEED!=parse_defaults(config, doc, cur->children))
        {
            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                    "parse <defaults>", G_sys_config.config_file_short);
            ret=EXFAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "servers")
                && EXSUCCEED!=parse_servers(config, doc, cur->children))
        {
            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                    "parse <servers>", G_sys_config.config_file_short);
            ret=EXFAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "services")
                && EXSUCCEED!=ndrx_services_parse(config, doc, cur->children))
        {
            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                    "parse <services>", G_sys_config.config_file_short);
            ret=EXFAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "routing")
                && EXSUCCEED!=ndrx_routing_parse(config, doc, cur->children))
        {
            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to "
                    "parse <routing>", G_sys_config.config_file_short);
            ret=EXFAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "appconfig")
                && (appconfig_found=EXTRUE)
                && EXSUCCEED!=parse_appconfig(config, doc, cur->children))
        {
            NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to parse <appconfig>",
                    G_sys_config.config_file_short);
            ret=EXFAIL;
            goto out;
        }
#if 0
        else
        {
            NDRX_LOG(log_error, "Unknown element %s!");
        }
#endif
        cur=cur->next;
    } while (cur);

    /* Check for appconfig presence */
    if (!appconfig_found)
    {
        NDRX_LOG(log_error, "<appconfig> section not found in config!");
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) <appconfig> "
                "section not found in config!", G_sys_config.config_file_short);
        ret=EXFAIL;
        goto out;
    }
    
    /* gen routing blocks */
    
    if (EXSUCCEED!=ndrx_ddr_gen_blocks(config))
    {
        NDRX_LOG(log_error, "Failed to generate routing blocks");
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Failed to generate routing blocks",
                G_sys_config.config_file_short);
        ret=EXFAIL;
        goto out;
    }

out:
    return ret;
}

/**
 * Set error from xml data
 * @param userData
 * @param error
 */
void ndrx_xmlStructuredErrorFunc(void *userData, xmlErrorPtr error)
{
    char tmp[512];
    int len;

    NDRX_STRCPY_SAFE(tmp, error->message);

    ndrx_str_rstrip(tmp, "\r\n");

    if (NULL==error->file)
    {
        NDRXD_set_error_fmt(NDRXD_EACCES, "(%s) Failed to open config file: %s", 
            G_sys_config.config_file_short, tmp);
    }
    else
    {
        NDRXD_set_error_fmt(NDRXD_ESYNTAX, "(%s) Parsing XML failed, near line %d: %s", 
            G_sys_config.config_file_short, error->line, tmp);
    }
    
    NDRX_LOG(log_error, "Parsing XML %s failed on line %d: %s", 
            error->file?error->file:"N/A", error->line, tmp);
}

/**
 * Parses & Loads the platform configuration file.
 *
 * This initially loads the configuration int internal represtation of the
 * configuration file. After that from this info we will build work structures.
 */
expublic int load_config(config_t *config, char *config_file)
{
    int ret=EXSUCCEED;
    xmlDocPtr doc;
    xmlNodePtr root;
#if 0
    reader = xmlNewTextReaderFilename(config_file);

    if(!reader) {
        NDRX_LOG(log_error, "Failed to open [%s] with error: %s",
                                        config_file, strerror(errno));
        ret=EXFAIL;
        goto out;
    }
#endif
    
    xmlSetStructuredErrorFunc	(NULL, ndrx_xmlStructuredErrorFunc);
    
    doc = xmlReadFile(config_file, NULL, XML_PARSE_NOENT);

    if (!doc)
    {
        NDRX_LOG(log_error, "Failed to open or parse %s", config_file);
        NDRXD_set_error_fmt(NDRXD_EACCES, "(%s) Failed to open or parse XML", 
                G_sys_config.config_file_short);
        ret=EXFAIL;
        goto out;
    }

    /* Get root element */
    if (!(root = xmlDocGetRootElement(doc)))
    {
        NDRX_LOG(log_error, "Failed to get root XML element");
        NDRXD_set_error_fmt(NDRXD_ECFGINVLD, "(%s) Invalid XML config: "
                "Failed to get root XML element", G_sys_config.config_file_short);
        ret=EXFAIL;
        goto out;
    }

    /* Step into first children */
    ret=parse_config(config, doc, root->children);
    
out:

    if (NULL!=doc)
    {
        /*free the document */
        xmlFreeDoc(doc);
        /*
         *Free the global variables that may
         *have been allocated by the parser.
         */
        xmlCleanupParser();
    }

    return ret;
}


/**
 * This function does test the config. If active config is loaded, then process
 * model is being checked. If something removed from active config or collides with running
 * processes, then error is returned.
 * So basic rules is:
 * - If config does not changes, then process can be in any state
 * - If config changes, then:
 * --If new process is added, and srvid is free, then no prob
 * --If new process is added, but srvid is taken, then
 * ---Check if taken process is shutdown. If not shutdown, then raise error
 * --If in processess model there is srvid which is not present in new config, then
 * ---It will be removed from pmodel, but it must be shutdown, otherwise it is error.
 * @return
 */
expublic int test_config(int reload, command_call_t * call, 
        void (*p_reload_error)(command_call_t * call, int srvid, 
        char *old_bin, char *new_bin, int error, char *msg))
{
    int ret=EXSUCCEED;
    int new_error=EXFALSE;
    int old_error=EXFALSE;
    int do_free = EXFALSE;
    /*
     * Active monitor configuration
     */
    config_t *t_app_config=NULL;
    config_t *old_app_config=NULL;

    /*
     * Active process model handler - linked list
     */
    pm_node_t *t_process_model = NULL;
    pm_node_t *old_process_model = NULL;

    /*
     * Active Hash list by service IDs
     */
    pm_node_t **t_process_model_hash = NULL;
    pm_node_t **old_process_model_hash = NULL;

    /*
     * PID Hash table
     */
    pm_pidhash_t **t_process_model_pid_hash = NULL;
    pm_pidhash_t **old_process_model_pid_hash = NULL;


    pm_node_t *old, *new;

    if (EXSUCCEED!=load_active_config(&t_app_config, &t_process_model,
                &t_process_model_hash, &t_process_model_pid_hash))
    {
        NDRX_LOG(log_error, "Failed to load new configuration & build pmodel!");
        
        /* p_reload_error -> try to load config */
        if (NULL!=p_reload_error)
        {
            /* return config error */
            p_reload_error(call, 0, "", "", ndrxd_errno, 
                    ndrxd_strerror(ndrxd_errno));
        }
        
        ret=EXFAIL;
        goto out;
    }
    /* If not active config present, then we have nothing to do */
    if (NULL==G_app_config)
    {
        NDRX_LOG(log_debug, "Active config not loaded, nothing to do.");
        do_free=EXTRUE;
        /* free up the config otherwise we get leak.. */
        goto out;
    }
    
    /* I think we need two loops:
     * 1. loop
     * If first we go over all new configs PM, and check Old srv hash.
     * If server names & id the same, then OK
     * If server names differ for id, then old must be shut down
     * If server name not taken, then all, this is new binary
     * --------------------------
     * 2. loop
     * We go over the old PM, and check new hash.
     * If server names & id not the same, then current must be shutdown
     * If server id not present in hash, then this must be shutdown
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * We will loop over all details, so that in log we could lookup for full
     * error details.
     */
    DL_FOREACH(t_process_model, new)
    {
        /* check boundary! */
        old = G_process_model_hash[new->srvid];
        if (NULL==old)
        {
            NDRX_LOG(log_debug, "New server id=%d", new->srvid);
        }
        else if (0!=strcmp(old->binary_name, new->binary_name))
        {
            if (NDRXD_PM_EXIT!=old->state)

            {
                NDRX_LOG(log_error, "Renamed binary [%s] for "
                        "serverid=%d is in non shutdown state (%d)! New binary for this id is [%s]",
                        old->binary_name, new->srvid, old->state, new->binary_name);

                /* Give some feedback to console */
                if (NULL!=p_reload_error)
                    p_reload_error(call, new->srvid, old->binary_name,
                                new->binary_name, NDRXD_EREBBINARYRUN, "");
                
                if (!new_error)
                {
                    /* This is error case */
                    NDRXD_set_error_fmt(NDRXD_EREBBINARYRUN, "(%s) Renamed binary [%s] for "
                            "serverid=%d is in non shutdown state (%d)! New binary for this id is [%s]",
                            G_sys_config.config_file_short, 
                            old->binary_name, new->srvid, old->state, new->binary_name);
                }
                new_error=EXTRUE;
            }
            else
            {
                NDRX_LOG(log_debug, "Server id=%d renamed (from [%s] "
                                "to [%s]) and binary is shutdown",
                                new->srvid, old->binary_name, new->binary_name);
            }
        }
        else
        {
            NDRX_LOG(log_debug, "Binary name [%s] not "
                            "changed for server id=%d",
                            old->binary_name, new->srvid);
        }
    }
    /*
     * In this case we just test, that there are no binaries which are not found in
     * old config. If so then they must be shutdown.
     */
    DL_FOREACH(G_process_model, old)
    {
        /* check boundary! */
        new = t_process_model_hash[old->srvid];
        if (NULL==new)
        {
            if (!(PM_NOT_RUNNING(old->state)))
            {
                NDRX_LOG(log_error,"Removed binary [%s] for "
                        "serverid=%d is in non shutdown state (%d)!",
                        old->binary_name, old->srvid, old->state);

                /* Give some feedback to console */
                if (NULL!=p_reload_error)
                    p_reload_error(call, old->srvid, old->binary_name,
                                NULL, NDRXD_EBINARYRUN, "");

                if (!old_error)
                {
                    /* This is error case */
                    NDRXD_set_error_fmt(NDRXD_EBINARYRUN, "(%s) Removed "
                            "binary [%s] for srvid=%d is in non shutdown state (%d)!",
                            G_sys_config.config_file_short, 
                            old->binary_name, old->srvid, old->state);
                }
                old_error=EXTRUE;
            }
            else
            {
                NDRX_LOG(log_debug,"Binary [%s] for "
                        "serverid=%d is removed and is shutdown!",
                        old->binary_name, old->srvid);
            }
        }
    }

    if (old_error || new_error)
    {
        ret=EXFAIL;
        goto out;
    }

    if (reload)
    {
        /* If all ok, then we are about to replace config, we just need to merge pids */
        DL_FOREACH(t_process_model, new)
        {
            /* check boundary! */
            old = G_process_model_hash[new->srvid];
            if (NULL!=old && 0==strcmp(new->binary_name, old->binary_name))
            {
                NDRX_LOG(log_debug, "Saving pid %d in new config", old->pid);
                new->pid = old->pid;
                new->svpid = old->svpid;
                NDRX_STRCPY_SAFE(new->binary_name_real, old->binary_name_real);
                NDRX_STRCPY_SAFE(new->rqaddress, old->rqaddress);
                new->state = old->state;
                /* Link existing service info to new PM! */
                new->svcs = old->svcs;
                new->reqstate = old->reqstate;
                new->num_term_sigs = old->num_term_sigs;
                new->last_sig = old->last_sig;
                new->rspstwatch = old->rspstwatch;
                new->pingstwatch = old->pingstwatch;
                new->killreq = old->killreq;
                /* Flags were missing after reload! */
                new->flags = old->flags;
                new->pingtimer = old->pingtimer;
                new->pingseq = old->pingseq;
                new->pingroundtrip = old->pingroundtrip;
                
                /* So that we do not unlink the list later when old pm is freed */
                old->svcs = NULL;
                /* Add stuff to new pidhash! */
                if (EXSUCCEED!=add_to_pid_hash(t_process_model_pid_hash, new))
                {
                    NDRX_LOG(log_error, "Failed to register "
                                                "process in new pidhash!");
                    ret=EXFAIL;
                    goto out;
                }
            }
        }

        /* save current config */
        old_app_config=G_app_config;
        old_process_model=G_process_model;
        old_process_model_hash=G_process_model_hash;
        old_process_model_pid_hash=G_process_model_pid_hash;
        /* Update master config */
        G_app_config=t_app_config;
        G_process_model = t_process_model;
        G_process_model_hash = t_process_model_hash;
        G_process_model_pid_hash = t_process_model_pid_hash;
        
        ndrx_ddr_apply();

        /* so that in case of error we do not destroy already master config. */
        t_app_config = NULL;
        t_process_model = NULL;
        t_process_model_hash = NULL;
        t_process_model_pid_hash = NULL;

        /* free up old config */
        config_free(&old_app_config, &old_process_model, &old_process_model_hash, 
                &old_process_model_pid_hash);

        NDRX_LOG(log_debug, "Configuration successfully reloaded!");
    }
    else
    {
        /* free up test config */
        config_free(&t_app_config, &t_process_model, &t_process_model_hash, 
                &t_process_model_pid_hash);
    }

out:

    if (EXSUCCEED!=ret || do_free)
    {
        config_free(&t_app_config, &t_process_model, &t_process_model_hash, 
                &t_process_model_pid_hash);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
