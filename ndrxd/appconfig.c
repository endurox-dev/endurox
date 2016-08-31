/* 
** Configuration handling.
**
** @file appconfig.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <libxml/xmlreader.h>
#include <errno.h>

#include <ndrstandard.h>
#include <ndrxd.h>

#include "ndebug.h"
#include "utlist.h"
#include "nstdutil.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define     CHECK_PM_DEFAULT              2   /* Check process model on every */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*---------------------------Globals------------------------------------*/

/*
 * Active monitor configuration
 */
config_t *G_app_config=NULL;

/*
 * Active process model handler - linked list
 */
pm_node_t *G_process_model = NULL;

/*
 * Active Hash list by service IDs
 */
pm_node_t **G_process_model_hash = NULL;

/*
 * PID Hash table
 */
pm_pidhash_t **G_process_model_pid_hash = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Free up config memory
 * @param app_config
 * @param process_model
 * @param process_model_hash
 * @param process_model_pid_hash
 */
private void config_free(config_t **app_config, pm_node_t **process_model,
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
                DL_DELETE((*app_config)->monitor_config,elt);
                free(elt);
            }
        }
        free(*app_config);

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
            free(elt);
        }
    }

    if (*process_model_hash)
    {
        free(*process_model_hash);
        *process_model_hash=NULL;
    }

    if (*process_model_pid_hash)
    {
        free(*process_model_pid_hash);
        *process_model_pid_hash=NULL;
    }
}
/**
 * Load active configuration.
 * This should also build the main process model i.e. get ready for startup!
 * @return SUCCED/FAIL
 */
public int load_active_config(config_t **app_config, pm_node_t **process_model,
            pm_node_t ***process_model_hash, pm_pidhash_t ***process_model_pid_hash)
{
    int ret=SUCCEED;
    int cfg_ok = FALSE;

    if (*app_config!=NULL)
    {
        NDRX_LOG(log_debug, "Active configuration present - nothing to do");
        /* Reply to caller, that active configuration present */
        NDRXD_set_error(NDRXD_ECFGLDED);
        ret=FAIL;
        cfg_ok = TRUE;
        goto out;
    }
    else
    {
        NDRX_LOG(log_debug, "Active configuration not loaded - will load!");
        *app_config = calloc(1, sizeof(config_t));
    }

    if (SUCCEED!=load_config(*app_config, G_sys_config.config_file))
    {
        NDRX_LOG(log_debug, "Failed to load configuration");
        /* Reply to caller that config failed to load */
        NDRXD_set_error(NDRXD_ECFGINVLD);
        ret=FAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "building process model");

    /* Allocate hash, OK? */
    NDRX_LOG(log_debug, "G_sys_config.max_servers = %d", ndrx_get_G_atmi_env()->max_servers);
    *process_model_hash = (pm_node_t **)calloc(ndrx_get_G_atmi_env()->max_servers, sizeof(pm_node_t *));

    if (NULL==*process_model_hash)
    {
        NDRXD_set_error_msg(NDRXD_EOS, "Failed to allocate *process_model_hash");
        ret = FAIL;
        goto out;
    }

    *process_model_pid_hash = (pm_pidhash_t **)calloc(ndrx_get_G_atmi_env()->max_servers, sizeof(pm_pidhash_t *));

    if (NULL==*process_model_pid_hash)
    {
        NDRXD_set_error_fmt(NDRXD_EOS, "Failed to allocate *process_model_pid_hash - %d bytes",
                                ndrx_get_G_atmi_env()->max_servers * sizeof(pm_pidhash_t *) );
        ret = FAIL;
        goto out;
    }

    if (SUCCEED!=build_process_model((*app_config)->monitor_config,
                                &*process_model, /* proces model linked list */
                                *process_model_hash/* Hash table models */))
    {
        NDRXD_set_error_msg(NDRXD_EOS, "Failed to allocate *process_model_hash");
        ret = FAIL;
        goto out;
    }

out:

    /* Release memory, if config not loaded */
    if (SUCCEED!=ret && !cfg_ok)
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
private int parse_defaults(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    char *p;
    
    if (NULL!=cur)
    {
        do
        {
            if (0==strcmp((char*)cur->name, "min"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_min = atoi(p);
                NDRX_LOG(log_debug, "Got default min: [%s] - %d",
                                                  p, config->default_min);
                xmlFree(p);
            }
            else if (0==strcmp((char*)cur->name, "max"))
            {
                p = (char *)xmlNodeGetContent(cur);
                config->default_max = atoi(p);
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
                strncpy(config->default_env, p, sizeof(config->default_env)-1);
                
                /* Ensure that we terminate... */
                config->default_env[sizeof(config->default_env)-1] = EOS;
                
                /* process env */
                ndrx_str_env_subs(config->default_env);
                
                xmlFree(p);
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
                    p[sizeof(config->default_exportsvcs)-3] = EOS;
                }
                sprintf(config->default_exportsvcs, ",%s,", p);
                NDRX_LOG(log_debug, "exportsvcs: [%s]", 
                            config->default_exportsvcs);
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
                strncpy(config->default_cctag, p, sizeof(config->default_cctag)-1);
                
                /* Ensure that we terminate... */
                config->default_cctag[sizeof(config->default_cctag)-1] = EOS;
                
                /* process env */
                ndrx_str_env_subs_len(config->default_cctag, sizeof(config->default_cctag));
                
                xmlFree(p);
            }
            
#if 0
            else
            {
                NDRX_LOG(log_error, "Unknown element %s", cur->name);
            }
#endif
            cur = cur->next;
        } while (cur);
    }
    
    if (!config->default_start_max)
    {
        NDRX_LOG(log_debug, "appconfig: `start_max' not set!");
        ret=FAIL;
        goto out;
    }/* - Optional param. Default 0. 
    else if (!config->default_pingtime)
    {
        NDRX_LOG(log_debug, "appconfig: `pingtime' not set!");
        ret=FAIL;
        goto out;
    }
    */
    else if (config->default_pingtime && !config->default_ping_max)
    {
        NDRX_LOG(log_debug, "appconfig: `ping_max' not set!");
        ret=FAIL;
        goto out;
    }
    else if (!config->default_end_max)
    {
        NDRX_LOG(log_debug, "appconfig: `end_max' not set!");
        ret=FAIL;
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
private int parse_appconfig(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    char *p;
    
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
            
            cur = cur->next;
        } while (cur);
    }
    
    /* Now validate configuration - all values should have not be zero! */
    
    if (!config->sanity)
    {
        NDRX_LOG(log_debug, "appconfig: `sanity' not set!");
        ret=FAIL;
        goto out;
    } 
    else if (!config->restart_min)
    {
        NDRX_LOG(log_debug, "appconfig: `restart_min' not set!");
        ret=FAIL;
        goto out;
    }
    else if (!config->restart_step)
    {
        NDRX_LOG(log_debug, "appconfig: `restart_step' not set!");
        ret=FAIL;
        goto out;
    }
    else if (!config->restart_max)
    {
        NDRX_LOG(log_debug, "appconfig: `restart_max' not set!");
        ret=FAIL;
        goto out;
    }
    else if (!config->restart_to_check)
    {
        NDRX_LOG(log_debug, "appconfig: `restart_to_check' not set!");
        ret=FAIL;
        goto out;
    }
    
    if (!config->brrefresh)
    {
        NDRX_LOG(log_warn, "appconfig: `brrefresh' not set - "
                "period refreshes will not be sent!");
    }
    
    if (!config->checkpm)
    {
        config->checkpm = CHECK_PM_DEFAULT;
        NDRX_LOG(log_debug, "appconfig: `checkpm' not set using "
                "default %d sty!", config->checkpm);
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
private int parse_server(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    xmlAttrPtr attr;
    char srvnm[MAXTIDENT+1]={EOS};
    char tmp[128];
#if 0
    
    int srvid = FAIL;
#endif
    conf_server_node_t *p_srvnode=NULL;
    char *p;
    /* first of all, we need to get server name from attribs */
    
    p_srvnode = malloc(sizeof(conf_server_node_t));
    if (NULL==p_srvnode)
    {
        NDRX_LOG(log_error, "malloc failed for srvnode!");
        ret=FAIL;
        goto out;
    }
    memset(p_srvnode, 0, sizeof(conf_server_node_t));
    p_srvnode->srvid = FAIL;
    p_srvnode->min = FAIL;
    p_srvnode->max = FAIL;
    p_srvnode->autokill = FAIL;
    p_srvnode->start_max = FAIL;
                    
    p_srvnode->pingtime = FAIL;
    p_srvnode->ping_max = FAIL;
    p_srvnode->end_max = FAIL;
    p_srvnode->killtime = FAIL;
    p_srvnode->exportsvcs[0] = EOS;


    for (attr=cur->properties; attr; attr = attr->next)
    {
        if (0==strcmp((char *)attr->name, "name"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            strncpy(srvnm, p, MAXTIDENT);
            srvnm[MAXTIDENT] = EOS;
            xmlFree(p);
        }
    }
    /**/
    if (EOS==srvnm[0])
    {
        NDRX_LOG(log_error, "No server name at line %hd", cur->line);
        ret=FAIL;
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
            
            strcpy(tmp, p);
            ndrx_str_env_subs(tmp);

            p_srvnode->min = atoi(tmp);
            xmlFree(p);
        }
        else if (0==strcmp("max", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            
            strcpy(tmp, p);
            ndrx_str_env_subs(tmp);
            
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
                p[sizeof(p_srvnode->exportsvcs)-3] = EOS;
            }
            sprintf(p_srvnode->exportsvcs, ",%s,", p);
            NDRX_LOG(log_debug, "exportsvcs: [%s]", 
                    p_srvnode->exportsvcs);
            xmlFree(p);
        }
        else if (0==strcmp("sysopt", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            strncpy(p_srvnode->SYSOPT, p, sizeof(p_srvnode->SYSOPT)-1);
            /* process env */
            ndrx_str_env_subs(p_srvnode->SYSOPT);
            /* Ensure that we terminate... */
            p_srvnode->SYSOPT[sizeof(p_srvnode->SYSOPT)-1] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp("appopt", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            strncpy(p_srvnode->APPOPT, p, sizeof(p_srvnode->APPOPT)-1);
            /* process env */
            ndrx_str_env_subs(p_srvnode->SYSOPT);
            /* Ensure that we terminate... */
            p_srvnode->APPOPT[sizeof(p_srvnode->APPOPT)-1] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp("env", (char *)cur->name))
        {
            p = (char *)xmlNodeGetContent(cur);
            strncpy(p_srvnode->env, p, sizeof(p_srvnode->env)-1);

            /* Ensure that we terminate... */
            p_srvnode->env[sizeof(p_srvnode->env)-1] = EOS;

            /* process env */
            ndrx_str_env_subs(p_srvnode->env);

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
            strncpy(p_srvnode->cctag, p, sizeof(p_srvnode->cctag)-1);
            /* process env */
            ndrx_str_env_subs(p_srvnode->cctag);
            /* Ensure that we terminate... */
            p_srvnode->cctag[sizeof(p_srvnode->cctag)-1] = EOS;
            xmlFree(p);
        }
    }
    sprintf(p_srvnode->clopt, "%s -- %s", p_srvnode->SYSOPT, p_srvnode->APPOPT);
    strcpy(p_srvnode->binary_name, srvnm);

    if (FAIL==p_srvnode->max)
        p_srvnode->max=config->default_max;

    if (FAIL==p_srvnode->min)
        p_srvnode->min=config->default_min;
    
    if (FAIL==p_srvnode->autokill)
        p_srvnode->autokill=config->default_autokill;
    
    /* Process control params: */
    if (FAIL==p_srvnode->start_max)
        p_srvnode->start_max=config->default_start_max;
    
    if (FAIL==p_srvnode->end_max)
        p_srvnode->end_max=config->default_end_max;
    
    if (FAIL==p_srvnode->pingtime)
        p_srvnode->pingtime=config->default_pingtime;
    
    if (FAIL==p_srvnode->ping_max)
        p_srvnode->ping_max=config->default_ping_max;
    
    /* Copy default env override */
    if (EOS==p_srvnode->env[0] && EOS!=config->default_env[0])
        strcpy(p_srvnode->env, config->default_env);
    
    if (FAIL==p_srvnode->killtime)
        p_srvnode->killtime=config->default_killtime;
    
    /* it could be empty for defaults too - then no problem.  */
    if (EOS==p_srvnode->exportsvcs[0])
        strcpy(p_srvnode->exportsvcs, config->default_exportsvcs);
    
    if (!p_srvnode->srvstartwait)
        p_srvnode->srvstartwait=config->default_srvstartwait;
    
    if (!p_srvnode->srvstopwait)
        p_srvnode->srvstopwait=config->default_srvstopwait;
    
    if (EOS==p_srvnode->cctag[0])
        strcpy(p_srvnode->cctag, config->default_cctag);
    
    if (p_srvnode->ping_max && !p_srvnode->ping_max)
    {
        NDRX_LOG(log_error, "`ping_max' not set for server! at line %hd", 
                cur->line);
        ret=FAIL;
        goto out;
    }
    
    if (FAIL==p_srvnode->srvid)
    {
        NDRX_LOG(log_error, "No srvid near of line %hd", cur->line);
        ret=FAIL;
        goto out;
    }

    NDRX_LOG(log_debug, "Adding: %s SRVID=%d MIN=%d MAX=%d "
            "CLOPT=\"%s\" ENV=\"%s\" START_MAX=%d END_MAX=%d PINGTIME=%d PING_MAX=%d "
            "EXPORTSVCS=\"%s\" START_WAIT=%d STOP_WAIT=%d",
                    p_srvnode->binary_name, p_srvnode->srvid, p_srvnode->min,
                    p_srvnode->max, p_srvnode->clopt, p_srvnode->env,
                    p_srvnode->start_max, p_srvnode->end_max, p_srvnode->pingtime, p_srvnode->ping_max,
                    p_srvnode->exportsvcs,
                    p_srvnode->srvstartwait,
                    p_srvnode->srvstopwait
                    );
    DL_APPEND(config->monitor_config, p_srvnode);

out:
    if (FAIL==ret && p_srvnode)
        free(p_srvnode);
    return ret;
}
/**
 * parse server entries
 * @param doc
 * @param cur
 * @return
 */
private int parse_servers(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    char *p;
    
    for (; cur ; cur=cur->next)
    {
            if (0==strcmp((char*)cur->name, "server"))
            {
                /* Get the server name */
                if (SUCCEED!=parse_server(config, doc, cur))
                {
                    ret=FAIL;
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
private int parse_config(config_t *config, xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    int appconfig_found=FALSE;

    if (NULL==cur)
    {
        NDRX_LOG(log_error, "Empty config?");
        ret=FAIL;
        goto out;
    }

    /* Loop over root elements */
    do
    {
        
        if (0==strcmp((char*)cur->name, "defaults")
                && SUCCEED!=parse_defaults(config, doc, cur->children))
        {
            ret=FAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "servers")
                && SUCCEED!=parse_servers(config, doc, cur->children))
        {
            ret=FAIL;
            goto out;
        }
        else if (0==strcmp((char*)cur->name, "appconfig")
                && (appconfig_found=TRUE)
                && SUCCEED!=parse_appconfig(config, doc, cur->children))
        {
            ret=FAIL;
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
        ret=FAIL;
        goto out;
    }

out:
    return ret;
}

/**
 * Parses & Loads the platform configuration file.
 *
 * This initially loads the configuration int internal represtation of the
 * configuration file. After that from this info we will build work structures.
 */
public int load_config(config_t *config, char *config_file)
{
    int ret=SUCCEED;
    xmlDocPtr doc;
    xmlNodePtr root;
#if 0
    reader = xmlNewTextReaderFilename(config_file);

    if(!reader) {
        NDRX_LOG(log_error, "Failed to open [%s] with error: %s",
                                        config_file, strerror(errno));
        ret=FAIL;
        goto out;
    }
#endif
    doc = xmlReadFile(config_file, NULL, 0);

    if (!doc)
    {
        NDRX_LOG(log_error, "Failed to open or parse %s", config_file);
        ret=FAIL;
        goto out;
    }

    /* Get root element */
    if (!(root = xmlDocGetRootElement(doc)))
    {
        NDRX_LOG(log_error, "Failed to get root XML element");
        ret=FAIL;
        goto out;
    }

    /* Step into first childer */
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
public int test_config(int reload, command_call_t * call, 
        void (*p_reload_error)(command_call_t * call, int srvid, 
        char *old_bin, char *new_bin, int error))
{
    int ret=SUCCEED;
    int new_error=FALSE;
    int old_error=FALSE;
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

    if (SUCCEED!=load_active_config(&t_app_config, &t_process_model,
                &t_process_model_hash, &t_process_model_pid_hash))
    {
        NDRX_LOG(log_error, "Failed to load new configuration & build pmodel!");
        ret=FAIL;
        goto out;
    }
    /* If not active config present, then we have nothing to do */
    if (NULL==G_app_config)
    {
        NDRX_LOG(log_debug, "Active config not loaded, nothing to do.");
        return SUCCEED;
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
     * If server id not presnet in hash, then this must be shutdown
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
                                new->binary_name, NDRXD_EREBBINARYRUN);
                
                if (!new_error)
                {
                    /* This is error case */
                    NDRXD_set_error_fmt(NDRXD_EREBBINARYRUN, "Renamed binary [%s] for "
                            "serverid=%d is in non shutdown state (%d)! New binary for this id is [%s]",
                            old->binary_name, new->srvid, old->state, new->binary_name);
                }
                new_error=TRUE;
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
            if (!(NDRXD_PM_MIN_EXIT<=old->state && old->state<=NDRXD_PM_MAX_EXIT))
            {
                NDRX_LOG(log_error,"Removed binary [%s] for "
                        "serverid=%d is in non shutdown state (%d)!",
                        old->binary_name, old->srvid, old->state);

                /* Give some feedback to console */
                if (NULL!=p_reload_error)
                    p_reload_error(call, old->srvid, old->binary_name,
                                NULL, NDRXD_EBINARYRUN);

                if (!old_error)
                {
                    /* This is error case */
                    NDRXD_set_error_fmt(NDRXD_EBINARYRUN, "Removed binary [%s] for "
                            "serverid=%d is in non shutdown state (%d)!",
                            old->binary_name, old->srvid, old->state);
                }
                old_error=TRUE;
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
        ret=FAIL;
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
                new->state = old->state;
                /* Link existing service info to new PM! */
                new->svcs = old->svcs;
                new->reqstate = old->reqstate;
                new->num_term_sigs = old->num_term_sigs;
                new->last_sig = old->last_sig;
                new->rsptimer = old->rsptimer;
                new->killreq = old->killreq;
                /* Flags were missing after reload! */
                new->flags = old->flags;
                new->pingtimer = old->pingtimer;
                new->pingseq = old->pingseq;
                new->pingroundtrip = old->pingroundtrip;
                
                /* So that we do not unlink the list later when old pm is freed */
                old->svcs = NULL;
                /* Add stuff to new pidhash! */
                if (SUCCEED!=add_to_pid_hash(t_process_model_pid_hash, new))
                {
                    NDRX_LOG(log_error, "Failed to register "
                                                "process in new pidhash!");
                    ret=FAIL;
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
        /* so that in case of error we do not destroy already master config. */
        t_app_config = NULL;
        t_process_model = NULL;
        t_process_model_hash = NULL;
        t_process_model_pid_hash = NULL;

        /* free up old config */
        config_free(&old_app_config, &old_process_model, &old_process_model_hash, &old_process_model_pid_hash);

        NDRX_LOG(log_debug, "Configuration successfully reloaded!");
    }
    else
    {
        /* free up test config */
        config_free(&t_app_config, &t_process_model, &t_process_model_hash, &t_process_model_pid_hash);
    }

out:

    if (SUCCEED!=ret)
    {
        config_free(&t_app_config, &t_process_model, &t_process_model_hash, &t_process_model_pid_hash);
    }


    return ret;
}
