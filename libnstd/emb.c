/**
 * @brief Embed file
 *
 * @file emb.c
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
#include <ndrstandard.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "ndebug.h"
#include "userlog.h"
#include <errno.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Generate embed sources, in C code, header
 * @param in_fname input file name
 * @param out_fname output file name
 * @param out_suffix output suffix
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_file_gen_embed(char *in_fname, char *out_fname, char *out_suffix)
{
    int ret = EXSUCCEED;
    FILE *in=NULL, *out=NULL;
    int c;
    char outf[PATH_MAX+1] = {EXEOS};
    int counter = 0;
    int err;
    
    snprintf(outf, sizeof(outf), "%s.%s", out_fname, out_suffix);
    
    if (NULL==(in=fopen(in_fname, "rb")))
    {
        err = errno;
        fprintf(stderr, "Failed to open [%s]: %s\n", in_fname, strerror(err));
        NDRX_LOG(log_error, "Failed to open [%s]: %s", in_fname, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    if (NULL==(out=fopen(outf, "wb")))
    {
        err = errno;
        fprintf(stderr, "Failed to open [%s]: %s\n", outf, strerror(err));
        NDRX_LOG(log_error, "Failed to open [%s]: %s", outf, strerror(err));
        EXFAIL_OUT(ret);
    }
    
    /* write header */
    
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "const unsigned char ndrx_G_resource_%s[] = {\n", out_fname);
    
    /* read  & write */
    while (EOF!=(c=(fgetc(in))))
    {
        counter++;
        
        fprintf(out, "0x%02x,", c);
        
        if (0 == counter % 10)
        {
            fprintf(out, "\n");
        }
        else
        {
            fprintf(out, " ");
        }
    }
    
    /* close resource */
    fprintf(out, "0x00\n};\n");
    
    /* write off length (with out EOS..) */
    fprintf(out, "const size_t ndrx_G_resource_%s_len = %d;\n", out_fname, counter);
    fprintf(out, "#define ndrx_G_resource_%s_len_def %d\n", out_fname, counter);
    
    /* put the EOL... (additionally so that we can easy operate resource as string) */
    
out:
    
    if (NULL!=in)
    {
        fclose(in);
    }
    
    if (NULL!=out)
    {
        fclose(out);
    }
    
    if (EXSUCCEED!=ret) 
    {
        unlink(outf);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
