/* 
** Part of UBF library
** Utility for generating field header files.
** !!! THERE IS NO SUPPORT for multiple directories with in FLDTBLDIR!!!
** Also the usage of default `fld.tbl' is not supported, as seems to be un-needed
** feature.
**
** @file mkfldhdr.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
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
#include "mkfldhdr.h"
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
public int G_langmode = HDR_C_LANG; /* Default mode C */
public char G_privdata[FILENAME_MAX+1] = {EOS}; /* Private data for lang*/
public char G_active_file[FILENAME_MAX+1] = {EOS}; /* file in progress  */
public char *G_output_dir=".";

public renderer_descr_t *M_renderer = NULL;
/*---------------------------Statics------------------------------------*/
private char **M_argv;
private int M_argc;
public FILE *G_outf=NULL;

/*
 * Mode functions
 */
public renderer_descr_t M_renderer_tab[] =
{
    {HDR_C_LANG, c_get_fullname, c_put_text_line, c_put_def_line, 
                 c_put_got_base_line, c_file_open, c_file_close},
    {HDR_GO_LANG, go_get_fullname, go_put_text_line, go_put_def_line, 
                 go_put_got_base_line, go_file_open, go_file_close},
    {FAIL}
};

/*---------------------------Prototypes---------------------------------*/

/*
 * Function for retrieving next file name
 */
char *(*M_get_next) (int *ret);
private int generate_files(void);


/**
 * Return next file name from arg
 * @param ret
 * @return
 */
char *get_next_from_arg (int *ret)
{

    *ret=SUCCEED;
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
char *get_next_from_env (int *ret)
{
    static int first = 1;
    static char *flddir=NULL;
    static char *flds=NULL;
    static char tmp_flds[FILENAME_MAX+1];
    static char tmp[FILENAME_MAX+1];
    char *ret_ptr=NULL;
    
    if (first)
    {
        first=0;

        flddir = (char *)getenv(FLDTBLDIR);
        if (NULL==flddir)
        {
            _Fset_error_msg(BFTOPEN, "Field table directory not set - "
                         "check FLDTBLDIR env var");
            *ret=FAIL;
            return NULL;
        }
        UBF_LOG(log_debug, "Load field dir [%s]", flddir);

        flds = (char *)getenv(FIELDTBLS);
        if (NULL==flds)
        {
            _Fset_error_msg(BFTOPEN, "Field table list not set - "
                 "check FIELDTBLS env var");
            *ret=FAIL;
            return NULL;
        }

        UBF_LOG(log_debug, "About to load fields list [%s]", flds);

        strcpy(tmp_flds, flds);
        ret_ptr=strtok(tmp_flds, ",");
    }
    else
    {
        ret_ptr=strtok(NULL, ",");
    }

    if (NULL!=ret_ptr)
    {
        sprintf(tmp, "%s/%s", flddir, ret_ptr);
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
    int ret=SUCCEED;
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
                ret=FAIL;
                break;
            case 'h': case '?':
                printf("usage: %s [-D dbglev, legacy] [-d target directory] [field table ...]\n",
                        argv[0]);
                
                return FAIL;
                break;
            /* m - for mode, p - for private mode data (e.g. package name) */
            case 'm':
                G_langmode = atoi(optarg);
                
                if (HDR_MIN_LANG > G_langmode || HDR_MAX_LANG < G_langmode)
                {
                    NDRX_LOG(log_debug, "Invalid language mode %d", G_langmode);
                    return FAIL;
                }
                M_renderer = &M_renderer_tab[G_langmode];
                break;
            case 'p':
                strncpy(G_privdata, optarg, FILENAME_MAX);
                G_privdata[FILENAME_MAX] = EOS;
                break;
        }
    }
    
    if (SUCCEED==ret)
    {
        NDRX_LOG(log_debug, "Output directory is [%s]", G_output_dir);
        NDRX_LOG(log_debug, "Language mode [%d]", G_langmode);
        NDRX_LOG(log_debug, "Private data [%s]", G_privdata);
    }
    
    /* list other options */
    if (optind < argc)
    {
        NDRX_LOG(log_debug, "Reading files from command line");
        M_get_next = get_next_from_arg;
    }
    else
    {
        /* Use environment variables */
        NDRX_LOG(log_debug, "Use environment variables");
        M_get_next = get_next_from_env;
    }

    ret=generate_files();

    if (SUCCEED!=ret)
    {
        B_error("mkfldhdr");
    }


    NDRX_LOG(log_debug, "Finished with : %s",
            ret==SUCCEED?"SUCCESS":"FAILURE");

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
        int len = strlen(p+1);
        strncpy(fname_conv, p+1, len);
        fname_conv[len] = EOS;

        return fname_conv;
    }
}

/**
 * Do the hard work - generate header files!
 * @return SUCCEED/FAIL
 */
private int generate_files(void)
{
    int ret=SUCCEED;
    char *fname;
    FILE *inf=NULL, *outf=NULL;
    char out_f_name[FILENAME_MAX+1];

    _ubf_loader_init();

    NDRX_LOG(log_debug, "enter generate_files()");
    /*
     * Process file by file
     */
    while (SUCCEED==ret && NULL!=(fname=M_get_next(&ret)))
    {
        out_f_name[0] = EOS;

        /* Open field table file */
        if (NULL==(inf=fopen(fname, "r")))
        {
            _Fset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]",
                                fname, strerror(errno));
            ret=FAIL;
        }

        /* Open output file */
        if (SUCCEED==ret)
        {
            strcpy(G_active_file, get_file_name(fname));
            
            M_renderer->get_fullname(out_f_name);
            
            /* build up path for output file name */
            if (NULL==(G_outf=fopen(out_f_name, "w")))
            {
                _Fset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]",
                                            out_f_name, strerror(errno));
                ret=FAIL;
            }
            else
            {
                ret = M_renderer->file_open(out_f_name);
            }
        }

        if (SUCCEED==ret)
        {
            /* This will also do the check for duplicates! */
            ret=_ubf_load_def_file(inf, M_renderer->put_text_line, M_renderer->put_def_line, 
                                        M_renderer->put_got_base_line, fname, TRUE);
        }

        /* close opened files. */
        if (NULL!=inf)
        {
            fclose(inf);
            inf=NULL;
        }

        if (NULL!=G_outf)
        {
            M_renderer->file_close(out_f_name);
            fclose(G_outf);
            G_outf=NULL;
        }

        if (SUCCEED!=ret && EOS!=out_f_name[0])
        {
            /* unlink last file */
            NDRX_LOG(log_debug, "Unlinking %s",out_f_name);
            unlink(out_f_name);
        }
        else if (SUCCEED==ret)
        {
            NDRX_LOG(log_debug, "%s processed OK, output: %s",
                                            fname, out_f_name);
        }
    }
    
    return ret;
}
