/**
 * @brief Enduro/X client & server environment configuration structures
 *
 * @file exenv.h
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
#ifndef EXENV_H_
#define EXENV_H_

/*------------------------------Includes--------------------------------------*/
#include <ndrstandard.h>
#include <atmi.h>
#include <exhash.h>
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

/** unset field action */
#define NDRX_ENV_ACTION_UNSET              0x0001
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/* So we need following data structures:
 * 1. Linked list for storing key/values of the any group or process
 * 2. Hash list for storing group lists of envs
 * 3. Linked list for process storing key/values (could be structure 1.)
 * 4. Linked list for storing resolve links to 2. hashes for processing groups
 * firstly
 */

/**
 * Linked list of env variable values
 */
typedef struct ndrx_env_list ndrx_env_list_t;
struct ndrx_env_list
{
    /** env variable (allocated) */
    char *key;
    
    /** env value (allocated) */
    char *value;
    
    /** flags for the field */
    unsigned short flags;
    
    /** make it a linked list... */
    ndrx_env_list_t *next, *prev;
};

/**
 * Group hash of environment variables
 */
typedef struct ndrx_env_group ndrx_env_group_t;
struct ndrx_env_group
{
    /** name of the group */
    char group[MAXTIDENT+1];
    /** List of environment variable groups */
    ndrx_env_list_t *envs;
    
    /** makes this structure hashable */
    EX_hash_handle hh;
};

/**
 * List of environment groups
 */
typedef struct ndrx_env_grouplist ndrx_env_grouplist_t;
struct ndrx_env_grouplist
{
    /** pointer to group */
    ndrx_env_group_t *group;
    /** linked list */
    ndrx_env_grouplist_t *next, *prev;
};

/* Following API functions are needed:
 * 
 * 1) parse envs: IN: xmlDocPtr doc, xmlNodePtr cur, OUT: allocated: ndrx_env_list_t
 * Usable for both group level parsing and process level parsing.
 * 
 * 2) parse group  IN: xmlDocPtr doc, xmlNodePtr cur, OUT: allocated: ndrx_env_group_t
 * (i.e. added record to hash)
 * 
 * 3) update 1) to parse <usegroup>. In case if ndrx_env_grouplist_t parameter
 * is present, the load the resolved tags into this linked list. If parameter
 * is not present and usegroup is found, then merge the group into current
 * env list.
 */

/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

#endif /* EXENV_H_ */

/* vim: set ts=4 sw=4 et smartindent: */
