/* 
** ATMI tpimport/tpexport function implementation (Common version)
** TODO
** TODO
**
** @file tpimport.c
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
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>

#include <atmi.h>
#include <userlog.h>
#include <ndebug.h>
#include <tperror.h>
#include <typed_buf.h>
#include <atmi_int.h>

#include <exparson.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

expublic int ndrx_tpimportex(ndrx_expbufctl_t *bufctl,
        char *istr, long ilen, char **obuf, long *olen, long flags)
{
    int ret=EXSUCCEED;
    char *buftype;
    char *subtype;
    char *version;
    char *data;
    UBFH *p_ub;

    int type;
    char *str_val;
    EXJSON_Value *root_value;
    EXJSON_Object *root_object;
    EXJSON_Object *data_object;

    NDRX_LOG(log_debug, "%s: enter", __func__);

    /* TODO Check flag if base64 then decode from base64 */

    NDRX_LOG(log_debug, "Parsing buffer: [%s]", istr);

    root_value = exjson_parse_string_with_comments(istr);
    type = exjson_value_get_type(root_value);
    NDRX_LOG(log_error, "Type is %d", type);

    if (exjson_value_get_type(root_value) != EXJSONObject)
    {
        NDRX_LOG(log_error, "Failed to parse root element");
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: Failed to parse json root element");
        return EXFAIL;
    }

    root_object = exjson_value_get_object(root_value);

    buftype = (char *)exjson_object_get_string(root_object, "buftype" );
    subtype = (char *)exjson_object_get_string(root_object, "subtype" );
    version = (char *)exjson_object_get_string(root_object, "version" );
    
    NDRX_LOG(log_debug, 
             "buftype: [%s] subtype: [%s] version: [%s]", 
             buftype, subtype, version);
    
    if ( NULL != bufctl )
    {
        memset(bufctl, 0, sizeof(ndrx_expbufctl_t));
        NDRX_STRCPY_SAFE(bufctl->buftype, buftype);
        bufctl->buftype_ind = EXTRUE;

        NDRX_STRCPY_SAFE(bufctl->version, version);
        bufctl->version_ind = EXTRUE;
    }

    if ( 0==strcmp(BUF_TYPE_STRING_STR, buftype) )
    {
        data = (char *)exjson_object_get_string(root_object, "data" );
        *olen = (long)strlen(data)+1;
        NDRX_LOG(log_debug, "data: [%s], olen: [%ld]", data, *olen);
        if ( NULL == (*obuf = tpalloc(BUF_TYPE_STRING_STR, "", *olen)))
        {
            NDRX_LOG(log_error, "Failed to allocate string buffer %ld bytes: %s",
                     (*olen), tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        
        NDRX_STRNCPY_SAFE(*obuf, data, *olen);
        NDRX_LOG(log_debug, "obuf/olen: [%s]/[%ld]/[%s]", *obuf, *olen, data);
    }
    else if ( 0 == strcmp(BUF_TYPE_UBF_STR, buftype))
    {
        data_object = exjson_object_get_object(root_object, "data" );
        if (NULL == (*obuf = tpalloc("UBF", NULL, NDRX_MSGSIZEMAX)))
        {
            NDRX_LOG(log_error, "Failed to allocate UBFH %ld bytes: %s",
                     NDRX_MSGSIZEMAX, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        if ( EXSUCCEED!=ndrx_tpjsontoubf((UBFH *)*obuf, NULL, data_object) )
        {
            NDRX_LOG(log_error, "Failed to import UBF buffer");
            ret=EXFAIL;
            goto out;
        }
    }
    else if ( 0 == strcmp(BUF_TYPE_VIEW_STR, buftype))
    {
        /* TODO */
    }
    else if ( 0 == strcmp(BUF_TYPE_CARRAY_STR, buftype))
    {
        /* TODO */
    }
    else if ( 0 == strcmp(BUF_TYPE_JSON_STR, buftype))
    {
        /* TODO */
    }
    else
    {
        ret=EXFAIL;
        goto out;
    }

out:

    /* cleanup code */
    exjson_value_free(root_value);

    NDRX_LOG(log_debug, "%s: return %d", __func__, ret);

    return ret;
}

extern NDRX_API int ndrx_tpexportex(ndrx_expbufctl_t *bufctl, 
        char *ibuf, long ilen, char *ostr, long *olen, long flags)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_debug, "%s: enter", __func__);


    NDRX_LOG(log_debug, "%s: return %d", __func__, ret);

    return ret;
}
