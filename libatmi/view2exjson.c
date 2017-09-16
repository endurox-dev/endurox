/* 
** view<->json conversion lib
**
** @file view2exjson.c
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
*/#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#include <userlog.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <exparson.h>
#include <view2exjson.h>
#include <ubf.h>
#include <atmi_int.h>
#include <typed_buf.h>

#include "tperror.h"


/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
#define IS_INT(X) (BFLD_CHAR == X || BFLD_SHORT == X || BFLD_LONG == X || BFLD_INT == X)
#define IS_NUM(X) (BFLD_SHORT == X || BFLD_LONG == X || BFLD_FLOAT == X || BFLD_DOUBLE == X)
#define IS_BIN(X) (BFLD_CARRAY == X)

/* TODO: Fix atmi buffer size to match size of ATMI buffer size. */
#define CARR_BUFFSIZE       ATMI_MSG_MAX_SIZE
#define CARR_BUFFSIZE_B64   (4 * (CARR_BUFFSIZE) / 3)
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/


exprivate long round_long( double r ) {
    return (r > 0.0) ? (r + 0.5) : (r - 0.5); 
}

/**
 * Convert JSON text buffer to UBF
 * @param p_ub - UBF buffer to fill data in
 * @param buffer - json text to parse
 * @return SUCCEED/FAIL
 */
expublic char* ndrx_tpjsontoview(char *view, char *buffer)
{
    int ret = EXSUCCEED;
    EXJSON_Value *root_value;
    EXJSON_Object *root_object;
    
    EXJSON_Value *view_value;
    EXJSON_Object *view_object;
    
    EXJSON_Value *val;
    EXJSON_Array *array_val;
    size_t i, cnt, j, arr_cnt;
    int type;
    char *name;
    char    *str_val;
    double d_val;
    int f_type;
    short   bool_val;
    char bin_buf[CARR_BUFFSIZE+1];
    char    *s_ptr;
    long vsize;
    int cnametyp;
    char *cstruct = NULL;

    NDRX_LOG(log_debug, "Parsing buffer: [%s]", buffer);

    root_value = exjson_parse_string_with_comments(buffer);
    type = exjson_value_get_type(root_value);
    NDRX_LOG(log_debug, "Type is %d", type);

    if (exjson_value_get_type(root_value) != EXJSONObject)
    {
        NDRX_LOG(log_debug, "Failed to parse root element");
        ndrx_TPset_error_fmt(TPEINVAL, "exjson: Failed to parse root element");
        return NULL;
    }
    root_object = exjson_value_get_object(root_value);

    cnt = exjson_object_get_count(root_object);
    NDRX_LOG(log_debug, "cnt = %d", cnt);
    
    
    if (NULL==(name = (char *)exjson_object_get_name(root_object, 0)))
    {
	NDRX_LOG(log_error, "exjson: Invalid json no root VIEW object");
	ndrx_TPset_error_msg(TPEINVAL, "exjson: Invalid json no root VIEW object");
	EXFAIL_OUT(ret);
    }
    
    vsize = Bvsizeof(name);
    
    if (vsize < 0)
    {
        NDRX_LOG(log_error, "Failed to get view [%s] size: %s", 
                name, Bstrerror(Berror));
        
        ndrx_TPset_error_fmt(TPEINVAL, "Failed to get view [%s] size: %s", 
                name, Bstrerror(Berror));
        
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Allocating view [%s]: %ld", name, vsize);
    
    cstruct = tpalloc("VIEW", name, vsize);
    
    if (NULL==cstruct)
    {
        NDRX_LOG(log_error, "Failed to allocate view: %s", tpstrerror(tperrno));
        /* error must be set already! */
        EXFAIL_OUT(ret);
    }
    
    strcpy(view, name);
    
    view_object = exjson_object_get_object(root_object, name);
    
    if (NULL==view_object)
    {
        NDRX_LOG(log_error, "exjson: Failed to get view object");
        ndrx_TPset_error_msg(TPESYSTEM, "exjson: Failed to get view object");
        EXFAIL_OUT(ret);
    }

    cnt = exjson_object_get_count(view_object);
    NDRX_LOG(log_debug, "cnt = %d", cnt);
    
    for (i =0; i< cnt; i++)
    {
        name = (char *)exjson_object_get_name(view_object, i);

        NDRX_LOG(log_debug, "came: [%s]", name);
        
        if (EXFAIL==Bvoccur(cstruct, view, name, NULL, NULL, NULL, &cnametyp))
        {
            NDRX_LOG(log_error, "Error getting field %s.%s infos: %s",
                    view, name, Bstrerror(Berror));
            
            if (BNOCNAME==Berror)
            {
                NDRX_LOG(log_debug, "%s.%s not found in view -> ignore",
                        view, name);
                continue;
            }
            else
            {
                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get %s.%s infos: %s",
                        view, name, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }

        switch ((f_type=exjson_value_get_type(exjson_object_nget_value_n(view_object, i))))
        {
            case EXJSONString:
            {
                BFLDLEN str_len;
                s_ptr = str_val = (char *)exjson_object_get_string_n(view_object, i);
                NDRX_LOG(log_debug, "Str Value: [%s]", str_val);

                /* If it is carray - parse hex... */
                if (IS_BIN(cnametyp))
                {
                    size_t st_len;
                    NDRX_LOG(log_debug, "Field is binary..."
                            " convert from b64...");

                    if (NULL==atmi_base64_decode(str_val,
                            strlen(str_val),
                            &st_len,
                            bin_buf))
                    {
                        NDRX_LOG(log_debug, "Failed to "
                                "decode base64!");
                        
                        ndrx_TPset_error_fmt(TPEINVAL, "Failed to "
                                "decode base64: %s", name);
                        
                        EXFAIL_OUT(ret);
                    }
                    str_len = st_len;
                    s_ptr = bin_buf;
                    NDRX_LOG(log_debug, "got binary len [%d]", str_len);
                }
                else
                {
                    str_len = strlen(s_ptr);
                }

                if (EXSUCCEED!=CBvchg(cstruct, view, name, 0, s_ptr, 
                        str_len, BFLD_CARRAY))
                {
                    NDRX_LOG(log_error, "Failed to set view field %s.%s: %s",
                            view, name, Bstrerror(Berror));
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set view field %s.%s: %s",
                            view, name, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                
                break;
            }
            case EXJSONNumber:
            {
                long l;
                d_val = exjson_object_get_number_n(view_object, i);
                NDRX_LOG(log_debug, "Double Value: [%lf]", d_val);

                if (IS_INT(cnametyp))
                {
                    l = round_long(d_val);
                    if (EXSUCCEED!=CBvchg(cstruct, view, name, 0, 
                            (char *)&l, 0L, BFLD_LONG))
                    {
                        NDRX_LOG(log_error, "Failed to set [%s] to [%ld]!", 
                            name, l);
                        
                        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] to [%ld]!", 
                            name, l);
                        
                        EXFAIL_OUT(ret);
                    }
                }
                else if (EXSUCCEED!=CBvchg(cstruct, view, name, 0, 
                        (char *)&d_val, 0L, BFLD_DOUBLE))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] to [%lf]: %s", 
                            name, d_val, Bstrerror(Berror));
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] to [%lf]: %s", 
                            name, d_val, Bstrerror(Berror));
                    
                    EXFAIL_OUT(ret);
                }
            }
                    break;
            case EXJSONBoolean:
            {
                bool_val = (short)exjson_object_get_boolean_n(view_object, i);
                NDRX_LOG(log_debug, "Bool Value: [%hd]", bool_val);
                if (EXSUCCEED!=CBvchg(cstruct, view, name, 0, 
                        (char *)&bool_val, 0L, BFLD_SHORT))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] to [%hd]: %s", 
                            name, bool_val, Bstrerror(Berror));
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] to [%hd]: %s", 
                            name, bool_val, Bstrerror(Berror));
                    
                    EXFAIL_OUT(ret);
                }
            }
            break;
            /* Fielded buffer fields with more than one occurrance will go to array: 
             * Stuff here is almost identicial to above!
             */
            case EXJSONArray:
            {
                if (NULL==(array_val = exjson_value_get_array(
                        exjson_object_nget_value_n(view_object, i))))
                {
                    NDRX_LOG(log_error, "Failed to get array object!");
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get array object!");
                    EXFAIL_OUT(ret);
                }
                arr_cnt = exjson_array_get_count(array_val);

                for (j = 0; j<arr_cnt; j++ )
                {
                    switch (f_type = exjson_value_get_type(
                            exjson_array_get_value(array_val, j)))
                    {
                        case EXJSONString:
                        {
                            BFLDLEN str_len;
                            s_ptr = str_val = (char *)exjson_array_get_string(array_val, j);
                            NDRX_LOG(log_debug, 
                                        "Array j=%d, Str Value: [%s]", j, str_val);

                            /* If it is carray - parse hex... */
                            if (IS_BIN(cnametyp))
                            {
                                size_t st_len;
                                if (NULL==atmi_base64_decode(str_val,
                                        strlen(str_val),
                                        &st_len,
                                        bin_buf))
                                {
                                    NDRX_LOG(log_debug, "Failed to "
                                            "decode base64!");
                                    
                                    ndrx_TPset_error_fmt(TPEINVAL, "Failed to "
                                            "decode base64!");
                                    
                                    EXFAIL_OUT(ret);
                                }
                                str_len = st_len;
                                s_ptr = bin_buf;
                                NDRX_LOG(log_debug, "got binary len [%d]", str_len);
                            }
                            else
                            {
                                str_len = strlen(s_ptr);
                            }

                            if (EXSUCCEED!=CBvchg(cstruct, view, name, j, 
                                    s_ptr, str_len, BFLD_CARRAY))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%s]: %s", 
                                        name, str_val, Bstrerror(Berror));
                                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] "
                                        "to [%s]: %s", 
                                        name, str_val, Bstrerror(Berror));
                                
                                EXFAIL_OUT(ret);
                            }
                        }
                        break;
                        case EXJSONNumber:
                        {
                            long l;
                            d_val = exjson_array_get_number(array_val, j);
                            NDRX_LOG(log_debug, "Array j=%d, Double Value: [%lf]", j, d_val);

                            if (IS_INT(cnametyp))
                            {
                                l = round_long(d_val);
                                NDRX_LOG(log_debug, "Array j=%d, Long value: [%ld]", j, l);
                                if (EXSUCCEED!=CBvchg(cstruct, view, name, j, 
                                        (char *)&l, 0L, BFLD_LONG))
                                {
                                        NDRX_LOG(log_error, "Failed to set [%s] to [%ld]: %s", 
                                                name, l, Bstrerror(Berror));
                                        
                                        ndrx_TPset_error_fmt(TPESYSTEM, 
                                                "Failed to set [%s] to [%ld]: %s", 
                                                name, l, Bstrerror(Berror));
                                        
                                        EXFAIL_OUT(ret);
                                }
                            }
                            else if (EXSUCCEED!=CBvchg(cstruct, view, name, j, 
                                    (char *)&d_val, 0L, BFLD_DOUBLE))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%lf]: %s", 
                                        name, d_val, Bstrerror(Berror));
                                
                                ndrx_TPset_error_fmt(TPESYSTEM,"Failed to set "
                                        "[%s] to [%lf]: %s", 
                                        name, d_val, Bstrerror(Berror));
                                
                                EXFAIL_OUT(ret);
                            }
                        }
                        break;
                        case EXJSONBoolean:
                        {
                            bool_val = (short)exjson_array_get_boolean(array_val, j);
                            NDRX_LOG(log_debug, "Array j=%d, Bool Value: [%hd]", j, bool_val);
                            if (EXSUCCEED!=CBvchg(cstruct, view, name, j, 
                                    (char *)&bool_val, 0L, BFLD_SHORT))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%hd]: %s", 
                                        name, bool_val, Bstrerror(Berror));
                                
                                ndrx_TPset_error_fmt(TPESYSTEM,"Failed to set "
                                        "[%s] to [%hd]: %s", 
                                        name, bool_val, Bstrerror(Berror));
                                
                                EXFAIL_OUT(ret);
                            }
                        }
                        default:
                            NDRX_LOG(log_error, 
                                        "Unsupported array elem "
                                        "type: %d", f_type);                            
                        break;
                    }
                }

            }
            break;
            default:
            {
                NDRX_LOG(log_error, "Unsupported type: %d", f_type);
            }
            break;

        }
    }
    
out:
    /* cleanup code */
    exjson_value_free(root_value);

    if (EXSUCCEED!=ret && NULL!=cstruct)
    {
        tpfree(cstruct);
        cstruct = NULL;
    }

    
    return cstruct;
}


/**
 * Build json text from UBF buffer
 * @param p_ub  JSON buffer
 * @param buffer output json buffer
 * @param bufsize       output buffer size
 * @param flags BVACCESS_NOTNULL -> return only non NULL values, if not set,
 * return all
 * @return SUCCEED/FAIL 
 */
expublic int ndrx_tpviewtojson(char *cstruct, char *view, char *buffer, 
        int bufsize, long flags)
{
    int ret = EXSUCCEED;
    int occs;
    int is_array;
    double d_val;
    char strval[CARR_BUFFSIZE+1]; 
    char b64_buf[CARR_BUFFSIZE_B64+1];
    int is_num;
    char *s_ptr;
    BFLDLEN flen;
    
    Bvnext_state_t state;
    char cname[NDRX_VIEW_CNAME_LEN+1];
    int fldtype;
    BFLDOCC maxocc;
    long dim_size;
    
    EXJSON_Value *root_value = exjson_value_init_object();
    EXJSON_Object *root_object = exjson_value_get_object(root_value);
    
    EXJSON_Value *view_value = exjson_value_init_object();
    EXJSON_Object *view_object = exjson_value_get_object(view_value);
    
    
    char *serialized_string = NULL;
    BFLDOCC oc;

    EXJSON_Array * jarr;
    
    if( EXJSONSuccess != exjson_object_dotset_value(root_object, view, view_value) )
    {	
        NDRX_LOG(log_error, "exjson: Failed to set root value");
        ndrx_TPset_error_msg(TPESYSTEM, "exjson: Failed to set root value");
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==(ret=Bvnext(&state, view, cname, &fldtype, &maxocc, &dim_size)))
    {
        NDRX_LOG(log_error, "Failed to iterate VIEW: %s", Bstrerror(Berror));
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to iterate VIEW: %s",  
                Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
    while (ret)
    {
        int fulloccs;
        int realoccs;
        /* Get real occurrences */
        if (EXFAIL==(fulloccs=Bvoccur(cstruct, view, cname, NULL, &realoccs, NULL, NULL)))
        {
            NDRX_LOG(log_error, "Failed to get view field %s.%s infos: %s", 
                    view, cname, Bstrerror(Berror));
            ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get view field %s.%s infos: %s", 
                    view, cname, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (flags & BVACCESS_NOTNULL)
        {
            occs = realoccs;
            NDRX_LOG(log_dump, "Using REAL (non null) occs: %s", occs);
        }
        else
        {
            occs = fulloccs;
            
            NDRX_LOG(log_dump, "Using set occs: %s", occs);
        }
        
        for (oc=0; oc<occs; oc++)
        {
            NDRX_LOG(log_debug, "Field: [%s] occ %d", cname, oc);
            if (0==oc)
            {
                if (occs>1)
                {
                    /* create array */
                    is_array = EXTRUE;
                    if (NULL==(jarr = exjson_array_init()))
                    {
                            NDRX_LOG(log_error, "Failed to initialize array!");

                            ndrx_TPset_error_msg(TPESYSTEM, "Failed to initialize array!");
                            EXFAIL_OUT(ret);
                    }
                    /* add array to document... */
                    if (EXJSONSuccess!=exjson_object_set_array(view_object, cname, jarr))
                    {
                            NDRX_LOG(log_error, "exjson: Failed to add Array to root object!!");
                            ndrx_TPset_error_msg(TPESYSTEM, "exjson: Failed to add "
                                    "Array to root object!!");
                            EXFAIL_OUT(ret);

                            EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    is_array = EXFALSE;
                }
            }
            else
            {
                is_array = EXTRUE;
            }

            if (IS_NUM(fldtype))
            {
                if (EXSUCCEED!=CBvget(cstruct, view, cname, oc, 
                        (char *)&d_val, 0L, BFLD_DOUBLE, 0))
                {
                    NDRX_LOG(log_error, "Failed to get (double): %s.%s/%d: %s",
                                                    view, cname, oc, Bstrerror(Berror));

                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get (double): %s.%s/%d: %s",
                                                    view, cname, oc, Bstrerror(Berror));
                    EXFAIL_OUT(ret);
                }
                is_num = EXTRUE;
                NDRX_LOG(log_debug, "Numeric value: %lf", d_val);
            }
            else
            {
                is_num = EXFALSE;
                flen = sizeof(strval);
                if (EXSUCCEED!=CBvget(cstruct, view, cname, oc, 
                        strval, &flen, BFLD_CARRAY, 0))
                {
                    NDRX_LOG(log_error, "Failed to get (string): %s.%s/%d: %s",
                                            view, cname, oc, Bstrerror(Berror));

                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get (string): %s.%s/%d: %s",
                                            view, cname, oc, Bstrerror(Berror));

                    EXFAIL_OUT(ret);
                }

                /* If it is carray, then convert to hex... */
                if (IS_BIN(fldtype))
                {
                    size_t outlen;
                    NDRX_LOG(log_debug, "Field is binary... convert to b64");

                    if (NULL==atmi_base64_encode((unsigned char *)strval, flen, 
                                &outlen, b64_buf))
                    {
                        NDRX_LOG(log_error, "Failed to convert to b64!");

                        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to convert to b64!");

                        EXFAIL_OUT(ret);
                    }
                    b64_buf[outlen] = EXEOS;
                    s_ptr = b64_buf;

                }
                else
                {
                    strval[flen] = EXEOS;
                    s_ptr = strval;
                }

                NDRX_LOG(log_debug, "String value: [%s]", s_ptr);
            }

            if (is_array)
            {
                    /* Add array element 
                    exjson_object_set_value */

                    /* Add normal element */
                    if (is_num)
                    {
                        if (EXJSONSuccess!=exjson_array_append_number(jarr, d_val))
                        {
                            NDRX_LOG(log_error, "Failed to set array elem to [%lf]!", 
                                    d_val);

                            ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set array "
                                    "elem to [%lf]!", d_val);

                            EXFAIL_OUT(ret);
                        }
                    }
                    else
                    {
                        if (EXJSONSuccess!=exjson_array_append_string(jarr, s_ptr))
                        {
                            NDRX_LOG(log_error, "Failed to set array elem to [%s]!", 
                                    s_ptr);

                            ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set array "
                                    "elem to [%s]!", s_ptr);

                            EXFAIL_OUT(ret);
                        }
                    }

            }
            else
            {
                /* Add normal element */
                if (is_num)
                {
                    if (EXJSONSuccess!=exjson_object_set_number(view_object, cname, d_val))
                    {
                        NDRX_LOG(log_error, "Failed to set [%s] value to [%lf]!",
                                            cname, d_val);

                        ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set [%s] "
                                "value to [%lf]!", cname, d_val);

                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    if (EXJSONSuccess!=exjson_object_set_string(view_object, cname, s_ptr))
                    {
                        NDRX_LOG(log_error, "Failed to set [%s] value to [%s]!",
                                        cname, s_ptr);

                        ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set [%s] "
                                "value to [%s]!", cname, s_ptr);

                        EXFAIL_OUT(ret);
                    }
                }
            }
        } /* for occ */
	
	if (EXFAIL==(ret=Bvnext(&state, NULL, cname, &fldtype, &maxocc, &dim_size)))
	{
	    NDRX_LOG(log_error, "Failed to iterate VIEW: %s", Bstrerror(Berror));
	    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to iterate VIEW: %s",  
		    Bstrerror(Berror));
	    EXFAIL_OUT(ret);
	}

    } /* while ret */ 

    serialized_string = exjson_serialize_to_string(root_value);

    if (strlen(serialized_string) <= bufsize )
    {
	    
	NDRX_STRNCPY_SAFE(buffer, serialized_string, bufsize-1);
        NDRX_LOG(log_debug, "Got JSON: [%s]", buffer);
    }
    else
    {
        NDRX_LOG(log_error, "Buffer too short: Got json size: [%d] buffer size: [%d]", 
                strlen(serialized_string), bufsize);
        
        ndrx_TPset_error_fmt(TPEOS, "Buffer too short: Got json size: "
                "[%d] buffer size: [%d]",  strlen(serialized_string), bufsize);
        
        EXFAIL_OUT(ret);
    }

out:

    if (NULL!=serialized_string)
    {
        exjson_free_serialized_string(serialized_string);
    }

    if (NULL!=root_value)
    {
        exjson_value_free(root_value);
    }

    /* At iter end, ret normally becomes 0, thus fine here as SUCCEED */
    return ret;
}

/**
 * auto-buffer convert func. view->ubf
 * @param buffer
 * @return 
 */
expublic int typed_xcvt_json2view(buffer_obj_t **buffer)
{
    int ret = EXSUCCEED;
    buffer_obj_t *tmp_b;
    /* Allocate the max UBF buffer */
    char * tmp = NULL;
    char view[NDRX_VIEW_NAME_LEN+1];
    
    /* Do the convert */
    ndrx_TPunset_error();
    if (NULL==(tmp=ndrx_tpjsontoview(view, (*buffer)->buf)))
    {
        NDRX_LOG(log_error, "Failed to convert JSON->VIEW: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    tmp_b=ndrx_find_buffer((char *)tmp);
    tmp_b->autoalloc = (*buffer)->autoalloc;

    /* finally return the buffer */
    NDRX_LOG(log_info, "Returning new buffer %p", tmp_b);
    *buffer = tmp_b;
out:
    return ret;
}


/**
 * auto-buffer convert func. view->json
 * @param buffer
 * @param flags - conversion mode 0 (incl NULL0, BVACCESS_NOTNULL - not null
 * @return 
 */
expublic int typed_xcvt_view2json(buffer_obj_t **buffer, long flags)
{
    int ret = EXSUCCEED;
    buffer_obj_t *tmp_b;
    char type[XATMI_SUBTYPE_LEN+1];
    char subtype[XATMI_TYPE_LEN+1]={EXEOS};
   

    char * tmp = NULL;
    char * newbuf_out = NULL; /* real output buffer */

    if (NULL==(tmp = tpalloc("JSON", NULL, ATMI_MSG_MAX_SIZE)))
    {
        NDRX_LOG(log_error, "failed to convert UBF->JSON. JSON buffer alloc fail!: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    /* Get view name... */
    
    if (EXFAIL==tptypes((*buffer)->buf, type, subtype))
    {
         NDRX_LOG(log_error, "Failed to get view infos: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_debug, "Got types %s/%s", type, subtype);

    /* Do the convert */
    ndrx_TPunset_error();
    if (EXSUCCEED!=ndrx_tpviewtojson((*buffer)->buf, 
            subtype, tmp, ATMI_MSG_MAX_SIZE, flags))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to convert VIEW->JSON: %s", 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* Shrink the buffer (by reallocating) new! 
     * we will do the shrink because, msg Q might not have settings set for
     * max buffer size...
     */
    if (NULL==(newbuf_out = tpalloc("JSON", NULL, strlen(tmp)+1)))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to alloc output JSON %ld: %s", strlen(tmp)+1, 
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    strcpy(newbuf_out, tmp);

    tmp_b=ndrx_find_buffer((char *)newbuf_out);
    tmp_b->autoalloc = (*buffer)->autoalloc;

    /* Kill the buffers */
    tpfree((*buffer)->buf);
    tpfree((char *)tmp);

    /* finally return the buffer */
    NDRX_LOG(log_info, "Returning new buffer %p", tmp_b->buf);
    
    *buffer = tmp_b;
out:
    return ret;
}
