/**
 * @brief Generate main() for the XATMI server
 *
 * @file buildserver.c
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
#include <ndrx_config.h>
#include <atmi.h>
#include <atmi_int.h>
#include <sys_unix.h>
#include <ctype.h>
#include <limits.h>

#include <ubf.h>
#include <ferror.h>
#include <fieldtable.h>
#include <fdatatype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include "buildtools.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
/**
 * print help for the command
 * @param name name of the program, argv[0]
 */
exprivate void print_help(char *name)
{
    fprintf(stderr, "Usage: %s [options] -o filename \n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n               Do not generate main() entry\n");
    fprintf(stderr, "  -o <filename>    Output compiled file name\n");
    fprintf(stderr, "  -r <rm_name>     Resource manager name to be searched in $NDRX_HOME/udataobj/RM.\n");
    fprintf(stderr, "                   If not set, null switch is used.\n");
    fprintf(stderr, "  -k               Keep generated source file\n");
    fprintf(stderr, "  -v               Verbose mode (print build command)\n");
    fprintf(stderr, "  -h               Print this help\n");   
}

/**
 * Main entry point for view compiler
 */
int main(int argc, char **argv)
{
    int ret = EXSUCCEED;
    int c;
    char ofile[PATH_MAX+1]="";
    char cfile[PATH_MAX+1]="ndrx_tms_XXXXXX.c";
    int keep_main=EXFALSE;
    char firstfiles[PATH_MAX+1] = {EXEOS};
    char lastfiles[PATH_MAX+1] = {EXEOS};
    int nomain = EXFALSE;
    int verbose = EXFALSE;
    ndrx_rm_def_t rmdef;

    NDRX_BANNER;
    
    fprintf(stderr, "BUILDTMS Compiler\n\n");
    
    /* clear any error... */
    _Nunset_error();
    
    memset(&rmdef, 0, sizeof(rmdef));

    while ((c = getopt (argc, argv, "ho:r:vk")) != -1)
    {
        switch (c)
        {
            case 'h':
                print_help(argv[0]);
                return 0; /*<<<< RETURN ! */
                break;
            case 'o':
                NDRX_STRCPY_SAFE(ofile, optarg);
                NDRX_LOG(log_debug, "ofile: [%s]", ofile);
                break;
            case 'r':    
                if ( EXFAIL == (ret=ndrx_get_rm_name(optarg, &rmdef)))
                {
                    /* error is set */
                    NDRX_LOG(log_error, 
                         "Failed to parse resource manager: [%s], check -r", optarg);
                    EXFAIL_OUT(ret);
                }
                else if (EXTRUE!=ret)
                {
                    NDRX_LOG(log_error, 
                         "Resource manager not defined: [%s], check -r", optarg);
                    /* set error */
                    _Nset_error_fmt(NEINVAL, "Resourc manager [%s] not found "
                            "in udataobj/RM files", optarg);
                    EXFAIL_OUT(ret);
                }
                
                ret = EXSUCCEED;

                break;
            case 'k':
                keep_main=EXTRUE;
                break;
            case 'v':
                NDRX_LOG(log_debug, "running in verbose mode");
                verbose = EXTRUE;
                break;
            case '?':
            default:
            
                print_help(argv[0]);
                return EXFAIL; /*<<<< RETURN ! */
        }
    }
    
    if (EXEOS==ofile[0])
    {
        NDRX_LOG(log_error, "error! Output file name is not set, use -o");
        /* set error */
        _Nset_error_fmt(NEINVAL, "error! Output file name is not set, use -o");
        EXFAIL_OUT(ret);
    }
    
    if ( EXFAIL==mkstemps(cfile,2) )
    {
        int err = errno;
        NDRX_LOG(log_error, "Failed with error %s", strerror(err));
        _Nset_error_fmt(NEUNIX, "Failed to create temporary file: %s", 
                strerror(err));
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_buildsrv_generate_code(cfile, EXFALSE, 
                                               NULL, 
                                               NULL,
                                               &rmdef, nomain))
    {
        NDRX_LOG(log_error, "Failed to generate code!");
        EXFAIL_OUT(ret);
    }

    if (EXSUCCEED!=ndrx_compile_c(COMPILE_TMS, verbose, cfile, ofile, 
            firstfiles, lastfiles, &rmdef))
    {
        NDRX_LOG(log_error, "Failed to build");
        EXFAIL_OUT(ret);
    }
    
out:

    /* cleanup hash lists... */
    if (EXSUCCEED!=ret)
    {
        if (!_Nis_error())
        {
            _Nset_error_fmt(NESYSTEM, "Generic error - see logs.");
        }
        
        /* print error */
        fprintf(stderr, "%s: %s\n", argv[0], ndrx_Nstrerror2(Nerror));
    }

    if (EXFALSE == keep_main)
    {
        unlink(cfile);
    }


    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
