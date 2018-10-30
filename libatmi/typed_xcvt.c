/**
 * @brief Typed buffer conversion, core operations
 *
 * @file typed_xcvt.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#include <userlog.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <exparson.h>
#include <ubf2exjson.h>
#include <ubf.h>
#include <atmi_int.h>
#include <typed_buf.h>

/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

/**
 * Convert the buffer
 * @param buffer
 * @param xcvtflags
 * @param is_reverse
 * @return 
 */
expublic int typed_xcvt(buffer_obj_t **buffer, long xcvtflags, int is_reverse)
{
    int ret = EXSUCCEED;
    
    /* Make the convert in normal form: */
    
    /* ubf2json */
    if (xcvtflags & SYS_SRV_CVT_JSON2UBF && is_reverse)
    {
        xcvtflags = SYS_SRV_CVT_UBF2JSON;
    }
    else if (xcvtflags & SYS_SRV_CVT_UBF2JSON && is_reverse)
    {
        xcvtflags = SYS_SRV_CVT_JSON2UBF;
    }
    /* view2json */
    if (xcvtflags & SYS_SRV_CVT_JSON2VIEW && is_reverse)
    {
        xcvtflags = SYS_SRV_CVT_VIEW2JSON;
    }
    else if (xcvtflags & SYS_SRV_CVT_VIEW2JSON && is_reverse)
    {
        xcvtflags = SYS_SRV_CVT_JSON2VIEW;
    }

    /* Do the actual convert */
    /* ubf2json */
    if (xcvtflags & SYS_SRV_CVT_JSON2UBF)
    {
        ret = typed_xcvt_json2ubf(buffer);
    }
    else if (xcvtflags & SYS_SRV_CVT_UBF2JSON)
    {
        ret = typed_xcvt_ubf2json(buffer);
    }
    /* view2json */
    else if (xcvtflags & SYS_SRV_CVT_JSON2VIEW)
    {
        ret = typed_xcvt_json2view(buffer);
    }
    else if (xcvtflags & SYS_SRV_CVT_VIEW2JSON)
    {
        /* non null pls */
        ret = typed_xcvt_view2json(buffer, BVACCESS_NOTNULL);
    }
    
out:
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
