/* 
** VIEW buffer type support
** Basically all data is going to be stored in FB.
** For view not using base as 3000. And the UBF field numbers follows the 
** the field number in the view (1, 2, 3, 4, ...) 
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

#include <ndrstandard.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <tperror.h>

#include <userlog.h>
#include <typed_view.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define FIELD_SEPERATORS        " \t"
#define TOKEN_VIEW_START            "VIEW"
#define TOKEN_VIEW_END              "END"
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_typedview_t *NDRX_G_view_hash = NULL;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

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
    ndrx_typedview_t *v = NULL;
    char *saveptr1 = NULL; /* lookup section  */
    char *tok;
    enum states { INFILE, INVIEW} state = INFILE;
    int len;
    char *p;
    
    NDRX_LOG(log_debug, "%s - enter", __func__);
    
    if (NULL==(f=NDRX_FOPEN(fname, "r")))
    {
        int err;
        NDRX_LOG(log_error, "Failed to open view file: %s", strerror(err));
        ndrx_TPset_error_fmt(TPENOENT, "Failed to open view file: %s", strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* Read line by line... */
    while (NULL!=fgets(buf, sizeof(buf), f))
    {
        NDRX_LOG(log_dump, "Got VIEW file line: [%s]", p);
        
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
        ndrx_str_rstrip(p, FIELD_SEPERATORS);
        
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
                NDRX_LOG(log_error, "Expected [%s] but got [%s]", TOKEN_VIEW_START, p);
                ndrx_TPset_error_fmt(TPEINVAL, "Expected [%s] but got [%s]", 
                        TOKEN_VIEW_START, p);
                EXFAIL_OUT(ret);
            }

            if (NULL==(tok = strtok_r(NULL, FIELD_SEPERATORS, &saveptr1)))
            {
                NDRX_LOG(log_error, "Missing identifier after %s", TOKEN_VIEW_START);
                ndrx_TPset_error_fmt(TPEINVAL, "Missing identifier after %s", 
                        TOKEN_VIEW_START);
                EXFAIL_OUT(ret);
            }
            
            len = strlen(p);
            if (len > NDRX_VIEW_NAME_LEN)
            {
                NDRX_LOG(log_error, "View identifier [%s] too long! Max len: %d,"
                        " but got: %d", 
                        p, NDRX_VIEW_NAME_LEN, len);
                ndrx_TPset_error_fmt(TPEINVAL, "View identifier [%s] too long!"
                        " Max len: %d, but got: %d", 
                        p, NDRX_VIEW_NAME_LEN, len);
                EXFAIL_OUT(ret);
            }
            
            /* Got view ok, allocate the object */
            v = NDRX_CALLOC(1, sizeof(ndrx_typedview_t));

            if (NULL==v)
            {
                int err = errno;
                NDRX_LOG(log_error, "Failed to allocate ndrx_typedview_t: %s", 
                        strerror(err));

                ndrx_TPset_error_fmt(TPEOS, "Failed to allocate ndrx_typedview_t: %s", 
                        strerror(err));
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
            tok = strtok_r(p, FIELD_SEPERATORS, &saveptr1);
            
            if (0==strcmp(TOKEN_VIEW_END, tok))
            {
                NDRX_LOG(log_debug, "View [%s] finishing off -> add to hash", 
                        v->vname);
                
                EXHASH_ADD_STR(NDRX_G_view_hash, vname, v);
                v = NULL;
                continue;
            }
            
            /* OK this is the string line... */
            
            
        }
    }
    
out:

    NDRX_LOG(log_debug, "%s - return %d", ret);

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    /* shall be normally set to NULL in the end */
    if (NULL!=v)
    {
        NDRX_FREE(v);
    }

    return ret;
}



