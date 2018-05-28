/* 
** Client configuration
**
** @file cltconfig.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <libxml/xmlreader.h>
#include <errno.h>

#include <ndrstandard.h>
#include <userlog.h>
#include <atmi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nstdutil.h>
#include <exenv.h>
#include <exenvapi.h>

#include "cpmsrv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*---------------------------Globals------------------------------------*/

/**
 * Active monitor configuration
 */
expublic cpm_process_t *G_clt_config=NULL;

/**
 * Global environment groups
 */
exprivate ndrx_env_group_t * M_envgrouphash = NULL;

MUTEX_LOCKDECL(M_config_lock) 
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Lock config structure access
 */
expublic void cpm_lock_config(void)
{
    MUTEX_LOCK_V(M_config_lock)
}


/**
 * Unlock config access routines
 */
expublic void cpm_unlock_config(void)
{
    MUTEX_UNLOCK_V(M_config_lock)
}



/**
 * Get lookup key
 * @param key_out
 * @param tag
 * @param subsect
 */
expublic void cpm_get_key(char *key_out, char *tag, char *subsect)
{
    sprintf(key_out, "%s%c%s", tag, S_FS, subsect);
}
/**
 * Return client by tag & subsect
 * @param tag
 * @param subsect
 * @return ptr to client or NULL (if not found)
 */
expublic cpm_process_t * cpm_client_get(char *tag, char *subsect)
{
    cpm_process_t *r=NULL;
    
    char key[CPM_KEY_LEN];
    
    cpm_get_key(key, tag, subsect);
    
    EXHASH_FIND_STR( G_clt_config, key, r);
    
    if (NULL!=r)
    {
        return r;
    }
    else
    {
        return NULL;
    }
}

/**
 * Search for client by 
 * @param pid
 * @return NULL or clt 
 */
expublic cpm_process_t * cpm_get_client_by_pid(pid_t pid)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    /* Mark config as not refreshed */
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        if (c->dyn.pid == pid)
        {
            return c;
        }
    }
    
    return NULL;
}

/**
 * Mark all processes to be started.
 * @param pid
 * @return NULL or clt 
 */
expublic cpm_process_t * cpm_start_all(void)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    /* Mark config as not refreshed */
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        /* start those marked for autostart... */
        if (c->stat.flags & CPM_F_AUTO_START)
        {
            c->dyn.req_state = CLT_STATE_STARTED;
            c->dyn.cur_state = CLT_STATE_STARTING;
        }
    }
    
    return NULL;
}

/**
 * Set current time of the status change
 * @param p_cltproc
 */
expublic void cpm_set_cur_time(cpm_process_t *p_cltproc)
{
    time (&p_cltproc->dyn.stattime);
}

/**
 * Parser client entry
 * @param doc
 * @param cur
 * @param srvnm - name of the client
 * @return 
 */
exprivate int parse_client(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    xmlAttrPtr attr;
    
    cpm_process_t cltproc;
    cpm_process_t *p_cltproc = NULL;
    char *p;
    cpm_process_t * p_cl;
    
    memset(&cltproc, 0, sizeof(cpm_process_t));
    
    cltproc.stat.flags |= CPM_F_KILL_LEVEL_DEFAULT;
    
    for (attr=cur->properties; attr; attr = attr->next)
    {
        if (0==strcmp((char *)attr->name, "cmdline"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.command_line, p, PATH_MAX);
            cltproc.stat.command_line[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "env"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.env, p, PATH_MAX);
            cltproc.stat.env[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "cctag"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.cctag, p, NDRX_CCTAG_MAX);
            cltproc.stat.cctag[NDRX_CCTAG_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "wd"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.wd, p, PATH_MAX);
            cltproc.stat.wd[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "stdout"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.log_stdout, p, PATH_MAX);
            cltproc.stat.log_stdout[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "stderr"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRNCPY(cltproc.stat.log_stderr, p, PATH_MAX);
            cltproc.stat.log_stderr[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "log"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            /* Install both: */
            NDRX_STRNCPY(cltproc.stat.log_stdout, p, PATH_MAX);
            cltproc.stat.log_stdout[PATH_MAX] = EXEOS;
            
            NDRX_STRNCPY(cltproc.stat.log_stderr, p, PATH_MAX);
            cltproc.stat.log_stderr[PATH_MAX] = EXEOS;
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "autostart"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            
            if ('Y'==*p || 'y'==*p)
            {
                cltproc.stat.flags|=CPM_F_AUTO_START;
            }
            
            xmlFree(p);
        }
        else if (0==strcmp((char *)attr->name, "klevel"))
        {
            int d;
            p = (char *)xmlNodeGetContent(attr->children);

            d = atoi(p);

            switch (d)
            {
                case 2:
                    cltproc.stat.flags|=CPM_F_KILL_LEVEL_HIGH;
                case 1:
                    cltproc.stat.flags|=CPM_F_KILL_LEVEL_LOW;
                    break;
                case 0:
                    cltproc.stat.flags&=~CPM_F_KILL_LEVEL_HIGH;
                    cltproc.stat.flags&=~CPM_F_KILL_LEVEL_LOW;
            }

            xmlFree(p);
        }
    }
    
    /* Check the client config... */
    if (EXEOS==cltproc.stat.command_line[0])
    {
        NDRX_LOG(log_error, "No client name at line %hd", cur->line);
        userlog("No client name at line %hd", cur->line);
        EXFAIL_OUT(ret);
    }
    
    /* parse tags - we should also move out (save the tag to continue with different client) */
    cur=cur->children;

    for (; cur; cur=cur->next)
    {
        if (0==strcmp("envs", (char *)cur->name))
        {
            if (EXSUCCEED!=ndrx_ndrxconf_envs_parse(doc, cur, &cltproc.stat.envs,
                    M_envgrouphash, NULL))
            {
                NDRX_LOG(log_error, "Failed to parse environment groups for clients!");
                userlog("Failed to parse environment groups for clients!");
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp("exec", (char *)cur->name))
        {
           /* Copy stuff from root elem to heap */
            
            p_cltproc = NDRX_MALLOC(sizeof(cpm_process_t));
            if (NULL==p_cltproc)
            {
                NDRX_LOG(log_error, "malloc failed for p_cltproc!");
                userlog("malloc failed for p_cltproc!");
                EXFAIL_OUT(ret);
            }
            
            memcpy(p_cltproc, &cltproc, sizeof(cltproc));
            p_cltproc->stat.envs = NULL;
            
            if (EXSUCCEED!=ndrx_ndrxconf_envs_append(&p_cltproc->stat.envs, 
                    cltproc.stat.envs))
            {
                NDRX_LOG(log_error, "Failed to join envs %p %p", p_cltproc->stat.envs, 
                        cltproc.stat.envs);
                userlog("Failed to join envs %p %p", p_cltproc->stat.envs, 
                        cltproc.stat.envs);
                EXFAIL_OUT(ret);
            }
            
            /* Now override the config: */
            for (attr=cur->properties; attr; attr = attr->next)
            {
                if (0==strcmp((char *)attr->name, "tag"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->tag, p, CPM_TAG_LEN);
                    p_cltproc->tag[CPM_TAG_LEN] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "subsect"))
                {
                    /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->subsect, p, CPM_TAG_LEN);
                    p_cltproc->subsect[CPM_SUBSECT_LEN] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "env"))
                {
                     /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->stat.env, p, PATH_MAX);
                    p_cltproc->stat.env[PATH_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "cctag"))
                {
                     /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->stat.cctag, p, NDRX_CCTAG_MAX);
                    p_cltproc->stat.cctag[NDRX_CCTAG_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "wd"))
                {
                     /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->stat.wd, p, PATH_MAX);
                    p_cltproc->stat.wd[PATH_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "stdout"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->stat.log_stdout, p, PATH_MAX);
                    p_cltproc->stat.log_stdout[PATH_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "stderr"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    NDRX_STRNCPY(p_cltproc->stat.log_stderr, p, PATH_MAX);
                    p_cltproc->stat.log_stderr[PATH_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "log"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    /* Install both: */
                    NDRX_STRNCPY(p_cltproc->stat.log_stdout, p, PATH_MAX);
                    p_cltproc->stat.log_stdout[PATH_MAX] = EXEOS;

                    NDRX_STRNCPY(p_cltproc->stat.log_stderr, p, PATH_MAX);
                    p_cltproc->stat.log_stderr[PATH_MAX] = EXEOS;
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "autostart"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);

                    if ('Y'==*p || 'y'==*p)
                    {
                        p_cltproc->stat.flags|=CPM_F_AUTO_START;
                    }

                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "klevel"))
                {
                    int d;
                    p = (char *)xmlNodeGetContent(attr->children);

                    d = atoi(p);
                    
                    switch (d)
                    {
                        case 2:
                            p_cltproc->stat.flags|=CPM_F_KILL_LEVEL_HIGH;
                        case 1:
                            p_cltproc->stat.flags|=CPM_F_KILL_LEVEL_LOW;
                            break;
                            
                        case 0:
                            p_cltproc->stat.flags&=~CPM_F_KILL_LEVEL_HIGH;
                            p_cltproc->stat.flags&=~CPM_F_KILL_LEVEL_LOW;
                    }

                    xmlFree(p);
                }
            }
            
            NDRX_LOG(log_debug, "klevel = low=%d high=%d", 
                    p_cltproc->stat.flags & CPM_F_KILL_LEVEL_LOW,
                    p_cltproc->stat.flags & CPM_F_KILL_LEVEL_HIGH
                    );
            
            /* Check the client config... */
            if (EXEOS==p_cltproc->tag[0])
            {
                NDRX_LOG(log_error, "Missing tag at line %hd", cur->line);
                userlog("Missing tag at line %hd", cur->line);
                EXFAIL_OUT(ret);
            }
            
            /* Default the subsect */
            if (EXEOS==p_cltproc->subsect[0])
            {
                NDRX_STRCPY_SAFE(p_cltproc->subsect, "-");
            }
            
            /* Render the final command line */
            if (EXSUCCEED!=setenv(NDRX_CLTTAG, p_cltproc->tag, 1))
            {
                NDRX_LOG(log_error, "Failed to set %s on line %hd", NDRX_CLTTAG, cur->line);
                userlog("Failed to set %s on line %hd", NDRX_CLTTAG, cur->line);
                EXFAIL_OUT(ret);
            }
            
            if (EXSUCCEED!=setenv(NDRX_CLTSUBSECT, p_cltproc->subsect, 1))
            {
                NDRX_LOG(log_error, "Failed to set %s on line %hd", NDRX_CLTSUBSECT, cur->line);
                userlog("Failed to set %s on line %hd", NDRX_CLTSUBSECT, cur->line);
                EXFAIL_OUT(ret);   
            }
            
            /* format the command line (final) */
            ndrx_str_env_subs_len(p_cltproc->stat.command_line, sizeof(p_cltproc->stat.command_line));
            ndrx_str_env_subs_len(p_cltproc->stat.env, sizeof(p_cltproc->stat.env));
            ndrx_str_env_subs_len(p_cltproc->stat.cctag, sizeof(p_cltproc->stat.cctag));
            ndrx_str_env_subs_len(p_cltproc->stat.wd, sizeof(p_cltproc->stat.wd)); /* working dir */
            /* Expand the logfile path... */
            ndrx_str_env_subs_len(p_cltproc->stat.log_stdout, sizeof(p_cltproc->stat.log_stdout));
            ndrx_str_env_subs_len(p_cltproc->stat.log_stderr, sizeof(p_cltproc->stat.log_stderr));
            
            /* add to hash list */
            cpm_get_key(p_cltproc->key, p_cltproc->tag, p_cltproc->subsect);
            
            /* Try to lookup... */
            p_cl  = cpm_client_get(p_cltproc->tag, p_cltproc->subsect);
            
            if (NULL==p_cl)
            {
                
                /* Set the time of config load... */
                cpm_set_cur_time(p_cltproc);

                /* Add to hashlist */
                p_cltproc->is_cfg_refresh = EXTRUE;

                /* Try to get from hash, if found update the infos but keep the PID */

                NDRX_LOG(log_info, "Adding %s/%s [%s] to process list", 
                        p_cltproc->tag, p_cltproc->subsect, p_cltproc->stat.command_line);
                EXHASH_ADD_STR( G_clt_config, key, p_cltproc );
            }
            else
            {
                NDRX_LOG(log_info, "Refreshing %s/%s [%s] ...", 
                        p_cltproc->tag, p_cltproc->subsect, p_cltproc->stat.command_line);
                p_cl->is_cfg_refresh = EXTRUE;
                
                /* free up current env... */
                ndrx_ndrxconf_envs_envs_free(&p_cl->stat.envs);
                
                /* this will make use of newly allocated env */
                memcpy(&p_cl->stat, &p_cltproc->stat, sizeof(p_cl->stat));
                
                NDRX_FREE(p_cltproc);
            }
        }
    }
    
out:

    if (EXFAIL==ret && p_cltproc)
    {
        NDRX_FREE(p_cltproc);
    }

    /* free up envs of the temp process */
    if (NULL!=cltproc.stat.envs)
    {
        ndrx_ndrxconf_envs_envs_free(&cltproc.stat.envs);
    }

    return ret;
}

/**
 * parse client entries
 * @param doc XML document
 * @param cur current cursor pointing to <envs> tag
 * @return EXSUCCEED/EXFAIL
 */
exprivate int parse_envs(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;

    if (EXSUCCEED!=ndrx_ndrxconf_envs_group_parse(doc, cur, &M_envgrouphash))
    {
        NDRX_LOG(log_error, "Failed to parse environment groups for clients!");
        userlog("Failed to parse environment groups for clients!");
        EXFAIL_OUT(ret);
    }
    
out:    
    return ret;
}

/**
 * parse client entries
 * @param doc
 * @param cur
 * @return
 */
exprivate int parse_clients(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;

    for (; cur ; cur=cur->next)
    {
        if (0==strcmp((char*)cur->name, "client"))
        {
            /* Get the client name */
            if (EXSUCCEED!=parse_client(doc, cur))
            {
                ret=EXFAIL;
                goto out;
            }
        }
        else if (0==strcmp((char*)cur->name, "envs")
                && EXSUCCEED!=parse_envs(doc, cur))
        {
            EXFAIL_OUT(ret);
        }
    }
out:

    if (NULL!=M_envgrouphash)
    {
       ndrx_ndrxconf_envs_groups_free(&M_envgrouphash);
    }

    
    return ret;
}

/**
 * Parse config out...
 * @param doc
 * @return
 */
exprivate int parse_config(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=EXSUCCEED;
    
    if (NULL==cur)
    {
        NDRX_LOG(log_error, "Empty config?");
        ret=EXFAIL;
        goto out;
    }

    /* Loop over root elements */
    do
    {
        /* Parse client */
        if (0==strcmp((char*)cur->name, "clients")
                && EXSUCCEED!=parse_clients(doc, cur->children))
        {
            EXFAIL_OUT(ret);
        }
        
        cur=cur->next;
    } while (cur);
    
out:
    
    return ret;
}

/**
 * Parses & Loads the platform configuration file.
 *
 * This initially loads the configuration int internal represtation of the
 * configuration file. After that from this info we will build work structures.
 */
expublic int load_xml_config(char *config_file)
{
    int ret=EXSUCCEED;
    xmlDocPtr doc;
    xmlNodePtr root;
    
    doc = xmlReadFile(config_file, NULL, 0);

    if (!doc)
    {
        NDRX_LOG(log_error, "Failed to open or parse %s", config_file);
        ret=EXFAIL;
        goto out;
    }

    /* Get root element */
    if (!(root = xmlDocGetRootElement(doc)))
    {
        NDRX_LOG(log_error, "Failed to get root XML element");
        ret=EXFAIL;
        goto out;
    }

    /* Step into first childer */
    ret=parse_config(doc, root->children);
    
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
 * Load the active configuration.
 * @return EXSUCCEED/EXFAIL
 */
expublic int load_config(void)
{
    int ret = EXSUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    static struct stat prev_attr;
    static struct stat attr;

    static int first = EXTRUE;
    int was_locked = EXFALSE;
    
    /* Test for the file time stamp changes */
    
    if (first)
    {
        memset(&prev_attr, 0, sizeof(prev_attr));
        first = EXFALSE;
    }
    
    memset(&attr, 0, sizeof(attr));
    
    if (EXSUCCEED!=stat(G_config.config_file, &attr))
    {
        NDRX_LOG(log_error, "Config file error [%s]: [%s]",
                G_config.config_file, strerror(errno));
        userlog("Config file error [%s]: [%s]",
                G_config.config_file, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    if (0!=memcmp(&attr.st_mtime, &prev_attr.st_mtime, sizeof(attr.st_mtime)))
    {
        prev_attr = attr;
    }
    else
    {
        /* config not changed. */
        goto out;
    }
    
    
    /* Mark config as not refreshed */
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        c->is_cfg_refresh = EXFALSE;
    }

    
    /* Lock so that background thread cannot access the 
     * config during the changes in struct... 
     * Bug #108 01/04/2015, mvitolin
     */
    cpm_lock_config();
    was_locked = EXTRUE;
    
    if (EXSUCCEED!=load_xml_config(G_config.config_file))
    {
        NDRX_LOG(log_error, "Failed to parse config");
        userlog("Failed to parse config");
        EXFAIL_OUT(ret);
    }
    
    /* Remove dead un-needed processes (killed & not in new config) */
    
    EXHASH_ITER(hh, G_clt_config, c, ct)
    {
        if (!c->is_cfg_refresh && CLT_STATE_NOTRUN==c->dyn.cur_state)
        {
            NDRX_LOG(log_error, "Removing process: [%s]", c->stat.command_line);
            EXHASH_DEL(G_clt_config, c);
            NDRX_FREE(c);
        }
    }
    
out:

    if (was_locked)
    {
        cpm_unlock_config();
    }
    return ret;    
}

/* vim: set ts=4 sw=4 et cindent: */
