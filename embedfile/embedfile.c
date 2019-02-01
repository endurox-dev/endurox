/**
 * @brief Generate source code for resource file
 *
 * @file embedfile.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
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

#include <ndrstandard.h>
#include <nstdutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/ 
#define INF     1
#define OUTF    2
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Main entry point for `embedfile' utility
 */
int main(int argc, char** argv)
{
    int ret = EXSUCCEED;
    char *inf, *outfpfx;

    if (argc<3)
    {
        fprintf(stderr, "syntax: %s <input file> <output file> "
                "[extension_override]\n", argv[0]);
        EXFAIL_OUT(ret);
    }
    
    inf = argv[INF];
    outfpfx = argv[OUTF];
    
    if (argc > 3)
    {
        ret = ndrx_file_gen_embed(inf, outfpfx, argv[3]);
    }
    else
    {
        ret = ndrx_file_gen_embed(inf, outfpfx, "c");
    }   
    
out:
    
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
