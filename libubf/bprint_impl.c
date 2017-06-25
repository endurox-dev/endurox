/* 
** UBF library
** Bfprint & Bextread implementations.
**
** @file bprint_impl.c
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
#include <string.h>
#include <stdio.h>
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
 * @return SUCCEED/FAIL
 */
expublic int _Bfprint (UBFH *p_ub, FILE * outf)
{
    int ret=EXSUCCEED;
   /* static __thread Bnext_state_t state; */
    BFLDID bfldid;
    BFLDLEN  len;
    BFLDOCC occ;
    char *p;
    int fldtype;
    char *cnv_buf = NULL;
    char *tmp_buf = NULL;
    BFLDLEN cnv_len;
    char fn[] = "_Bfprint";
    
    UBF_TLS_ENTRY;
    
    memset(&G_ubf_tls->bprint_state, 0, sizeof(G_ubf_tls->bprint_state));

    bfldid = BFIRSTFLDID;

    while(1==_Bnext(&G_ubf_tls->bprint_state, 
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
            cnv_buf=_Btypcvt(&cnv_len, BFLD_STRING, p, fldtype, len);

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

            temp_len = get_nonprintable_char_tmpspace(p, len);

            if (temp_len!=len) /* for carray we need EOS at end! */
            {
                UBF_LOG(log_debug, "Containing special characters -"
                                    " needs to temp buffer for prefixing");
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                if (NULL==tmp_buf)
                {
                    _Fset_error_fmt(BMALLOC, "%s: Failed to allocate ", fn, temp_len+1);
                    EXFAIL_OUT(ret);
                }

                /* build the printable string */
                build_printable_string(tmp_buf, p, len);
                p = tmp_buf;
            }
            else if (BFLD_CARRAY==fldtype) /* we need EOS for carray... */
            {
                tmp_buf=NDRX_MALLOC(temp_len+1); /* adding +1 for EOS */
                
                memcpy(tmp_buf, p, temp_len);
                
                if (NULL==tmp_buf)
                {
                    _Fset_error_fmt(BMALLOC, "%s: Failed to allocate ", fn, temp_len+1);
                    EXFAIL_OUT(ret);
                }
                tmp_buf[temp_len] = EXEOS;
                p = tmp_buf;
            }
        }


        /* value is kept in p */
        if (len>0)
            fprintf(outf, "%s\t%s\n", _Bfname_int(bfldid), p);
        else
            fprintf(outf, "%s\t\n", _Bfname_int(bfldid));

        if (ferror(outf))
        {
            _Fset_error_fmt(BEUNIX, "Failed to write to file with error: [%s]",
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
 * Internal version of Bextread.
 * We will also s
 * @param p_ub
 * @param inf
 * @return
 */
expublic int _Bextread (UBFH * p_ub, FILE *inf)
{
    int ret=EXSUCCEED;
    int line=0;
    char readbuf[EXTREAD_BUFFSIZE];
    char fldnm[EXTREAD_BUFFSIZE];
    char value[EXTREAD_BUFFSIZE];
    
    char flag;
    char *p;
    char *tok;
    BFLDID bfldid;
    BFLDID bfldid_from;
    int fldtype;
    int len;

    char fn[] = "_Bextread";
    
    /* Read line by line */
    while(EXSUCCEED==ret && NULL!=fgets(readbuf, sizeof(readbuf), inf))
    {
        int len = strlen(readbuf);
        line++;
        bfldid=BBADFLDID;
        value[0] = EXEOS;
        fldnm[0] = EXEOS;
        p = readbuf;

        if ('#'==p[0])
            continue; /* <<<< nothing to do - continue */

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
                ret=EXFAIL;
                _Fset_error_fmt(BSYNTAX, "Space does not follow the flag on "
                                            "line %d!", line);
                break; /* <<< fail - break */
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
            ret=EXFAIL;
            _Fset_error_fmt(BSYNTAX, "No tab on "
                                        "line %d!", line);
        }
        else if (tok==readbuf)
        {
            ret=EXFAIL;
            _Fset_error_fmt(BSYNTAX, "Line should not start with tab on "
                                        "line %d!", line);
        }
        else if (p[strlen(p)-1]!='\n')
        {
            ret=EXFAIL;
            _Fset_error_fmt(BSYNTAX, "Line %d does not terminate with newline!", line);
        }
        else
        {
            /* seems to be ok, remove trailing newline */
            p[strlen(p)-1]=EXEOS;
        }

        if (EXSUCCEED==ret)
        {
            /* now read field number + value */
            int cpylen = (tok-p);
            /* Copy off field name */
            strncpy(fldnm, p, cpylen);
            fldnm[cpylen]=EXEOS;
            /* Copy off value */
            strcpy(value, tok+1);
            UBF_LOG(log_debug, "Got [%s]:[%s]", fldnm, value);
            
            /*
             * Resolve field name
             */
            if (EXSUCCEED==ret)
            {
                bfldid = _Bfldid_int(fldnm);
                if (BBADFLDID==bfldid)
                {
                    _Fset_error_fmt(BBADNAME, "Cannot resolve field ID from [%s] on"
                                                "line %d!", fldnm, line);
                    ret=EXFAIL;
                }
            }
        }

        /* Get new field type */
        if (EXSUCCEED==ret)
        {
            fldtype=bfldid >> EFFECTIVE_BITS;
            
            /* check type */
            if (IS_TYPE_INVALID(fldtype))
            {
                ret=EXFAIL;
                _Fset_error_fmt(BBADFLD, "BAD field's type [%d] on"
                                                "line %d!", fldtype, line);
            }

        }
        
        /* Check field type */
        if (EXSUCCEED==ret && 
                    (BFLD_STRING == fldtype || BFLD_CARRAY == fldtype) && '='!=flag)
        {
            if (EXFAIL==normalize_string(value, &len))
            {
                ret=EXFAIL;
                _Fset_error_fmt(BSYNTAX, "Cannot normalize value on line %d", line);
            }
        }
        
        /* now about to execute command */
        if (EXSUCCEED==ret && 0==flag)
        {
            ret=CBadd(p_ub, bfldid, value, len, BFLD_CARRAY);
        }
        else if (EXSUCCEED==ret && '+'==flag)
        {
            ret=CBchg(p_ub, bfldid, 0, value, len, BFLD_CARRAY);
        }
        else if (EXSUCCEED==ret && '-'==flag)
        {
            ret=Bdel(p_ub, bfldid, 0);
        }
        else if (EXSUCCEED==ret && '='==flag)
        {
            /* Resolve field to-field id */
            bfldid_from = _Bfldid_int(value);
            if (BBADFLDID==bfldid_from)
            {
                _Fset_error_fmt(BBADNAME, "Cannot resolve field ID from [%s] on"
                                            "line %d!", value, line);
                ret=EXFAIL;
            }
            else
            {
                BFLDLEN len_from=0;
                char *copy_form = Bfind(p_ub, bfldid_from, 0, &len_from);

                /* Find the value and put into buffer. */
                if (NULL!=copy_form)
                {
                    ret=Bchg(p_ub, bfldid, 0, copy_form, len_from);
                }
                else
                {
                    ret=EXFAIL;
                }
            }
        } /* '='==flag */
    } /* while */

    /*
     * Check errors on file.
     */
    if (!feof(inf))
    {
       /* some other error happened!?! */
       _Fset_error_fmt(BEUNIX, "Failed to read from file with error: [%s]",
                            strerror(errno));
       ret=EXFAIL;
    }

    UBF_LOG(log_debug, "%s: return %d", fn, ret);
    
    return ret;
}

