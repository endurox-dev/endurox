/**
 * @brief 'C' lang support
 *
 * @file clang.c
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
#include <errno.h>
#include "viewc.h"
#include <typed_view.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Plot the C header file.
 * This will generate all the stuff loaded into memory, if multiple view files
 * loaded, then output will go to this one. This must be followed by developers.
 * @param basename - file name with out extension
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_plot_c_header(char *outdir, char *basename)
{
    int ret = EXSUCCEED;
    char  fname[PATH_MAX+1];
    FILE *f = NULL;
    ndrx_typedview_t * views = ndrx_view_get_handle();
    ndrx_typedview_t * vel, *velt;
    ndrx_typedview_field_t * fld;
    char default_null[NDRX_VIEW_NULL_LEN+32];
            
    /* we generate headers in current directoy */
    snprintf(fname, sizeof(fname), "%s.h", basename);
    
    NDRX_LOG(log_info, "Opening output file: [%s]", fname);
    
    if (NULL==(f=NDRX_FOPEN(fname, "w")))
    {
        NDRX_LOG(log_error, "Failed to open output file [%s]: %s", 
                fname, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* Iterate the hash... */
    
    EXHASH_ITER(hh, views, vel, velt)
    {
        fprintf(f, "\nstruct %s {\n", vel->vname);
        
        DL_FOREACH(vel->fields, fld)
        {
            if (!fld->nullval_default)
            {
                if (fld->nullval_quotes)
                {
                    snprintf(default_null, sizeof(default_null), "\t/* null=\"%s\" */",
                        fld->nullval);
                }
                else
                {
                    snprintf(default_null, sizeof(default_null), "\t/* null=%s */",
                        fld->nullval);
                }
            }
            else
            {
                default_null[0]=EXEOS;
            }
                    
            if (fld->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
            {
                fprintf(f, "\tshort\tC_%s;\n", fld->cname);
            }
            
            if (fld->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
            {
                if (fld->count > 1)
                {
                    fprintf(f, "\tunsigned short\tL_%s[%d];\n", fld->cname, fld->count);
                }
                else
                {
                    fprintf(f, "\tunsigned short\tL_%s;\n", fld->cname);
                }

            }
            
            switch (fld->typecode_full)
            {
                case BFLD_CHAR:
                    
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tchar\t%s[%d];%s\n", fld->cname, fld->count, 
                                default_null);
                    }
                    else
                    {
                        fprintf(f, "\tchar\t%s;%s\n", fld->cname,
                                default_null);
                    }
                    
                    break;
                case BFLD_INT:
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tint\t%s[%d];%s\n", fld->cname, fld->count,
                                    default_null);
                    }
                    else
                    {
                        fprintf(f, "\tint\t%s;%s\n", fld->cname, default_null);
                    }
                    break;
                    
                case BFLD_SHORT:
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tshort\t%s[%d];%s\n", fld->cname, fld->count, 
                                default_null);
                    }
                    else
                    {
                        fprintf(f, "\tshort\t%s;%s\n", fld->cname, default_null);
                    }
                    break;
                case BFLD_LONG:
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tlong\t%s[%d];%s\n", fld->cname, fld->count,
                                default_null);
                    }
                    else
                    {
                        fprintf(f, "\tlong\t%s;%s\n", fld->cname, default_null);
                    }
                    
                    break;
                case BFLD_FLOAT:
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tfloat\t%s[%d];%s\n", fld->cname, fld->count,
                                default_null);
                    }
                    else
                    {
                        fprintf(f, "\tfloat\t%s;%s\n", fld->cname, default_null);
                    }
                    break;
                case BFLD_DOUBLE:
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tdouble\t%s[%d];%s\n", fld->cname, fld->count,
                                default_null);
                    }
                    else
                    {
                        fprintf(f, "\tdouble\t%s;%s\n", fld->cname,
                                default_null);
                    }
                    
                    break;
                    
                case BFLD_STRING:
                    
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tchar\t%s[%d][%d];%s\n", fld->cname, 
                                fld->count, fld->size, default_null);
                    }
                    else
                    {
                        fprintf(f, "\tchar\t%s[%d];%s\n", fld->cname, fld->size,
                                default_null);
                    }
                    
                    break;
                    
                case BFLD_CARRAY:
                    
                    if (fld->count > 1)
                    {
                        fprintf(f, "\tchar\t%s[%d][%d];%s\n", fld->cname, 
                                fld->count, fld->size, default_null);
                    }
                    else
                    {
                        fprintf(f, "\tchar\t%s[%d];%s\n", fld->cname, fld->size,
                                default_null);
                    }
                    
                    break;
            }
        }
        
        fprintf(f, "};\n\n");
    }
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
        f = NULL;
    }
    return ret;
}
/* vim: set ts=4 sw=4 et smartindent: */
