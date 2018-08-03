/**
 * @brief Environment API (groups & lists) for XATMI servers and clients (by cpmsrv)
 *
 * @file envapi.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <ndebug.h>
#include <utlist.h>
#include <nstdutil.h>
#include <exenv.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

exprivate ndrx_env_group_t * ndrx_endrxconf_envs_group_get(ndrx_env_group_t *grouphash, 
        char *group);

exprivate void env_free(ndrx_env_list_t *env);


/**
 * Allocate and copy list entry
 * @param el element to duplicate
 * @return NULL (in case of errro) or ptr to copy
 */
exprivate ndrx_env_list_t * copy_list_entry(ndrx_env_list_t *el)
{
    int ret = EXSUCCEED;
    ndrx_env_list_t * cpy = NULL;
    
    NDRX_MALLOC_OUT(cpy, sizeof(ndrx_env_list_t), ndrx_env_list_t);    
    NDRX_STRDUP_OUT(cpy->key, el->key);
    
    if (NULL!=el->value)
    {
        /* in case of unset this is NULL */
        NDRX_STRDUP_OUT(cpy->value, el->value);
    }
    cpy->flags = el->flags;
    
out:
    
    if (EXSUCCEED!=ret)
    {
        env_free(cpy);
        return NULL;
    }

    return cpy;
}

/**
 * Append to dest list all from source list
 * @param dest destination env list
 * @param source source env list
 * @return EXUSCCEED/EXFAIL
 */
expublic int ndrx_ndrxconf_envs_append(ndrx_env_list_t **dest, ndrx_env_list_t *source)
{
    int ret = EXSUCCEED;
    ndrx_env_list_t *el, *cpy;
    
    
    DL_FOREACH(source, el)
    {
        cpy = copy_list_entry(el);
        
        if (NULL==cpy)
        {
            NDRX_LOG(log_error, "Failed to cpy env entry");
            EXFAIL_OUT(ret);
        }    
        
        DL_APPEND(*dest, cpy);
    }
    
out:
    return ret;
}

/**
 * Parse environments. This could be done for group closure or process closure
 * @param doc xml doc
 * @param cur xml cursor pointing on envs block 
 * @param envs where to store environments
 * @param grouphash list of groups currently known in system
 * @param grouplist if provided then any groups by <usegroup> tag will be populated
 *  to this list, otherwise group will be merged into current list.
 * @return 
 */
expublic int ndrx_ndrxconf_envs_parse(xmlDocPtr doc, xmlNodePtr cur, 
        ndrx_env_list_t **envs, ndrx_env_group_t *grouphash, 
        ndrx_env_grouplist_t **grouplist)
{
    int ret = EXSUCCEED;
    xmlAttrPtr attr;
    char *p;
    ndrx_env_list_t *env = NULL;
    ndrx_env_grouplist_t *glist = NULL;
    
    /* loop over the current element and parse tags accordingly */
    cur=cur->children;
    
    for (; cur; cur=cur->next)
    {
        if (0==strcmp("env", (char *)cur->name))
        {
            /* allocate new env variable */
            NDRX_CALLOC_OUT(env, 1, sizeof(*env), ndrx_env_list_t);
            
            /* extract attributes */
            for (attr=cur->properties; attr; attr = attr->next)
            {
                if (0==strcmp((char *)attr->name, "name"))
                {
                    p = (char *)xmlNodeGetContent(attr->children);
                    
                    env->key = NDRX_STRDUP(p);
                    
                    if (NULL==env->key)
                    {
                        NDRX_LOG(log_error, "Failed to strdup: %s", 
                                strerror(errno));
                        xmlFree(p);
                        EXFAIL_OUT(ret);
                    }
                    xmlFree(p);
                }
                else if (0==strcmp((char *)attr->name, "unset"))
                {
                    
                    p = (char *)xmlNodeGetContent(attr->children);
                    
                    if (0==strcmp(p, "Y") || 0==strcmp(p, "y") ||
                            0==strcmp(p, "Yes") || 0==strcmp(p, "yes"))
                    {
                        env->flags|=NDRX_ENV_ACTION_UNSET;
                    }
                    xmlFree(p);
                }
            }
            
            if (EXEOS==env->key[0])
            {
                NDRX_LOG(log_error, "XML config <envs> tag name attrib!", p);
                userlog("XML config <envs> tag name attrib!", p);
                EXFAIL_OUT(ret);
            }
            
            /* extract value */
            p = (char *)xmlNodeGetContent(cur);
            
            /* no need for value if action unset */
            if (!(env->flags & NDRX_ENV_ACTION_UNSET))
            {
                env->value = NDRX_STRDUP(p);

                if (NULL==env->value)
                {
                    NDRX_LOG(log_error, "Failed to strdup: %s", 
                                    strerror(errno));
                    xmlFree(p);
                    EXFAIL_OUT(ret);
                }
            }
            xmlFree(p);
            
            /* if we are ok, add to linked list */
            DL_APPEND(*envs, env);
            env = NULL;

        }
        else if (0==strcmp("usegroup", (char *)cur->name))
        {
            ndrx_env_group_t * pullin;
            p = (char *)xmlNodeGetContent(cur);
            
            /* user wants to pull in the group p */
            
            pullin = ndrx_endrxconf_envs_group_get(grouphash,p);
            
            if (NULL==pullin)
            {
                NDRX_LOG(log_error, "ndrxconfig.xml error: group [%s] not defined!", p);
                userlog("ndrxconfig.xml error: group [%s] not defined!", p);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
            xmlFree(p);
            
            if (NULL!=grouplist)
            {
                /* add the group to the list */
                NDRX_CALLOC_OUT(glist, 1, sizeof(ndrx_env_grouplist_t), 
                        ndrx_env_grouplist_t);
                glist->group = pullin;
                
                DL_APPEND(*grouplist, glist);
                glist = NULL;
            }
            else
            {
                /* merge their group into this group */
                if (EXSUCCEED!=ndrx_ndrxconf_envs_append(envs, pullin->envs))
                {
                    NDRX_LOG(log_error, "Failed to append envs");
                    EXFAIL_OUT(ret);
                }
            }
        } /* use group */
    } /* for cursor... */
    
out:
    
    if (EXSUCCEED!=ret)
        
    {
        if (NULL!=env)
        {
            env_free(env);
        }
        
        if (NULL!=glist)
        {
            NDRX_FREE(glist);
        }
    }
    
    return ret;
}

/**
 * Resolve group by group name
 * @param grouphash group hash
 * @param group group name
 * @return NULL (not found) or ptr to group found
 */
exprivate ndrx_env_group_t * ndrx_endrxconf_envs_group_get(ndrx_env_group_t *grouphash, 
        char *group)
{
    ndrx_env_group_t * ret = NULL;
    
    EXHASH_FIND_STR(grouphash, group, ret);
    
    return ret;
}

/**
 * Free up single env entry
 * @param env env list entry
 */
exprivate void env_free(ndrx_env_list_t *env)
{
    if (NULL!=env->key)
    {
        NDRX_FREE(env->key);
    }

    if (NULL!=env->value)
    {
        NDRX_FREE(env->value);
    }
    
    if (NULL!=env)
    {
        NDRX_FREE(env);
    }
}

/**
 * Free up the environments list
 * @param envs list of 
 */
expublic void  ndrx_ndrxconf_envs_envs_free(ndrx_env_list_t **envs)
{
    ndrx_env_list_t * el, *elt;
    
    DL_FOREACH_SAFE(*envs, el, elt)
    {
        DL_DELETE(*envs, el);
        
        env_free(el);
    }
}

/**
 * Free up the group, we shall clean up the envs
 * and free up the group by it self
 * @param grouphash hash list to remove from (if set)
 * @param group group to delete
 */
exprivate void  ndrx_ndrxconf_envs_group_free(ndrx_env_group_t **grouphash, 
        ndrx_env_group_t *group)
{
     ndrx_ndrxconf_envs_envs_free(&group->envs);
    
    if (NULL!=grouphash)
    {
        EXHASH_DEL(*grouphash, group);
    }
    
    NDRX_FREE(group);
}

/**
 * Delete group lists
 * @param grouplist group list
 */
expublic void ndrx_ndrxconf_envs_grouplists_free(ndrx_env_grouplist_t **grouplist)
{
    ndrx_env_grouplist_t *el, *elt;
    
    DL_FOREACH_SAFE(*grouplist, el, elt)
    {
        DL_DELETE(*grouplist, el);
        NDRX_FREE(el);
    }
}

/**
 * Remove all groups noted in hash
 * @param grouphash group hash to clean up
 */
expublic void ndrx_ndrxconf_envs_groups_free(ndrx_env_group_t **grouphash)
{
    ndrx_env_group_t *el, *elt;
    
    EXHASH_ITER(hh, *grouphash, el, elt)
    {
        ndrx_ndrxconf_envs_group_free(grouphash, el);
    }
}

/**
 * Parse group tag and fill up the hash list
 * @param doc xml document
 * @param cur current cursor
 * @param grouphash Hash list of the groups
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ndrxconf_envs_group_parse(xmlDocPtr doc, xmlNodePtr cur, 
         ndrx_env_group_t **grouphash)
{
    int ret = EXSUCCEED;
    ndrx_env_group_t *group=NULL;
    xmlAttrPtr attr;
    char *p;
    
    NDRX_CALLOC_OUT(group, 1, sizeof(ndrx_env_group_t), ndrx_env_group_t);
    
    /* extract the  group code */
    for (attr=cur->properties; attr; attr = attr->next)
    {
        /* TODO: Test that name is mandatory! */
        if (0==strcmp((char *)attr->name, "group"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRCPY_SAFE(group->group, p);
            xmlFree(p);
        }
    }
    
    if (EXEOS==group->group[0])
    {
        NDRX_LOG(log_error, "ERROR ! missing group name in XML config, <envs> tag");
        userlog("ERROR ! missing group name in XML config, <envs> tag");
        EXFAIL_OUT(ret);
    }
    
    /* test group code against hash list if found, then reject with error */
    if (NULL!=ndrx_endrxconf_envs_group_get(*grouphash, group->group))
    {
        NDRX_LOG(log_error, "ERROR ! Group [%s] already defined in xml config", 
                group->group);
        userlog("ERROR ! Group [%s] already defined in xml config", 
                group->group);
        EXFAIL_OUT(ret);
    }
    
    /* load the environments 
     * This shall loop over the all items in the cursor closure
     */
    if (EXSUCCEED!=ndrx_ndrxconf_envs_parse(doc, cur, &group->envs, *grouphash, NULL))
    {
        NDRX_LOG(log_error, "Failed to parse environment variables from xml");
        EXFAIL_OUT(ret);
    }
    
    /* finally add group to hash list */
    
    EXHASH_ADD_STR(*grouphash, group, group);
    
    
out:
    
    /* delete invalid group */
    if (EXSUCCEED!=ret && NULL!=group)
    {
         ndrx_ndrxconf_envs_group_free(NULL, group);
    }
        
    return ret;
}

/**
 * Load environment from list
 * @param envs linked list with env settings
 * @return EXSUCCED/EXFAIL
 */
expublic int ndrx_ndrxconf_envs_apply(ndrx_env_list_t *envs)
{
    int ret = EXSUCCEED;
    char tmp[PATH_MAX];
    ndrx_env_list_t *el;
    
    DL_FOREACH(envs, el)
    {   
        if (el->flags & NDRX_ENV_ACTION_UNSET)
        {
            NDRX_LOG(log_dump, "Unsetting env [%s]", el->key);

            if (EXSUCCEED!=unsetenv(el->key))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to set [%s]=[%s]: %s",
                        el->key, tmp, strerror(err));
                userlog("Failed to set [%s]=[%s]: %s",
                        el->key, tmp, strerror(err));
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_STRCPY_SAFE(tmp, el->value);
            ndrx_str_env_subs_len(tmp, sizeof(tmp));

            NDRX_LOG(log_dump, "Setting env [%s]=[%s]",
                    el->key, tmp);

            if (EXSUCCEED!=setenv(el->key, tmp, EXTRUE))
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to set [%s]=[%s]: %s",
                        el->key, tmp, strerror(err));
                userlog("Failed to set [%s]=[%s]: %s",
                        el->key, tmp, strerror(err));
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    return ret;
}

/**
 * Apply environment variables from the group list and individuals from the
 * list (load variables into environment)
 * @param grouplist process references to group listings
 * @param envs process specific environment variables
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_ndrxconf_envs_applyall(ndrx_env_grouplist_t *grouplist, 
        ndrx_env_list_t *envs)
{
    int ret = EXSUCCEED;
    ndrx_env_grouplist_t *el;
    
    DL_FOREACH(grouplist, el)
    {
        NDRX_LOG(log_debug, "Loading group envs [%s]", el->group->group);
     
        if (EXSUCCEED!=ndrx_ndrxconf_envs_apply(el->group->envs))
        {
            NDRX_LOG(log_error, "Failed to load group envs [%s]", 
                    el->group->group);
            EXFAIL_OUT(ret);
        }
    }
    
    if (EXSUCCEED!=ndrx_ndrxconf_envs_apply(envs))
    {
        NDRX_LOG(log_error, "Failed to load process specific envs!");
        EXFAIL_OUT(ret);
    }
out:
    return ret;
}

/* vim: set ts=4 sw=4 et cindent: */

