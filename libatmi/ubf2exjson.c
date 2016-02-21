/* 
** ubf<->json conversion lib
**
** @file ubf2exjson.c
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
#define IS_INT(X) (BFLD_CHAR == Bfldtype(X) || BFLD_SHORT == Bfldtype(X) || BFLD_LONG == Bfldtype(X))
#define IS_NUM(X) (BFLD_SHORT == Bfldtype(X) || BFLD_LONG == Bfldtype(X) || BFLD_FLOAT == Bfldtype(X) || BFLD_DOUBLE == Bfldtype(X))
#define IS_BIN(X) (BFLD_CARRAY == Bfldtype(X))

/* TODO: Fix atmi buffer size to match size of ATMI buffer size. */
#define CARR_BUFFSIZE		ATMI_MSG_MAX_SIZE
#define CARR_BUFFSIZE_B64	(4 * (CARR_BUFFSIZE) / 3)
/*------------------------------Enums-----------------------------------------*/
/*------------------------------Typedefs--------------------------------------*/
/*------------------------------Globals---------------------------------------*/
/*------------------------------Statics---------------------------------------*/
/*------------------------------Prototypes------------------------------------*/


private long round_long( double r ) {
    return (r > 0.0) ? (r + 0.5) : (r - 0.5); 
}

/**
 * Convert JSON text buffer to UBF
 * @param p_ub - UBF buffer to fill data in
 * @param buffer - json text to parse
 * @return SUCCEED/FAIL
 */
public int _tpjsontoubf(UBFH *p_ub, char *buffer)
{
    int ret = SUCCEED;
    EXJSON_Value *root_value;
    EXJSON_Object *root_object;
    EXJSON_Value *val;
    EXJSON_Array *array_val;
    size_t i, cnt, j, arr_cnt;
    int type;
    char *name;
    char 	*str_val;
    BFLDID	fid;
    double d_val;
    int f_type;
    short	bool_val;
    char bin_buf[CARR_BUFFSIZE+1];
    char	*s_ptr;

    NDRX_LOG(log_error, "Parsing buffer: [%s]", buffer);

    root_value = exjson_parse_string_with_comments(buffer);
    type = exjson_value_get_type(root_value);
    NDRX_LOG(log_error, "Type is %d", type);

    if (exjson_value_get_type(root_value) != EXJSONObject)
    {
        NDRX_LOG(log_error, "Failed to parse root element");
        return FAIL;
    }
    root_object = exjson_value_get_object(root_value);

    cnt = exjson_object_get_count(root_object);
    NDRX_LOG(log_error, "cnt = %d", cnt);

    for (i =0; i< cnt; i++)
    {
        name = (char *)exjson_object_get_name(root_object, i);

        NDRX_LOG(log_error, "Name: [%s]", name);
        fid = Bfldid(name);

        if (BBADFLDID==fid)
        {
            NDRX_LOG(log_warn, "Name: [%s] - not known in UBFTAB - ignore", name);
            continue;
        }

        switch ((f_type=exjson_value_get_type(exjson_object_nget_value_n(root_object, i))))
        {
            case EXJSONString:
            {
                BFLDLEN str_len;
                s_ptr = str_val = (char *)exjson_object_get_string_n(root_object, i);
                NDRX_LOG(log_debug, "Str Value: [%s]", str_val);

                /* If it is carray - parse hex... */
                if (IS_BIN(fid))
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
                        FAIL_OUT(ret);
                    }
                    str_len = st_len;
                    s_ptr = bin_buf;
                    NDRX_LOG(log_error, "got binary len [%d]", str_len);
                }
                else
                {
                    str_len = strlen(s_ptr);
                }

                if (SUCCEED!=CBchg(p_ub, fid, 0, s_ptr, str_len, BFLD_CARRAY))
                {
                    ret=FAIL;
                    goto out;
                }
                break;
            }
            case EXJSONNumber:
            {
                long l;
                d_val = exjson_object_get_number_n(root_object, i);
                NDRX_LOG(log_debug, "Double Value: [%lf]", d_val);

                if (IS_INT(fid))
                {
                    l = round_long(d_val);
                    if (SUCCEED!=CBchg(p_ub, fid, 0, 
                            (char *)&l, 0L, BFLD_LONG))
                    {
                        NDRX_LOG(log_error, "Failed to set [%s] to [%ld]!", 
                        name, l);
                        ret=FAIL;
                        goto out;
                    }
                }
                else if (SUCCEED!=CBchg(p_ub, fid, 0, (char *)&d_val, 0L, BFLD_DOUBLE))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] to [%lf]!", 
                            name, d_val);
                    ret=FAIL;
                    goto out;
                }
            }
                    break;
            case EXJSONBoolean:
            {
                bool_val = (short)exjson_object_get_boolean_n(root_object, i);
                NDRX_LOG(log_debug, "Bool Value: [%hd]", bool_val);
                if (SUCCEED!=CBchg(p_ub, fid, 0, (char *)&bool_val, 0L, BFLD_SHORT))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] to [%hd]!", 
                            name, bool_val);
                    ret=FAIL;
                    goto out;
                }
            }
            break;
            /* Fielded buffer fields with more than one occurrance will go to array: 
             * Stuff here is almost identicial to above!
             */
            case EXJSONArray:
            {
                if (NULL==(array_val = exjson_value_get_array(
                        exjson_object_nget_value_n(root_object, i))))
                {
                    NDRX_LOG(log_error, "Failed to get array object!");
                    ret=FAIL;
                    goto out;
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
                            if (IS_BIN(fid))
                            {
                                size_t st_len;
                                if (NULL==atmi_base64_decode(str_val,
                                        strlen(str_val),
                                        &st_len,
                                        bin_buf))
                                {
                                    NDRX_LOG(log_debug, "Failed to "
                                            "decode base64!");
                                    FAIL_OUT(ret);
                                }
                                str_len = st_len;
                                s_ptr = bin_buf;
                                NDRX_LOG(log_error, "got binary len [%d]", str_len);
                            }
                            else
                            {
                                str_len = strlen(s_ptr);
                            }

                            if (SUCCEED!=CBchg(p_ub, fid, j, s_ptr, str_len, BFLD_CARRAY))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%s]!", 
                                        name, str_val);
                                ret=FAIL;
                                goto out;
                            }
                        }
                        break;
                        case EXJSONNumber:
                        {
                            long l;
                            d_val = exjson_array_get_number(array_val, j);
                            NDRX_LOG(log_debug, "Array j=%d, Double Value: [%lf]", j, d_val);

                            if (IS_INT(fid))
                            {
                                l = round_long(d_val);
                                NDRX_LOG(log_debug, "Array j=%d, Long value: [%ld]", j, l);
                                if (SUCCEED!=CBchg(p_ub, fid, j, 
                                        (char *)&l, 0L, BFLD_LONG))
                                {
                                        NDRX_LOG(log_error, "Failed to set [%s] to [%ld]!", 
                                                name, l);
                                        ret=FAIL;
                                        goto out;
                                }
                            }
                            else if (SUCCEED!=CBchg(p_ub, fid, j, 
                                    (char *)&d_val, 0L, BFLD_DOUBLE))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%lf]!", 
                                        name, d_val);
                                ret=FAIL;
                                goto out;
                            }
                        }
                        break;
                        case EXJSONBoolean:
                        {
                            bool_val = (short)exjson_array_get_boolean(array_val, j);
                            NDRX_LOG(log_debug, "Array j=%d, Bool Value: [%hd]", j, bool_val);
                            if (SUCCEED!=CBchg(p_ub, fid, j, (char *)&bool_val, 0L, BFLD_SHORT))
                            {
                                NDRX_LOG(log_error, "Failed to set [%s] to [%hd]!", 
                                        name, bool_val);
                                ret=FAIL;
                                goto out;
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
    return ret;
}


/**
 * Build json text from UBF buffer
 * @param p_ub  JSON buffer
 * @param buffer output json buffer
 * @param bufsize       output buffer size
 * @return SUCCEED/FAIL 
 */
public int _tpubftojson(UBFH *p_ub, char *buffer, int bufsize)
{
    int ret = SUCCEED;
    BFLDID fldid;
    int occs;
    int is_array;
    double d_val;
    char strval[CARR_BUFFSIZE+1]; 
    char b64_buf[CARR_BUFFSIZE_B64+1];
    int is_num;
    char *s_ptr;
    BFLDLEN flen;
    EXJSON_Value *root_value = exjson_value_init_object();
    EXJSON_Object *root_object = exjson_value_get_object(root_value);
    char *serialized_string = NULL;
    BFLDOCC oc;
    BFLDLEN fldlen;

    char *nm;
    EXJSON_Array * jarr;

    for (fldid = BFIRSTFLDID, oc = 0;
            1 == (ret = Bnext(p_ub, &fldid, &oc, NULL, &fldlen));)
    {
        nm = Bfname(fldid);
        NDRX_LOG(log_debug, "Field: [%s] occ %d", nm, oc);
        if (0==oc)
        {
            occs = Boccur(p_ub, fldid);

            if (occs>1)
            {
                /* create array */
                is_array = TRUE;
                if (NULL==(jarr = exjson_array_init()))
                {
                        NDRX_LOG(log_error, "Failed to initialize array!");
                }
                /* add array to document... */
                if (EXJSONSuccess!=exjson_object_set_array(root_object, nm, jarr))
                {
                        NDRX_LOG(log_error, "Failed to add Array to root object!!");
                }
            }
            else
            {
                is_array = FALSE;
            }
        }
        else
        {
            is_array = TRUE;
        }

        if (IS_NUM(fldid))
        {
            if (SUCCEED!=CBget(p_ub, fldid, oc, (char *)&d_val, 0L, BFLD_DOUBLE))
            {
                NDRX_LOG(log_error, "Failed to get (double): %ld/%d",
                                                fldid, oc);
                ret=FAIL;
                goto out;
            }
            is_num = TRUE;
            NDRX_LOG(log_debug, "Numeric value: %lf", d_val);
        }
        else
        {
            is_num = FALSE;
            flen = sizeof(strval);
            if (SUCCEED!=CBget(p_ub, fldid, oc, strval, &flen, BFLD_CARRAY))
            {
                NDRX_LOG(log_error, "Failed to get (string): %ld/%d",
                                                fldid, oc);
                ret=FAIL;
                goto out;
            }

            /* If it is carray, then convert to hex... */
            if (IS_BIN(fldid))
            {
                size_t outlen;
                NDRX_LOG(log_debug, "Field is binary... "
                                            "convert to b64");

                if (NULL==atmi_base64_encode(strval, flen, 
                            &outlen, b64_buf))
                {
                    NDRX_LOG(log_error, "Failed to convert to b64!");
                    FAIL_OUT(ret);
                }
                b64_buf[outlen] = EOS;
                s_ptr = b64_buf;

            }
            else
            {
                strval[flen] = EOS;
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
                            NDRX_LOG(log_error, "Failed to set array elem to [%lf]!", d_val);
                    }
                }
                else
                {
                    if (EXJSONSuccess!=exjson_array_append_string(jarr, s_ptr))
                    {
                            NDRX_LOG(log_error, "Failed to set array elem to [%s]!", s_ptr);
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
                }
            }
            else
            {
                if (EXJSONSuccess!=exjson_object_set_string(root_object, nm, s_ptr))
                {
                    NDRX_LOG(log_error, "Failed to set [%s] value to [%s]!",
                                    nm, s_ptr);
                }
            }
        }
    }

    serialized_string = exjson_serialize_to_string(root_value);

    if (strlen(serialized_string) <= bufsize )
    {
        strcpy(buffer, serialized_string);
        NDRX_LOG(log_debug, "Got JSON: [%s]", buffer);
    }
    else
    {
        NDRX_LOG(log_error, "Buffer too short: Got json size: [%d] buffer size: [%d]", 
                strlen(serialized_string), bufsize);
        FAIL_OUT(ret);
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
    return ret;
}

/**
 * auto-buffer convert func. json->ubf
 * @param buffer
 * @return 
 */
public int typed_xcvt_json2ubf(buffer_obj_t **buffer)
{
    int ret = SUCCEED;
    buffer_obj_t *tmp_b;
    /* Allocate the max UBF buffer */
    UBFH * tmp = NULL;
    UBFH * newbuf_out = NULL; /* real output buffer */

    if (NULL==(tmp = (UBFH *)tpalloc("UBF", NULL, ATMI_MSG_MAX_SIZE)))
    {
        NDRX_LOG(log_error, "failed to convert JSON->UBF. UBF buffer alloc fail!");
        FAIL_OUT(ret);
    }

    /* Do the convert */
    if (SUCCEED!=_tpjsontoubf(tmp, (*buffer)->buf))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to convert JSON->UBF!");
        FAIL_OUT(ret);
    }

    /* Shrink the buffer (by reallocating) new! 
     * we will do the shrink because, msg Q might not have settings set for
     * max buffer size...
     */
    if (NULL==(newbuf_out = (UBFH *)tpalloc("UBF", NULL, Bused(tmp))))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to alloc output UBF %ld !", Bused(tmp));
        FAIL_OUT(ret);
    }

    if (SUCCEED!=Bcpy(newbuf_out, tmp))
    {
        tpfree((char *)tmp);
        tpfree((char *)newbuf_out);

        NDRX_LOG(log_error, "Failed to copy tmp UBF to output: %s !", Bstrerror(Berror));
        FAIL_OUT(ret);

    }

    tmp_b=find_buffer((char *)newbuf_out);
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


/**
 * auto-buffer convert func. ubf->json
 * @param buffer
 * @return 
 */
public int typed_xcvt_ubf2json(buffer_obj_t **buffer)
{
    int ret = SUCCEED;
    buffer_obj_t *tmp_b;
    
    char * tmp = NULL;
    char * newbuf_out = NULL; /* real output buffer */

    if (NULL==(tmp = tpalloc("JSON", NULL, ATMI_MSG_MAX_SIZE)))
    {
        NDRX_LOG(log_error, "failed to convert UBF->JSON. JSON buffer alloc fail!");
        FAIL_OUT(ret);
    }

    /* Do the convert */
    if (SUCCEED!=_tpubftojson((UBFH *)(*buffer)->buf, tmp, ATMI_MSG_MAX_SIZE))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to convert UBF->JSON!");
        FAIL_OUT(ret);
    }

    /* Shrink the buffer (by reallocating) new! 
     * we will do the shrink because, msg Q might not have settings set for
     * max buffer size...
     */
    if (NULL==(newbuf_out = tpalloc("JSON", NULL, strlen(tmp)+1)))
    {
        tpfree((char *)tmp);
        NDRX_LOG(log_error, "Failed to alloc output JSON %ld !", strlen(tmp)+1);
        FAIL_OUT(ret);
    }

    strcpy(newbuf_out, tmp);

    tmp_b=find_buffer((char *)newbuf_out);
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

