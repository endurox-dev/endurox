/* 
** Dynamic VIEW access functions
**
** @file view_access.c
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
#include <memory.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>
#include <atmi_tls.h>
#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

expublic int ndrx_CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN *len, int usrtype)
{
	return EXFAIL;
}

expublic int ndrx_CBvset(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN len, int usrtype)
{
	return EXFAIL;
}

expublic long ndrx_Bvsizeof(char *view)
{
	return EXFAIL;
}

expublic BFLDOCC ndrx_Bvoccur(char *cstruct, char *view, char *cname)
{
	return EXFAIL;
}

expublic int ndrx_Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ)
{
	return EXFAIL;
}

/**
 * API would require state to be memset to 0, to start scan
 */
expublic int ndrx_Bvnext (Bvnext_state_t *state, char *cstruct, 
		char *view, char *cname, BFLDLEN * cname_len, BFLDOCC *occ, 
			  char *buf, BFLDLEN *len)
{
	return EXFAIL;
}

