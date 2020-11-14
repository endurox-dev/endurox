/**
 * @brief ubf<->json conversion lib
 *
 * @file ubf2exjson.c
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
#include <fieldtable.h>
#include <exbase64.h>
#include <view2exjson.h>
#include "tperror.h"


/*------------------------------Externs---------------------------------------*/
/*------------------------------Macros----------------------------------------*/
#define IS_INT(X) (BFLD_CHAR == X || BFLD_SHORT == X || BFLD_LONG == X)
#define IS_NUM(X) (BFLD_SHORT == X || BFLD_LONG == X || BFLD_FLOAT == X || BFLD_DOUBLE == X)
#define IS_BIN(X) (BFLD_CARRAY == X)

/* TODO: Fix atmi buffer size to match size of ATMI buffer size. */
#define CARR_BUFFSIZE		NDRX_MSGSIZEMAX
#define CARR_BUFFSIZE_B64	(4 * (CARR_BUFFSIZE) / 3)
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/


exprivate long round_long( double r ) {
    return (r > 0.0) ? (r + 0.5) : (r - 0.5); 
}

/**
 * Load VIEW or UBF object
 * @param p_ub parent UBF into which load the data
 * @param fldnm field name in json (UBF field name)
 * @param fldid resolved filed id
 * @param fldtyp UBF field type
 * @param bin_buf temporary working space
 * @param bin_buf_len working space length
 * @param innerobj data object under the field in JSON
 * @param occ occurrence to set in UBF
 * @return EXSUCCED/EXFAIL (error loaded if any)
 */
exprivate int ndrx_load_object(UBFH *p_ub, char *fldnm, BFLDID fldid, int fldtyp, 
        char *bin_buf, size_t bin_buf_len, EXJSON_Object *innerobj, BFLDOCC occ)
{
    
    int ret = EXSUCCEED;
    
    if (BFLD_UBF==fldtyp)
    {
        UBFH *p_ub_tmp = (UBFH *)bin_buf;

        if (EXFAIL==Binit(p_ub_tmp, bin_buf_len))
        {
            ndrx_TPset_error_fmt(TPESYSTEM, 
                    "Failed to init temporary UBF for [%s]: %s", 
                    fldnm, Bstrerror(Berror));
            NDRX_LOG(log_error, "Failed to init temporary UBF for [%s]: %s", 
                    fldnm, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }


        if (EXSUCCEED!=ndrx_tpjsontoubf(p_ub_tmp, NULL, innerobj))
        {
            NDRX_LOG(log_error, "Failed to parse UBF json at field [%s]", 
                    fldnm);
            EXFAIL_OUT(ret);
        }

        /* Add UBF to buffer */
        if (EXSUCCEED!=Bchg(p_ub, fldid, occ, (char *)p_ub_tmp, 0L))
        {
            ndrx_TPset_error_fmt(TPESYSTEM, 
                    "Failed to add to parent UBF inner UBF [%s] (fldid=%d): %s", 
                    fldnm, fldid, Bstrerror(Berror));
            NDRX_LOG(log_error, "Failed to add to parent UBF inner UBF [%s] (fldid=%d): %s", 
                    fldnm, fldid, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }

        NDRX_LOG(log_debug, "Added sub-ubf [%s] fldid=%d to UBF buffer %p",
                fldnm, fldid, p_ub);

    }
    else if (BFLD_PTR==fldtyp)
    {
        char *allocptr = NULL;
        long len;
        
        if (EXSUCCEED!=ndrx_tpimportex(NULL, NULL, 0, &allocptr, &len, 0, innerobj))
        {
            NDRX_LOG(log_error, "Failed to parse PTR object");
            EXFAIL_OUT(ret);
        }
        
        NDRX_LOG(log_debug, "Got PTR field: %p", allocptr);
        
        if (EXSUCCEED!=Bchg(p_ub, fldid, occ, (char *)&allocptr, 0L))
        {
            ndrx_TPset_error_fmt(TPESYSTEM, 
                    "Failed to add to parent UBF inner PTR field [%p] [%s] (fldid=%d): %s", 
                    fldnm, allocptr, fldid, Bstrerror(Berror));
            NDRX_LOG(log_error, 
                    "Failed to add to parent UBF inner PTR field [%p] [%s] (fldid=%d): %s", 
                    fldnm, allocptr, fldid, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    else if (BFLD_VIEW==fldtyp)
    {
        BVIEWFLD vdata;
        int null_view=EXFALSE;
        vdata.vflags=0;
        
        if (NULL==(vdata.data=ndrx_tpjsontoview(vdata.vname, NULL, innerobj, 
                &null_view)) && !null_view)
        {
            NDRX_LOG(log_error, "Failed to parse UBF json at field [%s]", 
                    fldnm);
            EXFAIL_OUT(ret);
        }

        /* Add UBF to buffer */
        if (EXSUCCEED!=Bchg(p_ub, fldid, occ, (char *)&vdata, 0L))
        {
            ndrx_TPset_error_fmt(TPESYSTEM, 
                    "Failed to add to parent UBF inner VIEW[%s] [%s] (fldid=%d): %s", 
                    vdata.vname, fldnm, fldid, Bstrerror(Berror));
            NDRX_LOG(log_error, "Failed to add to parent UBF inner VIEW[%s] [%s] (fldid=%d): %s", 
                    vdata.vname, fldnm, fldid, Bstrerror(Berror));

            NDRX_FREE(vdata.data);
            EXFAIL_OUT(ret);
        }

        NDRX_FREE(vdata.data);

        NDRX_LOG(log_debug, "Added sub-view[%s] [%s] fldid=%d to UBF buffer %p",
                vdata.vname, fldnm, fldid, p_ub);
    }
    else
    {
        ndrx_TPset_error_fmt(TPEINVAL, "Field [%s] type is %s but object received",
                fldnm, (Btype(fldtyp)?Btype(fldtyp):"(null)"));
        NDRX_LOG(log_error, "Field [%s] type is %s but object received",
                fldnm, (Btype(fldtyp)?Btype(fldtyp):"(null)"));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Common parser for string data, allows to use escape sequences if API is
 * configured so
 * @param p_ub parent UBF into which load the data
 * @param fldnm field name in json (UBF field name)
 * @param fldid resolved filed id
 * @param fldtyp UBF field type
 * @param bin_buf temporary working space
 * @param bin_buf_len working space length
 * @param str_val string data
 * @param occ occurrence to set in UBF
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_load_string(UBFH *p_ub, char *fldnm, BFLDID fldid, int fldtyp, 
        char *bin_buf, size_t bin_buf_len, char* str_val, BFLDOCC occ)
{
    int ret = EXSUCCEED;
    char	*s_ptr;
    BFLDLEN     str_len;
    
    /* If it is carray - parse hex... */
    if (IS_BIN(fldtyp))
    {
        size_t st_len = bin_buf_len;
        NDRX_LOG(log_debug, "Field is binary..."
                " convert from b64...");

        if (NULL==ndrx_base64_decode(str_val,
                strlen(str_val),
                &st_len,
                bin_buf))
        {
            NDRX_LOG(log_debug, "Failed to "
                    "decode base64!");

            ndrx_TPset_error_fmt(TPEINVAL, "Failed to "
                    "decode base64: %s", fldnm);

            EXFAIL_OUT(ret);
        }
        str_len = st_len;
        s_ptr = bin_buf;
        NDRX_LOG(log_debug, "got binary len [%d]", str_len);
    }
    else if (G_atmi_env.apiflags & NDRX_APIFLAGS_JSONESCAPE)
    {
        /* convert string from C escape... */
        if (EXSUCCEED!=ndrx_normalize_string(str_val, &str_len))
        {
            NDRX_LOG(log_error, "Invalid C escape used in field [%s] data: [%s]",
                    fldnm, str_val);
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        s_ptr = str_val;
        str_len = strlen(str_val);
    }

    if (EXSUCCEED!=CBchg(p_ub, fldid, occ, s_ptr, str_len, BFLD_CARRAY))
    {
        NDRX_LOG(log_error, "Failed to set UBF field (%s) %d: %s",
                fldnm, fldid, Bstrerror(Berror));
        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set UBF field (%s) %d: %s",
                fldnm, fldid, Bstrerror(Berror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Convert JSON text buffer to UBF
 * TODO: Add support for embedded ubf/view
 * TODO: Reset the UBF buffer at the start, so that we do not conflict
 * with existing fields
 * @param p_ub - UBF buffer to fill data in
 * @param buffer - json text to parse
 * @data_object - already parsed json in object
 * @return SUCCEED/FAIL
 */
expublic int ndrx_tpjsontoubf(UBFH *p_ub, char *buffer, EXJSON_Object *data_object)
{
    int ret = EXSUCCEED;
    EXJSON_Value *root_value=NULL;
    EXJSON_Object *root_object=data_object;
    EXJSON_Object *innerobj;
    EXJSON_Array *array;
    size_t i, cnt, j, arr_cnt;
    int type;
    char *name;
    char 	*str_val;
    BFLDID	fid;
    double d_val;
    int f_type;
    short	bool_val;
    char    *bin_buf=NULL;
    size_t bin_buf_len;
    char	*s_ptr;
    int fldtyp;

    /* allocate dynamically... */
    bin_buf_len=CARR_BUFFSIZE+1;
    NDRX_MALLOC_OUT(bin_buf, bin_buf_len, char);

    if ( NULL != buffer )
    {
        NDRX_LOG(log_debug, "Parsing buffer: [%s]", buffer);

        root_value = exjson_parse_string_with_comments(buffer);
        type = exjson_value_get_type(root_value);
        NDRX_LOG(log_debug, "Type is %d", type);

        if (exjson_value_get_type(root_value) != EXJSONObject)
        {
            NDRX_LOG(log_error, "Failed to parse root element");
            ndrx_TPset_error_fmt(TPEINVAL, "exjson: Failed to parse root element");
            return EXFAIL;
        }
        root_object = exjson_value_get_object(root_value);
    }
    else
    {
        NDRX_LOG(log_debug, "Parsing from data_object");
    }

    cnt = exjson_object_get_count(root_object);
    NDRX_LOG(log_debug, "cnt = %d", cnt);

    for (i =0; i< cnt; i++)
    {
        name = (char *)exjson_object_get_name(root_object, i);

        NDRX_LOG(log_debug, "Name: [%s]", name);
        fid = Bfldid(name);

        if (BBADFLDID==fid)
        {
            NDRX_LOG(log_warn, "Name: [%s] - not known in UBFTAB - ignore", name);
            continue;
        }
        
        fldtyp=Bfldtype(fid);

        switch ((f_type=exjson_value_get_type(exjson_object_get_value_at(root_object, i))))
        {
            case EXJSONString:
            {
                str_val = (char *)exjson_object_get_string(root_object, name);
                NDRX_LOG(log_debug, "Str Value: [%s]", str_val);
                
                if (EXSUCCEED!=ndrx_load_string(p_ub, name, fid, fldtyp, 
                        bin_buf, bin_buf_len, str_val, 0))
                {
                    NDRX_LOG(log_error, "Failed to set array string value for [%s]", 
                            name);
                    EXFAIL_OUT(ret);
                }
                break;
            }
            case EXJSONNumber:
            {
                long l;
                d_val = exjson_object_get_number(root_object, name);
                NDRX_LOG(log_debug, "Double Value: [%lf]", d_val);

                if (IS_INT(fldtyp))
                {
                    l = round_long(d_val);
                    if (EXSUCCEED!=CBchg(p_ub, fid, 0, 
                            (char *)&l, 0L, BFLD_LONG))
                    {
                        NDRX_LOG(log_error, "Failed to set [%s] to [%ld]!", 
                            name, l);
                        
                        ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] to [%ld]!", 
                            name, l);
                        
                        EXFAIL_OUT(ret);
                    }
                }
                else if (EXSUCCEED!=CBchg(p_ub, fid, 0, (char *)&d_val, 0L, BFLD_DOUBLE))
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
                bool_val = (short)exjson_object_get_boolean(root_object, name);
                NDRX_LOG(log_debug, "Bool Value: [%hd]", bool_val);
                if (EXSUCCEED!=CBchg(p_ub, fid, 0, (char *)&bool_val, 0L, BFLD_SHORT))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] to [%hd]: %s", 
                            name, bool_val, Bstrerror(Berror));
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to set [%s] to [%hd]: %s", 
                            name, bool_val, Bstrerror(Berror));
                    
                    EXFAIL_OUT(ret);
                }
            }
            break;
            
            /* parse embedded object */
            case EXJSONObject:
                
                innerobj = exjson_object_get_object(root_object, name);

                if (NULL==innerobj)
                {
                    ndrx_TPset_error_fmt(TPEINVAL, 
                            "Null object received for field [%s]", name);
                    NDRX_LOG(log_error, "Null object received for field [%s]", 
                            name);
                    EXFAIL_OUT(ret);
                }

                if (EXSUCCEED!=ndrx_load_object(p_ub, name, fid, fldtyp, 
                        bin_buf, bin_buf_len, innerobj, 0))
                {
                    NDRX_LOG(log_error, "Failed to parse inner object of [%s]", 
                            name);
                    EXFAIL_OUT(ret);
                }
                
                break;
                
            /* Fielded buffer fields with more than one occurrance will go to array: 
             * Stuff here is almost identicial to above!
             */
            case EXJSONArray:
            {
                if (NULL==(array = exjson_object_get_array(root_object, name)))
                {
                    NDRX_LOG(log_error, "Failed to get array object!");
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get array object!");
                    EXFAIL_OUT(ret);
                }
                arr_cnt = exjson_array_get_count(array);

                for (j = 0; j<arr_cnt; j++ )
                {
                    switch (f_type = exjson_value_get_type(
                            exjson_array_get_value(array, j)))
                    {
                        case EXJSONString:
                        {
                            str_val = (char *)exjson_array_get_string(array, j);
                            NDRX_LOG(log_debug, 
                                        "Array j=%d, Str Value: [%s]", j, str_val);
                            
                            if (EXSUCCEED!=ndrx_load_string(p_ub, name, fid, fldtyp, 
                                    bin_buf, bin_buf_len, str_val, j))
                            {
                                NDRX_LOG(log_error, "Failed to set array string value for [%s]", 
                                        name);
                                EXFAIL_OUT(ret);
                            }
                        }
                        break;
                        case EXJSONNumber:
                        {
                            long l;
                            d_val = exjson_array_get_number(array, j);
                            NDRX_LOG(log_debug, "Array j=%d, Double Value: [%lf]", j, d_val);

                            if (IS_INT(fldtyp))
                            {
                                l = round_long(d_val);
                                NDRX_LOG(log_debug, "Array j=%d, Long value: [%ld]", j, l);
                                if (EXSUCCEED!=CBchg(p_ub, fid, j, 
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
                            else if (EXSUCCEED!=CBchg(p_ub, fid, j, 
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
                            bool_val = (short)exjson_array_get_boolean(array, j);
                            NDRX_LOG(log_debug, "Array j=%d, Bool Value: [%hd]", j, bool_val);
                            if (EXSUCCEED!=CBchg(p_ub, fid, j, (char *)&bool_val, 0L, BFLD_SHORT))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%hd]: %s", 
                                        name, bool_val, Bstrerror(Berror));
                                
                                ndrx_TPset_error_fmt(TPESYSTEM,"Failed to set "
                                        "[%s] to [%hd]: %s", 
                                        name, bool_val, Bstrerror(Berror));
                                
                                EXFAIL_OUT(ret);
                            }
                        }
                        
                        case EXJSONObject:
                            
                            innerobj = exjson_array_get_object(array, j);

                            if (NULL==innerobj)
                            {
                                ndrx_TPset_error_fmt(TPEINVAL, 
                                        "Null object received for array field [%s]", name);
                                NDRX_LOG(log_error, "Null object received for array field [%s]", 
                                        name);
                                EXFAIL_OUT(ret);
                            }

                            if (EXSUCCEED!=ndrx_load_object(p_ub, name, fid, fldtyp, 
                                    bin_buf, bin_buf_len, innerobj, j))
                            {
                                NDRX_LOG(log_error, "Failed to parse inner array object of [%s]", 
                                        name);
                                EXFAIL_OUT(ret);
                            }
                             
                        break;
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
    if (NULL != root_value)
    {
        exjson_value_free(root_value);
    }

    if (NULL!=bin_buf)
    {
        NDRX_FREE(bin_buf);
    }

    return ret;
}

/**
 * Build json text from UBF buffer
 * @param p_ub  JSON buffer
 * @param buffer output json buffer
 * @param bufsize       output buffer size
 * @return SUCCEED/FAIL 
 */
expublic int ndrx_tpubftojson(UBFH *p_ub, char *buffer, int bufsize, EXJSON_Object *data_object)
{
    int ret = EXSUCCEED;
    BFLDID fldid;
    int occs;
    int is_array;
    double d_val;
    size_t strval_len = CARR_BUFFSIZE+1;
    char *strval=NULL; 
    size_t b64_buf_len =CARR_BUFFSIZE_B64+1;
    char *b64_buf=NULL;
    int is_num;
    char *s_ptr;
    char *d_ptr;
    BFLDLEN flen;
    /* use if there is no root */
    EXJSON_Value *root_value=NULL;
    EXJSON_Object *root_object=NULL;
    char *serialized_string = NULL;
    BFLDOCC oc;
    BFLDLEN fldlen;
    int fldtyp;
    EXJSON_Value *emb_value = NULL;
    EXJSON_Object *emb_object = NULL;
    Bnext_state_t state;
    char *nm;
    EXJSON_Value *jarr_value=NULL;
    EXJSON_Array *jarr=NULL;
    
    NDRX_MALLOC_OUT(strval, strval_len, char);
    NDRX_MALLOC_OUT(b64_buf, b64_buf_len, char);
    
    if ( NULL == data_object )
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
        root_object = data_object;
    }
    
    memset(&state, 0, sizeof(state));
    
    for (fldid = BFIRSTFLDID, oc = 0;
            1 == (ret = ndrx_Bnext(&state, p_ub, &fldid, &oc, NULL, &fldlen, &d_ptr));)
    {
        /* Feature #232 return ID if field not found in tables... */
        nm = ndrx_Bfname_int(fldid);
        NDRX_LOG(log_debug, "Field: [%s] occ %d id: %d", nm, oc, fldid);
        if (0==oc)
        {
            occs = Boccur(p_ub, fldid);
            if (occs>1)
            {
                /* create array */
                is_array = EXTRUE;
                /* add array to document... */
                if (EXJSONSuccess!=exjson_object_set_value(root_object, 
                        nm, exjson_value_init_array()))
                {
                    NDRX_LOG(log_error, "Failed to add Array to root object!!");

                    ndrx_TPset_error_msg(TPESYSTEM, "Failed to add Array "
                            "to root object!!");
                    EXFAIL_OUT(ret);
                }
                if (NULL == (jarr=exjson_object_get_array(root_object, nm)))
                {
                    NDRX_LOG(log_error, "Failed to initialize array!!");

                    ndrx_TPset_error_msg(TPESYSTEM, "Failed to initialize array");
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
        
        fldtyp=Bfldtype(fldid);

        if (IS_NUM(fldtyp))
        {
            if (EXSUCCEED!=CBget(p_ub, fldid, oc, (char *)&d_val, 0L, BFLD_DOUBLE))
            {
                NDRX_LOG(log_error, "Failed to get (double): %ld/%d: %s",
                                                fldid, oc, Bstrerror(Berror));
                
                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get (double): %ld/%d: %s",
                                                fldid, oc, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
            is_num = EXTRUE;
            NDRX_LOG(log_debug, "Numeric value: %lf", d_val);
        }
        else if (BFLD_UBF==fldtyp || BFLD_VIEW==fldtyp || BFLD_PTR==fldtyp)
        {
            if (NULL==(emb_value = exjson_value_init_object()))
            {
                NDRX_LOG(log_error, "Failed to init data_value");
                ndrx_TPset_error_fmt(TPESYSTEM, "exparson: failed to init data_value");
                
                EXFAIL_OUT(ret);
            }

            if (NULL==(emb_object = exjson_value_get_object(emb_value)))
            {
                NDRX_LOG(log_error, "Failed to get object value");
                ndrx_TPset_error_fmt(TPESYSTEM, "exparson: Failed to get object");
                EXFAIL_OUT(ret);
            }
            
             /* process embedded buffer */
            if (BFLD_UBF==fldtyp)
            {
                if (EXSUCCEED!=ndrx_tpubftojson((UBFH *)d_ptr, NULL, 0, emb_object))
                {
                    NDRX_LOG(log_error, "Failed to build embedded data object from UBF!");
                    EXFAIL_OUT(ret);
                }
            }
            else if (BFLD_PTR==fldtyp)
            {
                /* export whole buffer... run as new tpexport */
                
                ndrx_longptr_t *ptr =(ndrx_longptr_t *)d_ptr;
                NDRX_LOG(log_debug, "About to export ptr: [%p]", (char *)*ptr);
                
                if (EXSUCCEED!=ndrx_tpexportex(NULL, 
                        (char *)*ptr, 0, NULL, NULL, 0, emb_object))
                {
                    NDRX_LOG(log_error, "Failed to export PTR (%p)!", *ptr);
                    EXFAIL_OUT(ret);
                }
                
                NDRX_LOG(log_debug, "Export ptr returns: [%p]", (char *)*ptr);
                /* set value? */
            }
            else
            {
                /* if this is a view... needs to get view struct data... */
                ndrx_ubf_tls_bufval_t *vf = (ndrx_ubf_tls_bufval_t *)d_ptr;
                
                if (EXSUCCEED!=ndrx_tpviewtojson(vf->vdata.data, 
                        vf->vdata.vname, NULL, 0, BVACCESS_NOTNULL, emb_object))
                {
                    NDRX_LOG(log_error, "Failed to build embedded data object from VIEW!");
                    EXFAIL_OUT(ret);
                }
            }
        }
        else
        {
            is_num = EXFALSE;
            flen = strval_len;
            if (EXSUCCEED!=CBget(p_ub, fldid, oc, strval, &flen, BFLD_CARRAY))
            {
                NDRX_LOG(log_error, "Failed to get (string): %ld/%d: %s",
                                        fldid, oc, Bstrerror(Berror));
                
                ndrx_TPset_error_fmt(TPESYSTEM, "Failed to get (string): %ld/%d: %s",
                                        fldid, oc, Bstrerror(Berror));
                
                EXFAIL_OUT(ret);
            }

            /* If it is carray, then convert to hex... */
            if (IS_BIN(fldtyp))
            {
                size_t outlen = b64_buf_len;
                NDRX_LOG(log_debug, "Field is binary... convert to b64");

                if (NULL==ndrx_base64_encode((unsigned char *)strval, flen, 
                            &outlen, b64_buf))
                {
                    NDRX_LOG(log_error, "Failed to convert to b64!");
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "Failed to convert to b64!");
                    
                    EXFAIL_OUT(ret);
                }
                /* b64_buf[outlen] = EXEOS; */
                s_ptr = b64_buf;

            }
            else if (G_atmi_env.apiflags & NDRX_APIFLAGS_JSONESCAPE)
            {
                int tmp_len = ndrx_get_nonprintable_char_tmpspace(strval, flen);
                
                if (tmp_len+1 > b64_buf_len)
                {
                    NDRX_LOG(log_error, "Field [%s] value too long for json "
                            "escape temporary buffer: required %d have: %z - "
                            "increase NDRX_MSGSIZEMAX",
                            nm, tmp_len+1, b64_buf_len);
                    
                    ndrx_TPset_error_fmt(TPEINVAL, "Field [%s] value too long for json "
                            "escape temporary buffer: required %d have: %z - "
                            "increase NDRX_MSGSIZEMAX",
                            nm, tmp_len+1, b64_buf_len);
                    
                    EXFAIL_OUT(ret);
                }
                
                ndrx_build_printable_string(b64_buf, b64_buf_len, strval, flen);
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
            else if (BFLD_UBF==fldtyp || BFLD_VIEW==fldtyp || BFLD_PTR==fldtyp)
            {
                /* append  */
                if (EXJSONSuccess!=exjson_array_append_value(jarr, emb_value))
                {
                    NDRX_LOG(log_error, "exjson: Failed to set array "
                            "elem [%s] to embedded object (view/ubf) occ: %d fldid: %d!", 
                            nm, oc, fldid);
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set array "
                            "elem [%s] to embedded object (view/ubf) occ: %d fldid: %d!", 
                            nm, oc, fldid);
                    EXFAIL_OUT(ret);
                }
                
                /* set to not to free up... */
                emb_value=NULL;
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
                if (EXJSONSuccess!=exjson_object_set_number(root_object, nm, d_val))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] value to [%lf]!",
                                        nm, d_val);
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set [%s] "
                            "value to [%lf]!", nm, d_val);
                    
                    EXFAIL_OUT(ret);
                }
            }
            else if (BFLD_UBF==fldtyp || BFLD_VIEW==fldtyp || BFLD_PTR==fldtyp)
            {
                if (EXJSONSuccess!=exjson_object_set_value(root_object, nm, emb_value))
                {
                    NDRX_LOG(log_error, "Failed to add embedded VIEW/UBF [%s]", nm);
                    EXFAIL_OUT(ret);
                }
                /* set to not to free up... */
                emb_value=NULL;
            }
            else
            {
                if (EXJSONSuccess!=exjson_object_set_string(root_object, nm, s_ptr))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] value to [%s]!",
                                    nm, s_ptr);
                    
                    ndrx_TPset_error_fmt(TPESYSTEM, "exjson: Failed to set [%s] "
                            "value to [%s]!", nm, s_ptr);
                    
                    EXFAIL_OUT(ret);
                }
            }
        }
    }

    if (NULL != buffer)
    {
        serialized_string = exjson_serialize_to_string(root_value);

        if (strlen(serialized_string) < bufsize ) /* needs space for EOS */
        {
            NDRX_STRCPY_SAFE_DST(buffer, serialized_string, bufsize);

            NDRX_LOG(log_debug, "Got JSON: [%s]", buffer);
        }
        else
        {
            NDRX_LOG(log_error, "Buffer too short: Got json size: [%d] buffer size: [%d]", 
                    strlen(serialized_string)+1, bufsize);

            ndrx_TPset_error_fmt(TPEOS, "Buffer too short: Got json size: "
                    "[%d] buffer size: [%d]",  strlen(serialized_string)+1, bufsize);

            EXFAIL_OUT(ret);
        }
    }

out:

    if (NULL!=serialized_string)
    {
        exjson_free_serialized_string(serialized_string);
    }

    if (NULL!=emb_value)
    {
        exjson_value_free(emb_value);
    }

    /* kill the root value if any... */
    if (NULL!=root_value)
    {
        exjson_value_free(root_value);
    }

    if ( NULL != jarr_value )
    {
        exjson_value_free(jarr_value);
    }

    if (NULL!=strval)
    {
        NDRX_FREE(strval);
    }
    
    
    if (NULL!=b64_buf)
    {
        NDRX_FREE(b64_buf);
    }
    
    return ret;
}

/**
 * auto-buffer convert func. json->ubf
 * @param buffer
 * @return 
 */
expublic int typed_xcvt_json2ubf(buffer_obj_t **buffer)
{
    int ret = EXSUCCEED;
    buffer_obj_t *tmp_b;
    /* Allocate the max UBF buffer */
    UBFH * tmp = NULL;
    UBFH * newbuf_out = NULL; /* real output buffer */

    if (NULL==(tmp = (UBFH *)tpalloc("UBF", NULL, NDRX_MSGSIZEMAX)))
    {
        NDRX_LOG(log_error, "failed to convert JSON->UBF. UBF buffer alloc fail!");
        EXFAIL_OUT(ret);
    }

    /* Do the convert */
    ndrx_TPunset_error();
    if (EXSUCCEED!=ndrx_tpjsontoubf(tmp, (*buffer)->buf, NULL))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to convert JSON->UBF: %s", tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* Shrink the buffer (by reallocating) new! 
     * we will do the shrink because, msg Q might not have settings set for
     * max buffer size...
     */
    if (NULL==(newbuf_out = (UBFH *)tpalloc("UBF", NULL, Bused(tmp))))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to alloc output UBF %ld !", Bused(tmp));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=Bcpy(newbuf_out, tmp))
    {
        tpfree((char *)tmp);
        tpfree((char *)newbuf_out);

        NDRX_LOG(log_error, "Failed to copy tmp UBF to output: %s !", Bstrerror(Berror));
        EXFAIL_OUT(ret);

    }

    tmp_b=ndrx_find_buffer((char *)newbuf_out);
    tmp_b->autoalloc = (*buffer)->autoalloc;

    /* Kill the buffers */
    tpfree((*buffer)->buf);
    tpfree((char *)tmp);

    /* finally return the buffer */
    NDRX_LOG(log_info, "Returning new buffer %p", tmp_b);
    *buffer = tmp_b;
out:
    return ret;
}


/**
 * auto-buffer convert func. ubf->json
 * @param buffer
 * @return 
 */
expublic int typed_xcvt_ubf2json(buffer_obj_t **buffer)
{
    int ret = EXSUCCEED;
    buffer_obj_t *tmp_b;
    
    char * tmp = NULL;
    char * newbuf_out = NULL; /* real output buffer */

    if (NULL==(tmp = tpalloc("JSON", NULL, NDRX_MSGSIZEMAX)))
    {
        NDRX_LOG(log_error, "failed to convert UBF->JSON. JSON buffer alloc fail!: %s",
                tpstrerror(tperrno));
        EXFAIL_OUT(ret);
    }

    /* Do the convert */
    ndrx_TPunset_error();
    if (EXSUCCEED!=ndrx_tpubftojson((UBFH *)(*buffer)->buf, tmp, NDRX_MSGSIZEMAX, NULL))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to convert UBF->JSON: %s", 
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

/* vim: set ts=4 sw=4 et smartindent: */
