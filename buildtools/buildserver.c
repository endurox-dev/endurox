/**
 * @brief Generate main() for the XATMI server
 *
 * @file buildserver.c
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
exprivate bs_svcnm_lst_t *M_bs_svcnm_lst = NULL;  /* buildserver cache  */
exprivate bs_svcnm_lst_t *M_bs_funcnm_lst = NULL; /* buildserver cache  */

/**
 * Check service name in list. If found then skip
 * @param svcnm - Service name to check
 * @return EXSUCCEED(found & skip)/EXFAIL
 */
exprivate int chk_listed_svcnm(char *svcnm)
{
    bs_svcnm_lst_t * ret = NULL;

    EXHASH_FIND_STR( M_bs_svcnm_lst, svcnm, ret);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_debug, "Service name [%s] not in list", svcnm);
        goto out;
    }

out:
    if (NULL==ret)
        return EXFAIL;
    else
        return EXSUCCEED;
}

/**
 * Check function name in list. If found then skip
 * @param funcnm function name
 * @param funcreg if set to EXTRUE, it will be registered in service 
 *  table with out name
 * @return EXSUCCEED(found & skip)/EXFAIL
 */
exprivate int chk_listed_funcnm(char *funcnm, int funcreg)
{
    bs_svcnm_lst_t * ret = NULL;

    EXHASH_FIND_STR( M_bs_funcnm_lst, funcnm, ret);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_debug, "Function name [%s] not in list", funcnm);
        goto out;
    }

    if (funcreg && !ret->funcreg)
    {
        ret->funcreg = EXTRUE;
    }

out:
    if (NULL==ret)
    {
        return EXFAIL;
    }
    else
    {
        return EXSUCCEED;
    }
}

/**
 * Add add SVCNM/FUNCNM to list
 * @param svcnm - Service name add to list
 * @param funcnm - Function name add to list
 * @return EXSUCCEED/EXFAIL
 */
exprivate int add_listed_svcnm(char *svcnm, char *funcnm)
{
    bs_svcnm_lst_t * ret = NDRX_CALLOC(1, sizeof(bs_svcnm_lst_t));
    
    if (NULL==ret)
    {
        int err = errno;
        
        _Nset_error_fmt(NEMALLOC, "Failed to alloc: %s", strerror(err));
        
        NDRX_LOG(log_error, 
                 "Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
        goto out;
    }
    
    NDRX_STRCPY_SAFE(ret->svcnm, svcnm);
    NDRX_STRCPY_SAFE(ret->funcnm, funcnm);
    
    EXHASH_ADD_STR( M_bs_svcnm_lst, svcnm, ret );
    
out:
    if (NULL==ret)
    {
        return EXFAIL;
    }
    
    return EXSUCCEED;
}

/**
 * Add add FUNCNM to list
 * @param funcnm - Function name add to cache
 * @param funcreg if set to EXTRUE, it will be registered in service 
 *  table with out name
 * @return EXSUCCEED/EXFAIL
 */
exprivate int add_listed_funcnm(char *funcnm, int funcreg)
{
    bs_svcnm_lst_t * ret = NDRX_CALLOC(1, sizeof(bs_svcnm_lst_t));
    
    if (NULL==ret)
    {
        int err = errno;
        _Nset_error_fmt(NEMALLOC, "Failed to alloc: %s", strerror(err));
        NDRX_LOG(log_error, 
                 "Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
        goto out;
    }
    
    NDRX_STRCPY_SAFE(ret->funcnm, funcnm);
    ret->funcreg = funcreg;
    EXHASH_ADD_STR( M_bs_funcnm_lst, funcnm, ret );
    
out:
    if (NULL==ret)
    {
        return EXFAIL;
    }
    
    return EXSUCCEED;
}

/**
 * Function to parse Service/Function from string 
 * Format: service[,service...][:function] | :function } ]
 * this could be:
 * 
 * -s a,b,c
 * -s a,b,c:FUNC
 * -s :FUNC
 * 
 * NOTE! Empty functions are allowed too. They will be registered in the service
 * table.
 * 
 * @param s_value string to be parsed
 * @param advtbl advertising table to return
 * @return EXSUCCEED/EXFAIL
 */
exprivate int parse_s_string(char *p_string)
{
    int ret = EXSUCCEED;
    char svcnm[128+1]={EXEOS};
    char funcnm[MAXTIDENT+1]={EXEOS};
    char *f=NULL, *p=NULL, *str=NULL;
    
    f=strchr(p_string, ':');
    
    if (NULL != f)
    {
        NDRX_STRCPY_SAFE(funcnm, f+1);
        *f = EXEOS; /* terminate the parsing string here.. */
    }

    /* In case when not provided SVCNM 
     * If : was in start of the string.. the pointer matches
     */
    if (NULL != f && p_string==f)
    {
        NDRX_LOG(log_debug, "FUNCNM=[%s] Only", funcnm);
        
        if (EXSUCCEED != chk_listed_funcnm(funcnm, EXTRUE) && 
                EXSUCCEED!=add_listed_funcnm(funcnm, EXTRUE))
        {
            EXFAIL_OUT(ret);
        }
        /* we are done... */
        return ret;
    }
    
    /* lets continue to parse -s A,B,C with */
    for (p = strtok_r(p_string, ",", &str);p != NULL; p = strtok_r(NULL, ",", &str))
    {
        /* Function name is provided */
        if (NULL != f)
        {
            NDRX_STRCPY_SAFE(funcnm, f+1);
        }
        /* Function name not provided, set as Service name*/
        else 
        {
            NDRX_STRCPY_SAFE(funcnm, p);
        }

        NDRX_STRCPY_SAFE(svcnm, p);

        NDRX_LOG(log_debug, "SVCNM=[%s] FUNCNM=[%s]\n", svcnm, funcnm);
        
        /* Add function */
        if (EXSUCCEED != chk_listed_funcnm(funcnm, EXFALSE) && 
                EXSUCCEED!=add_listed_funcnm(funcnm, EXFALSE))
        {
            NDRX_LOG(log_error, "Failed to list the function name");
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED == chk_listed_svcnm(svcnm))
        {
            NDRX_LOG(log_debug, 
                     "Warning svcnm=[%s] already exist SKIP!!!", svcnm);
            /* already present, process next */
            continue;
        }
        
        if (EXSUCCEED!=add_listed_svcnm(svcnm, funcnm))
        {
            NDRX_LOG(log_error, "Failed to add service [%s]", svcnm);
            EXFAIL_OUT(ret);
        }
    }
   

out:
    return ret;
}

/**
 * print help for the command
 * @param name name of the program, argv[0]
 */
exprivate void print_help(char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n               Do not generate main() entry\n");
    fprintf(stderr, "  -C               COBOL code (RFU)\n");
    fprintf(stderr, "  -s {@filename|svc1,svc2,...:func|:func}\n");
    fprintf(stderr, "                   Services to be advertised. In case of @filename. \n");
    fprintf(stderr, "                   Each line in file shall contain the {svc1,svc2,...:func|:func} expression.\n");
    fprintf(stderr, "                   Lines starting with # are ignored\n");
    fprintf(stderr, "  -o <filename>    Output compiled file name, default is SERVER\n");
    fprintf(stderr, "  -f <firstfiles>  File names to be passed to compiler, on left side before Enduro/X libraries\n");
    fprintf(stderr, "  -l <lastfiles>   File names to be passed to compiler, on right side after Enduro/X libraries\n");
    fprintf(stderr, "  -r <RM_NAME>     Resource manager name to be searched in $NDRX_HOME/udataobj/RM\n");
    fprintf(stderr, "  -g <RM_NAME>     Same as -r\n");
    fprintf(stderr, "  -k               Keep generated source file\n");
    fprintf(stderr, "  -t               Program is built for threaded mode\n");
    fprintf(stderr, "  -v               Verbose mode (print build command)\n");
    fprintf(stderr, "  -h               Print this help\n");   
}

/**
 * Function to read Service/Function from file
 * @param s_value File name to load Services/Function
 * @param advtbl advertising table to return
 * @return EXSUCCEED/EXFAIL
 */
exprivate int parse_s_file(char *infile)
{
    int ret = EXSUCCEED;
    FILE * fp;
    char buf[PATH_MAX+1];

    if (NULL==(fp=NDRX_FOPEN(infile, "r")))
    {
        int err = errno;
        
        _Nset_error_fmt(NENOENT, "Failed to open file [%s] for reading: %s", 
                infile, strerror(err));
        
        NDRX_LOG(log_error, "Failed to open file [%s] for reading: %s", 
                infile, strerror(errno));
        EXFAIL_OUT(ret);
    }

    while (NULL!=fgets(buf, sizeof(buf), fp))
    {
        ndrx_str_rstrip(buf," \t\n\r");
        
        if ('#'==buf[0] || EXEOS==buf[0])
        {
            continue;
        }
        
        if ( EXSUCCEED!= parse_s_string(buf) )
        {
            EXFAIL_OUT(ret);
        }
    }
    
out:
    
    if (NULL!=fp)
    {
        NDRX_FCLOSE(fp);
        fp = NULL;
    }

    return ret;
}

/**
 * Main entry point for view compiler
 */
int main(int argc, char **argv)
{
    int ret = EXSUCCEED;
    int c;
    int lang_mode = HDR_C_LANG;
    char ofile[PATH_MAX+1]="SERVER";
    char cfile[PATH_MAX+1]="ndrx_bs_XXXXXX.c";
    char *s_value=NULL;
    int thread_option=EXFALSE;
    int keep_buildserver_main=EXFALSE;
    char firstfiles[PATH_MAX+1] = {EXEOS};
    char lastfiles[PATH_MAX+1] = {EXEOS};
    int nomain = EXFALSE;
    int verbose = EXFALSE;
    ndrx_rm_def_t rmdef;
    
    NDRX_BANNER;
    
    fprintf(stderr, "BUILDSERVER Compiler\n\n");
    
    /* clear any error... */
    _Nunset_error();
    
    memset(&rmdef, 0, sizeof(rmdef));

    while ((c = getopt (argc, argv, "Cktvrgs:o:f:l:nh")) != -1)
    {
        switch (c)
        {
            case 'h':
                print_help(argv[0]);
                return 0; /*<<<< RETURN ! */
                break;
            case 'n':
                /* No main... */
                NDRX_LOG(log_debug, "Not generating main...");
                nomain=EXTRUE;
                break;
            case 'C':
                NDRX_LOG(log_warn, "COBOL mode not yet supported");
                _Nset_error_fmt(NESUPPORT, "COBOL mode not yet supported");
                EXFAIL_OUT(ret);
                break;
            case 's':
                s_value= optarg;
                NDRX_LOG(log_debug, "s_value: [%s]", s_value);
                if ( '@'==s_value[0] )
                {
                    if (EXSUCCEED != parse_s_file(s_value) )
                    {
                        NDRX_LOG(log_warn, 
                             "Failed to read Service/Function from [%s]", 
                             s_value);
                        EXFAIL_OUT(ret);
                    }
                }
                else 
                {
                    if ( EXSUCCEED != parse_s_string(s_value) )
                    {
                        NDRX_LOG(log_error, 
                             "Failed to parse Service/Function from value [%s]", 
                             s_value);
                        EXFAIL_OUT(ret);
                    }
                }
                break;
            case 'o':
                NDRX_STRCPY_SAFE(ofile, optarg);
                NDRX_LOG(log_debug, "ofile: [%s]", ofile);
                break;
            case 'f':
                if (EXEOS==firstfiles[0])
                {
                    NDRX_STRCPY_SAFE(firstfiles, optarg);
                }
                else
                {
                    NDRX_STRCAT_S(firstfiles, sizeof(firstfiles), " ");
                    NDRX_STRCAT_S(firstfiles, sizeof(firstfiles), optarg);
                }
                break;
            case 'l':
                if (EXEOS==lastfiles[0])
                {
                    NDRX_STRCPY_SAFE(lastfiles, optarg);
                }
                else
                {
                    NDRX_STRCAT_S(lastfiles, sizeof(lastfiles), " ");
                    NDRX_STRCAT_S(lastfiles, sizeof(lastfiles), optarg);
                }
                break;
            case 'r':
            case 'g':
                
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
                keep_buildserver_main=EXTRUE;
                break;
            case 't':
                thread_option = EXTRUE;
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
    
    /* Plot the the header */
    if (HDR_C_LANG==lang_mode)
    {
        if ( EXFAIL==mkstemps(cfile,2) )
        {
            int err = errno;
            NDRX_LOG(log_error, "Failed with error %s", strerror(err));
            _Nset_error_fmt(NEUNIX, "Failed to create temporary file: %s", 
                    strerror(err));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_buildsrv_generate_code(cfile, thread_option, 
                                                   M_bs_svcnm_lst, 
                                                   M_bs_funcnm_lst,
                                                   &rmdef, nomain))
        {
            NDRX_LOG(log_error, "Failed to generate code!");
            EXFAIL_OUT(ret);
        }
    }
    
    if (HDR_C_LANG==lang_mode)
    {
        if (EXSUCCEED!=ndrx_compile_c(COMPILE_SRV, verbose, cfile, ofile, 
                firstfiles, lastfiles, &rmdef))
        {
            NDRX_LOG(log_error, "Failed to build");
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        NDRX_LOG(log_error, "Invalid language mode: %d", lang_mode);
        _Nset_error_fmt(NEINVAL, "Invalid language mode %d", lang_mode);
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

    if (EXFALSE == keep_buildserver_main)
    {
        unlink(cfile);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
