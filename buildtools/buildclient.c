/**
 * @brief Part of UBF library
 *   Utility for generating field header files.
 *   Also the usage of default `fld.tbl' is not supported, as seems to be un-needed
 *   feature.
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

#include <ubf.h>
#include <ferror.h>
#include <fieldtable.h>
#include <fdatatype.h>

#include <ndrstandard.h>
#include <ndebug.h>
#include "buildserver.h"

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate bs_svcnm_lst_t *M_bs_svcnm_lst = NULL; /* buildserver cache . */
exprivate bs_svcnm_lst_t *M_bs_funcnm_lst = NULL; /* buildserver cache . */


exprivate int get_rm_name(char *xaswitch, char *optarg)
{
    int ret = EXSUCCEED;
    


out:
    return ret;
}


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
 * Check funtion name in list. If found then skip
 * @param svcnm - Service name to check
 * @return EXSUCCEED(found & skip)/EXFAIL
 */
exprivate int chk_listed_funcnm(char *funcnm)
{
    bs_svcnm_lst_t * ret = NULL;

    EXHASH_FIND_STR( M_bs_funcnm_lst, funcnm, ret);
    
    if (NULL==ret)
    {
        NDRX_LOG(log_debug, "Function name [%s] not in list", funcnm);
        goto out;
    }

out:
    if (NULL==ret)
        return EXFAIL;
    else
        return EXSUCCEED;
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
        NDRX_LOG(log_error, "Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
        userlog("Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
    }
    
    NDRX_STRCPY_SAFE(ret->svcnm, svcnm);
    NDRX_STRCPY_SAFE(ret->funcnm, funcnm);
    
    EXHASH_ADD_STR( M_bs_svcnm_lst, svcnm, ret );
    
    if (NULL!=ret)
        return EXSUCCEED;
    else
        return EXFAIL;
}

/**
 * Add add FUNCNM to list
 * @param svcnm - Service name add to cache
 * @param funcnm - Function name add to cache
 * @return EXSUCCEED/EXFAIL
 */
exprivate int add_listed_funcnm(char *funcnm)
{
    bs_svcnm_lst_t * ret = NDRX_CALLOC(1, sizeof(bs_svcnm_lst_t));
    
    if (NULL==ret)
    {
        NDRX_LOG(log_error, "Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
        userlog("Failed to alloc bs_svcnm_lst_t: %s", strerror(errno));
    }
    
    NDRX_STRCPY_SAFE(ret->funcnm, funcnm);
    
    EXHASH_ADD_STR( M_bs_funcnm_lst, funcnm, ret );
    
    if (NULL!=ret)
        return EXSUCCEED;
    else
        return EXFAIL;
}

/**
 * Function to parse Service/Function from string 
 * @param s_value string to be parsed
 * @param advtbl advertising table to return
 * @return 
 */
exprivate int parse_s_string(char *p_string)
{
    int ret = EXSUCCEED;
    char svcnm[128+1]={EXEOS};
    char funcnm[15+1]={EXEOS};
    char *f=NULL, *p=NULL, *str=NULL;
    bs_svcnm_lst_t *tmp=NULL;

    f=strchr(p_string, ':');
    if (NULL != f)
    {
        NDRX_STRCPY_SAFE(funcnm, f+1);
    }

    p = strtok_r(p_string, ",:", &str);

    /* In case when not provided SVCNM */
    if (NULL != f && 0==strcmp(f+1,p))
    {
        NDRX_STRCPY_SAFE(svcnm, funcnm);
        NDRX_LOG(log_debug, "SVCNM=[%s] FUNCNM=[%s]\n", svcnm, funcnm);
        if (EXSUCCEED == chk_listed_svcnm(svcnm))
        {
            NDRX_LOG(log_debug, "Warning svcnm=[%s] already exist SKIP!!!", svcnm);
            goto out;
        }
        if (EXSUCCEED!=add_listed_svcnm(svcnm, funcnm))
        {
            EXFAIL_OUT(ret);
        }
        if (EXSUCCEED != chk_listed_funcnm(funcnm))
        {
            if (EXSUCCEED!=add_listed_funcnm(funcnm))
            {
                EXFAIL_OUT(ret);
            }
        }
    }

    while (p != NULL )
    {
        if ( NULL != f && 0==strcmp(f+1,p) )
        {
            break;
        }

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
        if (EXSUCCEED == chk_listed_svcnm(svcnm))
        {
            NDRX_LOG(log_debug, "Warning svcnm=[%s] already exist SKIP!!!", svcnm);
            goto out;
        }
        if (EXSUCCEED!=add_listed_svcnm(svcnm, funcnm))
        {
            EXFAIL_OUT(ret);
        }
        if (EXSUCCEED != chk_listed_funcnm(funcnm))
        {
            if (EXSUCCEED!=add_listed_funcnm(funcnm))
            {
                EXFAIL_OUT(ret);
            }
        }

        p = strtok_r(NULL, ",:", &str);
    }

    out:
    return ret;
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
    char *string = NULL;
    size_t len = 0;

    if (NULL==(fp=NDRX_FOPEN(infile+1, "r")))
    {
        NDRX_LOG(log_error, "Failed to open file [%s] for reading: %s", 
                infile+1, strerror(errno));
        EXFAIL_OUT(ret);
    }

    while (EXFAIL != ndrx_getline(&string, &len, fp))
    {
        ndrx_str_rstrip(string," \t\n");
NDRX_LOG(log_error, "Parse string [%s] len=[%d]", string, (int)len);
        if ('#'==string[0])
        {
            NDRX_LOG(log_error, "Skip comment [%s]", string);
        }
        else if ( EXSUCCEED!= parse_s_string(string) )
        {
            EXFAIL_OUT(ret);
        }
    }

out:
    NDRX_FCLOSE(fp);
    fp = NULL;

    if (NULL!=string)
    {
        NDRX_FREE(string);
    }

    return ret;
}

/**
 * Main entry point for view compiler
 */
int main(int argc, char **argv)
{
    int ret = EXSUCCEED;
    int i;
    int c;
    int lang_mode = HDR_C_LANG;
    char ofile[PATH_MAX+1]="SERVER";
    char cfile[PATH_MAX+1]="ndrx_bs_XXXXXX.c";
    FILE *f=NULL;
    char *env_cc, *env_cflags, *env_ndrx_home;
    char ndrx_lib[PATH_MAX+1]={EXEOS};
    char ndrx_inc[PATH_MAX+1]={EXEOS};
    char *s_value=NULL;
    char svcnm[15+1]={EXEOS};
    char funcnm[128+1]={EXEOS};
    char tmp[PATH_MAX];
    int thread_option=EXFALSE;
    int keep_buildserver_main=EXFALSE;
    char build_cmd[PATH_MAX+1];
    char firstfiles[PATH_MAX+1];
    char lastfiles[PATH_MAX+1];
    char *xaswitch=NULL;
    
    NDRX_BANNER;
    
    fprintf(stderr, "BUILDSERVER Compiler\n\n");

    while ((c = getopt (argc, argv, "Cktvrgs:o:f:l:")) != -1)
    {
        switch (c)
        {
            case 'C':
                NDRX_LOG(log_warn, "Ignoring option C for COBOL");
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
                        NDRX_LOG(log_warn, 
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
                NDRX_STRCPY_SAFE(firstfiles, optarg);
                break;
            case 'l':
                NDRX_STRCPY_SAFE(lastfiles, optarg);
                break;
            case 'r':
            case 'g':
                /*NDRX_LOG(log_error, "ERROR! Please use tmsrv with corresponding "
                    "shared libraries! No need to build resource manager.");
                EXFAIL_OUT(ret);*/
                if ( EXSUCCEED != get_rm_name(xaswitch, optarg) )
                {
                        NDRX_LOG(log_warn, 
                             "Failed to {parse Service(s)/Function from value ", 
                             s_value);
                        EXFAIL_OUT(ret);
                }
                break;
            case 'k':
                keep_buildserver_main=EXTRUE;
                break;
            case 't':
                thread_option = EXTRUE;
                break;
            case 'v':
                fprintf(stderr, "Debug output shall be configured in "
                    "ndrxdebug.conf or CCONFIG\n");
                break;
            case '?':
                EXFAIL_OUT(ret);
            default:
                NDRX_LOG(log_error, "Default case...");
                EXFAIL_OUT(ret);
        }
    }
    
    /* Plot the the header */
    if (HDR_C_LANG==lang_mode)
    {
        if ( EXFAIL==mkstemps(cfile,2) )
        {
            NDRX_LOG(log_error, "Failed with error %s\n", strerror(errno));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=ndrx_buildserver_generate_code(cfile, thread_option, 
                                                      M_bs_svcnm_lst, M_bs_funcnm_lst,
                                                      xaswitch))
        {
            NDRX_LOG(log_error, "Failed to generate code!");
            EXFAIL_OUT(ret);
        }
    }
    
    if (HDR_C_LANG==lang_mode)
    {
        if (EXSUCCEED!=ndrx_compile_c(cfile, COMPILE_SRV))
        {
            EXFAIL_OUT(ret);
        }
    }

out:

    if (EXFALSE == keep_buildserver_main)
    {
        unlink(cfile);
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
