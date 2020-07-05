/**
 * @brief UBF library
 *   Bfprint & Bextread implementations.
 *
 * @file bprint_impl.c
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
/* needed for asprintf */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>

#include <string.h>

#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#include <ubf.h>
#include <ubf_int.h>	/* Internal headers for UBF... */
#include <fdatatype.h>
#include <ferror.h>
#include <fieldtable.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <cf.h>
#include <utils.h>
#include <ubf_tls.h>
#include <ubfview.h>

#include "atmi_int.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/

#define OUTPUT_FORMAT_WDATA fmt_wdata, ndrx_Bfname_int(bfldid), p
#define OUTPUT_FORMAT_NDATA fmt_ndata, ndrx_Bfname_int(bfldid)

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Internal implementation of Bfprintf - print buffer to file.
 * Have to use buffering for non-printable characters, because printing one-by-one
 * is slow?
 * Also will re-use Fnext for iterating throught the buffer.
 * @param p_ub - UBF buffer
 * @param outf - file descriptor to print to
 * @param p_writef callback function which overrides outf (i.e. it can be set to
 *  NULL for particular case). Then for data output callback is used.
 *  if do_write is set, the data in buffer will be written to output.
 *  'buffer' may be reallocated.
 * @param dataptr1 optional argument to p_writef if callback present
 * @param level this is recursive level used for printing embedded buffers
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bfprint (UBFH *p_ub, FILE * outf,
          ndrx_plugin_tplogprintubf_hook_t p_writef, void *dataptr1, int level)
{
    int ret=EXSUCCEED;
    BFLDID bfldid;
    BFLDLEN  len;
    BFLDOCC occ;
    char *p;
    int fldtype;
    char *cnv_buf = NULL;
    char *tmp_buf = NULL;
    BFLDLEN cnv_len;
    char fmt_wdata[256];
    char fmt_ndata[256];
    int i;
    BVIEWFLD *vdata;
    Bnext_state_t bprint_state;
    
    UBF_TLS_ENTRY;
    
    UBF_LOG(log_debug, "%s enter at level %d", __func__, level);
    
    memset(&bprint_state, 0, sizeof(bprint_state));
    
    for (i=0; i<level; i++)
    {
        fmt_wdata[i]='\t';
        fmt_ndata[i]='\t';
    }
    
    fmt_wdata[i]='%';
    fmt_ndata[i]='%';
    
    i++;
    fmt_wdata[i]='s';
    fmt_ndata[i]='s';
    
    i++;
    fmt_wdata[i]='\\';
    fmt_ndata[i]='\\';
            
    i++;
    fmt_wdata[i]='t';
    fmt_ndata[i]='t';
    
    i++;
    fmt_wdata[i]='%';
    fmt_ndata[i]='\\';
    
    i++;
    fmt_wdata[i]='s';
    fmt_ndata[i]='n';
    fmt_ndata[i+1]=EXEOS;
    
    i++;
    fmt_wdata[i]='\\';
    
    i++;
    fmt_wdata[i]='n';
    fmt_ndata[i+1]=EXEOS;
    
    bfldid = BFIRSTFLDID;

    while(1==ndrx_Bnext(&bprint_state, 
            p_ub, &bfldid, &occ, NULL, &len, &p))
    {
        if (NULL!=tmp_buf)
        {
            NDRX_FREE(tmp_buf);
            tmp_buf = NULL;
        }

        if (NULL!=cnv_buf)
        {
            NDRX_FREE(cnv_buf);
            cnv_buf = NULL;
        }

        fldtype=bfldid>>EFFECTIVE_BITS;

        /* All other data types needs to be converted */
        if (BFLD_VIEW==fldtype)
        {
            vdata = (BVIEWFLD *)p;
            /* for value we print the view name */
            p=vdata->vname;
            len=strlen(vdata->vname)+1;
        }
        else if (BFLD_UBF==fldtype)
        {
            /* for UBFs we just print the field name */
            len = 0;
        }
        else if (BFLD_STRING!=fldtype && BFLD_CARRAY!=fldtype)
        {
            cnv_buf=ndrx_Btypcvt(&cnv_len, BFLD_STRING, p, fldtype, len);

            if (NULL==cnv_buf)
            {
                /* we failed to convert - FAIL! No buffers should be allocated
                 * at the moment. */
                break; /* <<< BREAK */
            }
            else
            {
                p=cnv_buf;
            }
            
            len=cnv_len;
        }
        /* now check are we printable? */
        else if (BFLD_STRING==fldtype || BFLD_CARRAY==fldtype)
        {
            int temp_len;
            
            /* For strings we must count off trailing EOS */
            if (BFLD_STRING==fldtype)
            {
                len--;
            }

            temp_len = ndrx_get_nonprintable_char_tmpspace(p, len);

            if (temp_len!=len) /* for carray we need EOS at end! */
            {
                UBF_LOG(log_debug, "Containing special characters -"
                                    " needs to temp buffer for prefixing");
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                if (NULL==tmp_buf)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: Failed to allocate ",
                            __func__, temp_len+1);
                    EXFAIL_OUT(ret);
                }

                /* build the printable string */
                ndrx_build_printable_string(tmp_buf, temp_len+1, p, len);
                p = tmp_buf;
            }
            else if (BFLD_CARRAY==fldtype) /* we need EOS for carray... */
            {
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                
                memcpy(tmp_buf, p, temp_len);
                
                if (NULL==tmp_buf)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: Failed to allocate ", 
                            __func__, temp_len+1);
                    EXFAIL_OUT(ret);
                }
                tmp_buf[temp_len] = EXEOS;
                p = tmp_buf;
            }
        }

        /* value is kept in p */
        if (len>0)
        {
/* #define OUTPUT_FORMAT_WDATA "%s\t%s\n", ndrx_Bfname_int(bfldid), p*/

            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                int do_write = EXFALSE;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_WDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(&tmp, tmp_len, dataptr1, &do_write, 
                        outf, bfldid)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s]", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
                }
                
                if (do_write)
                {
                    fprintf(outf, "%s", tmp);
                }
                        
                NDRX_FREE(tmp);
            }
            else
            {
                fprintf(outf, OUTPUT_FORMAT_WDATA);
            }
            
        }
        else
        {
            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                int do_write = EXFALSE;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_NDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed 2", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(&tmp, tmp_len, dataptr1, &do_write, outf,
                        bfldid)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s] 2", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
                }
                
                if (do_write)
                {
                    fprintf(outf, "%s", tmp);
                }
                
                NDRX_FREE(tmp);
            }
            else
            {
                fprintf(outf, OUTPUT_FORMAT_NDATA);
            }
   
        }
        
        if (NULL!=outf && ferror(outf))
        {
            ndrx_Bset_error_fmt(BEUNIX, "Failed to write to file with error: [%s]",
                        strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        /* Step into printing the inner ubf */
        if (BFLD_UBF==fldtype)
        {
            if (EXSUCCEED!=ndrx_Bfprint ((UBFH *)p, outf, p_writef, dataptr1, level+1))
            {
                UBF_LOG(log_error, "ndrx_Bfprint failed at level %d", level+1);
                EXFAIL_OUT(ret);
            }
        }
        /* the view is not empty one... */
        else if (BFLD_VIEW==fldtype && len > 1)
        {
            /* at this step we shall print the VIEW at given indentation level */
            if (EXSUCCEED!=ndrx_Bvfprint (vdata->data, vdata->vname, outf,
                    p_writef, dataptr1, level+1))
            {
                UBF_LOG(log_error, "ndrx_Bvfprint failed at level %d", level+1);
                EXFAIL_OUT(ret);
            }
        }
    }
    
out:
    /* Free up allocated resources */
    if (NULL!=tmp_buf)
    {
        NDRX_FREE(tmp_buf);
    }

    if (NULL!=cnv_buf)
    {
        NDRX_FREE(cnv_buf);
    }

    /* release the stuff... */
    if (0==level)
    {
        fflush(outf);
    }

    return ret;
}

/**
 * Internal version of Bextread. This accepts either file stream or callback
 * function. One or other must be present. The output may be produced by
 * \ref Bfprint or similar funcs.
 * @param p_ub ptr to UBF buffer
 * @param inf input file stream
 * @param p_readf callback to read function. Function shall provide back data
 *  to ndrx_Bextread(). The reading must be feed line by line. The input line
 *  must be terminated with EOS. The buffer size which accepts the input line
 *  is set by `bufsz'. The function receives forwarded \p dataptr1 argument.
 *  Once EOF is reached, the callback shall return read of 0 bytes. Otherwise
 *  it must return number of bytes read, including EOS.
 * @param dataptr1 option user pointer forwarded to \p p_readf.
 * @param level recursion level
 * @param p_readbuf_buffered (if previous recursion terminated with next line
 *  from current buffer
 * @param p_is_eof is EOF set by recusrsion
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bextread (UBFH * p_ub, FILE *inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), 
        void *dataptr1, int level, char **p_readbuf_buffered)
{
    int ret=EXSUCCEED;
    int line=0;
    char *readbuf=NULL;
    size_t readbuf_len;
    char fldnm[UBFFLDMAX+1];
    char *value=NULL;
    size_t value_len;
    char flag;
    char *p;
    char *tok;
    BFLDID bfldid;
    BFLDID bfldid_from;
    int fldtype;
    int cpylen;
    int len;
    char *readbuf_buffered=NULL;
    int nr_lead_tabs;
    int is_eof=EXFALSE;
    NDRX_USYSBUF_MALLOC_WERR_OUT(readbuf, readbuf_len, ret);
    NDRX_USYSBUF_MALLOC_WERR_OUT(value, value_len, ret);
    
    /* Read line by line */
    while(!is_eof)
    {
        /* use buffered line from inner reads */
        if (NULL!=readbuf_buffered)
        {
            NDRX_SYSBUF_FREE(readbuf);
            readbuf=readbuf_buffered;
            readbuf_buffered=NULL;
        }
        else if (NULL!=p_readf)
        {
            /* read the data from callback */
            ret = (int)p_readf(readbuf, readbuf_len, dataptr1);
            
            if (0==ret)
            {
                /* eof reached */
                break;
            }
            if (ret < 0)
            {
                ndrx_Bset_error_fmt(BEUNIX, "p_readf() user callback failed");
                
                EXFAIL_OUT(ret);
            }
            ret = EXSUCCEED;
        }
        else 
        {
            if (NULL==fgets(readbuf, readbuf_len, inf))
            {
                /* terminate the loop */
                /*
                 * Check errors on file.
                 */
                if (!feof(inf))
                {
                   /* some other error happened!?! */
                   ndrx_Bset_error_fmt(BEUNIX, "Failed to read from file "
                           "with error: [%s]", strerror(errno));
                   EXFAIL_OUT(ret);
                }

                break;
            }
        }
                
        len = strlen(readbuf);
        line++;
        bfldid=BBADFLDID;
        value[0] = EXEOS;
        fldnm[0] = EXEOS;
        p = readbuf;

        if ('#'==p[0])
        {
            continue; /* <<<< nothing to do - continue */
        }

        /* Ignore any newline we get, so that we are backwards compatible
         * with original logic
         */
        if (0==strcmp(p, "\n"))
        {
            continue; /* <<<< nothing to do - continue */
        }
        
        /* check the leading tabs, to see the current nesting level */
        nr_lead_tabs=0;
        while (*p=='\t')
        {
            nr_lead_tabs++;
            p++;
        }
        
        /* terminate the function if found less tabs than current level */
        if (nr_lead_tabs < level)
        {
            UBF_LOG(log_debug, "Found tab level %d current %d, popping up line %d", 
                    nr_lead_tabs, level, line);
            if (NULL!=p_readbuf_buffered)
            {
                /* pass to upper level */
                *p_readbuf_buffered=readbuf;
                readbuf=NULL;
            }
            /* terminate the read */
            goto out;
        }
        else if (nr_lead_tabs > level)
        {
            ndrx_Bset_error_fmt(BSYNTAX, "Tab level %d expected %d or less - "
                    "invalid data at line %d", nr_lead_tabs, level, line);
            EXFAIL_OUT(ret);
        }
        /* else: it is current level and process ok */
        
        flag = 0;

        if ('-'==p[0] || '+'==p[0] || '='==p[0])
        {
            /* Check syntax with flags... */

            flag=p[0];

            if (' '!=p[1])
            {
                ndrx_Bset_error_fmt(BSYNTAX, "Space does not follow the flag on "
                                            "line %d!", line);
                
                EXFAIL_OUT(ret);
            }
            else
            {
                /* step forward, flag + eos*/
                p+=2;
            }
        }
        
        tok = strchr(p, '\t');
        if (NULL==tok)
        {
            ndrx_Bset_error_fmt(BSYNTAX, "No tab on "
                                        "line %d!", line);
            EXFAIL_OUT(ret);
        }
        else if (tok==readbuf)
        {
            ndrx_Bset_error_fmt(BSYNTAX, "Line should not start with tab on "
                                        "line %d!", line);
            EXFAIL_OUT(ret);
        } 
        else
        {
            int tmpl = strlen(p);
            /* seems to be ok, remove trailing newline */
            
            if (p[tmpl-1]!='\n')
            {
                /* new line at the end is optional for callbacks... */
                if (NULL==p_readf)
                {
                    ndrx_Bset_error_fmt(BSYNTAX, "Line %d does not "
                            "terminate with newline!", line);
                    EXFAIL_OUT(ret);
                }
            }
            else
            {
                p[tmpl-1]=EXEOS;
            }
        }

        /* now read field number + value */
        cpylen = (tok-p);
        /* Copy off field name */
        NDRX_STRNCPY_EOS(fldnm, p, cpylen, sizeof(fldnm));
        
        /* Copy off value */
        NDRX_STRCPY_SAFE_DST(value, tok+1, value_len);
        UBF_LOG(log_debug, "Got [%s]:[%s]", fldnm, value);

        /*
         * Resolve field name
         */
        bfldid = ndrx_Bfldid_int(fldnm);
        if (BBADFLDID==bfldid)
        {
            ndrx_Bset_error_fmt(BBADNAME, "Cannot resolve field ID from [%s] on"
                                        "line %d!", fldnm, line);
            EXFAIL_OUT(ret);
        }

        /* Get new field type */
        fldtype=bfldid >> EFFECTIVE_BITS;

        /* check type */
        if (IS_TYPE_INVALID(fldtype))
        {
            ndrx_Bset_error_fmt(BBADFLD, "BAD field's type [%d] on"
                                            "line %d!", fldtype, line);
            EXFAIL_OUT(ret);
        }
        
        /* Check field type */
        if ((BFLD_STRING == fldtype || BFLD_CARRAY == fldtype) && '='!=flag)
        {
            if (EXFAIL==ndrx_normalize_string(value, &len))
            {
                ndrx_Bset_error_fmt(BSYNTAX, "Cannot normalize value on line %d", 
                        line);
                EXFAIL_OUT(ret);
            }
        }
        else if (BFLD_UBF == fldtype)
        {
            /* init the buffer */
            if (EXSUCCEED!=Binit((UBFH*)value, value_len))
            {
                UBF_LOG(log_error, "Failed to init %p/%z level: %d", 
                        value, value_len, level);
                EXFAIL_OUT(ret);
            }
            
            /* start to parse inner struct.. */
            if (EXSUCCEED!=ndrx_Bextread ((UBFH*)value, inf,
                p_readf, dataptr1, level+1, &readbuf_buffered))
            {
                UBF_LOG(log_error, "Failed to parse inner UBF level %d", level+1);
                EXFAIL_OUT(ret);
            }
            
            /* if no next line found, then it is EOF */
            if (NULL==readbuf_buffered)
            {
                is_eof=EXTRUE;
            }
        }
        else if (BFLD_VIEW == fldtype)
        {
            /* now parse the view  */
        }
        
        /* now about to execute command */
        if (0==flag)
        {
            if (BFLD_UBF == fldtype)
            {
                if (EXSUCCEED!=(ret=Badd(p_ub, bfldid, value, 0)))
                {
                    EXFAIL_OUT(ret);
                }
            }
            else if (EXSUCCEED!=(ret=CBadd(p_ub, bfldid, value, len, BFLD_CARRAY)))
            {
                EXFAIL_OUT(ret);
            }
        }
        else if ('+'==flag)
        {
            if (BFLD_UBF == fldtype)
            {
                if (EXSUCCEED!=(ret=Bchg(p_ub, bfldid, 0, value, 0)))
                {
                    EXFAIL_OUT(ret);
                }
            }
            else if (EXSUCCEED!=(ret=CBchg(p_ub, bfldid, 0, value, len, BFLD_CARRAY)))
            {
                EXFAIL_OUT(ret);
            }
        }
        else if ('-'==flag)
        {
            if (EXSUCCEED!=(ret=Bdel(p_ub, bfldid, 0)))
            {
                EXFAIL_OUT(ret);
            }
        }
        else if ('='==flag)
        {
            /* Resolve field to-field id */
            bfldid_from = ndrx_Bfldid_int(value);
            if (BBADFLDID==bfldid_from)
            {
                ndrx_Bset_error_fmt(BBADNAME, "Cannot resolve field ID from [%s] on"
                                            "line %d!", value, line);
                EXFAIL_OUT(ret);
            }
            else
            {
                BFLDLEN len_from=0;
                /* TODO: use Bgetalloc to copy value from */
                char *copy_form = Bgetalloc (p_ub, bfldid_from, 0, &len_from);
                
                /* Find the value and put into buffer. */
                if (NULL!=copy_form)
                {
                    
                    /* WARNING! This might move the source buffer before setting
                     * Say: We set field_id=1 from field_id=2. Thus we will move
                     * the 2 to get space for 1.
                     * Fixed: moved from Bfind to ndrx_Bgetalloc
                     */
                    if (EXSUCCEED!=(ret=Bchg(p_ub, bfldid, 0, copy_form, len_from)))
                    {
                        NDRX_FREE(copy_form);
                        EXFAIL_OUT(ret);
                    }
                    
                    NDRX_FREE(copy_form);
                }
                else
                {
                    EXFAIL_OUT(ret);
                }
            }
        } /* '='==flag */
    } /* while */
    
out:
    
    if (NULL!=readbuf_buffered)
    {
        NDRX_SYSBUF_FREE(readbuf_buffered);
    }

    if (NULL!=readbuf)
    {
        NDRX_SYSBUF_FREE(readbuf);
    }

    if (NULL!=value)
    {
        NDRX_SYSBUF_FREE(value);
    }

    UBF_LOG(log_debug, "%s: return %d", __func__, ret);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
