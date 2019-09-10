/**
 * @brief Parse XA Configuration
 *
 * @file pgconfig.c
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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#include <userlog.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <exparson.h>
#include <pgxa.h>

/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/

/* values from ecpglib/extern.h */
#define NDRX_ECPG_COMPAT_PGSQL              0
#define NDRX_ECPG_COMPAT_INFORMIX           1
#define NDRX_ECPG_COMPAT_INFORMIX_SE        1

#define CONF_URL        1
#define CONF_USER       2
#define CONF_PASS       3
#define CONF_COMPAT     4
        
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/

/**
 * Parse XA Config.
 * JSON to parse is following:
 * { "url":"unix:postgresql://sql.mydomain.com:5432/mydb", "user":"test", 
 *  "password":"test1", "compat":"INFORMIX|INFORMIX_SE|PGSQL"}
 * @param[in] buffer JSON buffer
 * @param[out] conndata connection data
 */
expublic int ndrx_pg_xa_cfgparse(char *buffer, ndrx_pgconnect_t *conndata)
{
    int ret = EXSUCCEED;
    EXJSON_Value *root_value=NULL;
    EXJSON_Object *root_object;
    size_t i, cnt, j, n;
    char *name;
    char    *str_val;
    int typ;
    int param;
    
    NDRX_LOG(log_debug, "Parsing buffer: [%s]", buffer);

    root_value = exjson_parse_string_with_comments(buffer);

    if (exjson_value_get_type(root_value) != EXJSONObject)
    {
        NDRX_LOG(log_debug, "Failed to parse root element");
        EXFAIL_OUT(ret);
        goto out;
    }
    
    memset(conndata, 0, sizeof(ndrx_pgconnect_t));
    
    root_object = exjson_value_get_object(root_value);

    cnt = exjson_object_get_count(root_object);
    NDRX_LOG(log_debug, "cnt = %d", cnt);
    
    for (i=0; i<cnt; i++)
    {
        name = (char *)exjson_object_get_name(root_object, i);

        if (
                (param=CONF_URL) && 0==strcmp(name, "url") ||
                (param=CONF_USER) && 0==strcmp(name, "user") ||
                (param=CONF_PASS) && 0==strcmp(name, "password") ||
                (param=CONF_COMPAT) && 0==strcmp(name, "compat")
            )
        {
            
            typ = exjson_value_get_type(exjson_object_get_value_at(root_object, i));

            if (EXJSONString!=typ)
            {
                NDRX_LOG(log_error, "Invalid type %d for `%s', must be string", 
                        typ, name);
                EXFAIL_OUT(ret);
            }
            
            str_val = (char *)exjson_object_get_string(root_object, name);
            
            NDRX_LOG(log_debug, "Got [%s] = [%s]", name, str_val);
            
            switch (param)
            {
                case CONF_URL:
                    NDRX_STRCPY_SAFE(conndata->url, str_val);
                    break;
                case CONF_USER:
                    NDRX_STRCPY_SAFE(conndata->user, str_val);
                    break;
                case CONF_PASS:
                    NDRX_STRCPY_SAFE(conndata->password, str_val);
                    break;
                case CONF_COMPAT:
                    
                    if (0==strncmp(str_val, "INFORMIX", 8))
                    {
                        if (0==strcmp(str_val, "INFORMIX"))
                        {
                            conndata->c = NDRX_ECPG_COMPAT_INFORMIX;
                        }
                        else
                        {
                            conndata->c = NDRX_ECPG_COMPAT_INFORMIX_SE;
                        }
                    }
                    else if (0!=strcmp(str_val, "PGSQL"))
                    {
                        NDRX_LOG(log_error, "Invalid compat node [%s]", str_val);
                        EXFAIL_OUT(ret);
                    }
                    
                    break;
            }
        } 
        else
        {
            NDRX_LOG(log_warn, "Skipping [%s] - unsupported", name);
        }
    } /* for root object elements */
    
out:
    /* cleanup code */
    if (NULL != root_value)
    {
        exjson_value_free(root_value);
    }
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
