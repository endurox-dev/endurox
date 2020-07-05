/**
 * @brief VIEW buffer type support - parser
 *
 * @file view_parser.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dirent.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>
#include <ubfview.h>
#include <ferror.h>

#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {ndrx_Bunset_error(); \
}
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate MUTEX_LOCKDECL(M_view_change_lock);
exprivate MUTEX_LOCKDECL(M_views_init_lock);
exprivate int M_no_ubf_proc = EXFALSE; /* Do not process UBF during loading... */

exprivate  volatile int M_views_loaded = EXFALSE; /* Is views loaded? */

/*---------------------------Prototypes---------------------------------*/

/**
 * Configure view loader
 * @param no_ubf_proc Do not process UBF during loading...
 */
expublic void ndrx_view_loader_configure(int no_ubf_proc)
{
    M_no_ubf_proc = no_ubf_proc;
    
    UBF_LOG(log_warn, "Do not process UBF: %s", M_no_ubf_proc?"Yes":"No");
    
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
    char *tok, *tok2;
    enum states { INFILE, INVIEW} state = INFILE;
    enum nulltypes { NTYPNO, NTYPSTD, NTYPSQUOTE, NTYPDQUOTE} nulltype = NTYPNO;
    int len;
    int was_quotes;
    char *p, *p2, *pend, *null_val_start, *p3;
    long line=0;
    dtype_str_t *dtyp;
    ndrx_typedview_field_t *fld = NULL;
    int i;
    int esc_open;
    int dim_size;
    /* Additional checks for compiled object: */
    int file_platform_ok = EXFALSE;
    int file_arch_ok = EXFALSE;
    int file_wsize_ok = EXFALSE;
    int view_ssize_ok;
    int view_cksum_ok;
    
    API_ENTRY;
    
    UBF_LOG(log_debug, "%s - enter", __func__);
    
    if (NULL==(f=NDRX_FOPEN(fname, "r")))
    {
        int err = errno;
        UBF_LOG(log_error, "Failed to open view file [%s]: %s", 
                fname, strerror(err));
        ndrx_Bset_error_fmt(BVFOPEN, "Failed to open view file [%s]: %s", 
                fname, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* Read line by line... */
    while (NULL!=fgets(buf, sizeof(buf), f))
    {
        line++;
        
        /* remove trailing newline... */
        ndrx_chomp(buf);
        
        orglen = strlen(buf);
        
        UBF_LOG(log_dump, "Got VIEW file line: [%s], line: %ld", buf, line);
        
        if ('#'==buf[0])
        {
            /* this is comment, ignore */
            
            if (is_compiled)
            {
                if (0==strncmp("#@__platform=", buf, 13))
                {
                    UBF_LOG(log_debug, "Found platform data, parsing...");
                    tok2=strtok_r (buf,";", &saveptr2);
                    while( tok2 != NULL ) 
                    {
                        int cmplen;
                        char *p3;
                        /* get the setting... */
                        p3 = strchr(tok2, '=');
                        cmplen = p3-tok2;

                        if (0==strncmp("#@__platform", tok2, cmplen))
                        {
                            if (0!=strcmp(NDRX_BUILD_OS_NAME, p3+1))
                            {
                                UBF_LOG(log_error, "Invalid platform, expected: "
                                        "[%s] got [%s] - please recompile the "
                                        "view file with viewc, line: %ld", 
                                        NDRX_BUILD_OS_NAME, p3+1, line);
                                ndrx_Bset_error_fmt(BBADVIEW, "Invalid platform "
                                        "expected: [%s] got [%s] - please recompile "
                                        "the view file with viewc, line: %ld", 
                                        NDRX_BUILD_OS_NAME, p3+1, line);
                                EXFAIL_OUT(ret);
                            }
                            else
                            {
                                file_platform_ok = EXTRUE;
                            }
                        }
                        else if (0==strncmp("@__arch", tok2, cmplen))
                        {
                            if (0!=strcmp(NDRX_CPUARCH, p3+1))
                            {
                                UBF_LOG(log_error, "Invalid CPU arch, expected: "
                                            "[%s] got [%s] - please recompile the "
                                            "view file with viewc, line: %ld", 
                                            NDRX_CPUARCH, p3+1, line);
                                ndrx_Bset_error_fmt(BBADVIEW, "Invalid CPU arch, expected: "
                                        "expected: [%s] got [%s] - please recompile "
                                        "the view file with viewc, line: %ld", 
                                        NDRX_CPUARCH, p3+1, line);
                                EXFAIL_OUT(ret);
                            }
                            else
                            {
                                file_arch_ok = EXTRUE;
                            }
                        }
                        else if (0==strncmp("@__wsize", tok2, cmplen))
                        {
                            int ws=atoi(p3+1);
                            
                            if (ws!=NDRX_WORD_SIZE)
                            {
                                UBF_LOG(log_error, "Invalid platform word size, expected: "
                                        "[%d] got [%d] - please recompile the "
                                        "view file with viewc, line: %ld", 
                                        NDRX_WORD_SIZE, ws, line);
                                ndrx_Bset_error_fmt(BBADVIEW, "Invalid platfrom "
                                        "word size, expected: "
                                        "expected: [%d] got [%d] - please recompile "
                                        "the view file with viewc, line: %ld", 
                                        NDRX_WORD_SIZE, ws, line);
                                EXFAIL_OUT(ret);
                            }
                            else
                            {
                                file_wsize_ok = EXTRUE;
                            }
                        }
                        
                        /* Ignore others...  */
                        
                        tok2=strtok_r (NULL,";", &saveptr2);
                    }
                }
                else if (0==strncmp("#@__ssize=", buf, 10))
                {
                    UBF_LOG(log_debug, "Structure data, parsing...");
                    tok2=strtok_r (buf,";", &saveptr2);
                    while( tok2 != NULL ) 
                    {
                        int cmplen;
                        char *p3;
                        /* get the setting... */
                        p3 = strchr(tok2, '=');
                        cmplen = p3-tok2;

                        if (0==strncmp("#@__ssize", tok2, cmplen))
                        {
                            v->ssize = atol(p3+1);
                            
                            UBF_LOG(log_debug, "Struct size loaded: [%ld]",
                                    v->ssize);
                            
                            if (v->ssize<0)
                            {
                                UBF_LOG(log_error, "Invalid size %ld, line %ld", 
                                        v->ssize, line);
                                ndrx_Bset_error_fmt(BBADVIEW, "Invalid size %ld, line %ld", 
                                        v->ssize, line);
                                EXFAIL_OUT(ret);   
                            }
                            
                            view_ssize_ok = EXTRUE;
            
                        }
                        else if (0==strncmp("@__cksum", tok2, cmplen))
                        {
                            long cksum = atoll(p3+1);
                            long cksum_built = v->cksum;
                            
                            if (cksum!=cksum_built)
                            {
                                UBF_LOG(log_error, "Invalid VIEW [%s] checksum, expected: "
                                        "[%ld] got [%ld] - please recompile the "
                                        "view file with viewc, line: %ld", 
                                        v->vname, cksum_built, cksum, line);
                                ndrx_Bset_error_fmt(BBADVIEW, "Invalid VIEW [%s] "
                                        "checksum, expected: "
                                        "[%ld] got [%ld] - please recompile the "
                                        "view file with viewc, line: %ld", 
                                        v->vname, cksum_built, cksum, line);
                                EXFAIL_OUT(ret);
                            }
                            
                            view_cksum_ok = EXTRUE;
                        }
                       
                        /* Ignore others...  */
                        
                        tok2=strtok_r (NULL,";", &saveptr2);
                    }
                }
            }
            
            continue;
        }
        else if (('\n'==buf[0]) || (('\r'==buf[0]) && ('\n'==buf[1])))
        {
            /* this is newline, ignore */
            continue;
        }
        else if (EXEOS==buf[0])
        {
            /* Some strange line? */
            continue;
        }
        
        /* Remove trailing white space.. */
        ndrx_str_rstrip(buf, NDRX_VIEW_FIELD_SEPERATORS);
        
        p = buf;
        /* Strip off the leading white space */
        while (' '==*p || '\t'==*p)
        {
            p++;
        }
        
        /* tokenize the line  */
        if (INFILE==state)
        {   
            tok = strtok_r(p, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            
            if (0!=strcmp(tok, NDRX_VIEW_TOKEN_START))
            {
                /* Not in view -> FAIL */
                UBF_LOG(log_error, "Expected [%s] but got [%s], line: %ld", 
                        NDRX_VIEW_TOKEN_START, tok, line);
                ndrx_Bset_error_fmt(BVFSYNTAX, "Expected [%s] but got [%s], line: %ld", 
                        NDRX_VIEW_TOKEN_START, tok, line);
                EXFAIL_OUT(ret);
            }

            if (NULL==(tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1)))
            {
                UBF_LOG(log_error, "Missing identifier after %s, line: %ld", 
                        NDRX_VIEW_TOKEN_START, line);
                ndrx_Bset_error_fmt(BVFSYNTAX, "Missing identifier after %s, line: %ld", 
                        NDRX_VIEW_TOKEN_START, line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len > NDRX_VIEW_NAME_LEN)
            {
                UBF_LOG(log_error, "View identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_NAME_LEN, len, line);
                ndrx_Bset_error_fmt(BVFSYNTAX, "View identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, NDRX_VIEW_NAME_LEN, len, line);
                EXFAIL_OUT(ret);
            }
            
            /* Got view ok, allocate the object */
            v = NDRX_CALLOC(1, sizeof(ndrx_typedview_t));

            if (NULL==v)
            {
                int err = errno;
                UBF_LOG(log_error, "Failed to allocate ndrx_typedview_t: %s, "
                        "line: %ld", 
                        strerror(err), line);

                ndrx_Bset_error_fmt(BEUNIX, "Failed to allocate "
                        "ndrx_typedview_t: %s, line: %ld", 
                        strerror(err), line);
                EXFAIL_OUT(ret);    
            }
            
            NDRX_STRCPY_SAFE(v->vname, tok);
            /* setup file name too */
            NDRX_STRCPY_SAFE(v->filename, fname);
            
            UBF_LOG(log_debug, "Parsing view [%s]", v->vname);
            state = INVIEW;
            view_ssize_ok = EXFALSE;
            view_cksum_ok = EXFALSE;
            
        }
        else if (INVIEW==state)
        {
            short typ=EXFAIL;
            short typfull=EXFAIL;
            
            tok = strtok_r(p, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (0==strcmp(NDRX_VIEW_TOKEN_END, tok))
            {
                /* Bug #191 */
                if (is_compiled && (!view_ssize_ok || !view_cksum_ok))
                {
                    UBF_LOG(log_error, "Compiled view data not found ssize=%d, cksum=%d, "
                        "line: %ld",  view_ssize_ok, view_cksum_ok, line);

                    ndrx_Bset_error_fmt(BBADVIEW, "Compiled view data not found "
                            "ssize=%d, cksum=%d, line: %ld",  
                            view_ssize_ok, view_cksum_ok, line);
                    EXFAIL_OUT(ret);    
                }
                
                UBF_LOG(log_debug, "View [%s] finishing off -> add to hash", 
                        v->vname);
                EXHASH_ADD_STR(ndrx_G_view_hash, vname, v);
                v = NULL;
                state = INFILE;
                continue;
            }
            
            /******************************************************************* 
             * OK this is the string line... 
             * The first is field type name
             * Here we might get 'int'. this will be internally stored as long.
             *******************************************************************/
            dtyp = G_dtype_str_map;
            while(EXEOS!=dtyp->fldname[0])
            {
                if (0==strcmp(dtyp->fldname, tok) || 
                        (dtyp->altname && 0==strcmp(dtyp->altname, tok)))
                {
                    typ = dtyp->fld_type;
                    break;
                }
                dtyp++;
            }
            
            if (BFLD_INT==typ)
            {
                if (0==strcmp(tok, "int"))
                {
                    typ = BFLD_LONG; 
                    typfull = BFLD_INT;
                }
                else
                {
                    UBF_LOG(log_error, "Invalid data type [%s], line: %ld", 
                            tok, line);
                    ndrx_Bset_error_fmt(BVFSYNTAX, "Invalid data type [%s], line: %ld", 
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
                UBF_LOG(log_error, "Failed to allocate ndrx_typedview_field_t: %s,"
                        " line: %ld", 
                        strerror(err), line);

                ndrx_Bset_error_fmt(BEUNIX, "Failed to allocate ndrx_typedview_field_t: "
                        "%s, line: %ld", 
                        strerror(err));
                EXFAIL_OUT(ret);    
            }
            
            fld->typecode = typ;
            fld->typecode_full = typfull;
            NDRX_STRCPY_SAFE(fld->type_name, tok);
            
            /* Add type to checksum */
            ndrx_view_cksum_update(v, fld->type_name, strlen(fld->type_name));
            UBF_LOG(log_dump, "Got type code UBF=%d full code=%d", 
                    fld->typecode, fld->typecode_full);
            
            /******************************************************************* 
             * C Field name 
             *******************************************************************/
            tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                UBF_LOG(log_error, "Expected C field name, got EOS, line: %ld");
                ndrx_Bset_error_fmt(BVFSYNTAX, "Expected C field name, got EOS, "
                        "line %ld", line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>NDRX_VIEW_CNAME_LEN)
            {
                UBF_LOG(log_error, "C field identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_CNAME_LEN, len, line);
                
                ndrx_Bset_error_fmt(BVFSYNTAX, "C field identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, NDRX_VIEW_CNAME_LEN, len, line);
                
                EXFAIL_OUT(ret);
            }
            
            /* TODO: Might consider to validate the identifier
             * but anyway this will be done by compiler... Thus to save the loading
             * time, we will skip this step.
             */
            NDRX_STRCPY_SAFE(fld->cname, tok);
            
            /* Add cname to checksum */
            ndrx_view_cksum_update(v, fld->cname, strlen(fld->cname));
            
            UBF_LOG(log_dump, "Got c identifier [%s]", fld->cname);
            
            /******************************************************************* 
             * FB Name -> projection to fielded buffer.
             *******************************************************************/
            tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                UBF_LOG(log_error, "Expected FB Name field, got EOS, line: %ld", 
                        line);
                ndrx_Bset_error_fmt(BVFSYNTAX, "Expected FB Name field, "
                        "got EOS, line: %ld", line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>UBFFLDMAX)
            {
                UBF_LOG(log_error, "UBF name identifier [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, UBFFLDMAX, len, line);
                
                ndrx_Bset_error_fmt(BVFSYNTAX, "UBF name identifier [%s] too long!"
                        " Max len: %d, but got: %d, line: %ld", 
                        tok, UBFFLDMAX, len, line);
                
                EXFAIL_OUT(ret);
            }
            
            /* ok, save the field */
            NDRX_STRCPY_SAFE(fld->fbname, tok);
            UBF_LOG(log_dump, "Got UBF identifier [%s]", fld->fbname);
            
            /*TODO: add - to defines ... */
            if (is_compiled && !M_no_ubf_proc && 0!=strcmp("-", fld->fbname))
            {
                UBF_LOG(log_dump, "About to resolve field id..");
                
                fld->ubfid = Bfldid(fld->fbname);
                
                if (BBADFLDID==fld->ubfid)
                {
                    UBF_LOG(log_error, "Failed to resolve id for field [%s], line: %ld", 
                        fld->fbname, line);
                
                    ndrx_Bset_error_fmt(BBADFLD, "Failed to resolve id for "
                            "field [%s], line: %ld", 
                        fld->fbname, line);

                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                fld->ubfid = BBADFLDID;
            }
            
            /******************************************************************* 
             * Parse count...
             *******************************************************************/
            tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            fld->count = (short)atoi(tok);
            
            if (fld->count<1)
            {
                UBF_LOG(log_error, "Invalid count: %d (parsed from [%s]), line: %ld", 
                        fld->count, tok, line);
                
                ndrx_Bset_error_fmt(BVFSYNTAX, "Invalid count: %d "
                        "(parsed from [%s]), line: %ld", 
                        fld->count, tok, line);
                
                EXFAIL_OUT(ret);
            }
            
            if (fld->count>NDRX_VIEW_FLD_COUNT_MAX)
            {
                UBF_LOG(log_error, "Invalid count: %d (parsed from [%s]) max: %d,"
                        " line: %ld", 
                        fld->count, tok, NDRX_VIEW_FLD_COUNT_MAX, line);
                
                ndrx_Bset_error_fmt(BVFSYNTAX, "Invalid count: %d (parsed from [%s]) "
                        "max: %d, line: %ld", 
                        fld->count, tok, NDRX_VIEW_FLD_COUNT_MAX, line);
                
                EXFAIL_OUT(ret);
            }

            /* Add count to checksum */
            ndrx_view_cksum_update(v, tok, strlen(tok));
            
            UBF_LOG(log_dump, "Got count [%hd]", fld->count);
            
            /******************************************************************* 
             * Parse flags
             *******************************************************************/
            tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (NULL==tok)
            {
                UBF_LOG(log_error, "Expected flags, got EOS, line: %ld", 
                        line);
                ndrx_Bset_error_fmt(BVFSYNTAX, "Expected flags, got EOS, line: %ld",
                        line);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(tok);
            if (len>NDRX_VIEW_FLAGS_LEN)
            {
                UBF_LOG(log_error, "Flags [%s] too long! Max len: %d,"
                        " but got: %d, line: %ld", 
                        tok, NDRX_VIEW_FLAGS_LEN, len, line);
                
                ndrx_Bset_error_fmt(BVFSYNTAX, "Flags [%s] too long!"
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
                        
                        if (0==strcmp(fld->fbname, NDRX_VIEW_EMPTY_PARAM))
                        {
                            ndrx_Bset_error_fmt(BVFSYNTAX, "FB name not set, but "
                                    "F flag found, line: %ld", line);
                            EXFAIL_OUT(ret);
                        }
                        
                        break;
                    case 'L':
                        /* Add length indicator
                         * L_<field> or L_<field>[N] - in case if count > 0
                         * One way mapping struct -> to UBF */
                        fld->flags|=NDRX_VIEW_FLAG_LEN_INDICATOR_L;
                        
                        if (BFLD_STRING != fld->typecode_full && 
                                BFLD_CARRAY != fld->typecode_full)
                        {
                            ndrx_Bset_error_fmt(BVFSYNTAX, "Flag L not valid for type %s! "
                                    "L is valid only for string and carray, on line %ld", 
                                    fld->type_name, line);
                            EXFAIL_OUT(ret);
                        }
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
                        fld->flags|=NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S;
                        
                        if (0==strcmp(fld->fbname, NDRX_VIEW_EMPTY_PARAM))
                        {
                            ndrx_Bset_error_fmt(BVFSYNTAX, "FB name not set, but "
                                    "S flag found, line: %ld", line);
                            EXFAIL_OUT(ret);
                        }
                        
                        break;
                    case '-':
                        UBF_LOG(log_dump, "No flags set...");
                        break;    
                    default:
                        UBF_LOG(log_error, "Unknown field flag [%c], line: %ld", 
                                tok[i], line);

                        ndrx_Bset_error_fmt(BVFSYNTAX, "Unknown field flag [%c], "
                                "line: %ld", 
                                tok[i], line);

                        EXFAIL_OUT(ret);
                        break;
                }
            }/* for tok[i] */
            
            if ((fld->flags & NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F || 
                    fld->flags & NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S) && 
                    fld->flags & NDRX_VIEW_FLAG_0WAYMAP_N)
            {
                ndrx_Bset_error_fmt(BVFSYNTAX, "F & S flags are not compatible with N flag, "
                                "line: %ld", 
                                tok[i], line);
                EXFAIL_OUT(ret);
            }
            
            /* Add flags to checksum */
            ndrx_view_cksum_update(v, fld->flagsstr, strlen(fld->flagsstr));

            UBF_LOG(log_dump, "Got flags [%s] -> %lx", fld->flagsstr, fld->flags);
            
            /******************************************************************* 
             * Parse size
             *******************************************************************/
            tok = strtok_r(NULL, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
            UBF_LOG(log_dump, "Got token [%s]", (NULL==tok?"<NULL>":tok));
            
            if (0==strcmp(tok, "-"))
            {
                UBF_LOG(log_dump, "Empty token -> no size");
                
                if (fld->typecode==BFLD_CARRAY || fld->typecode==BFLD_STRING)
                {
                    UBF_LOG(log_error, "Size must be specified for string or "
                            "carray, line: %ld", line);

                    ndrx_Bset_error_fmt(BVFSYNTAX, "Size must be specified for string or "
                            "carray, line: %ld", line);

                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                fld->size = (short)atoi(tok);

                if (fld->size>NDRX_VIEW_FLD_SIZE_MAX)
                {
                    UBF_LOG(log_error, "Invalid size: %d (parsed from [%s]) max: %d,"
                            " line: %ld", 
                            fld->size, tok, NDRX_VIEW_FLD_SIZE_MAX, line);

                    ndrx_Bset_error_fmt(BVFSYNTAX, "Invalid size: %d (parsed from [%s]) "
                            "max: %d, line: %ld", 
                            fld->size, tok, NDRX_VIEW_FLD_SIZE_MAX, line);

                    EXFAIL_OUT(ret);
                }
            }
            
            /* Add size to checksum */
            ndrx_view_cksum_update(v, tok, strlen(tok));
            
            UBF_LOG(log_dump, "Got size [%hd]", fld->size);
            
            /******************************************************************* 
             * Parse NULL value & the system flags..
             *******************************************************************/

            p2 = tok+strlen(tok);
            p3 = fld->nullval_bin;
            pend = buf + orglen;
            was_quotes = EXFALSE;
            /*UBF_LOG(log_debug, "p2=%p pend=%p", p2, pend);*/
            
            /* Search the opening of the data block */
            while (p2<pend)
            {
                /*UBF_LOG(log_debug, "At: %p testing [%c]", p2, *p2);*/
                
                if (*p2!=EXEOS && *p2!=' ' && *p2!='\t')
                {
                    break;
                }
                /*UBF_LOG(log_debug, "At: %p skipping [%c]", p2, *p2);*/
                
                p2++;
            }
            
            UBF_LOG(log_dump, "At %p value [%c]", p2, *p2);
            
            if (p2==pend)
            {
                UBF_LOG(log_error, "Missing NULL value, line: %ld", 
                                tok[i], line);

                ndrx_Bset_error_fmt(BVFSYNTAX, "Missing NULL value, line: %ld", 
                        tok[i], line);
                EXFAIL_OUT(ret);
            }
            
            /* detect the null value type (if have one..) */
            
            if (*p2=='\'')
            {
                nulltype = NTYPSQUOTE;
                fld->nullval_quotes = NDRX_VIEW_QUOTES_SINGLE;
                p2++;
                null_val_start = p2;
            }
            else if (*p2=='"')
            {
                nulltype = NTYPDQUOTE;
                fld->nullval_quotes = NDRX_VIEW_QUOTES_DOUBLE;
                p2++;
                null_val_start = p2;
            }
            else
            {
                fld->nullval_quotes = NDRX_VIEW_QUOTES_NONE;
                nulltype = NTYPSTD;
                null_val_start = p2;
            }
            
            /* scan for closing element... */
            esc_open = EXFALSE;
            while (p2<pend)
            {
                if (!esc_open && '\\'==*p2)
                {
                    esc_open = EXTRUE;
                }
                else if (esc_open)
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
                            *p3='\\';
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
                    fld->nullval_bin_len++;
                    esc_open = EXFALSE;
                }
                else if (*p2=='\'' || *p2=='"')
                {
                    if (nulltype != NTYPSQUOTE && nulltype != NTYPDQUOTE)
                    {
                        UBF_LOG(log_error, "Un-escaped quote [%c], line %ld", 
                                *p2, line);

                        ndrx_Bset_error_fmt(BVFSYNTAX, "Un-escaped quote [%c], line %ld", 
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
                    UBF_LOG(log_dump, "Terminating non quoted NULL data");
                    /* Terminate value here too.. */
                    
                    *p2=EXEOS;
                    NDRX_STRCPY_SAFE(fld->nullval, null_val_start);
                    nulltype = NTYPNO;
                    break;
                    
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
                if (nulltype == NTYPSTD)
                {
                    /* we are at the end... (no compiled data) */
                    UBF_LOG(log_dump, "At th end, no compiled data");
                    *p2 = EXEOS;
                    NDRX_STRCPY_SAFE(fld->nullval, null_val_start);
                    nulltype = NTYPNO;
                    
                }
                else
                {
                    UBF_LOG(log_error, "Looks like unclosed quotes for "
                            "NULL value, line %ld", line);

                    ndrx_Bset_error_fmt(BVFSYNTAX, "Looks like unclosed quotes for "
                            "NULL value, line %ld", line);
                    EXFAIL_OUT(ret);
                }
            }
            
            UBF_LOG(log_dump, "Got NULL value [%s]", fld->nullval);
            
            /* for doubles, etc... parsing! */
            fld->nullval_bin[fld->nullval_bin_len]=EXEOS;
            
            UBF_DUMP(log_dump, "Got binary version of NULL value", fld->nullval_bin,
                        fld->nullval_bin_len);
                     
            if (!fld->nullval_quotes)
            {
                if (0==strcmp(fld->nullval, "NONE"))
                {
                    fld->nullval_none = EXTRUE;
                    UBF_LOG(log_debug, "NONE keyword specified -> no NULL value...");
                }
                else if (0==strcmp(fld->nullval, "-"))
                {
                    fld->nullval_bin[0] = EXEOS;
                    fld->nullval_bin_len = 1;
                    fld->nullval_default = EXTRUE;
                    UBF_LOG(log_dump, "Default NULL value used...");
                }
            }
            
            /* Build pre-compile compare value... */
            if (fld->nullval_default || fld->nullval_none)
            {
                /* These are loaded by calloc:
                fld->nullval_short=0;
                fld->nullval_int=0;
                fld->nullval_long=0;
                fld->nullval_float=0.0;
                fld->nullval_double=0.0;
                */
            }
            else
            {
                switch (fld->typecode_full)
                {
                    case BFLD_SHORT:
                        fld->nullval_short = (short)atoi(fld->nullval_bin);
                        UBF_LOG(log_dump, "nullval_short=%hd (%s)", 
                                fld->nullval_short, fld->nullval_bin);
                        break;
                    case BFLD_INT:
                        fld->nullval_int = atoi(fld->nullval_bin);
                        UBF_LOG(log_dump, "nullval_int=%hd (%s)", 
                                fld->nullval_int, fld->nullval_bin);
                        break;
                    case BFLD_LONG:
                        fld->nullval_long = atol(fld->nullval_bin);
                        UBF_LOG(log_dump, "nullval_long=%hd (%s)", 
                                fld->nullval_long, fld->nullval_bin);
                        break;
                    case BFLD_FLOAT:
                        fld->nullval_float = (float)atof(fld->nullval_bin);
                        UBF_LOG(log_dump, "nullval_float=%f (%s)", 
                                fld->nullval_float, fld->nullval_bin);
                        break;
                    case BFLD_DOUBLE:
                        fld->nullval_double = atof(fld->nullval_bin);
                        UBF_LOG(log_dump, "nullval_double=%lf (%s)", 
                                fld->nullval_double, fld->nullval_bin);
                        break;
                    case BFLD_STRING:
                        
                        /* needs currently to use count only.. */
                        dim_size = fld->size;
                        /* -1 for EOS */
                        if (dim_size-1 < fld->nullval_bin_len)
                        {
                            UBF_LOG(log_error, "Invalid NULL length: %d, "
                                    "string buffer max with out EOS: %d"
                                ", line %ld", fld->nullval_bin_len, dim_size-1, line);

                            ndrx_Bset_error_fmt(BBADVIEW, "Invalid NULL length: %d, "
                                    "string buffer max with out EOS: %d"
                                ", line %ld", fld->nullval_bin_len, dim_size-1, line);
                            EXFAIL_OUT(ret);
                        }
                        break;
                    case BFLD_CARRAY:
                        dim_size = fld->size;
                        
                        if (dim_size < fld->nullval_bin_len)
                        {
                            UBF_LOG(log_error, "Invalid NULL length: %d, "
                                    "carray buffer max: %d"
                                ", line %ld", fld->nullval_bin_len, dim_size, line);

                            ndrx_Bset_error_fmt(BBADVIEW, "Invalid NULL length: %d, "
                                    "carray buffer max: %d"
                                ", line %ld", fld->nullval_bin_len, dim_size, line);
                            EXFAIL_OUT(ret);
                        }
                        break;
                }
            }
            
            /* Add null value too to the checksum */
            ndrx_view_cksum_update(v, fld->nullval, strlen(fld->nullval));
            
            /******************************************************************* 
             * Parse NULL value & the system flags..
             *******************************************************************/
            
            if (p2<pend)
            {
                p2++;
                
                tok = strtok_r(p2, NDRX_VIEW_FIELD_SEPERATORS, &saveptr1);
                
                /* Extract compiled data... */
                
                if (NULL==tok)
                {
                    if (is_compiled) 
                    {
                        UBF_LOG(log_error, "Expected compiled data, but not found"
                            ", line %ld", line);

                        ndrx_Bset_error_fmt(BBADVIEW, "Expected compiled data, but not found"
                            ", line %ld", line);
                        EXFAIL_OUT(ret);
                    }
                    else
                    {
                        UBF_LOG(log_dump, "No compiled data found at the view file...");
                    }
                }
                else
                {
                    /* OK we got some token... */
                    UBF_LOG(log_dump, "Compiled data: [%s]", tok);
                    
                    len=strlen(tok);
                    
                    if (len>NDRX_VIEW_COMPFLAGS_LEN)
                    {
                        UBF_LOG(log_error, "Compiled data [%s] too long! Max len: %d,"
                            " but got: %d, line: %ld", 
                            tok, NDRX_VIEW_COMPFLAGS_LEN, len, line);
                
                        ndrx_Bset_error_fmt(BBADVIEW, "Compiled data [%s] too long! Max len: %d,"
                            " but got: %d, line: %ld", 
                            tok, NDRX_VIEW_COMPFLAGS_LEN, len, line);

                        EXFAIL_OUT(ret);
                    }
                    
                    /* ok now extract the data... 
                     * Initially we will have a format:
                     * <key>=value;..;<key>=<value>
                     */
                    tok2=strtok_r (tok,";", &saveptr2);
                    fld->length_fld_offset=EXFAIL;
                    fld->count_fld_offset=EXFAIL;
                    while( tok2 != NULL ) 
                    {
                        int cmplen;
                        char *p3;
                        /* get the setting... */
                        p3 = strchr(tok2, '=');
                        cmplen = p3-tok2;

                        if (0==strncmp("offset", tok2, cmplen))
                        {
                            fld->offset = atol(p3+1);
                        }
                        else if (0==strncmp("fldsize", tok2, cmplen))
                        {
                            fld->fldsize = atol(p3+1);
                        }
                        else if (0==strncmp("loffs", tok2, cmplen))
                        {
                            fld->length_fld_offset = atol(p3+1);
                        }
                        else if (0==strncmp("coffs", tok2, cmplen))
                        {
                            fld->count_fld_offset = atol(p3+1);
                        }
                        
                        /* Ignore others...  */
                        
                        tok2=strtok_r (NULL,";", &saveptr2);
                    }
                    
                    UBF_LOG(log_dump, "Compiled offset loaded: %ld, element size: %ld", 
                            fld->offset, fld->fldsize);
                }
                
            }
            else if (is_compiled) 
            {
                UBF_LOG(log_error, "Expected compiled data, but not found"
                    ", line %ld", line);

                ndrx_Bset_error_fmt(BBADVIEW, "Expected compiled data, but not found"
                    ", line %ld", line);
                EXFAIL_OUT(ret);
            }
            else
            {
                UBF_LOG(log_dump, "No compiled data found at the view file...");
            }
            
            /******************************************************************* 
             * Finally add field to linked list...
             *******************************************************************/
            DL_APPEND(v->fields, fld);
            
            /* Add field to HASH too */
            
            EXHASH_ADD_STR(v->fields_h, cname, fld);
            
            fld = NULL;            
        }
    }
    
    if (INFILE!=state)
    {
        UBF_LOG(log_error, "Invalid state [%d] -> VIEW not terminated with "
                "END, line: %ld", state, line);
        ndrx_Bset_error_fmt(BVFSYNTAX, "Invalid state [%d] -> VIEW not terminated with "
                "END, line: %ld", state, line);
        EXFAIL_OUT(ret);
    }
    
    /* Bug #191 */
    if (is_compiled && (!file_platform_ok || !file_arch_ok || !file_wsize_ok))
    {
        UBF_LOG(log_error, "Compiled view data not found platform=%d "
                "arch=%d wsize=%d, line: %ld",  
                file_platform_ok, file_arch_ok, file_wsize_ok, line);
        
        ndrx_Bset_error_fmt(BBADVIEW, "Compiled view data not found platform=%d "
                "arch=%d wsize=%d, line: %ld",  
                file_platform_ok, file_arch_ok, file_wsize_ok, line);
        
        EXFAIL_OUT(ret);    
    }
    
out:

    UBF_LOG(log_debug, "%s - return %d", __func__, ret);

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
        UBF_LOG(log_error, "Missing env [%s]", CONF_VIEWFILES);
        ndrx_Bset_error_fmt(BEUNIX, "Missing env [%s]", CONF_VIEWFILES);
        EXFAIL_OUT(ret);
    }
    
    if (strlen(env)+2 > PATH_MAX)
    {
        UBF_LOG(log_error, "Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        
        ndrx_Bset_error_fmt(BEUNIX, "Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        
        userlog("Invalid [%s] -> too long, max: %d", 
                CONF_VIEWFILES, PATH_MAX-2);
        EXFAIL_OUT(ret);
    }
    
    snprintf(dup, sizeof(dup), ",%s,", env);
    
    ndrx_str_strip(dup, " \t");
   
    /* Open dir for scan */
    n = scandir(dir, &namelist, 0, alphasort);
    if (n < 0)
    {
        int err = errno;
        UBF_LOG(log_error, "Failed to scan q directory [%s]: %s", 
               dir, strerror(err));
       
        ndrx_Bset_error_fmt(BEUNIX, "Failed to scan q directory [%s]: %s", 
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
        snprintf(fname_chk, sizeof(fname_chk), ",%s,", namelist[n]->d_name);
        
        if (NULL!=strstr(dup, fname_chk))
        {
            snprintf(full_fname, sizeof(full_fname), "%s/%s", dir, namelist[n]->d_name);
            UBF_LOG(log_debug, "File name [%s] accepted for view object load. "
                    "full path: [%s]", 
                    namelist[n]->d_name, full_fname);
            
            if (EXSUCCEED!=ndrx_view_load_file(full_fname, EXTRUE))
            {
                UBF_LOG(log_error, "Failed to load view object file: [%s]", full_fname);
                EXFAIL_OUT(ret);
            }
            
            UBF_LOG(log_debug, "VIEW [%s] loaded OK.", namelist[n]->d_name);
            
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
    char *env;
    char dirs[PATH_MAX+1];
    env = getenv(CONF_VIEWDIR);
    if (NULL==env)
    {
        UBF_LOG(log_error, "Missing env [%s]", CONF_VIEWDIR);
        ndrx_Bset_error_fmt(BEUNIX, "Missing env [%s]", CONF_VIEWDIR);
        EXFAIL_OUT(ret);
    }
    
    NDRX_STRCPY_SAFE(dirs, env);
    
    UBF_LOG(log_debug, "Splitting: [%s]", dirs);
    tok=strtok_r (dirs,":", &saveptr1);
    while( tok != NULL ) 
    {
        UBF_LOG(log_debug, "Loading directory [%s]...", tok);
        if (EXSUCCEED!=ndrx_view_load_directory(tok))
        {
            EXFAIL_OUT(ret);
        }
        
        tok=strtok_r (NULL,":", &saveptr1);
    }
    
    
out:
                        
    if (EXSUCCEED==ret)
    {
        M_views_loaded = EXTRUE;
        UBF_LOG(log_info, "Views loaded OK");
    }

    return ret;
}

/**
 * init the views if needed.
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_init(void)
{
    int ret = EXSUCCEED;
    
    if (!M_views_loaded)
    {
        MUTEX_LOCK_V(M_views_init_lock);
        
        /* Check so that concurrent process did not load the tables already... */
        if (!M_views_loaded)
        {
            if (NULL==getenv(CONF_VIEWDIR) || NULL==getenv(CONF_VIEWFILES))
            {
                UBF_LOG(log_warn, "%s or %s not defined -> Not loading view files...", 
                        CONF_VIEWDIR, CONF_VIEWFILES);
                M_views_loaded=EXTRUE; /* Assume init done..., just not views will be found */
            }
            else
            {
                ret = ndrx_view_load_directories();
            }
        }
        
        MUTEX_UNLOCK_V(M_views_init_lock);
        
    }
    
out:
    return ret;
}

/**
 * Set view field option
 * Thread safe.
 * @param v resolved view object
 * @param f resolved view field object
 * @param option B_FTOS/B_STOF/B_OFF/B_BOTH
 * @return 
 */
expublic int ndrx_Bvopt_int(ndrx_typedview_t *v, ndrx_typedview_field_t *f, int option)
{
    int ret = EXSUCCEED;
    
    MUTEX_LOCK_V(M_view_change_lock);
    UBF_LOG(log_debug, "%s: Current flags: [%lx]", __func__, f->flags);
    switch (option)
    {
        case B_FTOS: 
            f->flags&=~NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F;
            f->flags&=~NDRX_VIEW_FLAG_0WAYMAP_N;
            f->flags|=NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S;
            break;
        case B_STOF:
            f->flags&=~NDRX_VIEW_FLAG_0WAYMAP_N;
            f->flags&=~NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S;
            f->flags|=NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F;
            break;
        case B_OFF:
            f->flags&=~NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S;
            f->flags&=~NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F;
            f->flags|=NDRX_VIEW_FLAG_0WAYMAP_N;
            break;
        case B_BOTH:
            f->flags&=~NDRX_VIEW_FLAG_0WAYMAP_N;
            f->flags|=NDRX_VIEW_FLAG_1WAYMAP_UBF2C_S;
            f->flags|=NDRX_VIEW_FLAG_1WAYMAP_C2UBF_F;
            break;
        default:
            ndrx_Bset_error_fmt(BEINVAL, "Invalid option for %s: %d", 
                    __func__, option);
            EXFAIL_OUT(ret);
            break;
    }
    
    UBF_LOG(log_debug, "%s: new flags: [%lx]", __func__, f->flags);
    
out:
    MUTEX_UNLOCK_V(M_view_change_lock);
    return ret;
}

/**
 * Set view option..
 * @param cname field name
 * @param option option
 * @param view view name
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bvopt(char *cname, int option, char *view) 
{
    int ret = EXFALSE;
    ndrx_typedview_t *v = NULL;
    ndrx_typedview_field_t *f = NULL;
    
    if (NULL==(v = ndrx_view_get_view(view)))
    {
        ndrx_Bset_error_fmt(BBADVIEW, "View [%s] not found!", view);
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(f = ndrx_view_get_field(v, cname)))
    {
        ndrx_Bset_error_fmt(BNOCNAME, "Field [%s] of view [%s] not found!", 
                cname, v->vname);
        EXFAIL_OUT(ret);
    }
    
    if (EXFAIL==(ret=ndrx_Bvopt_int(v, f, option)))
    {
        /* should not get here.. */
        ndrx_Bset_error_fmt(BBADVIEW, "System error occurred.");
        goto out;
    }
    
out:
    return ret;
}



/* vim: set ts=4 sw=4 et smartindent: */
