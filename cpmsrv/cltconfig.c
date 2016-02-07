/* 
** Client configuration
**
** @file cltconfig.c
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
#include <userlog.h>
#include <atmi.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cpmsrv.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/*---------------------------Globals------------------------------------*/

/*
 * Active monitor configuration
 */
public cpm_process_t *G_clt_config=NULL;

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Get lookup key
 * @param key_out
 * @param tag
 * @param subsect
 */
public void cpm_get_key(char *key_out, char *tag, char *subsect)
{
    sprintf(key_out, "%s%c%s", tag, S_FS, subsect);
}
/**
 * Return client by tag & subsect
 * @param tag
 * @param subsect
 * @return ptr to client or NULL (if not found)
 */
public cpm_process_t * cpm_client_get(char *tag, char *subsect)
{
    cpm_process_t *r=NULL;
    
    char key[CPM_KEY_LEN];
    
    cpm_get_key(key, tag, subsect);
    
    HASH_FIND_STR( G_clt_config, key, r);
    
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
public cpm_process_t * cpm_get_client_by_pid(pid_t pid)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    /* Mark config as not refreshed */
    HASH_ITER(hh, G_clt_config, c, ct)
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
public cpm_process_t * cpm_start_all(void)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    /* Mark config as not refreshed */
    HASH_ITER(hh, G_clt_config, c, ct)
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
public void cpm_set_cur_time(cpm_process_t *p_cltproc)
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
private int parse_client(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    xmlAttrPtr attr;
    
    cpm_process_t cltproc;
    cpm_process_t *p_cltproc = NULL;
    char *p;
    cpm_process_t * p_cl;
    
    memset(&cltproc, 0, sizeof(cpm_process_t));
    
    for (attr=cur->properties; attr; attr = attr->next)
    {
        if (0==strcmp(attr->name, "cmdline"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            strncpy(cltproc.stat.command_line, p, PATH_MAX);
            cltproc.stat.command_line[PATH_MAX] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp(attr->name, "env"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            strncpy(cltproc.stat.env, p, PATH_MAX);
            cltproc.stat.env[PATH_MAX] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp(attr->name, "stdout"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            strncpy(cltproc.stat.log_stdout, p, PATH_MAX);
            cltproc.stat.log_stdout[PATH_MAX] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp(attr->name, "stderr"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            strncpy(cltproc.stat.log_stderr, p, PATH_MAX);
            cltproc.stat.log_stderr[PATH_MAX] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp(attr->name, "log"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            /* Install both: */
            strncpy(cltproc.stat.log_stdout, p, PATH_MAX);
            cltproc.stat.log_stdout[PATH_MAX] = EOS;
            
            strncpy(cltproc.stat.log_stderr, p, PATH_MAX);
            cltproc.stat.log_stderr[PATH_MAX] = EOS;
            xmlFree(p);
        }
        else if (0==strcmp(attr->name, "autostart"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            
            if ('Y'==*p || 'y'==*p)
            {
                cltproc.stat.flags|=CPM_F_AUTO_START;
            }
            
            xmlFree(p);
        }
    }
    
    /* Check the client config... */
    if (EOS==cltproc.stat.command_line[0])
    {
        NDRX_LOG(log_error, "No client name at line %hd", cur->line);
        userlog("No client name at line %hd", cur->line);
        FAIL_OUT(ret);
    }
    
    /* parse tags - we should also move out (save the tag to continue with different client) */
    cur=cur->children;

    for (; cur; cur=cur->next)
    {
        if (0==strcmp("exec", cur->name))
         {
           /* Copy stuff from root elem to heap */
            
            p_cltproc = malloc(sizeof(cpm_process_t));
            if (NULL==p_cltproc)
            {
                NDRX_LOG(log_error, "malloc failed for p_cltproc!");
                userlog("malloc failed for p_cltproc!");
                FAIL_OUT(ret);
            }
            
            memcpy(p_cltproc, &cltproc, sizeof(cltproc));

            /* Now override the config: */
            for (attr=cur->properties; attr; attr = attr->next)
            {
                if (0==strcmp(attr->name, "tag"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    strncpy(p_cltproc->tag, p, CPM_TAG_LEN);
                    p_cltproc->tag[CPM_TAG_LEN] = EOS;
                    xmlFree(p);
                }
                else if (0==strcmp(attr->name, "subsect"))
                {
                    /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    strncpy(p_cltproc->subsect, p, CPM_TAG_LEN);
                    p_cltproc->subsect[CPM_SUBSECT_LEN] = EOS;
                    xmlFree(p);
                }
                else if (0==strcmp(attr->name, "env"))
                {
                     /* Optional */
                    p = (char *)xmlNodeGetContent(attr->children);
                    strncpy(p_cltproc->stat.env, p, PATH_MAX);
                    p_cltproc->stat.env[PATH_MAX] = EOS;
                    xmlFree(p);
                } 
                else if (0==strcmp(attr->name, "stdout"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    strncpy(p_cltproc->stat.log_stdout, p, PATH_MAX);
                    p_cltproc->stat.log_stdout[PATH_MAX] = EOS;
                    xmlFree(p);
                }
                else if (0==strcmp(attr->name, "stderr"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    strncpy(p_cltproc->stat.log_stderr, p, PATH_MAX);
                    p_cltproc->stat.log_stderr[PATH_MAX] = EOS;
                    xmlFree(p);
                }
                else if (0==strcmp(attr->name, "log"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    /* Install both: */
                    strncpy(p_cltproc->stat.log_stdout, p, PATH_MAX);
                    p_cltproc->stat.log_stdout[PATH_MAX] = EOS;

                    strncpy(p_cltproc->stat.log_stderr, p, PATH_MAX);
                    p_cltproc->stat.log_stderr[PATH_MAX] = EOS;
                    xmlFree(p);
                }
                else if (0==strcmp(attr->name, "autostart"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);

                    if ('Y'==*p || 'y'==*p)
                    {
                        p_cltproc->stat.flags|=CPM_F_AUTO_START;
                    }

                    xmlFree(p);
                }
            }
            
            /* Check the client config... */
            if (EOS==p_cltproc->tag[0])
            {
                NDRX_LOG(log_error, "Missing tag at line %hd", cur->line);
                userlog("Missing tag at line %hd", cur->line);
                FAIL_OUT(ret);
            }
            
            /* Default the subsect */
            if (EOS==p_cltproc->subsect[0])
            {
                strcpy(p_cltproc->subsect, "-");
            }
            
            /* Render the final command line */
            if (SUCCEED!=setenv(NDRX_CLTTAG, p_cltproc->tag, 1))
            {
                NDRX_LOG(log_error, "Failed to set %s on line %hd", NDRX_CLTTAG, cur->line);
                userlog("Failed to set %s on line %hd", NDRX_CLTTAG, cur->line);
                FAIL_OUT(ret);
            }
            
            if (SUCCEED!=setenv(NDRX_CLTSUBSECT, p_cltproc->subsect, 1))
            {
                NDRX_LOG(log_error, "Failed to set %s on line %hd", NDRX_CLTSUBSECT, cur->line);
                userlog("Failed to set %s on line %hd", NDRX_CLTSUBSECT, cur->line);
                FAIL_OUT(ret);   
            }
            
            /* format the command line (final) */
            nstdutil_str_env_subs(p_cltproc->stat.command_line);
            /* TODO: We should have length specifier here (so that we do not overrun the mem): */
            nstdutil_str_env_subs(p_cltproc->stat.env);
            
            /* add to hash list */
            cpm_get_key(p_cltproc->key, p_cltproc->tag, p_cltproc->subsect);
            
            /* Try to lookup... */
            p_cl  = cpm_client_get(p_cltproc->tag, p_cltproc->subsect);
            
            if (NULL==p_cl)
            {
                
                /* Set the time of config load... */
                cpm_set_cur_time(p_cltproc);

                /* Add to hashlist */
                p_cltproc->is_cfg_refresh = TRUE;

                /* Try to get from hash, if found update the infos but keep the PID */

                NDRX_LOG(log_info, "Adding %s/%s [%s] to process list", 
                        p_cltproc->tag, p_cltproc->subsect, p_cltproc->stat.command_line);
                HASH_ADD_STR( G_clt_config, key, p_cltproc );
            }
            else
            {
                NDRX_LOG(log_info, "Refreshing %s/%s [%s] ...", 
                        p_cltproc->tag, p_cltproc->subsect, p_cltproc->stat.command_line);
                p_cl->is_cfg_refresh = TRUE;
                
                memcpy(&p_cl->stat, &p_cltproc->stat, sizeof(p_cl->stat));
                free(p_cltproc);
            }
        }
    }
    
out:

    if (FAIL==ret && p_cltproc)
    {
        free(p_cltproc);
    }

    return ret;
}
/**
 * parse client entries
 * @param doc
 * @param cur
 * @return
 */
private int parse_clients(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    char *p;
    
    for (; cur ; cur=cur->next)
    {
            if (0==strcmp((char*)cur->name, "client"))
            {
                /* Get the client name */
                if (SUCCEED!=parse_client(doc, cur))
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
private int parse_config(xmlDocPtr doc, xmlNodePtr cur)
{
    int ret=SUCCEED;
    
    if (NULL==cur)
    {
        NDRX_LOG(log_error, "Empty config?");
        ret=FAIL;
        goto out;
    }

    /* Loop over root elements */
    do
    {
        /* Parse client */
        if (0==strcmp((char*)cur->name, "clients")
                && SUCCEED!=parse_clients(doc, cur->children))
        {
            ret=FAIL;
            goto out;
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
public int load_xml_config(char *config_file)
{
    int ret=SUCCEED;
    xmlDocPtr doc;
    xmlNodePtr root;
    
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
 * @return 
 */
public int load_config(void)
{
    int ret = SUCCEED;
    cpm_process_t *c = NULL;
    cpm_process_t *ct = NULL;
    
    static struct stat prev_attr;
    static struct stat attr;
    static int first = TRUE;
    
    /* Test for the file time stamp changes */
    
    if (first)
    {
        memset(&prev_attr, 0, sizeof(prev_attr));
        first = FALSE;
    }
    
    memset(&attr, 0, sizeof(attr));
    
    if (SUCCEED!=stat(G_config.config_file, &attr))
    {
        NDRX_LOG(log_error, "Config file error [%s]: [%s]",
                G_config.config_file, strerror(errno));
        userlog("Config file error [%s]: [%s]",
                G_config.config_file, strerror(errno));
        FAIL_OUT(ret);
    }
    
    if (0!=memcmp(&attr.st_mtim, &prev_attr.st_mtim, sizeof(attr.st_mtim)))
    {
        prev_attr = attr;
    }
    else
    {
        /* config not changed. */
        goto out;
    }
    
    
    /* Mark config as not refreshed */
    HASH_ITER(hh, G_clt_config, c, ct)
    {
        c->is_cfg_refresh = FALSE;
    }

    if (SUCCEED!=load_xml_config(G_config.config_file))
    {
        NDRX_LOG(log_error, "Failed to parse config");
        userlog("Failed to parse config");
        FAIL_OUT(ret);
    }
    
    /* Remove dead un-needed processes (killed & not in new config) */
    HASH_ITER(hh, G_clt_config, c, ct)
    {
        if (!c->is_cfg_refresh && CLT_STATE_NOTRUN==c->dyn.cur_state)
        {
            NDRX_LOG(log_error, "Removing process: [%s]", c->stat.command_line);
            HASH_DEL(G_clt_config, c);
            free(c);
        }
    }
    
out:
    return ret;    
}


