/**
 * @brief Enduro/X client & server environment configuration structures
 *
 * @file exenvapi.h
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
#ifndef EXENVAPI_H_
#define EXENVAPI_H_

/*------------------------------Includes--------------------------------------*/
#include <libxml/xmlreader.h>

#include <ndrstandard.h>
#include <atmi.h>
#include <exhash.h>
#include <exenv.h>
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

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

#endif /* EXENVAPI_H_ */

/* vim: set ts=4 sw=4 et smartindent: */
