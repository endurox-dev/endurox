/* 
** Environment API (groups & lists) for XATMI servers and clients (by cpmsrv)
**
** @file envapi.c
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
    NDRX_STRDUP_OUT(cpy->value, el->value);
    
out:
    
    if (EXSUCCEED!=ret)
    {
        env_free(cpy);
        return NULL;
    }

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
        ndrx_env_list_t **envs, ndrx_env_group_t *grouphash, ndrx_env_grouplist_t **grouplist)
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
            }
            
            /* extract value */
            p = (char *)xmlNodeGetContent(cur);
            env->value = NDRX_STRDUP(p);
            
            if (NULL==env->value)
            {
                NDRX_LOG(log_error, "Failed to strdup: %s", 
                                strerror(errno));
                xmlFree(p);
                EXFAIL_OUT(ret);
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
                NDRX_CALLOC_OUT(glist, 1, sizeof(ndrx_env_grouplist_t), ndrx_env_grouplist_t);
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
exprivate void  ndrx_ndrxconf_envs_envs_free(ndrx_env_list_t **envs)
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
exprivate void  ndrx_ndrxconf_envs_group_free(ndrx_env_group_t **grouphash, ndrx_env_group_t *group)
{
     ndrx_ndrxconf_envs_envs_free(&group->envs);
    
    if (NULL!=grouphash)
    {
        EXHASH_DEL(*grouphash, group);
    }
    
    NDRX_FREE(group);
}

/* TODO: Have a envs_grouplists_free() */

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
        if (0==strcmp((char *)attr->name, "group"))
        {
            p = (char *)xmlNodeGetContent(attr->children);
            NDRX_STRCPY_SAFE(group->group, p);
            xmlFree(p);
        }
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


/* vim: set ts=4 sw=4 et cindent: */

