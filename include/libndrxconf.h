/**
 * @brief Enduro/X client & server environment configuration structures
 *
 * @file ndrxconf.h
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
#ifndef NDRXCONF_H_
#define NDRXCONF_H_

/*------------------------------Includes--------------------------------------*/
#include <libxml/xmlreader.h>

#include <ndrstandard.h>
#include <atmi.h>
#include <exhash.h>
#include <exenv.h>
#include <exhash.h>
#include <nerror.h>
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
#define NDRX_PG_USED        0x00000001 /**< This groups is used                     */
#define NDRX_PG_SINGLETON   0x00000002 /**< This is singlelton group                */
#define NDRX_PG_NOORDER     0x00000004 /**< Do not use startup order for singleton  */
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/

/**
 * Process group entry
 */
typedef struct
{
    char grpname[MAXTIDENT+1]; /**< group name                                */
    int grpno;                /**< group number, 1..ndrx_G_libnstd_cfg.sgmax  */
    long flags;                 /**< group flags                                */
    EX_hash_handle hh;         /**< makes this structure hashable               */
} ndrx_procgroup_t;

/**
 * Process group handler
 */
typedef struct
{
    /* hashmap handler for groups->id */
    ndrx_procgroup_t *groups_by_name;
    /* hashmap of ids. Structure size depends on the number of SG groups
     * defined
     */
    ndrx_procgroup_t groups_by_no[0];
} ndrx_procgroups_t;


/**
 * Error structure used by the ndrxconf libary 
 */
typedef struct
{
    char error_msg[MAX_ERROR_LEN+1];
    int error_code;
} ndrx_ndrxconf_err_t;

/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

/* groups api: */

/* envs api: */
extern int ndrx_ndrxconf_envs_group_parse(xmlDocPtr doc, xmlNodePtr cur, 
         ndrx_env_group_t **grouphash);

extern int ndrx_ndrxconf_envs_parse(xmlDocPtr doc, xmlNodePtr cur, 
        ndrx_env_list_t **envs, ndrx_env_group_t *grouphash, 
        ndrx_env_grouplist_t **grouplist);

extern void ndrx_ndrxconf_envs_groups_free(ndrx_env_group_t **grouphash);
extern void ndrx_ndrxconf_envs_grouplists_free(ndrx_env_grouplist_t **grouplist);
extern void ndrx_ndrxconf_envs_envs_free(ndrx_env_list_t **envs);

extern int ndrx_ndrxconf_envs_applyall(ndrx_env_grouplist_t *grouplist, 
        ndrx_env_list_t *envs);
extern int ndrx_ndrxconf_envs_apply(ndrx_env_list_t *envs);

extern int ndrx_ndrxconf_envs_append(ndrx_env_list_t **dest, ndrx_env_list_t *source);

/* Process groups: */
extern int ndrx_ndrxconf_procgroups_apply_singlegrp(ndrx_procgroups_t *handle);
extern ndrx_procgroup_t* ndrx_ndrxconf_procgroups_resolveno(ndrx_procgroups_t *handle, int procgrpno);
extern ndrx_procgroup_t* ndrx_ndrxconf_procgroups_resolvenm(ndrx_procgroups_t *handle, char *name);
extern int ndrx_ndrxconf_procgroups_is_singleton(ndrx_procgroups_t *handle, int procgrp_no);
extern void ndrx_ndrxconf_procgroups_free(ndrx_procgroups_t *handle);
extern int ndrx_ndrxconf_procgroups_parse(ndrx_procgroups_t **config, 
    xmlDocPtr doc, xmlNodePtr cur, 
    char *config_file_short, ndrx_ndrxconf_err_t *err);


#endif /* NDRXCONF_H_ */

/* vim: set ts=4 sw=4 et smartindent: */
