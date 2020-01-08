/**
 * @brief Generate code for C lang
 * TODO use: "tmnull_switch" when XA switch is not available for compatilibty..
 *
 * @file codegen_c.c
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
#include "buildtools.h"
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * Generate C code for build server / build transaction manager
 * @param cfile - Auto generated buildeserver main code
 * @param thread_option - 
 * @param p_svcnm_lst - list of Srevice names
 * @param p_funcnm_lst - list of Function names
 * @param p_rmdef - resource manager definition
 * @param nomain - do not generate main if 1
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_buildsrv_generate_code(char *cfile, int thread_option, 
                                        bs_svcnm_lst_t *p_svcnm_lst, 
                                        bs_svcnm_lst_t *p_funcnm_lst,
                                        ndrx_rm_def_t *p_rmdef, int nomain)
{
    int ret = EXSUCCEED;
    FILE *f = NULL;
    bs_svcnm_lst_t *bs, *bst;

    NDRX_LOG(log_info, "C-Code to compile: [%s]", cfile);

    if (NULL==(f=NDRX_FOPEN(cfile, "w")))
    {
        int err = errno;
        _Nset_error_fmt(NEWRITE, "Failed to open file [%s] for write: %s", 
                cfile, strerror(err));
        NDRX_LOG(log_error, "Failed to open for write [%s]: %s", 
                cfile, strerror(errno));
        EXFAIL_OUT(ret);
    }

    fprintf(f, "/* Buildserver auto-generated code */\n");
    fprintf(f, "/*---------------------------Includes-----------------------------------*/\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <ndebug.h>\n");
    fprintf(f, "#include <atmi.h>\n");
    fprintf(f, "#include <ndrstandard.h>\n");
    fprintf(f, "#include <ubf.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <unistd.h>\n");
    fprintf(f, "#include <xa.h>\n");
    
    fprintf(f, "/*---------------------------Externs------------------------------------*/\n");
    fprintf(f, "/* Buildserver auto generated extern service list */\n");

    /* Generate definitions */
    if (EXEOS!=p_rmdef->structname[0])
    {
        fprintf(f, "extern struct xa_switch_t %s;\n", p_rmdef->structname);
    }
    
    EXHASH_ITER(hh, p_funcnm_lst, bs, bst)
    {
        fprintf(f, "extern void %s (TPSVCINFO *);\n", bs->funcnm);
    }
    
    fprintf(f, "/*---------------------------Macros-------------------------------------*/\n");
    fprintf(f, "/*---------------------------Enums--------------------------------------*/\n");
    fprintf(f, "/*---------------------------Typedefs-----------------------------------*/\n");
    fprintf(f, "/*---------------------------Globals------------------------------------*/\n");
    fprintf(f, "/*---------------------------Statics------------------------------------*/\n");
    fprintf(f, "/* Auto generated system advertise table */\n");
    fprintf(f, "expublic struct tmdsptchtbl_t ndrx_G_tmdsptchtbl[] = {\n");
    
    EXHASH_ITER(hh, p_svcnm_lst, bs, bst)
    {
        fprintf(f, "    {\"%s\",\"%s\",(void (*)(TPSVCINFO *)) %s, 0, 0 },\n", 
                                bs->svcnm, bs->funcnm, bs->funcnm);
    }
        
    /* list empty functions */
    EXHASH_ITER(hh, p_funcnm_lst, bs, bst)
    {
        if (bs->funcreg)
        {
            fprintf(f, "    {\"%s\",\"%s\",(void (*)(TPSVCINFO *)) %s, 0, 0 },\n", 
                                    bs->svcnm, bs->funcnm, bs->funcnm);
        }
    }
    
    fprintf(f, "    { NULL, NULL, NULL, 0, 0 }\n");
    fprintf(f, "};\n");

    fprintf(f, "/*---------------------------Prototypes---------------------------------*/\n");
    fprintf(f, "\n");
    
    /* if not no main requested... */
    if (!nomain)
    {
        fprintf(f, "/* Buildserver main  */\n");
        fprintf(f, "int main( int argc, char** argv )\n");
        fprintf(f, "{\n");
        fprintf(f, "    _tmbuilt_with_thread_option=%d;\n",thread_option);
        fprintf(f, "    struct tmsvrargs_t tmsvrargs =\n");
        fprintf(f, "    {\n");
        fprintf(f, "        &%s,\n", (EXEOS!=p_rmdef->structname[0]?p_rmdef->structname:""));
        fprintf(f, "        &ndrx_G_tmdsptchtbl[0],\n");
        fprintf(f, "        0,\n");
        fprintf(f, "        tpsvrinit,\n");
        fprintf(f, "        tpsvrdone,\n");
        fprintf(f, "        NULL,\n");
        fprintf(f, "        NULL,\n");
        fprintf(f, "        NULL,\n");
        fprintf(f, "        NULL,\n");
        fprintf(f, "        NULL\n");
        fprintf(f, "    };\n");
        fprintf(f, "#ifdef TMMAINEXIT\n");
        fprintf(f, "#include \"mainexit.h\"\n");
        fprintf(f, "#endif\n");
        fprintf(f, "    return( _tmstartserver( argc, argv, &tmsvrargs ));\n");
        fprintf(f, "}\n");
    }

out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
        f = NULL;
    }

    return ret;
}

/**
 * Generate build client C code
 * @param cfile Auto generated buildclient main code
 * @param p_rmdef resource manager definition
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_buildclt_generate_code(char *cfile, ndrx_rm_def_t *p_rmdef)
{
    int ret = EXSUCCEED;
    FILE *f = NULL;
    bs_svcnm_lst_t *bs, *bst;

    NDRX_LOG(log_info, "C-Code to compile: [%s]", cfile);

    if (NULL==(f=NDRX_FOPEN(cfile, "w")))
    {
        NDRX_LOG(log_error, "Failed to open for write [%s]: %s", 
                cfile, strerror(errno));
        EXFAIL_OUT(ret);
    }

    fprintf(f, "/* Buildclient auto-generated code */\n");
    fprintf(f, "/*---------------------------Includes-----------------------------------*/\n");
    fprintf(f, "#include <atmi.h>\n");
    fprintf(f, "#include <xa.h>\n");

    /* Generate definitions */
    if (EXEOS!=p_rmdef->structname[0])
    {
        fprintf(f, "extern struct xa_switch_t %s;\n", p_rmdef->structname);
    }

    fprintf(f, "expublic struct tmsvrargs_t ndrx_G_tmsvrargs_stat = "
                "{&%s, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL};\n",
                (EXEOS!=p_rmdef->structname[0]?p_rmdef->structname:"tmnull_switch"));
    fprintf(f, "expublic struct tmsvrargs_t *ndrx_G_tmsvrargs = &ndrx_G_tmsvrargs_stat;\n");

    NDRX_FCLOSE(f);
    f = NULL;

out:

    if (NULL!=f)
    {
        NDRX_FCLOSE(f);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
