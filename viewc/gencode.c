/* 
** Generate code for size & offset calculations done by compile & exec of the binary
** The generated binary will actually plot the object .V file.
**
** @file gencode.c
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
 * Compile the code and invoke offset calculator...
 * @param sourcecode
 * @param ofile
 * @return EXSUCCEED/EXFAIL
 */
exprivate int ndrx_view_invoke(char *sourcecode, char *ofile)
{
    int ret = EXSUCCEED;
    char build_cmd[PATH_MAX+1];
    
    
    snprintf(build_cmd, sizeof(build_cmd), "buildclient -f %s -o %s", 
            sourcecode, ofile);
    
    NDRX_LOG(log_info, ">>> About to execute: [%s]", build_cmd);
    
    if (EXSUCCEED!=system(build_cmd))
    {
        NDRX_LOG(log_error, "%s FAILED - Failed to build offset calc program", 
                build_cmd);
        EXFAIL_OUT(ret);
    }
    
    NDRX_LOG(log_info, ">>> About to execute compiled binary to build V object: [%s]", ofile);
    
    if (EXSUCCEED!=system(ofile))
    {
        NDRX_LOG(log_error, "%s FAILED - Failed to calculate offsets!", 
                ofile);
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Plot the C header file.
 * This will generate all the stuff loaded into memory, if multiple view files
 * loaded, then output will go to this one. This must be followed by developers.
 * @param basename - file name with out extension
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_view_generate_code(char *outdir, char *basename, char *vsrcfile)
{
    int ret = EXSUCCEED;
    FILE *f;
    char cfile[PATH_MAX+1];
    char ofile[PATH_MAX+1];
    char Vfile[PATH_MAX+1];
    ndrx_typedview_t * views = ndrx_view_get_handle();
    ndrx_typedview_t * vel, *velt;
    ndrx_typedview_field_t * fld;
    
    snprintf(cfile, sizeof(cfile), "%s_excompiled.c", basename);
    snprintf(ofile, sizeof(ofile), "./%s_excompiled", basename);
    snprintf(Vfile, sizeof(Vfile), "%s/%s.V", outdir, basename);
    
    NDRX_LOG(log_info, "C-Code to compile: [%s]", cfile);
    NDRX_LOG(log_info, "Compiled binary: [%s]", ofile);
    NDRX_LOG(log_info, "Compiled VIEW: [%s]", Vfile);
    NDRX_LOG(log_info, "Source VIEW: [%s]", vsrcfile);
    
    /* get the temp-file-name, it will be 
     * <basename>_excompiled.c and code <basename>_excompiled 
     */
    
    if (NULL==(f=NDRX_FOPEN(cfile, "w")))
    {
        NDRX_LOG(log_error, "Failed to open for write [%s]: %s", 
                cfile, strerror(errno));
        EXFAIL_OUT(ret);
    }
    
    /* TODO: We need offsets for L_ and C_ fields... */
    
    fprintf(f, "/* Offset calculation auto-generated code */\n");
    fprintf(f, "/*---------------------------Includes-----------------------------------*/\n");
    fprintf(f, "#include <unistd.h>\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <errno.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <ndrstandard.h>\n");
    fprintf(f, "#include <ndebug.h>\n");
    fprintf(f, "#include <errno.h>\n");
    fprintf(f, "#include <view_cmn.h>\n");
    fprintf(f, "#include <atmi.h>\n");
    fprintf(f, "\n");
    fprintf(f, "/* Include auto-generated hdr: */\n");
    fprintf(f, "#include \"%s.h\"\n", basename);
    fprintf(f, "/*---------------------------Externs------------------------------------*/\n");
    fprintf(f, "/*---------------------------Macros-------------------------------------*/\n");
    fprintf(f, "/*---------------------------Enums--------------------------------------*/\n");
    fprintf(f, "/*---------------------------Typedefs-----------------------------------*/\n");
    fprintf(f, "/*---------------------------Globals------------------------------------*/\n");
    fprintf(f, "/*---------------------------Statics------------------------------------*/\n");
    fprintf(f, "/*---------------------------Prototypes---------------------------------*/\n");
    fprintf(f, "\n");
    fprintf(f, "/* Offset Calculator main  */\n");
    fprintf(f, "int main (int argc, char **argv)\n");
    fprintf(f, "{\n");
    fprintf(f, "    FILE *f = NULL;\n");
    fprintf(f, "    int ret = EXSUCCEED;\n");
    fprintf(f, "    char *output_file = \"%s\";\n", Vfile);
    fprintf(f, "    char *input_file = \"%s\";\n\n", vsrcfile);
    
    /* Generate definitions */
    EXHASH_ITER(hh, views, vel, velt)
    {
        fprintf(f, "    static ndrx_view_offsets_t  offs_%s[]=\n", vel->vname);
        fprintf(f, "    {\n");
            
        DL_FOREACH(vel->fields, fld)
        {
            fprintf(f, "        {\"%s\", EXOFFSET(struct %s, %s), EXELEM_SIZE(struct %s, %s)},\n",
                    fld->cname, 
                    vel->vname, fld->cname, 
                    vel->vname, fld->cname
                    );
        }
        
        fprintf(f, "        {NULL}\n");
        fprintf(f, "    };\n\n");    
    }

    fprintf(f, "    /* Load view file.. */\n");
    fprintf(f, "    if (EXSUCCEED!=ndrx_view_load_file(input_file, EXFALSE))\n");
    fprintf(f, "    {\n");
    fprintf(f,"%s", "        NDRX_LOG(log_error, \"Failed to load source VIEW file [%s]: %s\", \n");
    fprintf(f, "                input_file, tpstrerror(tperrno));\n");
    fprintf(f, "        EXFAIL_OUT(ret);\n");
    fprintf(f, "    }\n\n");
    
    /* Generate update offsets... */
    EXHASH_ITER(hh, views, vel, velt)
    {
        fprintf(f, "    if (EXSUCCEED!=ndrx_view_update_offsets(\"%s\", offs_%s))\n", 
                vel->vname, vel->vname);
        
        fprintf(f, "    {\n");
        fprintf(f, "        EXFAIL_OUT(ret);\n");
        fprintf(f, "    }\n\n");
    }
    
    
    fprintf(f, "    if (NULL==(f=fopen(output_file, \"w\")))\n");
    fprintf(f, "    {");
    fprintf(f, "        int err =errno;\n");
    fprintf(f, "%s", "        NDRX_LOG(log_error, \"Failed to open output file: %s\", strerror(err));\n");
    fprintf(f, "        EXFAIL_OUT(ret);\n");
    fprintf(f, "    }\n\n");

    
    fprintf(f, "    if (EXSUCCEED!=ndrx_view_plot_object(f))\n");
    fprintf(f, "    {\n");
    fprintf(f, "        NDRX_LOG(log_error, \"Failed to plot VIEW object file!\");\n");
    fprintf(f, "        EXFAIL_OUT(ret);\n");
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "\n");
    fprintf(f, "out:\n");
    fprintf(f, "\n");
    fprintf(f, "    if (NULL!=f)\n");
    fprintf(f, "    {\n");
    fprintf(f, "        fclose(f);\n");
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "    if (EXSUCCEED!=ret)\n");
    fprintf(f, "    {\n");
    fprintf(f, "        unlink(output_file);\n");
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "    return ret;\n");
    fprintf(f, "}\n");

    
    NDRX_FCLOSE(f);
    f = NULL;
    
    if (EXSUCCEED!=ndrx_view_invoke(cfile, ofile))
    {
        NDRX_LOG(log_error, "Failed to compile VIEW!");
    }
    else  
    {
        NDRX_LOG(log_info, "Compiled VIEW file: %s", Vfile);
    }
    
out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    unlink(cfile);
    unlink(ofile);

    return ret;
}

