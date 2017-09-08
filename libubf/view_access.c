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

/**
 * Return the VIEW field according to user type
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ array occurrence of the field (if count > 0)
 * @param buf user buffer to install data to
 * @param len on input user buffer len, on output bytes written to (for carray only)
 * on input for string too.
 * @param usrtype of buf see BFLD_* types
 * @return 0 on success, or -1 on fail. 
 * 
 * The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BNOTPRES field not present (invalid occ)
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BEINVAL - invalid usrtype
 * - BNOSPACE - no space in buffer (the data is larger than user buf specified)
 */
expublic int ndrx_CBvget(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN *len, int usrtype)
{
	return EXFAIL;
}


/**
 * Return the VIEW field according to user type
 * If "C" (count flag) was used, and is less than occ+1, then C_<field> is incremented
 * to occ+1.
 * 
 * In case if "L" (length) was present and this is carray buffer, then L flag will
 * be set
 * 
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ array occurrence of the field (if count > 0)
 * @param buf data to set
 * @param len used only for carray, to indicate the length of the data
 * @param usrtype of buf see BFLD_* types
 * @return 0 on success, or -1 on fail. 
 * 
 * The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BNOTPRES field not present (invalid occ)
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BEINVAL invalid usrtype
 * - BNOSPACE the view field is shorter than data received
 */
expublic int ndrx_CBvset(char *cstruct, char *view, char *cname, BFLDOCC occ, 
			 char *buf, BFLDLEN len, int usrtype)
{
	return EXFAIL;
}

/**
 * Return size of the given view in bytes
 * @param view view name
 * @return >=0 on success, -1 on FAIL.
 * Errors:
 * BBADVIEW view not found
 * BEINVAL view field is NULL
 */
expublic long ndrx_Bvsizeof(char *view)
{
	return EXFAIL;
}

/**
 * Return the occurrences set in buffer. This will either return C_ count field
 * set, or will return max array size, this does not test field against NULL or not.
 * anything, 
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @return occurrences
 *  The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BEINVAL cstruct/view/cname/buf is NULL
 * 
 */
expublic BFLDOCC ndrx_Bvoccur(char *cstruct, char *view, char *cname)
{
	return EXFAIL;
}

/**
 * Set C_ occurrence indicator field, if "C" flag was set
 * If flag is not present, function succeeds and does not return error, but
 * data also is not changed.
 * @param cstruct instance of the view object
 * @param view view name
 * @param cname field name in view
 * @param occ occurrences
 * @return 0 on success or -1 on failure
 *  The following errors possible:
 * - BBADVIEW view not found
 * - BNOCNAME field not found
 * - BEINVAL cstruct/view/cname/buf is NULL
 * - BNOTPRES occ out of bounds
 */
expublic int ndrx_Bvsetoccur(char *cstruct, char *view, char *cname, BFLDOCC occ)
{
	return EXFAIL;
}

/**
 * API would require state to be memset to 0, to start scan
 * @param mode BVNEXT_ALL, BVNET_INDICATORS
 */
expublic int ndrx_Bvnext (Bvnext_state_t *state, char *cstruct, 
		char *view, char *cname, BFLDLEN * cname_len, BFLDOCC *occ, 
			  char *buf, BFLDLEN *len, long mode)
{
	return EXFAIL;
}

