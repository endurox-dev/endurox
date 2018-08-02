/* 
 * @brief 'C' lang support
 *
 * @file clang.c
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

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <atmi.h>

#include <ubf.h>
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
 * Get the c lang output file name
 * @param data
 */
expublic void c_get_fullname(char *data)
{
    sprintf(data, "%s/%s.h", G_output_dir, G_active_file);
}

/**
 * Write text line to output file
 * @param text
 * @return
 */
expublic int c_put_text_line (char *text)
{
    int ret=EXSUCCEED;
    
    fprintf(G_outf, "%s", text);
    
    /* Check errors */
    if (ferror(G_outf))
    {
        ndrx_Bset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=EXFAIL;
    }

    return ret;
}

/**
 * Process the baseline
 * @param base
 * @return
 */
expublic int c_put_got_base_line(char *base)
{

    int ret=EXSUCCEED;

    fprintf(G_outf, "/*\tfname\tbfldid            */\n"
                    "/*\t-----\t-----            */\n");

    /* Check errors */
    if (ferror(G_outf))
    {
        ndrx_Bset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=EXFAIL;
    }

    return ret;
}

/**
 * Write definition to output file
 * @param def
 * @return
 */
expublic int c_put_def_line (UBF_field_def_t *def)
{
    int ret=EXSUCCEED;
    int type = def->bfldid>>EFFECTIVE_BITS;
    BFLDID number = def->bfldid & EFFECTIVE_BITS_MASK;

    fprintf(G_outf, "#define\t%s\t((BFLDID32)%d)\t/* number: %d\t type: %s */\n",
            def->fldname, def->bfldid, number,
            G_dtype_str_map[type].fldname);
    
    /* Check errors */
    if (ferror(G_outf))
    {
        ndrx_Bset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=EXFAIL;
    }

    return ret;
}

/**
 * Output file have been open
 * @param fname
 * @return 
 */
expublic int c_file_open (char *fname)
{
    return EXSUCCEED;
}

/**
 * Output file have been closed
 * @param fname
 * @return 
 */
expublic int c_file_close (char *fname)
{
    return EXSUCCEED;
}

/* vim: set ts=4 sw=4 et cindent: */
