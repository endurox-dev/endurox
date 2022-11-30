/**
 * @brief Python language support
 *
 * @file python.c
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

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <atmi.h>

#include <ndrstandard.h>
#include <ubf.h>
#include <ubf_int.h>
#include <ferror.h>
#include <fieldtable.h>
#include <fdatatype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include "mkfldhdr.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Get the python lang output file name
 * @param data
 */
expublic void python_get_fullname(char *data)
{
    /* For python we shall strip off the file extension
     * as it might cause problems with the class name...
     */
    char *p;
    
    p = strrchr(G_active_file, '.');
    if (NULL!=p)
    {
        NDRX_LOG(log_debug, "Stripping of file extension for class");
        *p=EXEOS;
    }

    sprintf(data, "%s/%s.py", G_output_dir, G_active_file);
}

/**
 * Write text line to output file
 * @param text
 * @return
 */
expublic int python_put_text_line (char *text)
{
    int ret=EXSUCCEED;
    
    if (0==strncmp(text, "#ifndef", 7) || 
        0==strncmp(text, "#define", 7) ||
        0==strncmp(text, "#endif", 6))
    {
        /* just ignore these special C lines */
        goto out;
    }
    
    /* if line is not whitespace, add # leading comment. */
    if (ndrx_isempty(text))
    {
        fprintf(G_outf, "%s", text);
    }
    else
    {
        fprintf(G_outf, "#%s", text);
    }
    
    /* Check errors */
    if (ferror(G_outf))
    {
        ndrx_Bset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Process the baseline
 * @param base
 * @return
 */
expublic int python_put_got_base_line(char *base)
{

    int ret=EXSUCCEED;

    return ret;
}

/**
 * Write definition to output file
 * @param def
 * @return
 */
expublic int python_put_def_line (UBF_field_def_t *def)
{
    int ret=EXSUCCEED;
    int type = def->bfldid>>EFFECTIVE_BITS;
    BFLDID number = def->bfldid & EFFECTIVE_BITS_MASK;
    
    fprintf(G_outf, "%s = %d # number: %d type: %s \n", 
            def->fldname, def->bfldid, number, G_dtype_str_map[type].fldname);
    /* Check errors */
    if (ferror(G_outf))
    {
        ndrx_Bset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", 
                strerror(errno));
        ret=EXFAIL;
    }

    return ret;
}

/**
 * Output file have been open
 * @param fname
 * @return 
 */
expublic int python_file_open (char *fname)
{
    return EXSUCCEED;
}

/**
 * Output file have been closed
 * @param fname
 * @return 
 */
expublic int python_file_close (char *fname)
{
    return EXSUCCEED;   
}

/* vim: set ts=4 sw=4 et smartindent: */
