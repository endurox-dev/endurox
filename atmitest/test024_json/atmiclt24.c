/* 
** JSON<->UBF testing
**
** @file atmiclt24.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <atmi.h>
#include <ubf.h>
#include <ndebug.h>
#include <test.fd.h>
#include <ndrstandard.h>
#include "test024.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

int main(int argc, char **argv)
{
        int ret = SUCCEED;
        char buf[1024];
        UBFH *p_fb;
        int i;
        
        char *json = "{\"T_SHORT_FLD\":1765,"
        "\"T_LONG_FLD\":[333311199991.000000,2],"
        "\"T_CHAR_FLD\":\"A\","
        "\"T_FLOAT_FLD\":1.330000,"
        "\"T_DOUBLE_FLD\":[1111.220000,333,444],"
        "\"T_STRING_FLD\":\"HELLO WORLD\","
        "\"T_CARRAY_FLD\":\"AAECA0hFTExPIEJJTkFSWQQFAA==\"}";
        
        char jsonout[1024];
        
        for (i=0; i<10000; i++)
        {
            p_fb = (UBFH *)tpalloc("UBF", NULL, 4096);
            if (SUCCEED!=tpjsontoubf(p_fb, json))
            {
                NDRX_LOG(log_error, "TESTERROR: failed to alloc UBF buffer!");
                ret=FAIL;
                goto out;
            }

            Bprint(p_fb);

            memset(jsonout, 0, sizeof(jsonout));

            if (SUCCEED!=tpubftojson(p_fb, jsonout, sizeof(jsonout)))
            {
                NDRX_LOG(log_error, "TESTERROR: Failed to build json!!!!");
                ret=FAIL;
                goto out;
            }
            
            NDRX_LOG(log_info, "JSON built: [%s]", jsonout);
            
            if (0!=strcmp(json, jsonout))
            {
                NDRX_LOG(log_error, "Original json differs from built! org: [%s], built: [%s]", 
                        json, jsonout);
                ret=FAIL;
                goto out;
            }
            
            tpfree((char *)p_fb);
            
        }
        
out:
        return ret;
}
