/**
 * @brief Part of UBF library
 *   Utility for generating field header files.
 *   Also the usage of default `fld.tbl' is not supported, as seems to be un-needed
 *   feature.
 *
 * @file mkfldhdr.c
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
#include <atmi.h>
#include <unistd.h>    /* for getopt */

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
expublic int G_langmode = HDR_C_LANG; /* Default mode C */
expublic char G_privdata[FILENAME_MAX+1] = {EXEOS}; /* Private data for lang*/
expublic char G_active_file[FILENAME_MAX+1] = {EXEOS}; /* file in progress  */
expublic char *G_output_dir=".";

expublic renderer_descr_t *M_renderer = NULL;
/*---------------------------Statics------------------------------------*/
exprivate char **M_argv;
exprivate int M_argc;
expublic FILE *G_outf=NULL;

/**
 * Mode functions
 */
expublic renderer_descr_t M_renderer_tab[] =
{
    {HDR_C_LANG, c_get_fullname, c_put_text_line, c_put_def_line, 
                 c_put_got_base_line, c_file_open, c_file_close},
    {HDR_GO_LANG, go_get_fullname, go_put_text_line, go_put_def_line, 
                 go_put_got_base_line, go_file_open, go_file_close},
    {HDR_JAVA_LANG, java_get_fullname, java_put_text_line, java_put_def_line, 
                 java_put_got_base_line, java_file_open, java_file_close},
    {EXFAIL}
};

/*---------------------------Prototypes---------------------------------*/

/*
 * Function for retrieving next file name
 */
exprivate char *(*M_get_next) (int *ret);
exprivate int generate_files(void);


/**
 * Return next file name from arg
 * @param ret
 * @return
 */
exprivate char *get_next_from_arg (int *ret)
{

    *ret=EXSUCCEED;
    if (optind < M_argc)
    {
        char *fname = M_argv[optind];
        optind++;
        NDRX_LOG(log_debug, "Got filename: [%s]", fname);

        return fname;
    }
    
    return NULL;
}

/**
 * Read table list from environment variable.
 * @param ret
 * @return
 */
exprivate char *get_next_from_env (int *ret)
{
    static int first = 1;
    static char *flddir=NULL;
    static char *flds=NULL;
    static char *p_flds, *p_flddir;
    static char tmp_flds[FILENAME_MAX+1];
    static char tmp_flddir[FILENAME_MAX+1];
    static char tmp[FILENAME_MAX+1];
    char *ret_ptr=NULL;
    char *ret_dir=NULL;
    FILE *fp;
    
    NDRX_LOG(log_debug, "%s enter", __func__);
    if (first)
    {
        first=0;

        flddir = (char *)getenv(CONF_FLDTBLDIR);
        if (NULL==flddir)
        {
            ndrx_Bset_error_msg(BFTOPEN, "Field table directory not set - "
                         "check FLDTBLDIR env var");
            *ret=EXFAIL;
            return NULL;
        }
        NDRX_LOG(log_debug, 
                "Load field dir [%s] (multiple directories supported)", 
                 flddir);

        flds = (char *)getenv(CONF_FIELDTBLS);
        if (NULL==flds)
        {
            ndrx_Bset_error_msg(BFTOPEN, "Field table list not set - "
                 "check FIELDTBLS env var");
            *ret=EXFAIL;
            return NULL;
        }

        NDRX_LOG(log_debug, "About to load fields list [%s]", flds);

        NDRX_STRCPY_SAFE(tmp_flds, flds);
        ret_ptr=strtok_r(tmp_flds, ",", &p_flds);

    }
    else
    {
        ret_ptr=strtok_r(NULL, ",", &p_flds);
    }
    

    NDRX_STRCPY_SAFE(tmp_flddir, flddir);
    ret_dir=strtok_r(tmp_flddir, ":", &p_flddir);
    while (NULL!=ret_ptr && NULL != ret_dir)
    {
        snprintf(tmp, sizeof(tmp), "%s/%s", ret_dir, ret_ptr);
        /* maybe use stat here? */
	if (ndrx_file_exists(tmp))
        {
            break;
        }
#if 0
        if (NULL!=(fp=NDRX_FOPEN(tmp, "r")))
        {
            NDRX_FCLOSE(fp);
            break;
        }
#endif
        ret_dir=strtok_r(NULL, ":", &p_flddir);
    }

    if (NULL!=ret_ptr && NULL!=ret_dir)
    {
        snprintf(tmp, sizeof(tmp), "%s/%s", ret_dir, ret_ptr);
        ret_ptr=tmp;
    }

    return ret_ptr;
}

/**
 * Header builder main 
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv)
{
    int ret=EXSUCCEED;
    int c;
    int dbglev;
    extern char *optarg;
    
    M_argv = argv;
    M_argc = argc;
    
    M_renderer = &M_renderer_tab[HDR_C_LANG]; /* default renderer */

    /* Parse command line */
    while ((c = getopt(argc, argv, "h?D:d:m:p:")) != -1)
    {
        switch(c)
        {
            case 'D':
                NDRX_LOG(log_debug, "%c = %s", c, optarg);
                dbglev = atoi(optarg);
                break;
            case 'd':
                G_output_dir = optarg;
                NDRX_LOG(log_debug, "%c = %s", c, G_output_dir);
                break;
            case ':':
                NDRX_LOG(log_error,"-%c without filename\n", optopt);
                ret=EXFAIL;
                break;
            case 'h': case '?':
                printf("usage: %s [-D dbglev, legacy] [-d target directory] [-m lang_mode] [-p priv_data] [field table ...]\n\n",
                        argv[0]);
                printf("OPTIONS\n");
                printf("\tlang_mode:\n");
                printf("\t\t0 - C (default)\n");
                printf("\t\t1 - Go\n");
                printf("\t\t2 - Java\n");
                printf("\tpriv_data:\n");
                printf("\t\tFor Go - package name\n");
                printf("\t\tFor Java - package name\n");
                
                return EXFAIL;
                break;
            /* m - for mode, p - for private mode data (e.g. package name) */
            case 'm':
                G_langmode = atoi(optarg);
                
                if (HDR_MIN_LANG > G_langmode || HDR_MAX_LANG < G_langmode)
                {
                    NDRX_LOG(log_debug, "Invalid language mode %d", G_langmode);
                    return EXFAIL;
                }
                M_renderer = &M_renderer_tab[G_langmode];
                break;
            case 'p':
                NDRX_STRCPY_SAFE(G_privdata, optarg);
                break;
        }
    }
    
    if (EXSUCCEED==ret)
    {
        NDRX_LOG(log_debug, "Output directory is [%s]", G_output_dir);
        NDRX_LOG(log_debug, "Language mode [%d]", G_langmode);
        NDRX_LOG(log_debug, "Private data [%s]", G_privdata);
    }
    
    /* list other options */
    if (optind < argc)
    {
        M_get_next = get_next_from_arg;
        NDRX_LOG(log_debug, "Reading files from command line %p", M_get_next);
    }
    else
    {
        /* Use environment variables */
        M_get_next = get_next_from_env;
        NDRX_LOG(log_debug, "Use environment variables %p", M_get_next);
    }

    ret=generate_files();

    if (EXSUCCEED!=ret)
    {
        B_error("mkfldhdr");
    }


    NDRX_LOG(log_debug, "Finished with : %s",
            ret==EXSUCCEED?"SUCCESS":"FAILURE");

    return ret;
}

/**
 * Extract file name out of partial pathed file
 * 
 * @param
 * @return
 */
char *get_file_name(char *fname)
{
    static char fname_conv[FILENAME_MAX+1];
    char *p;
    
    p = strrchr(fname, '/');
    
    if (NULL==p)
    {
        /* No back slash found in  */
        return fname;
    }
    else
    {
        NDRX_STRCPY_SAFE(fname_conv, p+1);
        return fname_conv;
    }
}

/**
 * Do the hard work - generate header files!
 * @return SUCCEED/FAIL
 */
exprivate int generate_files(void)
{
    int ret=EXSUCCEED;
    char *fname;
    FILE *inf=NULL, *outf=NULL;
    char out_f_name[FILENAME_MAX+1]={EXEOS};

    _ubf_loader_init();

    NDRX_LOG(log_debug, "enter generate_files() func: %p", M_get_next);

    /*
     * Process file by file
     */
    while (NULL!=(fname=M_get_next(&ret)))
    {
        out_f_name[0] = EXEOS;

        /* Open field table file */
        if (NULL==(inf=NDRX_FOPEN(fname, "r")))
        {
            ndrx_Bset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]",
                                fname, strerror(errno));
            EXFAIL_OUT(ret);
        }

        /* Open output file */
        if (EXSUCCEED==ret)
        {
            NDRX_STRCPY_SAFE(G_active_file, get_file_name(fname));
            
            M_renderer->get_fullname(out_f_name);
            
            /* build up path for output file name */
            if (NULL==(G_outf=NDRX_FOPEN(out_f_name, "w")))
            {
                ndrx_Bset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]",
                                            out_f_name, strerror(errno));
                EXFAIL_OUT(ret);
            }
            else
            {
                ret = M_renderer->file_open(out_f_name);
            }
        }

            /* This will also do the check for duplicates! */
        if (EXSUCCEED!=ndrx_ubf_load_def_file(inf, M_renderer->put_text_line,
                                M_renderer->put_def_line, M_renderer->put_got_base_line,
                                fname, EXTRUE))
        {
            EXFAIL_OUT(ret);
        }

        /* close opened files. */
        if (NULL!=inf)
        {
            NDRX_FCLOSE(inf);
            inf=NULL;
        }

        if (NULL!=G_outf)
        {
            M_renderer->file_close(out_f_name);
            NDRX_FCLOSE(G_outf);
            G_outf=NULL;
        }

        NDRX_LOG(log_debug, "%s processed OK, output: %s",
                                            fname, out_f_name);
    }

out:
    if (EXSUCCEED!=ret && EXEOS!=out_f_name[0])
    {
        /* unlink last file */
        NDRX_LOG(log_debug, "Unlinking %s",out_f_name);
        unlink(out_f_name);
    }
    
    return ret;
}


/* vim: set ts=4 sw=4 et smartindent: */
