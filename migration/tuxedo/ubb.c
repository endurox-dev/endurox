/**
 * @brief Tuxedo ubbconfig parsing routines
 *
 * @file ubb.c
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
#include "ubb.h"
#include "ubb.tab.h"
#include "ddr.tab.h"
#include "ndebug.h"
#include <sys_unix.h>
#include <atmi_int.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
expublic ndrx_ubb_parser_t ndrx_G_ubbp;      /**< Parsing time attributes*/
expublic ndrx_ubb_parser_t ndrx_G_ddrp;      /**< Parsing time attributes*/
/*---------------------------Statics------------------------------------*/
exprivate ndrx_growlist_t  M_strbuf;
exprivate char M_curfile[PATH_MAX+1]="UBBCONFIG(stdin)";/**< current UBB config file*/
/*---------------------------Prototypes---------------------------------*/

extern int ubbdebug;

/**
 * DDR parsing error
 */
void ddrerror(char *s, ...)
{
    /* Log only first error! */
    if (EXFAIL!=ndrx_G_ddrp.error)
    {
        va_list ap;
        char context[31];
        char *p=NULL;
        int i, ctx_start;
        char *mem = ndrx_G_ddrp.parsebuf;
        int len=strlen(mem);
        
        ctx_start = ddrlloc.first_column;
        
        NDRX_LOG(log_error, "ctx_start=%d len=%d (%s)", ctx_start, len, mem);
        memset(context, 0, sizeof(context));
        
        /* prepare context of max 30 chars. */
        p = &context[16];
        
        for (i=15; i>=0 && (ctx_start-(15-i)) >= 0 && '\n'!=mem[ctx_start-(15-i)]; i--)
        {
            p--;
            *p=mem[ctx_start-(15-i)];
        }

        for (i=1; i<15 && ctx_start+i<len && '\n'!=mem[ctx_start+i]; i++)
        {
            context[15+i]=mem[ctx_start+i];
        }

        va_start(ap, s);
        snprintf(ndrx_G_ddrp.errbuf, sizeof(ndrx_G_ddrp.errbuf), 
                "%s DDR parse error, line: %d near expression [%s]: ", 
                M_curfile, ndrx_G_ubbline, p);
        
        len=strlen(ndrx_G_ddrp.errbuf);
        vsnprintf(ndrx_G_ddrp.errbuf+len, sizeof(ndrx_G_ddrp.errbuf)-len, s, ap);
        va_end(ap);
        NDRX_LOG(log_error, "Failed to parse: %s", ndrx_G_ddrp.errbuf);

        ndrx_G_ddrp.error = EXFAIL;
    }
}

/**
 * Generate parsing error
 * @param s
 * @param ...
 */
void ubberror(char *s, ...)
{
    /* Log only first error! */
    if (EXFAIL!=ndrx_G_ubbp.error)
    {
        va_list ap;
        char errbuf[2048];
        char context[31];
        char *p=NULL;
        int i, ctx_start;
        char *mem = M_strbuf.mem;
        int len=strlen(mem);
        
        ctx_start = ubblloc.first_column;
        
        NDRX_LOG(log_error, "ctx_start=%d len=%d", ctx_start, len);
        memset(context, 0, sizeof(context));
        
        /* prepare context of max 30 chars. */
        p = &context[16];
        
        for (i=15; i>=0 && (ctx_start-(15-i)) >= 0 && '\n'!=mem[ctx_start-(15-i)]; i--)
        {
            p--;
            *p=mem[ctx_start-(15-i)];
        }

        for (i=1; i<15 && ctx_start+i<len && '\n'!=mem[ctx_start+i]; i++)
        {
            context[15+i]=mem[ctx_start+i];
        }

        va_start(ap, s);
        snprintf(errbuf, sizeof(errbuf), "%s error line: %d near expression [%s]: ", 
                M_curfile, ndrx_G_ubbline, p);
        
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
        ndrx_G_ubbp.error = EXFAIL;
    }
}

extern void ubb_scan_string (char *yy_str  );
extern int ubblex_destroy  (void);
extern int ndrx_G_ubbcolumn;
/**
 * Parse range
 * @param p_crit current criterion in parse subject
 * @return EXSUCCEED/EXFAIL
 */
exprivate int parse_ubbconfig(char *expr)
{
    int ret = EXSUCCEED;
    memset(&ndrx_G_ubbp, 0, sizeof(ndrx_G_ubbp));
    /* init the string builder */
    ndrx_growlist_init(&ndrx_G_ubbp.stringbuffer, 200, sizeof(char));
    
    /* start to parse... */
    
    ndrx_G_ubbcolumn=0;
    /* NDRX_LOG(log_info, "Parsing config: [%s]", expr); */
    ubb_scan_string(expr);
            
    if (EXSUCCEED!=ubbparse() || EXSUCCEED!=ndrx_G_ubbp.error)
    {
        NDRX_LOG(log_error, "Failed to parse ubb config");
        
        /* free parsers... */
        ubblex_destroy();
        
        /* well if we hav */
        
        EXFAIL_OUT(ret);
    }
    ubblex_destroy();
    
out:
    /* free up string buffer */
    ndrx_growlist_free(&ndrx_G_ubbp.stringbuffer);

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
    fprintf(stderr, "  -c               RFU, same as -n\n");
    fprintf(stderr, "  -b <BLOCKS>      RFU, ignored\n");
    fprintf(stderr, "  -L <LMID>        Convert only specified LMID\n");
    fprintf(stderr, "  -A               Do not try to re-use SRVIDs, assign new\n");
    fprintf(stderr, "  -D               Debug level 0..5\n");
    fprintf(stderr, "  -S               Converter script name (if not using internal)\n"); 
    /* All directories and files created, shall be made with prefix, mostly needed for testing: */
    fprintf(stderr, "  -P               Output directories and files prefix\n"); 
    fprintf(stderr, "  -h               Print this help\n");
}

/**
 * Main entry point for view compiler
 */
int main(int argc, char **argv)
{
    int ret = EXSUCCEED;
    int c;
    char line[PATH_MAX+1];
    char opt_n[2] = ""; /**< no write */
    char opt_y[2] = ""; /**< auto confirm */
    char opt_A[2] = ""; /**< assing numbers */
    char opt_L[30+1] = "";  /**< generate particular LMID only */
    char opt_P[PATH_MAX+1] = "";  /**< output prefix */
    char script_nm[PATH_MAX+1]="";
    size_t len = 0;
    FILE *handle=NULL;
    NDRX_BANNER("UBB2EX Tool");
    
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
            
    while ((c = getopt (argc, argv, "nycb:hS:D:L:AP:")) != -1)
    {
        switch (c)
        {
            case 'P':
                NDRX_STRCPY_SAFE(opt_P, optarg);
                break;
            case 'A':
                opt_A[0]='1';
                break;
            case 'L':
                NDRX_STRCPY_SAFE(opt_L, optarg);
                break;
            case 'n':
                /* Pass this to the script */
                opt_n[0]='1';
                break;
            case 'y':
                /* Pass this to the script */
                opt_y[0]='1';
                break;
            case 'S':
                NDRX_STRCPY_SAFE(script_nm, optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return 0; /*<<<< RETURN ! */
                break;
            case 'D':
                tplogconfig(LOG_FACILITY_NDRX|LOG_FACILITY_UBF|LOG_FACILITY_TP
                        , atoi(optarg), "", "", "");
		break;
            case 'c':
                opt_n[0]='1';
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
            NDRX_STRCPY_SAFE(M_curfile, argv[optind]);
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
        if (EXSUCCEED!=init_vm(script_nm, opt_n, opt_y, opt_L, opt_A, opt_P))
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
            /* clean up...! */
            call_add_func("ex_cleanup", "");
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
