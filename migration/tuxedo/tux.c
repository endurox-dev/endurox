/**
 * @brief Tuxedo ubbconfig parsing routines
 *
 * @file tux.c
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <regex.h>
#include <nstdutil.h>

#include <exregex.h>
#include "tux.h"
#include "tux.tab.h"
#include "ndebug.h"
#include <sys_unix.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_tux_parser_t ndrx_G_tuxp;      /**< Parsing time attributes*/
expublic ndrx_tux_parser_t ndrx_G_ddrp;      /**< Parsing time attributes*/
/*---------------------------Statics------------------------------------*/
exprivate int M_syntax_check = EXFALSE;     /**< Syntax check only (no plot) */
exprivate ndrx_growlist_t  M_strbuf;
/*---------------------------Prototypes---------------------------------*/

extern int tuxdebug;

/**
 * DDR parsing error
 */
void ddrerror(char *s, ...)
{
    char errbuf[2048];
}

/**
 * Generate parsing error
 * @param s
 * @param ...
 */
void tuxerror(char *s, ...)
{
    /* Log only first error! */
    if (EXFAIL!=ndrx_G_tuxp.error)
    {
        va_list ap;
        char errbuf[2048];
        char context[31];
        char *p=NULL;
        int i, ctx_start;
        char *mem = M_strbuf.mem;
        int len=strlen(mem);
        
        ctx_start = tuxlloc.first_column;
        
        NDRX_LOG(log_error, "ctx_start=%d len=%d", ctx_start, len);
        memset(context, 0, sizeof(context));
        
        /* prepare context of max 30 chars. */
        p = &context[16];
        
        for (i=15; i>=0 && '\n'!=mem[ctx_start-(15-i)] && (ctx_start-(15-i)) >= 0; i--)
        {
            p--;
            *p=mem[ctx_start-(15-i)];
        }

        for (i=1; i<15 && ctx_start+i<len && '\n'!=mem[ctx_start+i]; i++)
        {
            context[15+i]=mem[ctx_start+i];
        }

        va_start(ap, s);
        snprintf(errbuf, sizeof(errbuf), "UBBConfig error line: %d near expression [%s]: ", 
                ndrx_G_tuxline, p);
        
        len=strlen(errbuf);
        vsnprintf(errbuf+len, sizeof(errbuf)-len, s, ap);
        va_end(ap);
        NDRX_LOG(log_error, "Failed to parse: %s", errbuf);
        
        if (_Nis_error())
        {
            _Nappend_error_msg(errbuf);
        }
        else
        {
            _Nset_error_msg(NEFORMAT, errbuf);
        }
        ndrx_G_tuxp.error = EXFAIL;
    }
}

extern void tux_scan_string (char *yy_str  );
extern int tuxlex_destroy  (void);
extern int ndrx_G_tuxcolumn;
/**
 * Parse range
 * @param p_crit current criterion in parse subject
 * @return EXSUCCEED/EXFAIL
 */
exprivate int parse_ubbconfig(char *expr)
{
    int ret = EXSUCCEED;
    memset(&ndrx_G_tuxp, 0, sizeof(ndrx_G_tuxp));
    /* init the string builder */
    ndrx_growlist_init(&ndrx_G_tuxp.stringbuffer, 200, sizeof(char));
    
    /* start to parse... */
    
    ndrx_G_tuxcolumn=0;
    /* NDRX_LOG(log_info, "Parsing config: [%s]", expr); */
    tux_scan_string(expr);
            
    if (EXSUCCEED!=tuxparse() || EXSUCCEED!=ndrx_G_tuxp.error)
    {
        NDRX_LOG(log_error, "Failed to parse tux config");
        
        /* free parsers... */
        tuxlex_destroy();
        
        /* well if we hav */
        
        EXFAIL_OUT(ret);
    }
    tuxlex_destroy();
    
out:
    /* free up string buffer */
    ndrx_growlist_free(&ndrx_G_tuxp.stringbuffer);

    return ret;    
}

/**
 * print help for the command
 * @param name name of the program, argv[0]
 */
exprivate void print_help(char *name)
{
    fprintf(stderr, "Usage: %s [options] {UBBCONFIG_file | -}\n", name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n               Do syntax check only\n");
    fprintf(stderr, "  -y               Do not ask for confirmation\n");
    fprintf(stderr, "  -c               RFU, syntax check only\n");
    fprintf(stderr, "  -b               RFU, ignored\n");
    fprintf(stderr, "  -l <LMID>        Convert only specified LMID\n");
    fprintf(stderr, "  -a               Do not try to re-use SRVIDs, assign new\n");
    fprintf(stderr, "  -d               Debug level 0..5\n");
    fprintf(stderr, "  -h               Print this help\n");
    fprintf(stderr, "  -s               Converter script name (if not using internal)\n"); 
}

/**
 * Main entry point for view compiler
 */
int main(int argc, char **argv)
{
    int ret = EXSUCCEED;
    int c;
    char line[PATH_MAX+1];
    char n_opt[2] = ""; /* no write */
    char y_opt[2] = ""; /* auto confirm */
    char l_opt[30+1] = "";  /* generate particular LMID only */
    char script_nm[PATH_MAX+1]="";
    size_t len = 0;
    FILE *handle=NULL;
    NDRX_BANNER("TMLOADCF Tool");
    
    /* if NDRX_DEBUG_CONF is set to empty... boot with minimum log level...*/

    /* put empty config -> let tool to control the debug level */
    if (NULL==getenv("NDRX_DEBUG_CONF"))
    {
        setenv("NDRX_DEBUG_CONF", "", 1);
    }
    tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF|LOG_FACILITY_TP, 0, "", "", "");
    ndrx_growlist_init(&M_strbuf, 1000, sizeof(char));

    /* clear any error... */
    _Nunset_error();
            
    while ((c = getopt (argc, argv, "nycb:hs:d:l:")) != -1)
    {
        switch (c)
        {
            case 'l':
                NDRX_STRCPY_SAFE(l_opt, optarg);
                break;
            case 'n':
                /* Pass this to the script */
                n_opt[0]='1';
                break;
            case 'y':
                /* Pass this to the script */
                y_opt[0]='1';
                break;
            case 's':
                NDRX_STRCPY_SAFE(script_nm, optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return 0; /*<<<< RETURN ! */
                break;
            case 'd':
                tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF|LOG_FACILITY_TP
                        , atoi(optarg), "", "", "");
            case 'c':
                M_syntax_check = EXTRUE;
                break;
            case 'b':
                NDRX_LOG(log_debug, "Ignored flag [%c]", c);
                break;
            case '?':
            default:
                print_help(argv[0]);
                return EXFAIL; /*<<<< RETURN ! */
        }
    }
    if (optind < argc)
    {
        if (0==strcmp(argv[optind], "-"))
        {
            /* read from stdin */
            handle=stdin;
        }
        else
        {
            /* read from file 
             *  + add 
             */
            if (NULL==(handle=NDRX_FOPEN(argv[optind], "r")))
            {
                NDRX_LOG(log_error, "Failed to open [%s]: %s", argv[optind], strerror(errno));
                
                _Nset_error_fmt(NENOENT, "Failed to open [%s]: %s", 
                        argv[optind], strerror(errno));
                EXFAIL_OUT(ret);
            }
        }
    
        len = sizeof(line);
        while (NULL!=fgets(line, len, handle))
        {
            if (EXSUCCEED!=ndrx_growlist_append_many(&M_strbuf, line, strlen(line)))
            {
                _Nset_error_msg(NEMALLOC, "Failed to buffer ubb data");
                EXFAIL_OUT(ret);
            }
        }
        
        /* add eos... */
        if (EXSUCCEED!=ndrx_growlist_append_many(&M_strbuf, "", 1))
        {
            _Nset_error_msg(NEMALLOC, "Failed to buffer ubb data (EOS)");
            EXFAIL_OUT(ret);
        }
        
        /* Init VM */
        if (EXSUCCEED!=tux_init_vm(script_nm, n_opt, y_opt, l_opt))
        {
            _Nset_error_msg(NESYSTEM, "Failed to load converter script");
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=parse_ubbconfig(M_strbuf.mem))
        {
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=call_add_func("ex_generate", ""))
        {
            EXFAIL_OUT(ret);
        }
    }
    else
    {
        print_help(argv[0]);
        return EXFAIL; /*<<<< RETURN ! */
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

    ndrx_growlist_free(&M_strbuf);
    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
