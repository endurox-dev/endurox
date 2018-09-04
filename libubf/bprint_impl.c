/**
 * @brief UBF library
 *   Bfprint & Bextread implementations.
 *
 * @file bprint_impl.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * GPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */

/*---------------------------Includes-----------------------------------*/
/* needed for asprintf */
#define _GNU_SOURCE
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
#include <ubf_impl.h>
#include <utils.h>
#include <ubf_tls.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
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
 * @param dataptr1 optional argument to p_writef if callback present
 * @return SUCCEED/FAIL
 */
expublic int ndrx_Bfprint (UBFH *p_ub, FILE * outf,
           int (*p_writef)(char *buffer, long datalen, void *dataptr1), void *dataptr1)
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
    
    UBF_TLS_ENTRY;
    
    memset(&G_ubf_tls->bprint_state, 0, sizeof(G_ubf_tls->bprint_state));

    bfldid = BFIRSTFLDID;

    while(1==ndrx_Bnext(&G_ubf_tls->bprint_state, 
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
        if (BFLD_STRING!=fldtype && BFLD_CARRAY!=fldtype)
        {
            cnv_buf=ndrx_Btypcvt(&cnv_len, BFLD_STRING, p, fldtype, len);

            if (NULL==cnv_buf)
            {
                /* we failed to convertet - FAIL! No buffers should be allocated
                 * at the moment. */
                break; /* <<< BREAK */
            }
            else
            {
                p=cnv_buf;
            }
        }
        else
        {
            cnv_len = len; /* for str & carray we have the same len */
        }

        /* now check are we printable? */
        if (BFLD_STRING==fldtype || BFLD_CARRAY==fldtype)
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
#define OUTPUT_FORMAT_WDATA "%s\t%s\n", ndrx_Bfname_int(bfldid), p
            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_WDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(tmp, tmp_len, dataptr1)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s]", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
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
#define OUTPUT_FORMAT_NDATA "%s\t\n", ndrx_Bfname_int(bfldid)
            
            if (NULL!=p_writef)
            {
                char *tmp;
                long tmp_len;
                
                NDRX_ASPRINTF(&tmp, &tmp_len, OUTPUT_FORMAT_WDATA);
                
                if (NULL==tmp)
                {
                    ndrx_Bset_error_fmt(BMALLOC, "%s: NDRX_ASPRINTF failed 2", 
                            __func__);
                    EXFAIL_OUT(ret);
                }
                
                tmp_len++;
                
                if (EXSUCCEED!=(ret=p_writef(tmp, tmp_len, dataptr1)))
                {
                    ndrx_Bset_error_fmt(BEINVAL, "%s: p_writef user function "
                            "failed with %d for [%s] 2", 
                            __func__, ret, tmp);
                    NDRX_FREE(tmp);
                    EXFAIL_OUT(ret);
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
    fflush(outf);

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
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_Bextread (UBFH * p_ub, FILE *inf,
        long (*p_readf)(char *buffer, long bufsz, void *dataptr1), void *dataptr1)
{
    int ret=EXSUCCEED;
    int line=0;
    char readbuf[NDRX_MSGSIZEMAX];
    char fldnm[UBFFLDMAX+1];
    char value[NDRX_MSGSIZEMAX];
    char flag;
    char *p;
    char *tok;
    BFLDID bfldid;
    BFLDID bfldid_from;
    int fldtype;
    int cpylen;
    int len;
    
    /* Read line by line */
    while(1)
    {
        if (NULL!=p_readf)
        {
            /* read the data from callback */
            ret = (int)p_readf(readbuf, sizeof(readbuf), dataptr1);
            
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
            if (NULL==fgets(readbuf, sizeof(readbuf), inf))
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

        /* Check syntax with/out flags, we should have tab inside */
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
        NDRX_STRNCPY(fldnm, p, cpylen);
        fldnm[cpylen]=EXEOS;
        /* Copy off value */
        NDRX_STRCPY_SAFE(value, tok+1);
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
        
        /* now about to execute command */
        if (0==flag)
        {
            if (EXSUCCEED!=(ret=CBadd(p_ub, bfldid, value, len, BFLD_CARRAY)))
            {
                EXFAIL_OUT(ret);
            }
        }
        else if ('+'==flag)
        {
            if (EXSUCCEED!=(ret=CBchg(p_ub, bfldid, 0, value, len, BFLD_CARRAY)))
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
                char *copy_form = Bfind(p_ub, bfldid_from, 0, &len_from);

                /* Find the value and put into buffer. */
                if (NULL!=copy_form)
                {
                    if (EXSUCCEED!=(ret=Bchg(p_ub, bfldid, 0, copy_form, len_from)))
                    {
                        EXFAIL_OUT(ret);
                    }
                }
                else
                {
                    EXFAIL_OUT(ret);
                }
            }
        } /* '='==flag */
    } /* while */
    
out:
    UBF_LOG(log_debug, "%s: return %d", __func__, ret);
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
