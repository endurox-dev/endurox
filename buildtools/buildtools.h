/**
 * @brief Commons for build tools
 *
 * @file buildtools.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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
#ifndef BUILDTOOLS_H_
#define BUILDTOOLS_H_
/*------------------------------Includes--------------------------------------*/
#include <utlist.h>
/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

#define NDRX_RMNAME_MAX         128

#define COMPILE_SRV             1       /**< Building the server process    */
#define COMPILE_CLT             2       /**< Building client process        */
#define COMPILE_TMS             3       /**< Building transaction manager   */
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/**
 * List of Function and Service names 
 */
typedef struct bs_svcnm_lst bs_svcnm_lst_t;
struct bs_svcnm_lst
{
    char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    char funcnm[128+1];
    int is_new_funcnm;
    EX_hash_handle hh; /* makes this structure hashable        */
};

/**
 * Resource manager definition from $NDRX_HOME/udataobj/RM
 */
typedef struct ndrx_rm_def ndrx_rm_def_t;
struct ndrx_rm_def
{
    char rmname[NDRX_RMNAME_MAX+1];
    char structname[NDRX_RMNAME_MAX+1];
    char libnames[PATH_MAX+1];
};
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/
extern int ndrx_buildsrv_generate_code(char *cfile, int thread_option, 
                                       bs_svcnm_lst_t *p_svcnm_lst, 
                                       bs_svcnm_lst_t *p_funcnm_lst,
                                       ndrx_rm_def_t *p_rmdef, int nomain);
extern int ndrx_compile_c(int buildmode, int verbose, char *cfile, char *ofile, 
        char *firstfiles, char *lastfiles, ndrx_rm_def_t *p_rmdef);

extern int ndrx_get_rm_name(char *rm_name, ndrx_rm_def_t *p_rmdef);

#endif /* BUILDTOOS_H_ */

/* vim: set ts=4 sw=4 et smartindent: */
