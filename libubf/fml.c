/**
 * @brief Few FML api specific handling
 *
 * @file fml.c
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

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>

#include "userlog.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * FML kind of field change
 * @param p_ub UBF buffer
 * @param bfldid field id
 * @param occ occurrence
 * @param buf btr to data. In case of FLD_PTR this contains the actual address
 * @param len buffer len (used for carray)
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Fchg (UBFH *p_ub, BFLDID bfldid, BFLDOCC occ, char * buf, BFLDLEN len)
{
   char *ptr = buf;
   
   /* UBF requires a double ptr for PTR type */
   if (BFLD_PTR==Bfldtype (bfldid))
   {
       ptr = (char *)&buf;
   }
   
   return Bchg (p_ub, bfldid, occ, ptr, len);
}

/**
 * Add field to UBF, FML specifics
 * @param p_ub UBF buffer
 * @param bfldid field id
 * @param buf buffer, for FLD_PTR actually contains the address that must be loaded
 * @param len data len, used for FLD_CARRAY
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Fadd (UBFH *p_ub, BFLDID bfldid, char *buf, BFLDLEN len)
{
    char *ptr = buf;
   
   /* UBF requires a double ptr for PTR type */
   if (BFLD_PTR==Bfldtype (bfldid))
   {
       ptr = (char *)&buf;
   }
   
   return Badd (p_ub, bfldid, ptr, len);
}

/* vim: set ts=4 sw=4 et smartindent: */
