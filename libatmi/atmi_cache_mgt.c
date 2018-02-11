/* 
** ATMI cache management routines
**
** @file atmi_cache_mgt.c
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

/*---------------------------Includes-----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ndrstandard.h>
#include <atmi.h>
#include <atmi_tls.h>
#include <typed_buf.h>

#include "thlock.h"
#include "userlog.h"
#include "utlist.h"
#include "exregex.h"
#include <exparson.h>
#include <atmi_cache.h>
#include <Exfields.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return Management service name
 * @return static char buffer
 */
expublic char* ndrx_cache_mgt_getsvc(void)
{
    static char svcnm[XATMI_SERVICE_NAME_LENGTH+1];
    
    snprintf(svcnm, sizeof(svcnm), NDRX_CACHE_MGSVC, tpgetnodeid());
    
    return svcnm;
}


expublic char* ndrx_cache_mgt_hdr2ubf(void)
{
    /* TODO: */
}

expublic char* ndrx_cache_mgt_ubf2hdr(void)
{
    /* TODO: */
}

expublic char* ndrx_cache_mgt_data2ubf(void)
{
    /* TODO: */
}

expublic char* ndrx_cache_mgt_ubf2data(void)
{
    /* TODO: */
}

