/* 
** ATMI tpimport function implementation (Common version)
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
#include <view2exjson.h>
#include <exbase64.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define CARR_BUFFSIZE       NDRX_MSGSIZEMAX
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * NOTE That this will always if case of success, reallocate the buffer
 * to new node - that will be manual buffer. If original buf, was auto,
 * then it is not freed. If it was manual buffer, then tpfree is done on it.
 * BE AWARE!
 * @param bufctl
 * @param istr
 * @param ilen
 * @param obuf
 * @param olen
 * @param flags
 * @return 
 */
expublic int ndrx_tpimportex(ndrx_expbufctl_t *bufctl,
        char *istr, long ilen, char **obuf, long *olen, long flags)
{
    int ret=EXSUCCEED;
    char *buftype;
    char buftype_obuf[XATMI_TYPE_LEN+1];
    char *subtype;
    char subtype_obuf[XATMI_SUBTYPE_LEN+1];
    double version;
    char *data;
    char data_bin[CARR_BUFFSIZE+1];
    char *serialized_data=NULL;
    UBFH *p_ub=NULL;
    long size_used=EXFAIL;
    long size_existing=EXFAIL;
    long new_size=EXFAIL;
    char *obuftemp=NULL;

    int type;
    char *str_val;
    EXJSON_Value *root_value=NULL;
    EXJSON_Object *root_object=NULL;
    EXJSON_Value *data_value=NULL;
    EXJSON_Object *data_object=NULL;

    NDRX_LOG(log_debug, "%s: enter", __func__);

    /* TODO Check flag if base64 then decode from base64 */

    NDRX_LOG(log_debug, "Parsing buffer: [%s]", istr);

    if ( NULL == (root_value = exjson_parse_string_with_comments(istr)))
    {
        NDRX_LOG(log_error, "Failed to parse istr");
        EXFAIL_OUT(ret);
    }
    type = exjson_value_get_type(root_value);
    NDRX_LOG(log_error, "Type is %d", type);

    if (exjson_value_get_type(root_value) != EXJSONObject)
    {
        NDRX_LOG(log_error, "Failed to parse root element");
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: Failed to parse json root element");
        EXFAIL_OUT(ret);
    }

    root_object = exjson_value_get_object(root_value);
    if (NULL==(buftype = (char *)exjson_object_get_string(root_object, "buftype" )))
    {
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: missing buftype in input buffer");
        EXFAIL_OUT(ret);
    }
    if (NULL==(subtype = (char *)exjson_object_get_string(root_object, "subtype" )) &&
        0 == strcmp(BUF_TYPE_VIEW_STR, buftype))
    {
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: missing subtype for view");
        EXFAIL_OUT(ret);
    }
    if (0==(version = exjson_object_get_number(root_object, "version" )))
    {
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: missing version in input buffer");
        EXFAIL_OUT(ret);
    }
    
    if ( version<TPIMPEXP_VERSION_MIN || version>TPIMPEXP_VERSION_MAX )
    {
        ndrx_TPset_error_fmt(TPEINVAL, 
                             "tpimport: version, expected min version %d max %d",
                             TPIMPEXP_VERSION_MIN, TPIMPEXP_VERSION_MAX);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, 
             "buftype: [%s] subtype: [%s] version: [%f]", 
             buftype, subtype?subtype:"null", version?version:0);
    
    if ( NULL != bufctl )
    {
        memset(bufctl, 0, sizeof(ndrx_expbufctl_t));
        NDRX_STRCPY_SAFE(bufctl->buftype, buftype);
        bufctl->buftype_ind = EXTRUE;

        bufctl->version = version;
        bufctl->version_ind = EXTRUE;
    }

    if (NULL!=*obuf)
    {
        if ( EXFAIL == (size_existing = ndrx_tptypes(*obuf, buftype_obuf, subtype_obuf)) )
        {
            NDRX_LOG(log_error, "Cannot determine buffer type");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        size_existing = EXFAIL;
    }

    /* 1. check the import buffer type match with passed obuf
     if types changed and no change passed, then reject import */
    if ( (NULL!=*obuf) && TPEX_NOCHANGE == flags && 0!=(strcmp(buftype,buftype_obuf)) )
    {
        ndrx_TPset_error_fmt(TPEINVAL, 
                    "tpimport: import buffer type [%s] not match "
                    "with passed obuf type [%s]", buftype, buftype_obuf);
        EXFAIL_OUT(ret);
    }
    if ( 0==strcmp(BUF_TYPE_STRING_STR, buftype) )
    {
        if ( NULL == (obuftemp = ndrx_tpalloc(NULL, buftype, NULL, NDRX_MSGSIZEMAX)) )
        {
            NDRX_LOG(log_error, "Failed to allocate %s %ld bytes: %s", 
                     buftype, NDRX_MSGSIZEMAX, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        if (NULL ==( data = (char *)exjson_object_get_string(root_object, "data" )) )
        {
            NDRX_LOG(log_error, "Failed get data for string import");
            EXFAIL_OUT(ret);
        }
        size_used = (long)strlen(data)+1;
        NDRX_STRNCPY_SAFE(obuftemp, data, size_used);
    }
    else if ( 0 == strcmp(BUF_TYPE_UBF_STR, buftype))
    {
        if ( NULL == (obuftemp = ndrx_tpalloc(NULL, buftype, NULL, NDRX_MSGSIZEMAX)) )
        {
            NDRX_LOG(log_error, "Failed to allocate %s %ld bytes: %s", 
                     buftype, NDRX_MSGSIZEMAX, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        if (NULL == (data_object = exjson_object_get_object(root_object, "data" )) )
        {
            NDRX_LOG(log_error, "Failed get data for UBF import");
            EXFAIL_OUT(ret);
        }
        if ( EXSUCCEED!=ndrx_tpjsontoubf((UBFH *)obuftemp, NULL, data_object) )
        {
            NDRX_LOG(log_error, "Failed to import UBF buffer");
            EXFAIL_OUT(ret);
        }
        size_used = Bused((UBFH *)obuftemp);
    }
    else if ( 0 == strcmp(BUF_TYPE_VIEW_STR, buftype))
    {
        if (NULL == (data_object = exjson_object_get_object(root_object, "data" )) )
        {
            NDRX_LOG(log_error, "Failed get data for VIEW import");
            EXFAIL_OUT(ret);
        }
        obuftemp=ndrx_tpjsontoview(subtype, NULL, data_object);
        if ( NULL==obuftemp )
        {
            NDRX_LOG(log_error, "Failed to import VIEW");
            EXFAIL_OUT(ret);
        }
        size_used = Bvsizeof(subtype);
    }
    else if ( 0 == strcmp(BUF_TYPE_CARRAY_STR, buftype))
    {
        if ( NULL == (data = (char *)exjson_object_get_string(root_object, "data" )) )
        {
            NDRX_LOG(log_error, "Failed get data for CARRAY import");
            EXFAIL_OUT(ret);
        }

        if ( NULL == (obuftemp = ndrx_tpalloc(NULL, buftype, NULL, NDRX_MSGSIZEMAX)))
        {
            NDRX_LOG(log_error, "Failed to allocate carray buffer %ld bytes: %s",
                     (*olen), tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }

        if (NULL==ndrx_base64_decode(data, strlen(data), (size_t*)&size_used, obuftemp))
        {
            NDRX_LOG(log_error, "Failed to decode CARRAY");
            EXFAIL_OUT(ret);
        }
        new_size = size_existing=size_used;
    }
    else if ( 0 == strcmp(BUF_TYPE_JSON_STR, buftype))
    {
        if ( NULL == (obuftemp = ndrx_tpalloc(NULL, buftype, NULL, NDRX_MSGSIZEMAX)) )
        {
            NDRX_LOG(log_error, "Failed to allocate %s %ld bytes: %s", 
                     buftype, NDRX_MSGSIZEMAX, tpstrerror(tperrno));
            EXFAIL_OUT(ret);
        }
        if (NULL == (data_value = exjson_object_get_value(root_object, "data" )) )
        {
            NDRX_LOG(log_error, "Failed get data for JSON import");
            EXFAIL_OUT(ret);
        }
        if (exjson_value_get_type(data_value) != EXJSONObject)
        {
            NDRX_LOG(log_error, "Failed to parse data element");
            ndrx_TPset_error_fmt(TPEINVAL, 
                                 "tpimport: Failed to parse json data element");
            EXFAIL_OUT(ret);
        }
        serialized_data = exjson_serialize_to_string(data_value);
        size_used = (long)strlen(serialized_data)+1;
        NDRX_STRNCPY_SAFE(obuftemp, serialized_data, size_used);
    }
    else
    {
        EXFAIL_OUT(ret);
    }
    
    if (size_existing > size_used)
    {
        new_size = size_existing;
    }
    else if (size_existing < size_used)
    {
        new_size = size_used;
    }
    NDRX_LOG(log_error, "new_size=[%ld] size_used=[%ld] size_existing=[%ld]",
         new_size, size_used, size_existing);
    
    if (EXFAIL!=new_size && NULL==(obuftemp = ndrx_tprealloc(obuftemp, new_size)))
    {
        EXFAIL_OUT(ret);
    }

    if (NULL!=olen)
    {
        *olen=new_size;
    }
    
    /* free up the but obuf if any */
    if ( NULL!=*obuf && !ndrx_tpisautobuf(*obuf) )
    {
        ndrx_tpfree(*obuf, NULL);
    }
    
    *obuf = obuftemp;
    
out:

    /* cleanup code */
    if (NULL!=root_value)
    {
        exjson_value_free(root_value);
    }

    if (NULL != serialized_data)
    {
        exjson_free_serialized_string(serialized_data);
    }
    NDRX_LOG(log_debug, "%s: return %d", __func__, ret);

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
