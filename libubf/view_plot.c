/* 
 * @brief VIEW buffer type support - generate view object file
 *
 * @file view_plot.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * GPL or ATR Baltic's license for commercial use.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include <ndrstandard.h>
#include <ubfview.h>
#include <ndebug.h>

#include <userlog.h>
#include <view_cmn.h>
#include <atmi_tls.h>
#include "Exfields.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {ndrx_Bunset_error(); \
}\

/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/
/**
 * This will plot the object file from the compiled data in the memory..
 * @param f file open for writting
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_plot_object(FILE *f)
{
    int ret = EXSUCCEED;
    ndrx_typedview_t * views = ndrx_view_get_handle();
    ndrx_typedview_t * vel, *velt;
    ndrx_typedview_field_t * fld;
    char tmp_count[32];
    char tmp_size[32];
    char tmp_null[NDRX_VIEW_NULL_LEN+3];
    int err;
    API_ENTRY;
    
#define WRITE_ERR \
                err = errno;\
                UBF_LOG(log_error, "Failed to write to file: %s", strerror(err));\
                ndrx_Bset_error_fmt(TPEOS, "Failed to write to file: %s", strerror(err));\
                EXFAIL_OUT(ret);
                
    if (0>fprintf(f, "#Compiled VIEW file %s %s %d bit, compiler: %s\n", 
            NDRX_VERSION, NDRX_BUILD_OS_NAME, (int)sizeof(void *)*8, NDRX_COMPILER))
    {
        WRITE_ERR;
    }
    
    if (0>fprintf(f, "#Time stamp: %s %s\n", __DATE__, __TIME__))
    {
        WRITE_ERR;
    }
    
    if (0>fprintf(f, "#It is possible to use this file as source of the "
            "view and recompile it\n"))
    {
        WRITE_ERR;
    }
    
    if (0>fprintf(f, "#that will basically update the offsets and sizes of "
            "target platform\n"))
    {
        WRITE_ERR;
    }
    
    if (0>fprintf(f, "#@__platform=%s;@__arch=%s;@__wsize=%d\n\n", 
            NDRX_BUILD_OS_NAME, NDRX_CPUARCH, (int)sizeof(void *)*8))
    {
        WRITE_ERR;
    }
    
        
    EXHASH_ITER(hh, views, vel, velt)
    {
        /* Open view... */
        if (0>fprintf(f, "\nVIEW %s\n", vel->vname))
        {
            WRITE_ERR;
        }
            
        if (0>fprintf(f, "#type  cname                fbname          count flag    size  null                 compiled_data\n"))
        {
            WRITE_ERR;
        }
        
        DL_FOREACH(vel->fields, fld)
        {
            char L_offs[256]="";
            char C_offs[256]="";
            snprintf(tmp_count, sizeof(tmp_count), "%d", fld->count);
            
            if (BFLD_CARRAY==fld->typecode || BFLD_STRING==fld->typecode)
            {
                snprintf(tmp_size, sizeof(tmp_size), "%d", fld->size);
            }
            else
            {
                NDRX_STRCPY_SAFE(tmp_size, "-");
            }
            
            if (fld->flags & NDRX_VIEW_FLAG_ELEMCNT_IND_C)
            {
                snprintf(C_offs, sizeof(C_offs), "coffs=%ld;", fld->count_fld_offset);
            }
            
            if (fld->flags & NDRX_VIEW_FLAG_LEN_INDICATOR_L)
            {
                snprintf(L_offs, sizeof(L_offs), "loffs=%ld;", fld->length_fld_offset);
            }
            
            if (NDRX_VIEW_QUOTES_SINGLE==fld->nullval_quotes)
            {
                snprintf(tmp_null, sizeof(tmp_null), "'%s'", fld->nullval);
            }
            else if (NDRX_VIEW_QUOTES_DOUBLE==fld->nullval_quotes)
            {
                snprintf(tmp_null, sizeof(tmp_null), "\"%s\"", fld->nullval);
            }
            else
            {
                snprintf(tmp_null, sizeof(tmp_null), "%s", fld->nullval);
            }
            
            if (0>fprintf(f, "%-6s %-20s %-15s %-5s %-7s %-5s %-20s offset=%ld;fldsize=%ld;%s%s\n", 
                        fld->type_name, 
                        fld->cname,
                        fld->fbname,
                        tmp_count,
                        fld->flagsstr,
                        tmp_size,
                        tmp_null,
                        fld->offset,
                        fld->fldsize,
                        C_offs,
                        L_offs
                    ))
            {
                WRITE_ERR;
            }
        }

        /* print stats.. */
        if (0>fprintf(f, "#@__ssize=%ld;@__cksum=%lu\n", vel->ssize, 
                (unsigned long)vel->cksum))
        {
            WRITE_ERR;
        }
        
        /* close view */
        if (0>fprintf(f, "END\n\n"))
        {
            WRITE_ERR;
        }
    }
    
out:
    UBF_LOG(log_debug, "%s terminates %d", __func__, ret);
    return ret;
}
