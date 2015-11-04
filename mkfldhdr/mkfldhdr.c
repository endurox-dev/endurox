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
/*---------------------------Externs------------------------------------*/
extern int optind, optopt, opterr;
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
char **M_argv;
int M_argc;
char *M_output_dir=".";
FILE *M_outf=NULL;
/*---------------------------Prototypes---------------------------------*/

/*
 * Function for retrieving next file name
 */
char *(*M_get_next) (int *ret);


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
 * Write text line to output file
 * @param text
 * @return
 */
int put_text_line (char *text)
{
    int ret=SUCCEED;
    
    fprintf(M_outf, "%s", text);
    
    /* Check errors */
    if (ferror(M_outf))
    {
        _Fset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=FAIL;
    }

    return ret;
}

/**
 * Process the baseline
 * @param base
 * @return
 */
int put_got_base_line(char *base)
{

    int ret=SUCCEED;

    fprintf(M_outf, "/*\tfname\tbfldid            */\n"
                    "/*\t-----\t-----            */\n");

    /* Check errors */
    if (ferror(M_outf))
    {
        _Fset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=FAIL;
    }

    return ret;
}

/**
 * Write definition to output file
 * @param def
 * @return
 */
int put_def_line (UBF_field_def_t *def)
{
    int ret=SUCCEED;
    int type = def->bfldid>>EFFECTIVE_BITS;
    BFLDID number = def->bfldid & EFFECTIVE_BITS_MASK;

    fprintf(M_outf, "#define\t%s\t((BFLDID32)%d)\t/* number: %d\t type: %s */\n",
            def->fldname, def->bfldid, number,
            G_dtype_str_map[type].fldname);
    
    /* Check errors */
    if (ferror(M_outf))
    {
        _Fset_error_fmt(BFTOPEN, "Failed to write to output file: [%s]", strerror(errno));
        ret=FAIL;
    }

    return ret;
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

    NDRX_DBG_INIT(("mkhbufhdr", "MKFLDHDR"));
    /*ndrx_dbg_setlev(log_always);  set default level to fatal */
    /* Parse command line */
    while ((c = getopt(argc, argv, "h?D:d:")) != -1)
    {
        switch(c)
        {
            case 'D':
                NDRX_LOG(log_debug, "%c = %s", c, optarg);
                dbglev = atoi(optarg);
                /*ndrx_dbg_setlev(dbglev);*/
                break;
            case 'd':
                M_output_dir = optarg;
                NDRX_LOG(log_debug, "%c = %s", c, M_output_dir);
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
        }
    }
    
    if (SUCCEED==ret)
    {
        NDRX_LOG(log_debug, "Output directory is [%s]", M_output_dir);
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
int generate_files(void)
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
            sprintf(out_f_name, "%s/%s.h", M_output_dir, get_file_name(fname));
            /* build up path for output file name */
            if (NULL==(M_outf=fopen(out_f_name, "w")))
            {
                _Fset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]",
                                            out_f_name, strerror(errno));
                ret=FAIL;
            }
        }

        if (SUCCEED==ret)
        {
            /* This will also do the check for duplicates! */
            ret=_ubf_load_def_file(inf, put_text_line, put_def_line, 
                                        put_got_base_line, fname, TRUE);
        }

        /* close opened files. */
        if (NULL!=inf)
        {
            fclose(inf);
            inf=NULL;
        }

        if (NULL!=M_outf)
        {
            fclose(M_outf);
            M_outf=NULL;
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
