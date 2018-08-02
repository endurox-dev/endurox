/* 
 * @brief Unified Buffer Format utils
 *
 * @file ubfutil.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dlfcn.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxdcmn.h>
#include <userlog.h>
#include <ubf.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/**
 * Convert c structure to UBF buffer
 * @param map buffer/c mapping
 * @param p_ub UBF buffer
 * @param c_struct c struct
 * @return SUCCEED/FAIL
 */
expublic int atmi_cvt_c_to_ubf(ubf_c_map_t *map, void *c_struct, UBFH *p_ub, long *rules)
{
    int ret = EXSUCCEED;
    
    while (BBADFLDID!=map->fld)
    {
        if (*rules & UBFUTIL_EXPORT)
        {
            char *data_ptr = (char *)(c_struct+map->offset);
            
            if (BFLD_INT==map->ftype)
            {
                long l;
                int *i = (int *)data_ptr;
                
                l = *i;
                
                if (EXSUCCEED!=CBchg(p_ub, map->fld, map->occ, (char *)&l, map->buf_size, 
                    BFLD_LONG))
                {
                    int be = Berror;
                    NDRX_LOG(log_error, "Failed to install mapped long "
                            "field %d:[%s] to UBF buffer: %s", 
                            Bfname(map->fld), Bstrerror(be));
                    EXFAIL_OUT(ret);
                }
            }
            else if (EXSUCCEED!=CBchg(p_ub, map->fld, map->occ, data_ptr, map->buf_size, 
                    map->ftype))
            {
                int be = Berror;
                NDRX_LOG(log_error, "Failed to install field %d:[%s] to UBF buffer: %s", 
                        Bfname(map->fld), Bstrerror(be));
                EXFAIL_OUT(ret);
            }
        }
        rules++;
        map++;
    }
out:
    return ret;
}

/**
 * Convert UBF buffer data to c structure
 * @param map buffer/c mapping
 * @param p_ub UBF buffer
 * @param c_struct c struct
 * @return SUCCEED/FAIL
 */
expublic int atmi_cvt_ubf_to_c(ubf_c_map_t *map, UBFH *p_ub, void *c_struct, long *rules)
{
    int ret = EXSUCCEED;
    BFLDLEN len;
    while (BBADFLDID!=map->fld)
    {
        if (*rules & UBFUTIL_EXPORT)
        {
            char *data_ptr = (char *)(c_struct+map->offset);
            len = map->buf_size; /* have the buffer size */
            
            if (BFLD_INT==map->ftype)
            {
                long l;
                int *i = (int *)data_ptr;
                if (EXSUCCEED!=CBget(p_ub, map->fld, map->occ, (char *)&l, 0, BFLD_LONG))
                {
                    int be = Berror;
                    NDRX_LOG(log_error, "Failed to get mapped in field %d:[%s"
                            "] from UBF buffer: %s", 
                            map->fld, Bfname(map->fld), Bstrerror(be));

                    if (*rules & UBFUTIL_OPTIONAL)
                    {
                        NDRX_LOG(log_warn, "Field %d:[%s] is optional - continue");
                    }
                    else
                    {
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    *i = (int)l;
                }
            }
            else if (EXSUCCEED!=CBget(p_ub, map->fld, map->occ, data_ptr, &len, map->ftype))
            {
                int be = Berror;
                NDRX_LOG(log_error, "Failed to get field %d:[%s] from UBF buffer: %s", 
                        map->fld, Bfname(map->fld), Bstrerror(be));

                if (*rules & UBFUTIL_OPTIONAL)
                {
                    NDRX_LOG(log_warn, "Field %d:[%s] is optional - continue");
                }
                else
                {
                    EXFAIL_OUT(ret);
                }
            }
        }
        rules++;
        map++;
    }
    
out:
    return ret;
}
