/**
 * @brief Process group API (shared between ndrxd and cpmsrv)
 *  Use standard errors here?
 *
 * @file procgroups.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2023, Mavimax, Ltd. All Rights Reserved.
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
#include <ndebug.h>
#include <utlist.h>
#include <nstdutil.h>
#include <exenv.h>
#include <libndrxconf.h>
#include <lcfint.h>
#include <ndrxdcmn.h>
#include <ndrx_intdef.h>
#include <exhash.h>
#include <singlegrp.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Desceing sort of the node array
 */
exprivate int compare_descending(const void *a, const void *b)
{
    char *aa = (char *)a;
    char *bb = (char *)b;

    if (*aa > *bb)
    {
        return -1;
    }
    else if (*aa < *bb)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * Parse process group
 * @param config config
 * @param doc xml doc
 * @param cur current node
 * @param is_defaults is defaults section
 * @param p_defaults defaults
 * @param config_file_short config file name (xml)
 * @param err error structure. Contains error codes from the ndrxdcmn.h
 * @return EXSUCCEED/EXSUCCEED
 */
expublic int ndrx_appconfig_procgroup(ndrx_procgroups_t **config, 
    xmlDocPtr doc, xmlNodePtr cur, int is_defaults, ndrx_procgroup_t *p_defaults, 
    char *config_file_short, ndrx_ndrxconf_err_t *err)
{
    int ret=EXSUCCEED;
    xmlAttrPtr attr;
    ndrx_procgroup_t *p_grp=NULL;
    ndrx_procgroup_t local;
    char *p;
    
    /* service shall not be defined */
    
    if (is_defaults)
    {
        p_grp=p_defaults;
    }
    else
    {
        p_grp = &local;
        memcpy(p_grp, p_defaults, sizeof(ndrx_procgroup_t));
    }
    
    for (attr=cur->properties; attr; attr = attr->next)
    {
        p = (char *)xmlNodeGetContent(attr->children);
        
        if (0==strcmp((char *)attr->name, "name"))
        {
            int len = strlen(p);

            if (len>MAXTIDENT || len < 1)
            {
                snprintf(err->error_msg, sizeof(err->error_msg),
                    "(%s) invalid length %d for `name' attribute at <procgroup> near line %d", 
                    config_file_short, len, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }

            if (!ndrx_str_valid_alphanumeric_(p, MAXTIDENT))
            {
                snprintf(err->error_msg, sizeof(err->error_msg),
                    "(%s) invalid characters used in `name' attribute at <procgroup> near line %d",
                    config_file_short, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
            NDRX_STRCPY_SAFE(p_grp->grpname, p);
        }
        else if (0==strcmp((char *)attr->name, "grpno"))
        {
            p_grp->grpno = atoi(p);
            /* check that grpno is in range (the upper range is singleton groups) */
            if (!ndrx_sg_is_valid(p_grp->grpno))
            {
                if (ndrx_G_libnstd_cfg.pgmax > 0)
                {
                    snprintf(err->error_msg, sizeof(err->error_msg), 
                        "(%s) Invalid `grpno' %d (valid values 1..%d) in <procgroup> "
                        "section near line %d", 
                        config_file_short, p_grp->grpno, ndrx_G_libnstd_cfg.pgmax, cur->line);
                }
                else
                {
                    snprintf(err->error_msg, sizeof(err->error_msg), 
                        "(%s) Process groups are disabled (NDRX_PGMAX=0) near line %d", 
                        config_file_short, cur->line);
                }
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp((char *)attr->name, "sg_nodes"))
        {
            ndrx_stdcfgstr_t *parsed=NULL, *el;
            /* process singleton group node_ids */
            if (EXSUCCEED!=ndrx_stdcfgstr_parse(p, &parsed))
            {
                snprintf(err->error_msg, sizeof(err->error_msg), 
                    "(%s) Failed to parse `sg_nodes' %s in <procgroup> section near line %d", 
                    config_file_short, p, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }

            DL_FOREACH(parsed, el)
            {
                int nodeid = atoi(el->key);

                /* check that node id is in range */
                if (nodeid<CONF_NDRX_NODEID_MIN || nodeid>CONF_NDRX_NODEID_MAX)
                {
                    snprintf(err->error_msg, sizeof(err->error_msg), 
                        "(%s) Invalid `sg_nodes' %d (valid values 1..%d) in <procgroup> "
                        "section near line %d", 
                        config_file_short, nodeid, CONF_NDRX_NODEID_MAX, cur->line);
                    err->error_code = NDRXD_EINVAL;
                    NDRX_LOG(log_error, "%s", err->error_msg);
                    xmlFree(p);
                    ndrx_stdcfgstr_free(parsed);
                    EXFAIL_OUT(ret);
                }

                /* set group as used */
                p_grp->sg_nodes[nodeid-1]=nodeid;
            }

            /* snow sort the array, so that we do not need to scan
             * all the time the array of seeking for nodes to check
             */
            qsort(p_grp->sg_nodes, CONF_NDRX_NODEID_COUNT, sizeof(char), compare_descending);
        }
        else if (0==strcmp((char *)attr->name, "noorder"))
        {
            /* y/Y */
            if (NDRX_SETTING_TRUE1==*p || NDRX_SETTING_TRUE2==*p)
            {
                p_grp->flags|=NDRX_SG_NO_ORDER;
            } /* n/N */
            else if (NDRX_SETTING_FALSE1==*p || NDRX_SETTING_FALSE2==*p)
            {
                p_grp->flags&=~NDRX_SG_NO_ORDER;
            }
            else
            {
                snprintf(err->error_msg, sizeof(err->error_msg), 
                    "(%s) Invalid `noorder' setting [%s] in "
                        "<procgroup> or <defaults> "
                        "section, expected values [%c%c%c%c] near line %d", 
                        config_file_short, p,
                        NDRX_SETTING_TRUE1, NDRX_SETTING_TRUE2,
                        NDRX_SETTING_FALSE1, NDRX_SETTING_FALSE2, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp((char *)attr->name, "singleton"))
        {
            /* y/Y */
            if (NDRX_SETTING_TRUE1==*p || NDRX_SETTING_TRUE2==*p)
            {
                p_grp->flags|=NDRX_SG_SINGLETON;
            } /* n/N */
            else if (NDRX_SETTING_FALSE1==*p || NDRX_SETTING_FALSE2==*p)
            {
                p_grp->flags&=~NDRX_SG_SINGLETON;
            }
            else
            {
                snprintf(err->error_msg, sizeof(err->error_msg), 
                    "(%s) Invalid `singleton' setting [%s] in "
                        "<procgroup> or <defaults> "
                        "section, expected values [%c%c%c%c] near line %d", 
                        config_file_short, p,
                        NDRX_SETTING_TRUE1, NDRX_SETTING_TRUE2,
                        NDRX_SETTING_FALSE1, NDRX_SETTING_FALSE2, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        else if (0==strcmp((char *)attr->name, "sg_verify"))
        {
            /* y/Y */
            if (NDRX_SETTING_TRUE1==*p || NDRX_SETTING_TRUE2==*p)
            {
                p_grp->flags|=NDRX_SG_VERIFY;
            } /* n/N */
            else if (NDRX_SETTING_FALSE1==*p || NDRX_SETTING_FALSE2==*p)
            {
                p_grp->flags&=~NDRX_SG_VERIFY;
            }
            else
            {
                snprintf(err->error_msg, sizeof(err->error_msg), 
                    "(%s) Invalid `sg_verify' setting [%s] in "
                        "<procgroup> or <defaults> "
                        "section, expected values [%c%c%c%c] near line %d", 
                        config_file_short, p,
                        NDRX_SETTING_TRUE1, NDRX_SETTING_TRUE2,
                        NDRX_SETTING_FALSE1, NDRX_SETTING_FALSE2, cur->line);
                err->error_code = NDRXD_EINVAL;
                NDRX_LOG(log_error, "%s", err->error_msg);
                xmlFree(p);
                EXFAIL_OUT(ret);
            }
        }
        
        xmlFree(p);
    }
    
    /* no hashing for defaults */
    if (!is_defaults)
    {
        /* Check that group name is set */
        if (EXEOS==p_grp->grpname[0])
        {
            snprintf(err->error_msg, sizeof(err->error_msg), 
                "(%s) `name' not set in <procgroup> section near line %d", 
                config_file_short, cur->line);
            err->error_code = NDRXD_ECFGINVLD;
            NDRX_LOG(log_error, "%s", err->error_msg);
            EXFAIL_OUT(ret);
        }

        /* Check that group no is set */
        if (0==p_grp->grpno)
        {
            snprintf(err->error_msg, sizeof(err->error_msg), 
                "(%s) `grpno' not set in <procgroup> section near line %d", 
                config_file_short, cur->line);
            err->error_code = NDRXD_ECFGINVLD;
            NDRX_LOG(log_error, "%s", err->error_msg);
            EXFAIL_OUT(ret);
        }

        /* Check that group name is not duplicate */
        if (NULL!=ndrx_ndrxconf_procgroups_resolvenm(*config, p_grp->grpname))
        {
            snprintf(err->error_msg, sizeof(err->error_msg), 
                "(%s) `name' %s is duplicate in <procgroup> section near line %d", 
                config_file_short, p_grp->grpname, cur->line);
            err->error_code = NDRXD_EINVAL;
            NDRX_LOG(log_error, "%s", err->error_msg);
            EXFAIL_OUT(ret);
        }

        /* Check that group id is not duplicate */
        if (NULL!=ndrx_ndrxconf_procgroups_resolveno(*config, p_grp->grpno))
        {
            snprintf(err->error_msg, sizeof(err->error_msg), 
                "(%s) `grpno' %d is duplicate in <procgroup> section near line %d", 
                config_file_short, p_grp->grpno, cur->line);
            err->error_code = NDRXD_EINVAL;
            NDRX_LOG(log_error, "%s", err->error_msg);
            EXFAIL_OUT(ret);
        }

        p_grp->flags|=NDRX_SG_IN_USE;

        /* copy stuff config entry */
        memcpy(&(*config)->groups_by_no[p_grp->grpno-1], p_grp, sizeof(ndrx_procgroup_t));

        p_grp=&(*config)->groups_by_no[p_grp->grpno-1];

        /* add to hashmap by name */
        EXHASH_ADD_STR((*config)->groups_by_name, grpname, p_grp);
    }
out:
    
    return ret;
}

/**
 * Parse singleton group options
 * @param config config (to allocate)
 * @param doc xml doc
 * @param cur current node
 * @param last_line last line number in XML parser
 * @param config_file_short config file name (xml)
 * @param err error structure. Contains error codes from the ndrxdcmn.h
 * @return error code (0 - success)
 */
expublic int ndrx_ndrxconf_procgroups_parse(ndrx_procgroups_t **config, 
    xmlDocPtr doc, xmlNodePtr cur, 
    char *config_file_short, ndrx_ndrxconf_err_t *err)
{
    int ret=EXSUCCEED;
    ndrx_procgroup_t default_opt;

    int is_procgroup;
    int is_defaults;

    memset(&default_opt, 0, sizeof(default_opt));

    *config = NDRX_CALLOC(1, sizeof(ndrx_procgroups_t) + 
            sizeof(ndrx_procgroup_t) * (ndrx_G_libnstd_cfg.pgmax));

    for (; cur ; cur=cur->next)
    {
        is_procgroup= (0==strcmp((char*)cur->name, "procgroup"));
        is_defaults= (0==strcmp((char*)cur->name, "defaults"));
        
        if (is_procgroup || is_defaults)
        {
            /* read the group... */
            if (EXSUCCEED!=ndrx_appconfig_procgroup(config, doc, cur,
                is_defaults, &default_opt, config_file_short, err))
            {
                EXFAIL_OUT(ret);
            }
        }
    }
out:

    return ret;
}

/**
 * Free process groups structure
 * @param handle process group handle to free
 */
expublic void ndrx_ndrxconf_procgroups_free(ndrx_procgroups_t *handle)
{
    ndrx_procgroup_t *p_grp, *tmp;

    if (NULL!=handle)
    {
        /* free by name */
        EXHASH_ITER(hh, handle->groups_by_name, p_grp, tmp) 
        {
            EXHASH_DEL(handle->groups_by_name, p_grp);
        }

        NDRX_FREE(handle);
    }
}

/**
 * Resolve group name to group name
 */
expublic ndrx_procgroup_t* ndrx_ndrxconf_procgroups_resolvenm(ndrx_procgroups_t *handle, char *name)
{
    ndrx_procgroup_t *ret;

    if (NULL==name || EXEOS==name[0])
    {
        return NULL;
    }

    EXHASH_FIND_STR(handle->groups_by_name, name, ret);

    return ret;
}

/**
 * Check if group is singleton
 * @param handle configuration handle
 * @param procgrp_no group number
 * @return EXTRUE (group is singleton)/EXFALSE (group is not singleton)
 */
expublic int ndrx_ndrxconf_procgroups_is_singleton(ndrx_procgroups_t *handle, int procgrp_no)
{
    ndrx_procgroup_t *p_grp;
    int ret = EXFALSE;

    if (NULL==handle)
    {
        goto out;
    }

    p_grp = &handle->groups_by_no[procgrp_no-1];

    if (p_grp->flags & NDRX_SG_SINGLETON)
    {
        ret=EXTRUE;
    }
    else
    {
        ret=EXFALSE;
    }
    
out:
    return ret;
}

/**
 * Resolve group name to group number
 */
expublic ndrx_procgroup_t* ndrx_ndrxconf_procgroups_resolveno(ndrx_procgroups_t *handle, int procgrpno)
{
    ndrx_procgroup_t *ret;
    if (procgrpno<1 || procgrpno>ndrx_G_libnstd_cfg.pgmax)
    {
        return NULL;
    }

    ret=&handle->groups_by_no[procgrpno-1];

    if (ret->flags & NDRX_SG_IN_USE)
    {
        return ret;
    }
    else
    {
        return NULL;
    }
}

/**
 * Apply settings to singleton groups.
 * This will synchronize all group flags to singleton group support library
 */
expublic void ndrx_ndrxconf_procgroups_apply_singlegrp(ndrx_procgroups_t *handle)
{
    int i;
    unsigned short flags;

    /* nothing to-do */
    if (NULL==handle)
    {
        return;
    }

    for (i=0; i<ndrx_G_libnstd_cfg.pgmax; i++)
    {
        ndrx_procgroup_t *p_grp = &handle->groups_by_no[i];
        ndrx_sg_flags_set(i+1, p_grp->flags);

        /* set cluster node_ids */
        ndrx_sg_nodes_set(i+1, p_grp->sg_nodes);
    }
}

/* vim: set ts=4 sw=4 et smartindent: */