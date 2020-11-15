/**
 * @brief ATMI tpexport function implementation (Common version)
 *
 * @file tpexport.c
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
#define CARR_BUFFSIZE_B64	(4 * (CARR_BUFFSIZE) / 3)
/* Support #553 fixed */
#define CARR_BUFFSIZE_B64_EOS   (CARR_BUFFSIZE_B64+1)
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Export Any buffer to JSON format
 * @param bufctl
 * @param ibuf
 * @param ilen
 * @param ostr
 * @param olen
 * @param flags
 * @param parent_root_object if exporting this as sub-buffer (PTR) in already in progress export
 * @return 
 */
extern NDRX_API int ndrx_tpexportex(ndrx_expbufctl_t *bufctl, 
            char *ibuf, long ilen, char *ostr, long *olen, long flags, 
            EXJSON_Object *parent_root_object)
{
    int ret=EXSUCCEED;
    char buftype[16+1]={EXEOS};
    char subtype[XATMI_SUBTYPE_LEN]={EXEOS};
    size_t outlen;
    char *b64_buf=NULL;
    long size_existing=EXFAIL;

    EXJSON_Value *root_value = NULL;
    EXJSON_Object *root_object = NULL;
    
    EXJSON_Value *data_value = NULL;
    EXJSON_Object *data_object = NULL;
    
    char *serialized_string = NULL;

    NDRX_LOG(log_debug, "%s: enter", __func__);
    
    if ( NULL == parent_root_object )
    {
        root_value = exjson_value_init_object();
        
        if (NULL==root_value)
        {
            ndrx_TPset_error_fmt(TPESYSTEM, "Failed to init json object value - mem issue?");
            EXFAIL_OUT(ret);
        }
        
        root_object = exjson_value_get_object(root_value);
    }
    else
    {
        root_object = parent_root_object;
    }
    
    b64_buf=NDRX_MALLOC(CARR_BUFFSIZE_B64_EOS);
    
    if (NULL==b64_buf)
    {
        NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
            CARR_BUFFSIZE_B64_EOS, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* how about carray ilen? */
    if (EXFAIL==(size_existing=ndrx_tptypes(ibuf, buftype, subtype)))
    {
        NDRX_LOG(log_error, "Cannot determine buffer type");
        EXFAIL_OUT(ret);
    }

    NDRX_LOG(log_debug, 
             "Got buftype=[%s] subtype=[%s] size=[%ld]",
             buftype, subtype, size_existing);

    if (EXJSONSuccess!=exjson_object_set_string(root_object, "buftype", buftype))
    {
        NDRX_LOG(log_error, "Failed to set buftype data");
        EXFAIL_OUT(ret);
    }

    if ( EXJSONSuccess!=exjson_object_set_number(root_object, "version", TPIMPEXP_VERSION_MAX) ) 
    {
        NDRX_LOG(log_error, "Failed to set version data");
        EXFAIL_OUT(ret);
    }
    if ( strlen(subtype)>0 )
    {
        if (EXJSONSuccess!=exjson_object_set_string(root_object, "subtype", subtype))
        {
            NDRX_LOG(log_error, "Failed to set subtype data");
            EXFAIL_OUT(ret);
        }
    }

    if ( 0==strcmp(BUF_TYPE_STRING_STR, buftype) )
    {
        if (EXJSONSuccess!=exjson_object_set_string(root_object, "data", ibuf))
        {
            NDRX_LOG(log_error, "Failed to set string data=[%s]", ibuf);
            EXFAIL_OUT(ret);
        }
    }
    else if ( 0 == strcmp(BUF_TYPE_UBF_STR, buftype))
    {
        if (NULL==(data_value = exjson_value_init_object()))
        {
            NDRX_LOG(log_error, "Failed to init data_value");
            EXFAIL_OUT(ret);
        }

        if (NULL==(data_object = exjson_value_get_object(data_value)))
        {
            NDRX_LOG(log_error, "Failed to init data_object");
            EXFAIL_OUT(ret);
        }

        if (EXSUCCEED!=ndrx_tpubftojson((UBFH *)ibuf, NULL, 0, data_object))
        {
            NDRX_LOG(log_error, "Failed to build data object from UBF!!!!");
            EXFAIL_OUT(ret);
        }
        
        if (EXJSONSuccess!=exjson_object_set_value(root_object,"data",data_value))
        {
            /* clean up objects? */
            exjson_value_free(data_value);
                    
            NDRX_LOG(log_error, "Failed to set data object with UBF!!!!");
            EXFAIL_OUT(ret);
        }
    }
    else if ( 0 == strcmp(BUF_TYPE_VIEW_STR, buftype))
    {
        if (NULL==(data_value = exjson_value_init_object()))
        {
            NDRX_LOG(log_error, "Failed to init data_value");
            EXFAIL_OUT(ret);
        }

        if (NULL==(data_object = exjson_value_get_object(data_value)))
        {
            NDRX_LOG(log_error, "Failed to init data_object");
            EXFAIL_OUT(ret);
        }
        if (EXSUCCEED!=ndrx_tpviewtojson((char *)ibuf, subtype, NULL, 0L, 
                BVACCESS_NOTNULL, data_object))
        {
            NDRX_LOG(log_error, "Failed to build data object from VIEW!!!!");
            EXFAIL_OUT(ret);
        }
        if (EXJSONSuccess!=exjson_object_set_value(root_object,"data",data_value))
        {
            NDRX_LOG(log_error, "Failed to set data object with VIEW!!!!");
            EXFAIL_OUT(ret);
        }
    }
    else if ( 0 == strcmp(BUF_TYPE_CARRAY_STR, buftype))
    {
        NDRX_LOG(log_debug, "ibuf is binary... convert to b64");
        outlen = CARR_BUFFSIZE_B64_EOS;
        if (NULL==ndrx_base64_encode((unsigned char *)ibuf, size_existing, 
                &outlen, b64_buf))
        {
                NDRX_LOG(log_error, "Failed to convert to b64!");
                EXFAIL_OUT(ret);
        }

        /* TODO: Where is EOS??? */
        
        if (EXJSONSuccess!=exjson_object_set_string(root_object, "data", b64_buf))
        {
            NDRX_LOG(log_error, "Failed to set carray data=[%s]", b64_buf);
            EXFAIL_OUT(ret);
        }
    }
    else if ( 0 == strcmp(BUF_TYPE_JSON_STR, buftype))
    {
        if (NULL==(data_value = exjson_parse_string(ibuf)))
        {
            NDRX_LOG(log_error, "Failed to parse ibuf");
            EXFAIL_OUT(ret);
        }
/*
        if (NULL==(data_object = exjson_value_get_object(data_value)))
        {
            NDRX_LOG(log_error, "Failed to init data_object");
            EXFAIL_OUT(ret);
        }
 */
        if (EXJSONSuccess!=exjson_object_set_value(root_object,"data",data_value))
        {
            NDRX_LOG(log_error, "Failed to set data object with JSON!!!!");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        EXFAIL_OUT(ret);
    }

    if (NULL==parent_root_object)
    {
        serialized_string = exjson_serialize_to_string(root_value);
        if (strlen(serialized_string) < *olen)
        {
            NDRX_LOG(log_debug, "Return JSON: [%s]", serialized_string);

            if ( TPEX_STRING == flags )
            {
                NDRX_LOG(log_debug, "convert to b64");
                outlen = *olen;
                if (NULL==ndrx_base64_encode((unsigned char *)serialized_string, 
                            strlen(serialized_string), &outlen, ostr))
                {
                    NDRX_LOG(log_error, "Failed to convert to b64!");
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                NDRX_STRCPY_SAFE_DST(ostr, serialized_string, *olen);
                *olen = strlen(ostr)+1;
            }
        }
        else
        {
            NDRX_LOG(log_error, "olen too short: ostr size: [%d] olen size: [%d]", 
                    strlen(serialized_string)+1, *olen);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_debug, "Not serializing as having parent_root_object");
    }

out:

    NDRX_LOG(log_debug, "%s: return %d", __func__, ret);

    if (NULL!=serialized_string)
    {
        exjson_free_serialized_string(serialized_string);
    }

    if (NULL==parent_root_object && NULL!=root_value)
    {
        exjson_value_free(root_value);
    }
    
    if (NULL!=b64_buf)
    {
        NDRX_FREE(b64_buf);
    }

    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
