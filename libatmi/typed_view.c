/* 
** VIEW buffer type support
** Basically all data is going to be stored in FB.
** For view not using base as 3000. And the UBF field numbers follows the 
** the field number in the view (1, 2, 3, 4, ...) 
** Sample file:
** 
**# 
**#type    cname   fbname count flag  size  null
**#
**int     in    -   1   -   -   -
**short    sh    -   2   -   -   -
**long     lo    -   3   -   -   -
**char     ch    -   1   -   -   -
**float    fl    -   1   -   -   -
**double    db    -   1   -   -   -
**string    st    -   1   -   15   -
**carray    ca    -   1   -   15   -
**END
** 
**
** @file typed_view.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dirent.h>

#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <tperror.h>

#include <userlog.h>
#include <typed_view.h>
#include <atmi_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define FIELD_SEPERATORS        " \t"
#define TOKEN_VIEW_START            "VIEW"
#define TOKEN_VIEW_END              "END"

#define FLD_SIZE_MAX                65535
#define FLD_COUNT_MAX                65535


#define API_ENTRY {ndrx_TPunset_error(); \
}\

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_typedview_t *ndrx_G_view_hash = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Return handle to current view objects
 * @return ndrx_G_view_hash var
 */
expublic ndrx_typedview_t * ndrx_view_get_handle(void)
{
    return ndrx_G_view_hash;
}

/**
 * Load single view file
 * @param fname - full path to the file to load
 * @param is_compiled - is this compiled format or source format
 * @return  EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_load_file(char *fname, int is_compiled)
{
    int ret = EXSUCCEED;
    FILE *f = NULL;
    char buf[PATH_MAX*2];
    int orglen;
    ndrx_typedview_t *v = NULL;
    char *saveptr1 = NULL; /* lookup section  */
    char *saveptr2 = NULL; /* lookup section  */
    char *tok;
    enum states { INFILE, INVIEW} state = INFILE;
    enum nulltypes { NTYPNO, NTYPSTD, NTYPSQUOTE, NTYPDQUOTE} nulltype = NTYPNO;
    int len;
    char *p, *p2, *pend, *null_val_start, *p3;
    long line=0;
    dtype_str_t *dtyp = G_dtype_str_map;
    ndrx_typedview_field_t *fld = NULL;
    int i;
    API_ENTRY;
    
    NDRX_LOG(log_debug, "%s - enter", __func__);
    
    if (NULL==(f=NDRX_FOPEN(fname, "r")))
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to open view file: %s", strerror(err));
        ndrx_TPset_error_fmt(TPENOENT, "Failed to open view file: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* Read line by line... */
    while (NULL!=fgets(buf, sizeof(buf), f))
    {
        line++;
        orglen = strlen(buf);
        
        NDRX_LOG(log_dump, "Got VIEW file line: [%s], line: %ld", p, line);
        
        if ('#'==buf[0])
        {
            /* this is comment, ignore */
            continue;
        }
        else if ('\n'==buf[0] || '\r'==buf[0] && '\n'==buf[1])
        {
            /* this is newline, ignore */
            continue;
        }
        else if (EXEOS==buf[0])
        {
            /* Some strange line? */
            continue;
        }
        
        /* remove trailing newline... */
        ndrx_chomp(buf);
        
        /* Remove trailing white space.. */
        ndrx_str_rstrip(buf, FIELD_SEPERATORS);
        
        p = buf;
        /* Strip off the leading white space */
        while (' '==*p || '\t'==*p)
        {
            p++;
        }
        
        /* tokenize the line  */
        if (INFILE==state)
        {
            tok = strtok_r(p, FIELD_SEPERATORS, &saveptr1);
            
            if (0!=strcmp(tok, TOKEN_VIEW_START))
            {
                /* Not in view -> FAIL */
                NDRX_LOG(log_error, "Expected [%s] but got [%s], line: %ld", 
                        TOKEN_VIEW_START, tok, line);
                ndrx_TPset_error_fmt(TPEINVAL, "Expected [%s] but got [%s], line: %ld", 
                        TOKEN_VIEW_START, tok, line);
                EXFAIL_OUT(ret);
            }

            if (NULL==(tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1)))
            {
                NDRX_LOG(log_error, "Missing identifier after %s, line: %ld", 
                        TOKEN_VIEW_START, line);
                ndrx_TPset_error_fmt(TPEINVAL, "Missing identifier after %s, line: %ld", 
                        TOKEN_VIEW_START, line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len > NDRX_VIEW_NAME_LEN)
            {
                NDRX_LOG(log_error, "View identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_NAME_LEN, len, line);
                ndrx_TPset_error_fmt(TPEINVAL, "View identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, NDRX_VIEW_NAME_LEN, len, line);
                EXFAIL_OUT(ret);
            }
            
            /* Got view ok, allocate the object */
            v = NDRX_CALLOC(1, sizeof(ndrx_typedview_t));

            if (NULL==v)
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to allocate ndrx_typedview_t: %s, "
                        "line: %ld", 
                        strerror(err), line);

                ndrx_TPset_error_fmt(TPEOS, "Failed to allocate "
                        "ndrx_typedview_t: %s, line: %ld", 
                        strerror(err), line);
                EXFAIL_OUT(ret);    
            }
            
            NDRX_STRCPY_SAFE(v->vname, p);
            /* setup file name too */
            NDRX_STRCPY_SAFE(v->filename, fname);
            
            NDRX_LOG(log_debug, "Parsing view [%s]", v->vname);
            state = INVIEW;
            
        }
        else if (INVIEW==state)
        {
            short typ=EXFAIL;
            short typfull=EXFAIL;
            
            tok = strtok_r(p, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (0==strcmp(TOKEN_VIEW_END, tok))
            {
                NDRX_LOG(log_debug, "View [%s] finishing off -> add to hash", 
                        v->vname);
                EXHASH_ADD_STR(ndrx_G_view_hash, vname, v);
                v = NULL;
                continue;
            }
            
            /******************************************************************* 
             * OK this is the string line... 
             * The first is field type name
             * Here we might get 'int'. this will be internally stored as long.
             *******************************************************************/
            while(EXEOS!=dtyp->fldname[0])
            {
                if (0==strcmp(dtyp->fldname, tok))
                {
                    typ = dtyp->fld_type;
                    break;
                }
                dtyp++;
            }
            
            if (EXFAIL==typ)
            {
                if (0==strcmp(tok, "int"))
                {
                    typ = BFLD_LONG; 
                    typfull = BFLD_INT;
                }
                else
                {
                    NDRX_LOG(log_error, "Invalid data type [%s], line: %ld", 
                            tok, line);
                    ndrx_TPset_error_fmt(TPEINVAL, "Invalid data type [%s], line: %ld", 
                            tok, line);
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                typfull = typ;
            }
            
            /* Allocate the field block  */
            fld = NDRX_CALLOC(1, sizeof(ndrx_typedview_field_t));

            if (NULL==fld)
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to allocate ndrx_typedview_field_t: %s,"
                        " line: %ld", 
                        strerror(err), line);

                ndrx_TPset_error_fmt(TPEOS, "Failed to allocate ndrx_typedview_field_t: "
                        "%s, line: %ld", 
                        strerror(err));
                EXFAIL_OUT(ret);    
            }
            
            fld->typecode = typ;
            fld->typecode_full = typfull;
            NDRX_STRCPY_SAFE(fld->type_name, tok);
            
            NDRX_LOG(log_debug, "Got type code UBF=%d full code=%d", 
                    fld->typecode, fld->typecode_full);
            
            /******************************************************************* 
             * C Field name 
             *******************************************************************/
            tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                NDRX_LOG(log_error, "Expected C field name, got EOS, line: %ld");
                ndrx_TPset_error_fmt(TPEINVAL, "Expected C field name, got EOS, "
                        "line %ld", line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>NDRX_VIEW_CNAME_LEN)
            {
                NDRX_LOG(log_error, "C field identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_CNAME_LEN, len, line);
                
                ndrx_TPset_error_fmt(TPEINVAL, "C field identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, NDRX_VIEW_CNAME_LEN, len, line);
                
                EXFAIL_OUT(ret);
            }
            
            /* TODO: Might consider to validate the identifier
             * but anyway this will be done by compiler... Thus to save the loading
             * time, we will skip this step.
             */
            NDRX_STRCPY_SAFE(fld->cname, tok);
            NDRX_LOG(log_debug, "Got c identifier [%s]", fld->cname);
            
            /******************************************************************* 
             * FB Name -> projection to fielded buffer.
             *******************************************************************/
            tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                NDRX_LOG(log_error, "Expected FB Name field, got EOS, line: %ld", 
                        line);
                ndrx_TPset_error_fmt(TPEINVAL, "Expected FB Name field, "
                        "got EOS, line: %ld", line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>UBFFLDMAX)
            {
                NDRX_LOG(log_error, "UBF name identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, UBFFLDMAX, len, line);
                
                ndrx_TPset_error_fmt(TPEINVAL, "UBF name identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, UBFFLDMAX, len, line);
                
                EXFAIL_OUT(ret);
            }
            
            /* ok, save the field */
            NDRX_STRCPY_SAFE(fld->fbname, tok);
            NDRX_LOG(log_debug, "Got UBF identifier [%s]", fld->fbname);
            
            /******************************************************************* 
             * Parse count...
             *******************************************************************/
            tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            fld->count = (short)atoi(tok);
            
            if (fld->count<1)
            {
                NDRX_LOG(log_error, "Invalid count: %d (parsed from [%s]), line: %ld", 
                        fld->count, tok, line);
                
                ndrx_TPset_error_fmt(TPEINVAL, "Invalid count: %d "
                        "(parsed from [%s]), line: %ld", 
                        fld->count, tok, line);
                
                EXFAIL_OUT(ret);
            }
            
            if (fld->count>FLD_COUNT_MAX)
            {
                NDRX_LOG(log_error, "Invalid count: %d (parsed from [%s]) max: %d,"
                        " line: %ld", 
                        fld->count, tok, FLD_COUNT_MAX, line);
                
                ndrx_TPset_error_fmt(TPEINVAL, "Invalid count: %d (parsed from [%s]) "
                        "max: %d, line: %ld", 
                        fld->count, tok, FLD_COUNT_MAX, line);
                
                EXFAIL_OUT(ret);
            }
            
            
            
            NDRX_LOG(log_debug, "Got count [%hd]", fld->count);
            
            /******************************************************************* 
             * Parse flags
             *******************************************************************/
            tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                NDRX_LOG(log_error, "Expected flags, got EOS, line: %ld", 
                        line);
                ndrx_TPset_error_fmt(TPEINVAL, "Expected flags, got EOS, line: %ld",
                        line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>NDRX_VIEW_FLAGS_LEN)
            {
                NDRX_LOG(log_error, "Flags [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_FLAGS_LEN, len, line);
                
                ndrx_TPset_error_fmt(TPEINVAL, "Flags [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, NDRX_VIEW_FLAGS_LEN, len, line);
                
                EXFAIL_OUT(ret);
            }
            
            /* Decode flags & build binary flags... */
            NDRX_STRCPY_SAFE(fld->flagsstr, tok);
            
            for (i=0; i<len; i++)
            {
                switch (tok[i])
                {
                    case 'C':
                        /* Add element count indicator:
                         * C_<field>
                         * if C is used, then buffer shall include the number
                         * this special field 'short' type to include the element
                         * counts transfered from or to buffer. This goes to UBF
                         * as the id = field number + 1
                         */
                        fld->flags|=NDRX_VIEW_FLAG_ELEMCNT_IND_C;
                        break;
                    case 'F':
                        /* One way mapping struct -> to UBF 
                         * meaning that transfer the data to target only if dong
                         * C->UBF
                         */
                        fld->flags|=NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F;
                        break;
                    case 'L':
                        /* Add length indicator
                         * L_<field> or L_<field>[N] - in case if count > 0
                         * One way mapping struct -> to UBF */
                        fld->flags|=NDRX_VIEW_FLAG_LEN_INDICATOR_L;
                        break;
                    case 'N':
                        /* Zero way mapping */
                        fld->flags|=NDRX_VIEW_FLAG_0WAYMAP_N;
                        break;
                    case 'P':
                        /* Interpret the NULL value's last char as the filler
                         * of the C field trailing.
                         */
                        fld->flags|=NDRX_VIEW_FLAG_NULLFILLER_P;
                        break;
                    case 'S':
                        /*One way mapping UBF -> C struct
                         * If set then transfer data from UBF -> to C at and not
                         * vice versa.
                         */
                        fld->flags|=NDRX_VIEW_FLAG_1WAYMAP_C2UBF_S;
                        break;
                    case '-':
                        NDRX_LOG(log_debug, "No flags set...");
                        break;    
                    default:
                        NDRX_LOG(log_error, "Unknown field flag [%c], line: %ld", 
                                tok[i], line);

                        ndrx_TPset_error_fmt(TPEINVAL, "Unknown field flag [%c], "
                                "line: %ld", 
                                tok[i], line);

                        EXFAIL_OUT(ret);
                        break;
                }
            }/* for tok[i] */
            NDRX_LOG(log_debug, "Got flags [%s] -> %lx", fld->flagsstr, fld->flags);
            
            /******************************************************************* 
             * Parse size
             *******************************************************************/
            tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1);
            NDRX_LOG(log_debug, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (0==strcmp(tok, "-"))
            {
                NDRX_LOG(log_debug, "Empty token -> no size");
                
                if (fld->typecode==BFLD_CARRAY || fld->typecode==BFLD_STRING)
                {
                    NDRX_LOG(log_error, "Size must be specified for string or "
                            "carray, line: %ld", line);

                    ndrx_TPset_error_fmt(TPEINVAL, "Size must be specified for string or "
                            "carray, line: %ld", line);

                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                fld->size = (short)atoi(tok);

                if (fld->size>FLD_SIZE_MAX)
                {
                    NDRX_LOG(log_error, "Invalid size: %d (parsed from [%s]) max: %d,"
                            " line: %ld", 
                            fld->size, tok, FLD_SIZE_MAX, line);

                    ndrx_TPset_error_fmt(TPEINVAL, "Invalid size: %d (parsed from [%s]) "
                            "max: %d, line: %ld", 
                            fld->size, tok, FLD_SIZE_MAX, line);

                    EXFAIL_OUT(ret);
                }
            }
            
            NDRX_LOG(log_debug, "Got size [%hd]", fld->size);
            
            /******************************************************************* 
             * Parse NULL value & the system flags..
             *******************************************************************/

            p2 = tok+strlen(tok);
            p3 = fld->nullval_bin;
            pend = buf + orglen;
            
            /* Search the opening of the data block */
            while (p2<pend)
            {
                if (*p!=EXEOS && *p!=' ' && *p!='\t')
                {
                    break;
                }    
                p2++;
            }
            
            if (p2==pend)
            {
                NDRX_LOG(log_error, "Missing NULL value, line: %ld", 
                                tok[i], line);

                ndrx_TPset_error_fmt(TPEINVAL, "Missing NULL value, line: %ld", 
                        tok[i], line);
                EXFAIL_OUT(ret);
            }
            
            /* detect the null value type (if have one..) */
            
            if (*p2=='\'')
            {
                nulltype = NTYPSQUOTE;
                p2++;
                null_val_start = p2;
            }
            else if (*p2=='"')
            {
                nulltype = NTYPDQUOTE;
                p2++;
                null_val_start = p2;
            }
            else
            {
                nulltype = NTYPSTD;
                null_val_start = p2;
            }
            
            /* scan for closing element... */
            while (p2<pend)
            {
                if (*(p2-1)=='\\')
                {
                    /* so previous was escape... */
                    switch (*p2)
                    {
                        case '0':
                            *p3='\0';
                            break;
                        case 'n':
                            *p3='\n';
                            break;
                        case 't':
                            *p3='\t';
                            break;
                        case 'f':
                            *p3='\f';
                            break;
                        case '\\':
                            *p3='\'';
                            break;
                        case '\'':
                            *p3='\'';
                            break;
                        case '"':
                            *p3='"';
                            break;
                        case 'v':
                            *p3='\v';
                            break;
                        default:
                            *p3=*p2; /* default unknown chars.. */
                            break;   
                    }
                    p3++;
                }
                else if (*p2=='\'' || *p2=='"')
                {
                    if (nulltype != NTYPSQUOTE && nulltype != NTYPDQUOTE)
                    {
                        NDRX_LOG(log_error, "Un-escaped quote [%c], line %ld", 
                                *p2, line);

                        ndrx_TPset_error_fmt(TPEINVAL, "Un-escaped quote [%c], line %ld", 
                                *p2, line);
                        EXFAIL_OUT(ret);
                    }
                    
                    /* this is terminator of our value... */
                    *p2=EXEOS;
                    NDRX_STRCPY_SAFE(fld->nullval, null_val_start);
                    
                    nulltype = NTYPNO;
                            
                    break;
                }
                else if (nulltype != NTYPSQUOTE && nulltype != NTYPDQUOTE &&
                        (*p2==' ' || *p2=='\t'))
                {
                    /* Terminate value here too.. */
                    
                    *p2=EXEOS;
                    NDRX_STRCPY_SAFE(fld->nullval, null_val_start);
                    nulltype = NTYPNO;
                    
                }
                else
                {
                    fld->nullval_bin_len++;
                    *p3=*p2;
                    p3++;
                }
                
                p2++;
            }
            
            if (nulltype != NTYPNO)
            {
                NDRX_LOG(log_error, "Looks like unclosed quotes for "
                        "NULL value, line %ld", line);

                ndrx_TPset_error_fmt(TPEINVAL, "Looks like unclosed quotes for "
                        "NULL value, line %ld", line);
                EXFAIL_OUT(ret);
            }
            
            NDRX_LOG(log_debug, "Got NULL value [%s]", fld->nullval);
            NDRX_DUMP(log_debug, "Got binary version of NULL value", fld->nullval_bin,
                        fld->nullval_bin_len);
            
            /******************************************************************* 
             * Parse NULL value & the system flags..
             *******************************************************************/
            
            if (p2<pend)
            {
                p2++;
                
                tok = strtok_r(p2, FIELD_SEPERATORS, &saveptr1);
                
                /* Extract compiled data... */
                
                if (NULL==tok)
                {
                    if (is_compiled) 
                    {
                        NDRX_LOG(log_error, "Expected compiled data, but not found"
                            ", line %ld", line);

                        ndrx_TPset_error_fmt(TPEINVAL, "Expected compiled data, but not found"
                            ", line %ld", line);
                        EXFAIL_OUT(ret);
                    }
                    else
                    {
                        NDRX_LOG(log_debug, "No compiled data found at the view file...");
                    }
                }
                else
                {
                    /* OK we got some token... */
                    NDRX_LOG(log_debug, "Compiled data: [%s]", tok);
                    
                    len=strlen(tok);
                    
                    if (len>NDRX_VIEW_COMPFLAGS_LEN)
                    {
                        NDRX_LOG(log_error, "Compiled data [%s] too long! Max len: %d,"
                            " but got: %d, line: %ld", 
                            tok, NDRX_VIEW_COMPFLAGS_LEN, len, line);
                
                        ndrx_TPset_error_fmt(TPEINVAL, "Compiled data [%s] too long! Max len: %d,"
                            " but got: %d, line: %ld", 
                            tok, NDRX_VIEW_COMPFLAGS_LEN, len, line);

                        EXFAIL_OUT(ret);
                    }
                    
                    /* ok now extract the data... 
                     * Initially we will have a format:
                     * <key>=value;..;<key>=<value>
                     */
                    tok=strtok_r (tok,";", &saveptr2);
                    while( tok != NULL ) 
                    {
                        int cmplen;
                        char *p3;
                        /* get the setting... */
                        p3 = strchr(tok, '=');
                        cmplen = p3-tok;

                        if (0==strncmp("offset", tok, cmplen))
                        {
                            fld->offset = atol(p+1);
                        }
                        else if (0==strncmp("elmsize", tok, cmplen))
                        {
                            fld->elmsize = atol(p+1);
                        }
                        
                        /* Ignore others...  */
                        
                        tok=strtok_r (NULL,";", &saveptr2);
                    }
                    
                    NDRX_LOG(log_debug, "Compiled offset loaded: %ld, element size: %ld", 
                            fld->offset, fld->elmsize);
                }
                
            }
            else if (is_compiled) 
            {
                NDRX_LOG(log_error, "Expected compiled data, but not found"
                    ", line %ld", line);

                ndrx_TPset_error_fmt(TPEINVAL, "Expected compiled data, but not found"
                    ", line %ld", line);
                EXFAIL_OUT(ret);
            }
            else
            {
                NDRX_LOG(log_debug, "No compiled data found at the view file...");
            }
            
            /******************************************************************* 
             * Finally add field to linked list...
             *******************************************************************/
            DL_APPEND(v->fields, fld);
            fld = NULL;
        }
    }
    
out:

    NDRX_LOG(log_debug, "%s - return %d", __func__, ret);

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    /* shall be normally set to NULL in the end */
    if (NULL!=v)
    {
        NDRX_FREE(v);
    }

    if (NULL!=fld)
    {
        NDRX_FREE(fld);
    }

    return ret;
}

/**
 * Load directory
 * We compare each file to be in the list of the files specified in env variable
 * if found then load it...
 * @param dir
 * @return 
 */
expublic int ndrx_view_load_directory(char *dir)
{
    int ret = EXSUCCEED;
    /* we must expand the env... so that file names would formatted as:
     * ;<fielname>;
     */
    char *env = getenv(CONF_VIEWFILES);
    char dup[PATH_MAX+1];
    char fname_chk[PATH_MAX+1];
    char full_fname[PATH_MAX+1];
    int n;
    struct dirent **namelist = NULL;
    
    if (NULL==env)
    {
        NDRX_LOG(log_error, "Missing env [%s]", CONF_VIEWFILES);
        ndrx_TPset_error_fmt(TPESYSTEM, "Missing env [%s]", CONF_VIEWFILES);
        EXFAIL_OUT(ret);
    }
    
    if (strlen(env)+2 > PATH_MAX)
    {
        NDRX_LOG(log_error, "Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        
        ndrx_TPset_error_fmt(TPESYSTEM, "Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        
        userlog("Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        EXFAIL_OUT(ret);
    }
    
    snprintf(dup, sizeof(dup), ";%s;", env);
    
    ndrx_str_strip(dup, " \t");
   
    /* Open dir for scan */
    n = scandir(dir, &namelist, 0, alphasort);
    if (n < 0)
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed to scan q directory [%s]: %s", 
               dir, strerror(err));
       
        ndrx_TPset_error_fmt(TPEOS, "Failed to scan q directory [%s]: %s", 
               dir, strerror(err));
       
        EXFAIL_OUT(ret);
    }
        
    while (n--)
    {
        if (0==strcmp(namelist[n]->d_name, ".") || 
            0==strcmp(namelist[n]->d_name, ".."))
        {
            NDRX_FREE(namelist[n]);
            continue;
        }
        
        /* Check the file name in list */
        snprintf(fname_chk, sizeof(fname_chk), ";%s;", namelist[n]->d_name);
        
        if (NULL!=strstr(dup, fname_chk))
        {
            snprintf(full_fname, sizeof(full_fname), "%s/%s", dir, namelist[n]->d_name);
            NDRX_LOG(log_debug, "File name [%s] accepted for view object load. "
                    "full path: [%s]", 
                    namelist[n]->d_name, full_fname);
            
            if (EXSUCCEED!=ndrx_view_load_file(full_fname, EXTRUE))
            {
                NDRX_LOG(log_error, "Failed to load view object file: [%s]", full_fname);
                EXFAIL_OUT(ret);
            }
            
        }
        
        NDRX_FREE(namelist[n]);
    }

    
out:
                            
    if (NULL!=namelist)
    {
        while (n>=0)
        {
            NDRX_FREE(namelist[n]);
            n--;
        }
        NDRX_FREE(namelist);
        namelist = NULL;
    }

    return ret;
    
}

/**
 * Load the directories by CONF_VIEWDIR
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_load_directories(void)
{
    int ret = EXSUCCEED;
    char *tok;
    char *saveptr1;
    char *env = getenv(CONF_VIEWDIR);
    
    if (NULL==env)
    {
        NDRX_LOG(log_error, "Missing env [%s]", CONF_VIEWDIR);
        ndrx_TPset_error_fmt(TPESYSTEM, "Missing env [%s]", CONF_VIEWDIR);
        EXFAIL_OUT(ret);
    }
    
    tok=strtok_r (tok,":", &saveptr1);
    while( tok != NULL ) 
    {
        if (EXSUCCEED!=ndrx_view_load_directory(tok))
        {
            EXFAIL_OUT(ret);
        }
        
        tok=strtok_r (NULL,":", &saveptr1);
    }
    
out:    
    return ret;
}
